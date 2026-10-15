// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include "misc.h"

#include <mutex>
#include <sstream>
#include <string>

#ifndef SP_VERSION
    #define SP_VERSION "dev"
#endif

namespace sp {

std::string engine_info(bool toUci) {
    std::ostringstream ss;
    ss << (toUci ? "id name " : "") << "Superpawn " << SP_VERSION;
    ss << (toUci ? "\nid author " : " by ") << "John Byrd and the Superpawn contributors";
    return ss.str();
}

std::string compiler_info() {
    std::ostringstream ss;
    ss << "Compiled by ";
#if defined(__clang__)
    ss << "clang++ " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__;
#elif defined(__GNUC__)
    ss << "g++ " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
#elif defined(_MSC_VER)
    ss << "MSVC " << _MSC_FULL_VER;
#else
    ss << "an unknown compiler";
#endif
    ss << "\nTarget:";
#if defined(__AVX512F__)
    ss << " AVX-512";
#elif defined(__AVX2__)
    ss << " AVX2";
#elif defined(__SSE4_1__)
    ss << " SSE4.1";
#elif defined(__aarch64__) || defined(_M_ARM64)
    ss << " ARM64";
#else
    ss << " generic";
#endif
#if defined(SP_USE_PEXT)
    ss << " PEXT";
#endif
#ifndef NDEBUG
    ss << " (debug build)";
#endif
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, SyncCout sc) {
    static std::mutex m;

    if (sc == IO_LOCK)
        m.lock();
    if (sc == IO_UNLOCK)
        m.unlock();

    return os;
}

}  // namespace sp
