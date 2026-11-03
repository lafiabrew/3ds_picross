/*
 * Unit & pass/fail tests for 3ds_picross:
 * - Pass-the-pass: RLE line clue calculation, chiseling valid cells, solving an entire 10x10 nonogram.
 * - Fail-the-fail: chiseling an empty cell increments mistakes and auto-marks cross, out-of-bounds indices rejected.
 */

#include "picross.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    uint8_t clues[PICR_MAX_CLUES];
    uint8_t clen = 0;
    /* 0b1101110001 = runs of 1, 3, 2 */
    picr_compute_line_clues(0x371u, clues, &clen);
    assert(clen == 3 && clues[0] == 1 && clues[1] == 3 && clues[2] == 2);

    picr_state_t st;
    assert(picr_init(&st, 0));
    assert(st.solved == false);

    /* Mistake test: row 0 col 0 in POKEBALL (0x078) is 0 -> must count mistake & mark CROSS */
    assert(picr_apply_action(&st, 0, 0, PICR_CELL_FILLED));
    assert(st.mistakes == 1);
    assert(st.grid[0][0] == PICR_CELL_CROSS);

    /* Solve entire puzzle */
    for (int r = 0; r < PICR_SIZE; r++) {
        for (int c = 0; c < PICR_SIZE; c++) {
            if ((st.solution[r] >> c) & 1u) {
                assert(picr_apply_action(&st, r, c, PICR_CELL_FILLED));
            }
        }
    }
    assert(st.solved == true);

    /* Fail-the-fail negative assertions */
    assert(picr_init(NULL, 0) == false);
    assert(picr_init(&st, -1) == false);
    assert(picr_init(&st, 99) == false);
    assert(picr_apply_action(&st, -1, 0, PICR_CELL_FILLED) == false);
    assert(picr_apply_action(&st, 0, 10, PICR_CELL_FILLED) == false);
    assert(picr_apply_action(&st, 0, 0, (picr_cell_t)99) == false);

    printf("PASS: test_picross all RLE clue, mistake guard, solve, and pass/fail checks succeeded.\n");
    return 0;
}
