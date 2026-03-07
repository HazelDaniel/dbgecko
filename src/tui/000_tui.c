#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <locale.h>
#include <time.h>
#include <sys/time.h>
#include "include/tui.h"
#include "include/tui_widgets.h"
#include "include/config_parser.h"


/* color pair identifiers */
#define CP_NORMAL    1
#define CP_ERROR     2
#define CP_WARNING   3
#define CP_ACCENT    4
#define CP_HIGHLIGHT 5
#define CP_BORDER    6
#define CP_LOGO      7
#define CP_SUCCESS   8


static TUIState_t *g_tui_state = NULL;

static void init_color_pairs() {
  start_color();
  use_default_colors();

  if (can_change_color()) {
    // Attempt to set a modern Dracula-esque palette
    // 1000 scale is used by ncurses for RGB
    init_color(COLOR_CYAN,     545, 843, 886);   // #8BE9FD (Cyan)
    init_color(COLOR_GREEN,    314, 980, 482);   // #50FA7B (Green)
    init_color(COLOR_YELLOW,   945, 980, 549);   // #F1FA8C (Yellow)
    init_color(COLOR_RED,      1000, 333, 333);  // #FF5555 (Red)
    init_color(COLOR_BLUE,     745, 576, 976);   // #BD93F9 (Purple/Blue-ish)
    init_color(COLOR_WHITE,    973, 973, 949);   // #F8F8F2 (Foreground)
  }

  init_pair(CP_NORMAL,    COLOR_WHITE,   -1);
  init_pair(CP_ERROR,     COLOR_RED,     -1);
  init_pair(CP_WARNING,   COLOR_YELLOW,  -1);
  init_pair(CP_ACCENT,    COLOR_CYAN,    -1);
  init_pair(CP_HIGHLIGHT, COLOR_BLACK,   COLOR_CYAN);
  init_pair(CP_BORDER,    COLOR_WHITE,   -1);
  init_pair(CP_LOGO,      COLOR_GREEN,   -1);
  init_pair(CP_SUCCESS,   COLOR_GREEN,   -1);
}

void tui_init(TUIState_t *state) {
  g_tui_state = state;
  setlocale(LC_ALL, "");

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  timeout(100);

  init_color_pairs();

  memset(state, 0, sizeof(TUIState_t));
  state->screen = TUI_SCREEN_STARTUP;
  state->active_section = TUI_SECTION_MENU;
  state->menu_selected = 0;
  state->config_loaded = false;
  state->modal_active = false;
  state->running = true;
  state->spinner_frame = 0;
  state->progress = 0.0f;
  state->progress_measurable = false;
  state->op_log_scroll = 0;
  state->status_log_scroll = 0;
  state->win_main = NULL;
  state->win_header = NULL;
  state->win_op_log = NULL;
  state->win_status_log = NULL;
  state->win_plugins = NULL;
  state->win_modal = NULL;
  state->op_executed = false;
  state->plugin_selected = 0;
  state->plugin_scroll = 0;
  state->plugin_count = 0;
  state->plugin_loaded = false;
  state->plugins_discovered = false;
  state->active_plugin_key[0] = '\0';

  state->status_log.head = 0;
  state->status_log.count = 0;
  state->op_log.head = 0;
  state->op_log.count = 0;
}

void tui_shutdown(TUIState_t *state) {
  if (state->win_modal) delwin(state->win_modal);
  if (state->win_status_log) delwin(state->win_status_log);
  if (state->win_op_log) delwin(state->win_op_log);
  if (state->win_plugins) delwin(state->win_plugins);
  if (state->win_header) delwin(state->win_header);
  if (state->win_main) delwin(state->win_main);

  endwin();
  g_tui_state = NULL;
}

TUIState_t *get_tui_state(void) {
  return g_tui_state;
}

void tui_push_log(TUIState_t *state, TUILogLevel_t level, const char *fmt, ...) {
  TUILogRing_t *ring = &state->status_log;
  TUILogEntry_t *entry = &ring->entries[ring->head];
  va_list ap;

  entry->level = level;
  entry->timestamp = time(NULL);
  entry->duration_ms = 0;

  va_start(ap, fmt);
  vsnprintf(entry->message, TUI_LOG_MSG_LEN, fmt, ap);
  va_end(ap);

  ring->head = (ring->head + 1) % TUI_MAX_LOG_ENTRIES;

  if (ring->count < TUI_MAX_LOG_ENTRIES) {
    ring->count++;
  }
}

void tui_push_op_log(TUIState_t *state, TUILogLevel_t level, const char *fmt, ...) {
  TUILogRing_t *ring = &state->op_log;
  TUILogEntry_t *entry = &ring->entries[ring->head];
  va_list ap;

  entry->level = level;
  entry->timestamp = time(NULL);
  entry->duration_ms = 0; // Duration updated elsewhere if needed

  va_start(ap, fmt);
  vsnprintf(entry->message, TUI_LOG_MSG_LEN, fmt, ap);
  va_end(ap);

  ring->head = (ring->head + 1) % TUI_MAX_LOG_ENTRIES;

  if (ring->count < TUI_MAX_LOG_ENTRIES) {
    ring->count++;
  }
}
