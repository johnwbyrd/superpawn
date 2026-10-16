// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.
//
// Trains the Superpawn NNUE with bullet. The architecture here must match
// src/nnue.h in the engine:
//
//   (768 -> HIDDEN)x2 -> OUTPUT_BUCKETS   with SCReLU, QA = 255, QB = 64, scale 400
//
// Usage:
//   cargo run --release -- --data shuffled.bin [--data more.bin] [--id spn-001]
//                         [--superbatches 40] [--lr 0.001] [--wdl 0.1]
//                         [--threads 4] [--out checkpoints] [--resume <checkpoint dir>]
//
// Each superbatch is 6104 batches of 16384 positions, about 100 million.
// The quantised network is written to <out>/<id>-<n>/quantised.bin; convert
// it with `superpawn convert <quantised.bin> <net.nnue>`.

use bullet::{
    game::{inputs::Chess768, outputs::MaterialCount},
    nn::optimiser::AdamW,
    trainer::{
        save::SavedFormat,
        schedule::{TrainingSchedule, TrainingSteps, lr, wdl},
        settings::LocalSettings,
    },
    value::{ValueTrainerBuilder, loader::DirectSequentialDataLoader},
};

// Keep in sync with src/nnue.h
const HIDDEN: usize = 1024;
const OUTPUT_BUCKETS: usize = 8;
const QA: i16 = 255;
const QB: i16 = 64;
const SCALE: f32 = 400.0;

struct Args {
    data: Vec<String>,
    id: String,
    superbatches: usize,
    lr: f32,
    wdl: f32,
    threads: usize,
    out: String,
    resume: Option<String>,
}

fn parse_args() -> Args {
    let mut args = Args {
        data: Vec::new(),
        id: "spn".to_string(),
        superbatches: 40,
        lr: 0.001,
        // Weight of the game result in the target; the rest is the search
        // score. Results from a weak engine's games are mostly noise, so the
        // first networks learn from the scores. Raise this as the engine and
        // its data improve.
        wdl: 0.1,
        threads: 4,
        out: "checkpoints".to_string(),
        resume: None,
    };

    let mut it = std::env::args().skip(1);
    while let Some(flag) = it.next() {
        let mut value = || it.next().unwrap_or_else(|| panic!("missing value for {flag}"));
        match flag.as_str() {
            "--data" => args.data.push(value()),
            "--id" => args.id = value(),
            "--superbatches" => args.superbatches = value().parse().expect("--superbatches N"),
            "--lr" => args.lr = value().parse().expect("--lr F"),
            "--wdl" => args.wdl = value().parse().expect("--wdl F"),
            "--threads" => args.threads = value().parse().expect("--threads N"),
            "--out" => args.out = value(),
            "--resume" => args.resume = Some(value()),
            "--help" | "-h" => {
                println!("{}", include_str!("main.rs").lines().skip(4).take(14).map(|l| l.trim_start_matches("// ")).collect::<Vec<_>>().join("\n"));
                std::process::exit(0);
            }
            other => panic!("unknown argument {other}"),
        }
    }

    if args.data.is_empty() {
        eprintln!("at least one --data file is required");
        std::process::exit(2);
    }
    args
}

fn main() {
    let args = parse_args();

    let mut trainer = ValueTrainerBuilder::default()
        .dual_perspective()
        .optimiser(AdamW)
        .inputs(Chess768)
        .output_buckets(MaterialCount::<OUTPUT_BUCKETS>)
        // The engine's file layout: feature weights [768][HIDDEN], feature
        // bias, output weights [bucket][2 * HIDDEN] (hence the transpose),
        // output bias. See src/nnue.h.
        .save_format(&[
            SavedFormat::id("l0w").round().quantise::<i16>(QA),
            SavedFormat::id("l0b").round().quantise::<i16>(QA),
            SavedFormat::id("l1w").round().quantise::<i16>(QB).transpose(),
            SavedFormat::id("l1b").round().quantise::<i16>(QA * QB),
        ])
        .loss_fn(|output, target| output.sigmoid().squared_error(target))
        .build(|builder, stm_inputs, ntm_inputs, output_buckets| {
            let l0 = builder.new_affine("l0", 768, HIDDEN);
            let l1 = builder.new_affine("l1", 2 * HIDDEN, OUTPUT_BUCKETS);

            let stm_hidden = l0.forward(stm_inputs).screlu();
            let ntm_hidden = l0.forward(ntm_inputs).screlu();
            let hidden_layer = stm_hidden.concat(ntm_hidden);
            l1.forward(hidden_layer).select(output_buckets)
        });

    if let Some(path) = &args.resume {
        trainer.load_from_checkpoint(path);
    }

    let schedule = TrainingSchedule {
        net_id: args.id.clone(),
        eval_scale: SCALE,
        steps: TrainingSteps {
            batch_size: 16_384,
            batches_per_superbatch: 6104,
            start_superbatch: 1,
            end_superbatch: args.superbatches,
        },
        wdl_scheduler: wdl::ConstantWDL { value: args.wdl },
        lr_scheduler: lr::CosineDecayLR {
            initial_lr: args.lr,
            final_lr: args.lr * 0.3f32.powi(5),
            final_superbatch: args.superbatches,
        },
        save_rate: 10,
    };

    let settings = LocalSettings {
        threads: args.threads,
        test_set: None,
        output_directory: &args.out,
        batch_queue_size: 64,
    };

    let data: Vec<&str> = args.data.iter().map(String::as_str).collect();
    let loader = DirectSequentialDataLoader::new(&data);

    trainer.run(&schedule, &settings, &loader);
}
