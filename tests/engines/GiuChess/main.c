/* This file is a part of the GiuChess Project. */
/* */
/* Copyright (c) 2005 Giuliano Ippoliti aka JSorel (ippo@linuxmail.org) */
/* */
/* This program is free software; you can redistribute it and/or */
/* modify it under the terms of the GNU General Public License */
/* as published by the Free Software Foundation; either */
/* version 2 of the License, or (at your option) any later version. */
/* */
/* This program is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* GNU General Public License for more details. */
/* */
/* You should have received a copy of the GNU General Public License */
/* along with this program; if not, write to the Free Software */
/* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#include "main.h"

void            test_game2()
{
    int             nlegmov;
    char            bestply2[10];
    float           tmpres;
    MOVE_LIST       movelist;

    string_execute_move("a2a4", w, b);
    string_execute_move("b8c6", w, b);
    string_execute_move("e2e3", w, b);
    string_execute_move("a8b8", w, b);
    string_execute_move("g1h3", w, b);
    string_execute_move("g8h6", w, b);
    string_execute_move("h3g5", w, b);
    string_execute_move("g7g6", w, b);
    string_execute_move("b2b3", w, b);
    string_execute_move("a7a6", w, b);
    string_execute_move("h2h3", w, b);
    string_execute_move("f7f6", w, b);
    string_execute_move("g5f3", w, b);
    string_execute_move("c6a7", w, b);
    string_execute_move("f3d4", w, b);
    string_execute_move("c7c5", w, b);
    string_execute_move("d4f3", w, b);
    string_execute_move("h8g8", w, b);
    string_execute_move("c2c4", w, b);
    string_execute_move("g6g5", w, b);
    string_execute_move("f3h2", w, b);
    string_execute_move("g5g4", w, b);
    string_execute_move("h2g4", w, b);
    string_execute_move("f6f5", w, b);
    string_execute_move("g4h6", w, b);
    string_execute_move("f8h6", w, b);
    string_execute_move("d1h5", w, b);
    string_execute_move("g8g6", w, b);
    string_execute_move("h5f5", w, b);
    string_execute_move("h6g7", w, b);
    string_execute_move("d2d4", w, b);
    string_execute_move("d7d6", w, b);
    string_execute_move("f5h5", w, b);
    string_execute_move("e8f8", w, b);
    string_execute_move("h5h7", w, b);
    string_execute_move("g6h6", w, b);
    string_execute_move("h7e4", w, b);
    string_execute_move("g7f6", w, b);
    string_execute_move("f1d3", w, b);
    string_execute_move("b8a8", w, b);
    string_execute_move("h1h2", w, b);
    string_execute_move("e7e6", w, b);
    string_execute_move("b1c3", w, b);
    string_execute_move("d8a5", w, b);
    string_execute_move("b3b4", w, b);
    string_execute_move("a5b4", w, b);
    string_execute_move("e1d2", w, b);
    string_execute_move("c5d4", w, b);

    white_leg_mov(w, b);

    crt_list(&movelist);
    nlegmov = push_leg_mov(w, &movelist);

    depthmax = STARTDPTH;

    tmpres = alphabeta(depthmax, -200, 200, -1, bestply2);

    printf("%s\n", bestply2);
}

void            test_game2b()
{
    int             nlegmov;
    MOVE_LIST       movelist;

    string_execute_move("e2e4", w, b);
    string_execute_move("d7d5", w, b);
    string_execute_move("e1e2", w, b);
    string_execute_move("c8g4", w, b);

    white_leg_mov(w, b);

    crt_list(&movelist);
    nlegmov = push_leg_mov(w, &movelist);
    printf("%d\n", nlegmov);
    log_lst(movelist);
}


void            test_game()
{
    int             nlegmov;
    char            bestply2[10];
    float           tmpres;
    MOVE_LIST       movelist;

    string_execute_move("d2d4", w, b);
    string_execute_move("e7e6", w, b);
    string_execute_move("c2c3", w, b);
    string_execute_move("d7d5", w, b);
    string_execute_move("g1h3", w, b);
    string_execute_move("d8f6", w, b);
    string_execute_move("c1g5", w, b);
    string_execute_move("f6f5", w, b);
    string_execute_move("d1d3", w, b);
    string_execute_move("f5d3", w, b);
    string_execute_move("e2d3", w, b);
    string_execute_move("f8d6", w, b);
    string_execute_move("b1d2", w, b);
    string_execute_move("f7f6", w, b);
    string_execute_move("g5e3", w, b);
    string_execute_move("c7c6", w, b);
    string_execute_move("h3g1", w, b);
    string_execute_move("b7b5", w, b);
    string_execute_move("b2b3", w, b);
    string_execute_move("c8d7", w, b);
    string_execute_move("h2h4", w, b);
    string_execute_move("g8e7", w, b);
    string_execute_move("f1e2", w, b);
    string_execute_move("e7f5", w, b);
    string_execute_move("e2g4", w, b);
    string_execute_move("f5e3", w, b);
    string_execute_move("f2e3", w, b);
    string_execute_move("h7h5", w, b);
    string_execute_move("g4h3", w, b);
    string_execute_move("d6g3", w, b);
    string_execute_move("e1e2", w, b);
    string_execute_move("g3h4", w, b);
    string_execute_move("h3e6", w, b);
    string_execute_move("d7e6", w, b);
    string_execute_move("h1h4", w, b);
    string_execute_move("g7g5", w, b);

    string_execute_move("h4h2", w, b);
    string_execute_move("h8h7", w, b);
    string_execute_move("a1f1", w, b);
    string_execute_move("h7f7", w, b);
    string_execute_move("h2h1", w, b);
    string_execute_move("e8f8", w, b);
    string_execute_move("b3b4", w, b);
    string_execute_move("a7a6", w, b);
    string_execute_move("e2f3", w, b);
    string_execute_move("b8d7", w, b);
    string_execute_move("h1h5", w, b);
    string_execute_move("a8a7", w, b);
    string_execute_move("a2a3", w, b);
    string_execute_move("e6f5", w, b);
    string_execute_move("f3g3", w, b);
    string_execute_move("f5d3", w, b);

    string_execute_move("f1f3", w, b);
    string_execute_move("g5g4", w, b);
    string_execute_move("g3g4", w, b);
    string_execute_move("f6f5", w, b);
    string_execute_move("g4h4", w, b);
    string_execute_move("d7f6", w, b);
    string_execute_move("h5h8", w, b);
    string_execute_move("f8g7", w, b);
    string_execute_move("h8d8", w, b);
    string_execute_move("f6e4", w, b);
    string_execute_move("d2e4", w, b);
    string_execute_move("f5e4", w, b);
    string_execute_move("f3f7", w, b);
    string_execute_move("a7f7", w, b);
    string_execute_move("d8e8", w, b);
    string_execute_move("g7f6", w, b);
    string_execute_move("e8h8", w, b);
    string_execute_move("f7g7", w, b);
    string_execute_move("h8c8", w, b);
    string_execute_move("g7h7", w, b);

    string_execute_move("h4g4", w, b);
    string_execute_move("h7g7", w, b);
    string_execute_move("g4h4", w, b);
    string_execute_move("g7h7", w, b);
    string_execute_move("h4g4", w, b);
    string_execute_move("h7g7", w, b);
    string_execute_move("g4h4", w, b);
    string_execute_move("g7h7", w, b);
    string_execute_move("h4g4", w, b);
    string_execute_move("h7g7", w, b);
    string_execute_move("g4h4", w, b);
    string_execute_move("g7h7", w, b);
    string_execute_move("h4g4", w, b);
    string_execute_move("h7g7", w, b);
    string_execute_move("g4h4", w, b);
    string_execute_move("g7h7", w, b);
    string_execute_move("h4g4", w, b);
    string_execute_move("h7g7", w, b);
    string_execute_move("g4h4", w, b);
    string_execute_move("g7h7", w, b);
    string_execute_move("h4g4", w, b);
    string_execute_move("h7g7", w, b);
    string_execute_move("g4h4", w, b);
    string_execute_move("g7h7", w, b);
    string_execute_move("h4g4", w, b);
    string_execute_move("h7g7", w, b);
    string_execute_move("g4h4", w, b);
    string_execute_move("g7h7", w, b);
    string_execute_move("h4g4", w, b);
    string_execute_move("h7g7", w, b);
    string_execute_move("g4h4", w, b);
    string_execute_move("g7h7", w, b);
    string_execute_move("h4g4", w, b);
    string_execute_move("h7g7", w, b);
    string_execute_move("g4h4", w, b);
    string_execute_move("g7h7", w, b);
    string_execute_move("h4g4", w, b);
    string_execute_move("h7g7", w, b);
    string_execute_move("g4h4", w, b);
    string_execute_move("g7h7", w, b);
    string_execute_move("h4g4", w, b);
    string_execute_move("h7g7", w, b);
/*ok     */
    string_execute_move("g4f4", w, b);
    string_execute_move("g7f7", w, b);
    string_execute_move("c8c6", w, b);
    string_execute_move("f6g7", w, b);
    string_execute_move("f4g5", w, b);
    string_execute_move("a6a5", w, b);
    string_execute_move("c6g6", w, b);
    string_execute_move("g7h8", w, b);
    string_execute_move("b4a5", w, b);
    string_execute_move("h8h7", w, b);
    string_execute_move("a5a6", w, b);
    string_execute_move("h7h8", w, b);
    string_execute_move("g2g4", w, b);
    string_execute_move("h8h7", w, b);
    string_execute_move("g6h6", w, b);
    string_execute_move("h7g8", w, b);
    string_execute_move("h6g6", w, b);
    string_execute_move("g8h8", w, b);
    string_execute_move("g5h5", w, b);
    string_execute_move("d3c4", w, b);

    string_execute_move("g4g5", w, b);
    string_execute_move("c4d3", w, b);
    string_execute_move("g6h6", w, b);
    string_execute_move("h8g8", w, b);
    string_execute_move("h6g6", w, b);
    string_execute_move("g8h8", w, b);
    string_execute_move("h5h4", w, b);
    string_execute_move("d3c4", w, b);
    string_execute_move("g6h6", w, b);
    string_execute_move("f7h7", w, b);
    string_execute_move("a6a7", w, b);

    black_leg_mov(w, b);

    crt_list(&movelist);
    nlegmov = push_leg_mov(b, &movelist);

    depthmax = STARTDPTH;

    tmpres = alphabeta(depthmax, -200, 200, 1, bestply2);

    printf("%s\n", bestply2);
}


int             main()
{

    init_array();

    init_matrix();

    init_rc_arr();

    iniz(w, b);

    wait_for_input();
}
