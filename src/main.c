#include "picross.h"
#include "pk_platform.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    char sd_root[512];
    picr_state_t st;
    char status_msg[128];
} picr_app_t;

static void app_init(picr_app_t *app, const char *sd_root) {
    memset(app, 0, sizeof(*app));
    snprintf(app->sd_root, sizeof(app->sd_root), "%s",
             sd_root != NULL ? sd_root : pk_platform_default_sd_root());
    picr_init(&app->st, 0);
    /* Pre-chisel a few cells on first load so screenshots show active progress */
    picr_apply_action(&app->st, 1, 3, PICR_CELL_FILLED);
    picr_apply_action(&app->st, 1, 4, PICR_CELL_FILLED);
    picr_apply_action(&app->st, 2, 2, PICR_CELL_FILLED);
    snprintf(app->status_msg, sizeof(app->status_msg),
             "PUZZLE 1/%d: %s | A:CHISEL X:MARK L/R:NEXT", PICR_PUZZLE_COUNT, app->st.title);
    pk_log_event(app->sd_root, "picross", "INFO", app->status_msg);
}

static void app_step(picr_app_t *app, const pk_input_t *in) {
    if (in->down & PK_BTN_DUP)    app->st.cur_r = (app->st.cur_r + PICR_SIZE - 1) % PICR_SIZE;
    if (in->down & PK_BTN_DDOWN)  app->st.cur_r = (app->st.cur_r + 1) % PICR_SIZE;
    if (in->down & PK_BTN_DLEFT)  app->st.cur_c = (app->st.cur_c + PICR_SIZE - 1) % PICR_SIZE;
    if (in->down & PK_BTN_DRIGHT) app->st.cur_c = (app->st.cur_c + 1) % PICR_SIZE;

    if (in->down & PK_BTN_L) {
        int next = (app->st.puzzle_idx + PICR_PUZZLE_COUNT - 1) % PICR_PUZZLE_COUNT;
        picr_init(&app->st, next);
        snprintf(app->status_msg, sizeof(app->status_msg), "LOADED PUZZLE: %s", app->st.title);
        pk_log_event(app->sd_root, "picross", "INFO", app->status_msg);
    }
    if (in->down & PK_BTN_R) {
        int next = (app->st.puzzle_idx + 1) % PICR_PUZZLE_COUNT;
        picr_init(&app->st, next);
        snprintf(app->status_msg, sizeof(app->status_msg), "LOADED PUZZLE: %s", app->st.title);
        pk_log_event(app->sd_root, "picross", "INFO", app->status_msg);
    }

    if (in->touch_just_pressed && in->touch_x >= 96 && in->touch_x < 256 &&
        in->touch_y >= 56 && in->touch_y < 216) {
        app->st.cur_c = (in->touch_x - 96) / 16;
        app->st.cur_r = (in->touch_y - 56) / 16;
        picr_apply_action(&app->st, app->st.cur_r, app->st.cur_c, PICR_CELL_FILLED);
    }
    if (in->down & PK_BTN_A) {
        picr_apply_action(&app->st, app->st.cur_r, app->st.cur_c, PICR_CELL_FILLED);
    }
    if (in->down & PK_BTN_X) {
        picr_apply_action(&app->st, app->st.cur_r, app->st.cur_c, PICR_CELL_CROSS);
    }
    if (app->st.solved) {
        snprintf(app->status_msg, sizeof(app->status_msg),
                 "SOLVED %s! MOVES:%u MISTAKES:%u (PRESS R FOR NEXT)",
                 app->st.title, (unsigned)app->st.moves, (unsigned)app->st.mistakes);
    }
}

static void app_render(const picr_app_t *app) {
    pk_render_begin_frame();
    pk_render_begin_target(PK_TARGET_TOP_LEFT, PK_RGBA8(16, 22, 38, 255));
    pk_draw_rect(0, 0, 400, 26, PK_RGBA8(34, 52, 92, 255));
    pk_draw_text(8, 7, 0.48f, PK_RGBA8(255, 225, 95, 255),
                 "PICROSS 3DS - NONOGRAM LOGIC STUDIO");

    char line[128];
    snprintf(line, sizeof(line), "PUZZLE [%d/%d]: %s | MOVES:%u | MISTAKES:%u",
             app->st.puzzle_idx + 1, PICR_PUZZLE_COUNT, app->st.title,
             (unsigned)app->st.moves, (unsigned)app->st.mistakes);
    pk_draw_text(10, 34, 0.45f, PK_RGBA8(170, 230, 255, 255), line);

    /* Top screen preview canvas (140x140 at x=130, y=58) */
    pk_draw_rect(128, 56, 144, 144, PK_RGBA8(28, 38, 62, 255));
    pk_draw_rect_outline(128, 56, 144, 144, 1.0f, PK_RGBA8(90, 140, 220, 255));
    for (int r = 0; r < PICR_SIZE; r++) {
        for (int c = 0; c < PICR_SIZE; c++) {
            uint32_t col = (app->st.grid[r][c] == PICR_CELL_FILLED)
                               ? PK_RGBA8(95, 245, 165, 255)
                               : PK_RGBA8(20, 28, 46, 255);
            pk_draw_rect((float)(130 + c * 14), (float)(58 + r * 14), 12.0f, 12.0f, col);
        }
    }

    pk_draw_rect(0, 216, 400, 24, PK_RGBA8(24, 36, 62, 255));
    pk_draw_text(8, 222, 0.44f, PK_RGBA8(120, 250, 175, 255), app->status_msg);

    /* Bottom Screen (320x240): Stylus Nonogram Grid + Row/Col Clues */
    pk_render_begin_target(PK_TARGET_BOTTOM, PK_RGBA8(12, 16, 28, 255));
    for (int r = 0; r < PICR_SIZE; r++) {
        char rclue[24] = {0};
        size_t pos = 0;
        for (uint8_t k = 0; k < app->st.row_clue_len[r]; k++) {
            pos += (size_t)snprintf(rclue + pos, sizeof(rclue) - pos, "%u ",
                                    (unsigned)app->st.row_clues[r][k]);
        }
        pk_draw_text(12.0f, (float)(59 + r * 16), 0.42f, PK_RGBA8(215, 230, 255, 255), rclue);

        for (int c = 0; c < PICR_SIZE; c++) {
            float x = (float)(96 + c * 16);
            float y = (float)(56 + r * 16);
            picr_cell_t cell = app->st.grid[r][c];
            uint32_t bg = (cell == PICR_CELL_FILLED)
                              ? PK_RGBA8(75, 225, 145, 255)
                              : (cell == PICR_CELL_CROSS ? PK_RGBA8(125, 45, 55, 255)
                                                         : PK_RGBA8(28, 38, 60, 255));
            pk_draw_rect(x, y, 14.0f, 14.0f, bg);
            if (r == app->st.cur_r && c == app->st.cur_c) {
                pk_draw_rect_outline(x, y, 14.0f, 14.0f, 1.0f, PK_RGBA8(255, 230, 80, 255));
            }
        }
    }
    pk_draw_text(10.0f, 222.0f, 0.42f, PK_RGBA8(180, 205, 235, 255),
                 "STYLUS TAP OR A:FILL | X:MARK | L/R:PUZZLE");
    pk_render_end_frame();
}

int main(int argc, char **argv) {
    const char *sd_root = NULL;
    bool one_shot = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--sd-root") == 0 && i + 1 < argc) sd_root = argv[++i];
        else if (strcmp(argv[i], "--once") == 0) one_shot = true;
    }
    if (!pk_platform_init()) return 1;
    picr_app_t app;
    app_init(&app, sd_root);
    while (pk_platform_main_loop()) {
        pk_input_t in;
        pk_platform_poll_input(&in);
        if (in.down & PK_BTN_START) break;
        app_step(&app, &in);
        app_render(&app);
        if (one_shot) break;
    }
    pk_platform_shutdown();
    return 0;
}
