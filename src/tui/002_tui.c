#include <string.h>
#include "include/tui.h"
#include "include/tui_widgets.h"
#include "include/config_parser.h"
#include "include/plugin_manager.h"


/* color pair references */
#define CP_NORMAL    1
#define CP_ERROR     2
#define CP_WARNING   3
#define CP_ACCENT    4
#define CP_HIGHLIGHT 5
#define CP_BORDER    6
#define CP_SUCCESS   8

/* forward declarations from 001_tui.c */
extern void tui_draw_startup_screen(TUIState_t *state);
extern int tui_handle_startup_input(TUIState_t *state, int ch);
extern _Bool tui_try_default_config(TUIState_t *state);

static const char *section_names[] = {
  "Operation Log",
  "Status Log"
};


static void draw_log_ring(WINDOW *win, TUILogRing_t *ring, int scroll, int max_lines, int max_w) {
  int draw_count, start_idx, i;
  size_t idx;

  if (!ring->count) {
    wattron(win, A_DIM | COLOR_PAIR(CP_ACCENT));
    mvwprintw(win, 1, 2, "No entries yet...");
    wattroff(win, A_DIM | COLOR_PAIR(CP_ACCENT));

    return;
  }

  draw_count = (int)ring->count;

  if (draw_count > max_lines) {
    draw_count = max_lines;
  }

  start_idx = (int)ring->count - draw_count - scroll;

  if (start_idx < 0) start_idx = 0;

  for (i = 0; i < draw_count; i++) {
    idx = ((ring->head - ring->count + (size_t)start_idx + (size_t)i) + TUI_MAX_LOG_ENTRIES * 2) % TUI_MAX_LOG_ENTRIES;
    widget_draw_log_entry(win, i + 1, &ring->entries[idx], max_w);
  }
}

static void draw_operation_screen(TUIState_t *state) {
  int max_y, max_x, op_log_h, op_log_w, status_log_h, status_log_w;
  int header_h, split_x;

  getmaxyx(stdscr, max_y, max_x);
  erase();

  header_h = 3;
  split_x = max_x * 2 / 3;
  op_log_h = max_y - header_h;
  op_log_w = split_x;
  status_log_h = max_y - header_h;
  status_log_w = max_x - split_x;

  /* draw header bar */
  attron(COLOR_PAIR(CP_BORDER));
  mvhline(0, 0, ' ', max_x);
  attron(A_BOLD);
  mvprintw(0, 2, " dbgecko ");
  attroff(A_BOLD);

  if (state->op_status_text[0]) {
    mvprintw(0, 14, "| %s ", state->op_status_text);
  }

  /* spinner or progress on header */
  if (state->progress_measurable) {
    widget_draw_progress(stdscr, 0, max_x - 40, state->progress);
  } else if (state->op_status_text[0]) {
    widget_draw_spinner(stdscr, 0, max_x - 6, state->spinner_frame);
  }

  attroff(COLOR_PAIR(CP_BORDER));

  mvhline(1, 0, ACS_HLINE, max_x);
  mvprintw(1, 2, " Operation ");
  mvhline(header_h - 1, 0, ACS_HLINE, max_x);

  /* draw operation log panel (left) */
  if (state->win_op_log) delwin(state->win_op_log);

  state->win_op_log = newwin(op_log_h, op_log_w, header_h, 0);

  if (state->active_section == TUI_SECTION_OP_LOG) {
    widget_draw_bordered_window(state->win_op_log, section_names[0], CP_ACCENT);
  } else {
    widget_draw_bordered_window(state->win_op_log, section_names[0], CP_BORDER);
  }

  draw_log_ring(state->win_op_log, &state->op_log,
    state->op_log_scroll, op_log_h - 2, op_log_w - 4);

  wrefresh(state->win_op_log);

  /* draw status log panel (right) */
  if (state->win_status_log) delwin(state->win_status_log);

  state->win_status_log = newwin(status_log_h, status_log_w, header_h, split_x);

  if (state->active_section == TUI_SECTION_STATUS_LOG) {
    widget_draw_bordered_window(state->win_status_log, section_names[1], CP_ACCENT);
  } else {
    widget_draw_bordered_window(state->win_status_log, section_names[1], CP_BORDER);
  }

  draw_log_ring(state->win_status_log, &state->status_log,
    state->status_log_scroll, status_log_h - 2, status_log_w - 4);

  wrefresh(state->win_status_log);

  /* keybinding hints at bottom of header */
  attron(COLOR_PAIR(CP_BORDER) | A_DIM);
  mvprintw(2, 2, "[Tab] Switch Panel  [j/k] Scroll  [Esc] Back  [q] Quit");
  attroff(COLOR_PAIR(CP_BORDER) | A_DIM);

  refresh();
}

static int handle_operation_input(TUIState_t *state, int ch) {
  switch (ch) {
    case '\t':
      if (state->active_section == TUI_SECTION_OP_LOG) {
        state->active_section = TUI_SECTION_STATUS_LOG;
      } else {
        state->active_section = TUI_SECTION_OP_LOG;
      }
      break;

    case 'k':
    case KEY_UP:
      if (state->active_section == TUI_SECTION_OP_LOG && state->op_log_scroll < (int)state->op_log.count - 1) {
        state->op_log_scroll++;
      } else if (state->active_section == TUI_SECTION_STATUS_LOG && state->status_log_scroll < (int)state->status_log.count - 1) {
        state->status_log_scroll++;
      }
      break;

    case 'j':
    case KEY_DOWN:
      if (state->active_section == TUI_SECTION_OP_LOG && state->op_log_scroll > 0) {
        state->op_log_scroll--;
      } else if (state->active_section == TUI_SECTION_STATUS_LOG && state->status_log_scroll > 0) {
        state->status_log_scroll--;
      }
      break;

    case 27: /* ESC */
      state->screen = TUI_SCREEN_STARTUP;
      state->active_section = TUI_SECTION_MENU;
      break;

    case 'q':
      state->running = false;
      break;

    default:
      break;
  }

  return -1;
}

void tui_run(TUIState_t *state, int argc, char **argv) {
  StackError_t *err = NULL;
  int ch;

  /* try loading config from CLI args first */
  if (argc > 1) {
    merge_configs(argc, argv, &err);

    if (!err) {
      state->config_loaded = true;
      tui_push_log(state, LOG_INFO, "Configuration loaded from CLI arguments");
    } else {
      tui_push_log(state, LOG_WARN, "CLI config: %s", err->message);
      destroy_stack_error(err);
      err = NULL;
    }
  }

  /* try default config path if CLI didn't provide one */
  if (!state->config_loaded) {
    if (tui_try_default_config(state)) {
      state->config_loaded = true;
    }
  }

  while (state->running) {
    switch (state->screen) {
      case TUI_SCREEN_STARTUP:
        tui_draw_startup_screen(state);
        break;

      case TUI_SCREEN_OPERATION:
        state->spinner_frame = (state->spinner_frame + 1) % TUI_SPINNER_FRAMES;
        draw_operation_screen(state);
        break;
    }

    ch = getch();

    if (ch == ERR) continue;

    if (ch == KEY_RESIZE) continue;

    switch (state->screen) {
      case TUI_SCREEN_STARTUP:
        tui_handle_startup_input(state, ch);
        break;

      case TUI_SCREEN_OPERATION:
        handle_operation_input(state, ch);
        break;
    }
  }
}
