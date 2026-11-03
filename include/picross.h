#ifndef PICR_H
#define PICR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PICR_SIZE 10
#define PICR_MAX_CLUES 5
#define PICR_PUZZLE_COUNT 6

typedef enum {
    PICR_CELL_EMPTY = 0,
    PICR_CELL_FILLED = 1,
    PICR_CELL_CROSS = 2
} picr_cell_t;

typedef struct {
    const char *title;
    uint16_t rows[PICR_SIZE]; /* 10-bit bitmask per row */
} picr_puzzle_def_t;

typedef struct {
    int puzzle_idx;
    char title[32];
    uint16_t solution[PICR_SIZE];
    picr_cell_t grid[PICR_SIZE][PICR_SIZE];
    uint8_t row_clues[PICR_SIZE][PICR_MAX_CLUES];
    uint8_t row_clue_len[PICR_SIZE];
    uint8_t col_clues[PICR_SIZE][PICR_MAX_CLUES];
    uint8_t col_clue_len[PICR_SIZE];
    int cur_r;
    int cur_c;
    uint32_t mistakes;
    uint32_t moves;
    bool solved;
} picr_state_t;

bool picr_init(picr_state_t *st, int puzzle_idx);
void picr_compute_line_clues(uint16_t mask10, uint8_t out_clues[PICR_MAX_CLUES], uint8_t *out_len);
bool picr_apply_action(picr_state_t *st, int r, int c, picr_cell_t action);
bool picr_check_solved(const picr_state_t *st);

#endif /* PICR_H */
