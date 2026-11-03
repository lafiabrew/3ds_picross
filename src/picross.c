#include "picross.h"

#include <stdio.h>
#include <string.h>

static const picr_puzzle_def_t k_puzzles[PICR_PUZZLE_COUNT] = {
    { "POKEBALL",   { 0x078, 0x1FE, 0x3FF, 0x3C3, 0x3DB, 0x3FF, 0x201, 0x201, 0x1FE, 0x078 } },
    { "GAMEBOY",    { 0x1FE, 0x102, 0x17A, 0x17A, 0x17A, 0x102, 0x14E, 0x162, 0x102, 0x1FE } },
    { "TRIFORCE",   { 0x030, 0x078, 0x0FC, 0x1FE, 0x000, 0x066, 0x0F6, 0x1FE, 0x3FF, 0x000 } },
    { "MUSHROOM",   { 0x0FC, 0x1FE, 0x39F, 0x36D, 0x3FF, 0x1FE, 0x0FC, 0x0D8, 0x0FC, 0x078 } },
    { "STARMAN",    { 0x030, 0x030, 0x3FF, 0x1FE, 0x0FC, 0x0FC, 0x1FE, 0x333, 0x201, 0x000 } },
    { "HEARTCONTAINER", { 0x1CE, 0x3FF, 0x3FF, 0x3FF, 0x1FE, 0x0FC, 0x078, 0x030, 0x000, 0x000 } }
};

void picr_compute_line_clues(uint16_t mask10, uint8_t out_clues[PICR_MAX_CLUES], uint8_t *out_len) {
    if (out_clues == NULL || out_len == NULL) return;
    memset(out_clues, 0, PICR_MAX_CLUES);
    uint8_t n = 0;
    uint8_t run = 0;
    for (int i = 0; i < PICR_SIZE; i++) {
        if ((mask10 >> i) & 1u) {
            run++;
        } else if (run > 0) {
            if (n < PICR_MAX_CLUES) out_clues[n++] = run;
            run = 0;
        }
    }
    if (run > 0 && n < PICR_MAX_CLUES) {
        out_clues[n++] = run;
    }
    if (n == 0) {
        out_clues[0] = 0;
        n = 1;
    }
    *out_len = n;
}

bool picr_check_solved(const picr_state_t *st) {
    if (st == NULL) return false;
    for (int r = 0; r < PICR_SIZE; r++) {
        uint16_t mask = 0;
        for (int c = 0; c < PICR_SIZE; c++) {
            if (st->grid[r][c] == PICR_CELL_FILLED) {
                mask |= (uint16_t)(1u << c);
            }
        }
        if (mask != (st->solution[r] & 0x3FFu)) {
            return false;
        }
    }
    return true;
}

bool picr_init(picr_state_t *st, int puzzle_idx) {
    if (st == NULL || puzzle_idx < 0 || puzzle_idx >= PICR_PUZZLE_COUNT) {
        return false;
    }
    memset(st, 0, sizeof(*st));
    st->puzzle_idx = puzzle_idx;
    snprintf(st->title, sizeof(st->title), "%s", k_puzzles[puzzle_idx].title);
    for (int r = 0; r < PICR_SIZE; r++) {
        st->solution[r] = k_puzzles[puzzle_idx].rows[r] & 0x3FFu;
        picr_compute_line_clues(st->solution[r], st->row_clues[r], &st->row_clue_len[r]);
    }
    for (int c = 0; c < PICR_SIZE; c++) {
        uint16_t col_mask = 0;
        for (int r = 0; r < PICR_SIZE; r++) {
            if ((st->solution[r] >> c) & 1u) {
                col_mask |= (uint16_t)(1u << r);
            }
        }
        picr_compute_line_clues(col_mask, st->col_clues[c], &st->col_clue_len[c]);
    }
    return true;
}

bool picr_apply_action(picr_state_t *st, int r, int c, picr_cell_t action) {
    if (st == NULL || r < 0 || r >= PICR_SIZE || c < 0 || c >= PICR_SIZE) {
        return false;
    }
    if (action != PICR_CELL_EMPTY && action != PICR_CELL_FILLED && action != PICR_CELL_CROSS) {
        return false;
    }
    if (action == PICR_CELL_FILLED) {
        bool should_fill = ((st->solution[r] >> c) & 1u) != 0;
        if (!should_fill) {
            st->mistakes++;
            st->grid[r][c] = PICR_CELL_CROSS;
            st->moves++;
            return true;
        }
    }
    st->grid[r][c] = (st->grid[r][c] == action) ? PICR_CELL_EMPTY : action;
    st->moves++;
    st->solved = picr_check_solved(st);
    return true;
}
