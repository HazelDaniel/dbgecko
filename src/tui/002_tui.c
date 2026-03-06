#include <string.h>
#include <time.h>
#include <sys/time.h>
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
  int max_y, max_x, op_log_h, op_log_w, status_log_h, status_log_w, header_h, nav_h;
  
  getmaxyx(stdscr, max_y, max_x);
  erase();
  wnoutrefresh(stdscr);
  
  header_h = 3;
  nav_h = 2;
  status_log_h = 7;
  
  // Guard against very small terminal windows
  if (max_y < header_h + nav_h + 4) {
    status_log_h = 2;
  }
  
  op_log_h = max_y - header_h - status_log_h - nav_h;
  if (op_log_h < 2) op_log_h = 2;
  
  op_log_w = max_x;
  status_log_w = max_x;
  
  /* draw header bar on stdscr */
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
  mvprintw(1, 2, " %s ", section_names[0]);
  mvhline(header_h - 1, 0, ACS_HLINE, max_x);
  
  wnoutrefresh(stdscr);
  
  /* draw operation log panel (main portion) */
  if (state->win_op_log) delwin(state->win_op_log);
  
  state->win_op_log = newwin(op_log_h, op_log_w, header_h, 0);
  
  if (state->active_section == TUI_SECTION_OP_LOG) {
    widget_draw_bordered_window(state->win_op_log, section_names[0], CP_ACCENT);
  } else {
    widget_draw_bordered_window(state->win_op_log, section_names[0], CP_BORDER);
  }
  
  draw_log_ring(state->win_op_log, &state->op_log, state->op_log_scroll, op_log_h - 2, op_log_w - 4);
  wnoutrefresh(state->win_op_log);
  
  /* draw status log panel (bottom portion above navigation) */
  if (state->win_status_log) delwin(state->win_status_log);
  
  state->win_status_log = newwin(status_log_h, status_log_w, header_h + op_log_h, 0);
  
  if (state->active_section == TUI_SECTION_STATUS_LOG) {
    widget_draw_bordered_window(state->win_status_log, section_names[1], CP_ACCENT);
  } else {
    widget_draw_bordered_window(state->win_status_log, section_names[1], CP_BORDER);
  }
  
  draw_log_ring(state->win_status_log, &state->status_log, state->status_log_scroll, status_log_h - 2, status_log_w - 4);
  wnoutrefresh(state->win_status_log);
  
  /* draw navigation guide at very bottom */
  if (state->win_nav) delwin(state->win_nav);
  // nav_h is 2, so starting at max_y - nav_h
  state->win_nav = newwin(nav_h, max_x, max_y - nav_h, 0);
  
  wattron(state->win_nav, COLOR_PAIR(CP_BORDER) | A_DIM);
  mvwhline(state->win_nav, 0, 0, ACS_HLINE, max_x);
  mvwprintw(state->win_nav, 1, 2, "[Tab] Switch Panel  [j/k] Scroll  [Esc] Back  [q] Quit");
  wattroff(state->win_nav, COLOR_PAIR(CP_BORDER) | A_DIM);
  
  wnoutrefresh(state->win_nav);
  
  /* update screen atomically */
  doupdate();
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

static void tui_execute_operation(TUIState_t *state) {
  StackErrorMessage_t err = NULL;
  StackStatus_t status;
  AppConfig_t *cfg = *get_app_config_handle();
  DriverStatus_t op_status = OP_FAIL;
  PluginRegistry_t *reg = NULL;
  PluginDriver_t *driver = NULL;
  struct timeval start_time, end_time;
  long duration_ms = 0;

  state->progress_measurable = false;
  state->op_status_text[0] = '\0';

  tui_push_log(state, LOG_INFO, "Loading plugin for %s...", cfg->db->type);

  status = load_plugin(&err, NULL);
  if (status != EXEC_SUCCESS) {
    tui_push_op_log(state, LOG_ERROR, "Plugin load failed: %s", err ? err : "unknown error");
    tui_push_log(state, LOG_ERROR, "Operation aborted");
    if (err) free(err);
    state->op_executed = true;
    return;
  }

  reg = get_plugin_registry_entry(NULL);
  if (!reg || !reg->driver) {
    tui_push_op_log(state, LOG_ERROR, "Plugin invalid or missing driver");
    state->op_executed = true;
    return;
  }

  driver = reg->driver;
  tui_push_log(state, LOG_INFO, "Plugin '%s' loaded (v%.1f)", driver->name, (double)driver->version);

  tui_push_op_log(state, LOG_INFO, "Connecting...");
  if (driver->connect(cfg, &err) != OP_SUCCESS) {
    tui_push_op_log(state, LOG_ERROR, "Connect failed: %s", err ? err : "unknown error");
    if (err) free(err);
    state->op_executed = true;
    return;
  }

  tui_push_op_log(state, LOG_INFO, "Executing operation...");
  gettimeofday(&start_time, NULL);

  switch (state->menu_selected) {
    case TUI_OP_BACKUP:
      op_status = driver->backup(cfg, &err);
      break;
    case TUI_OP_RESTORE:
      op_status = driver->restore(cfg, &err);
      break;
    default:
      tui_push_op_log(state, LOG_WARN, "Operation not implemented yet");
      op_status = OP_SUCCESS;
      break;
  }

  gettimeofday(&end_time, NULL);
  duration_ms = (end_time.tv_sec - start_time.tv_sec) * 1000 + (end_time.tv_usec - start_time.tv_usec) / 1000;

  if (op_status != OP_SUCCESS) {
    tui_push_op_log(state, LOG_ERROR, "Operation failed: %s", err ? err : "unknown error");
    if (err) free(err);
  } else {
    tui_push_op_log(state, LOG_INFO, "Operation completed successfully");
    tui_push_log(state, LOG_INFO, "Success");
  }

  // Update the duration of the execution log entry.
  // We assume the second to last entry in the op_log is the "Executing operation..." one, 
  // or we just attach duration to the completion log.
  // Actually, attaching it to the latest log is cleaner:
  if (state->op_log.count > 0) {
    size_t last_idx = (state->op_log.head - 1 + TUI_MAX_LOG_ENTRIES) % TUI_MAX_LOG_ENTRIES;
    state->op_log.entries[last_idx].duration_ms = duration_ms;
  }

  state->op_executed = true;
}

void tui_run(TUIState_t *state, int argc, char **argv) {
  int ch;

  while (state->running) {
    switch (state->screen) {
      case TUI_SCREEN_STARTUP:
        tui_draw_startup_screen(state);
        break;

      case TUI_SCREEN_OPERATION:
        state->spinner_frame = (state->spinner_frame + 1) % TUI_SPINNER_FRAMES;
        draw_operation_screen(state);
        if (!state->op_executed) {
          tui_execute_operation(state);
          draw_operation_screen(state);
        }
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
