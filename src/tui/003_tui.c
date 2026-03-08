#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "include/tui.h"
#include "include/tui_widgets.h"


/* color pair references */
#define CP_NORMAL    1
#define CP_ERROR     2
#define CP_WARNING   3
#define CP_ACCENT    4
#define CP_HIGHLIGHT 5
#define CP_BORDER    6
#define CP_LOGO      7
#define CP_SUCCESS   8

static const char *spinner_frames[] = {
  "⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"
};


void widget_draw_spinner(WINDOW *win, int y, int x, int frame) {
  int idx = frame % TUI_SPINNER_FRAMES;

  wattron(win, COLOR_PAIR(CP_ACCENT) | A_BOLD);
  mvwprintw(win, y, x, "%s", spinner_frames[idx]);
  wattroff(win, COLOR_PAIR(CP_ACCENT) | A_BOLD);
}

void widget_draw_progress(WINDOW *win, int y, int x, float pct) {
  int filled, i;
  char pct_text[16];

  if (pct < 0.0f) pct = 0.0f;
  if (pct > 1.0f) pct = 1.0f;

  filled = (int)(pct * WIDGET_PROGRESS_BAR_WIDTH);
  snprintf(pct_text, sizeof(pct_text), "%3d%%", (int)(pct * 100));

  wattron(win, COLOR_PAIR(CP_BORDER));
  mvwprintw(win, y, x, "[");

  for (i = 0; i < WIDGET_PROGRESS_BAR_WIDTH; i++) {
    if (i < filled) {
      wattron(win, COLOR_PAIR(CP_ACCENT) | A_BOLD);
      wprintw(win, "█");
      wattroff(win, A_BOLD);
    } else {
      wattron(win, COLOR_PAIR(CP_NORMAL) | A_DIM);
      wprintw(win, "░");
      wattroff(win, A_DIM);
    }
  }

  wattron(win, COLOR_PAIR(CP_BORDER));
  wprintw(win, "] %s", pct_text);
  wattroff(win, COLOR_PAIR(CP_BORDER));
}

void widget_draw_log_entry(WINDOW *win, int y, TUILogEntry_t *entry, int max_w) {
  const char *prefix;
  int color;
  char time_str[16] = {0};
  char dur_str[32] = {0};
  int msg_max_w;

  if (!entry || !entry->message[0]) return;

  switch (entry->level) {
    case LOG_ERROR:
      prefix = "[ERR]";
      color = CP_ERROR;
      break;

    case LOG_WARN:
      prefix = "[WRN]";
      color = CP_WARNING;
      break;

    case LOG_INFO:
    default:
      prefix = "[INF]";
      color = CP_ACCENT;
      break;
  }

  if (entry->timestamp > 0) {
    struct tm *tm_info = localtime(&entry->timestamp);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
  }

  if (entry->duration_ms > 0) {
    snprintf(dur_str, sizeof(dur_str), " (%.2fs)", entry->duration_ms / 1000.0);
  }

  msg_max_w = max_w - 8 - strlen(time_str) - strlen(dur_str) - 3; 

  wattron(win, COLOR_PAIR(color) | A_BOLD);
  mvwprintw(win, y, 2, "%s", prefix);
  wattroff(win, A_BOLD);

  wattron(win, COLOR_PAIR(CP_NORMAL) | A_DIM);
  if (time_str[0]) {
    wprintw(win, " [%s]", time_str);
  }
  wattroff(win, A_DIM);

  wattron(win, COLOR_PAIR(CP_NORMAL));
  wprintw(win, " %.*s", msg_max_w > 0 ? msg_max_w : 0, entry->message);
  wattroff(win, COLOR_PAIR(CP_NORMAL));

  if (dur_str[0]) {
    wattron(win, COLOR_PAIR(CP_ACCENT) | A_DIM);
    wprintw(win, "%s", dur_str);
    wattroff(win, COLOR_PAIR(CP_ACCENT) | A_DIM);
  }

  wattroff(win, COLOR_PAIR(color));
}

void widget_draw_bordered_window(WINDOW *win, const char *title, int color_pair) {
  wattron(win, COLOR_PAIR(color_pair));
  box(win, 0, 0);

  if (title) {
    wattron(win, A_BOLD);
    mvwprintw(win, 0, 2, " %s ", title);
    wattroff(win, A_BOLD);
  }

  wattroff(win, COLOR_PAIR(color_pair));
}

void widget_draw_menu(WINDOW *win, const char **items, int count, int selected, int start_y, int start_x) {
  int i;

  for (i = 0; i < count; i++) {
    if (i == selected) {
      wattron(win, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
      mvwprintw(win, start_y + i * 2, start_x, " > %s", items[i]);
      wattroff(win, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
    } else {
      wattron(win, COLOR_PAIR(CP_NORMAL));
      mvwprintw(win, start_y + i * 2, start_x, "   %s", items[i]);
      wattroff(win, COLOR_PAIR(CP_NORMAL));
    }
  }
}

void widget_draw_modal(TUIState_t *state, const char *title, const char *msg) {
  int max_y, max_x, modal_h, modal_w, start_y, start_x;
  WINDOW *modal;

  getmaxyx(stdscr, max_y, max_x);

  modal_h = 7;
  modal_w = 50;

  if (modal_w > max_x - 4) modal_w = max_x - 4;

  start_y = (max_y - modal_h) / 2;
  start_x = (max_x - modal_w) / 2;

  if (state->win_modal) delwin(state->win_modal);

  modal = newwin(modal_h, modal_w, start_y, start_x);
  state->win_modal = modal;

  wattron(modal, COLOR_PAIR(CP_ERROR));
  box(modal, 0, 0);

  wattron(modal, A_BOLD);
  mvwprintw(modal, 0, 2, "%s", title);
  wattroff(modal, A_BOLD);

  wattroff(modal, COLOR_PAIR(CP_ERROR));

  wattron(modal, COLOR_PAIR(CP_NORMAL));
  mvwprintw(modal, 2, 2, "%.*s", modal_w - 4, msg);
  wattroff(modal, COLOR_PAIR(CP_NORMAL));

  wattron(modal, COLOR_PAIR(CP_ACCENT) | A_DIM);
  mvwprintw(modal, modal_h - 2, 2, "[Enter] OK  [Esc] Cancel");
  wattroff(modal, COLOR_PAIR(CP_ACCENT) | A_DIM);

  wrefresh(modal);
}

int widget_draw_input_modal(TUIState_t *state, const char *title, const char *prompt, char *buf, size_t len) {
  int max_y, max_x, modal_h, modal_w, start_y, start_x;
  int ch, cursor_pos;
  WINDOW *modal;

  getmaxyx(stdscr, max_y, max_x);

  modal_h = 9;
  modal_w = 60;

  if (modal_w > max_x - 4) modal_w = max_x - 4;

  start_y = (max_y - modal_h) / 2;
  start_x = (max_x - modal_w) / 2;

  if (state->win_modal) delwin(state->win_modal);

  modal = newwin(modal_h, modal_w, start_y, start_x);
  state->win_modal = modal;
  keypad(modal, TRUE);
  cursor_pos = strlen(buf);

  for (;;) {
    werase(modal);

    wattron(modal, COLOR_PAIR(CP_ACCENT));
    box(modal, 0, 0);

    wattron(modal, A_BOLD);
    mvwprintw(modal, 0, 2, "%s", title);
    wattroff(modal, A_BOLD);
    wattroff(modal, COLOR_PAIR(CP_ACCENT));

    /* prompt label */
    wattron(modal, COLOR_PAIR(CP_NORMAL));
    mvwprintw(modal, 2, 2, "%s", prompt);
    wattroff(modal, COLOR_PAIR(CP_NORMAL));

    /* input field background */
    wattron(modal, COLOR_PAIR(CP_NORMAL) | A_UNDERLINE);
    mvwprintw(modal, 4, 2, "%-*.*s", modal_w - 4, modal_w - 4, buf);
    wattroff(modal, A_UNDERLINE);

    /* show cursor */
    curs_set(1);
    wmove(modal, 4, 2 + cursor_pos);

    /* keybinding hints */
    wattron(modal, COLOR_PAIR(CP_ACCENT) | A_DIM);
    mvwprintw(modal, modal_h - 2, 2, "[Enter] Confirm  [Esc] Cancel");
    wattroff(modal, COLOR_PAIR(CP_ACCENT) | A_DIM);

    wrefresh(modal);

    ch = wgetch(modal);

    if (ch == '\n' || ch == KEY_ENTER) {
      curs_set(0);
      delwin(modal);
      state->win_modal = NULL;

      return 0;
    }

    if (ch == 27) { /* ESC */
      curs_set(0);
      buf[0] = '\0';
      delwin(modal);
      state->win_modal = NULL;

      return -1;
    }

    if ((ch == KEY_BACKSPACE || ch == 127 || ch == '\b') && cursor_pos > 0) {
      cursor_pos--;
      buf[cursor_pos] = '\0';
    } else if (ch >= 32 && ch < 127 && cursor_pos < (int)(len - 1)) {
      buf[cursor_pos] = (char)ch;
      cursor_pos++;
      buf[cursor_pos] = '\0';
    }
  }
}

void widget_draw_logo(WINDOW *win, int start_y, int start_x) { }
void widget_draw_gecko_logo(WINDOW *win, int start_y, int start_x) { }

void widget_draw_brand_name(WINDOW *win, int start_y, int start_x) {
  wattron(win, COLOR_PAIR(CP_LOGO) | A_BOLD);

  mvwprintw(win, start_y,     start_x, "      _  _                 _         ");
  mvwprintw(win, start_y + 1, start_x, "   __| || |__   __ _  ___ | | __ ___  ");
  mvwprintw(win, start_y + 2, start_x, "  / _` || '_ \\ / _` |/ _ \\| |/ // _ \\ ");
  mvwprintw(win, start_y + 3, start_x, " | (_| || |_) | (_| |  __/|   <| (_) |");
  mvwprintw(win, start_y + 4, start_x, "  \\__,_||_.__/ \\__, |\\___||_|\\_\\\\___/ ");
  mvwprintw(win, start_y + 5, start_x, "               |___/                  ");

  wattroff(win, COLOR_PAIR(CP_LOGO) | A_BOLD);
  
  wattron(win, COLOR_PAIR(CP_ACCENT) | A_DIM);
  mvwprintw(win, start_y + 7, start_x + 6, "Database Backup & Restore Utility");
  wattroff(win, COLOR_PAIR(CP_ACCENT) | A_DIM);
}

void widget_draw_plugin_list(WINDOW *win, TUIState_t *state) {
  int max_h, max_w, draw_count, i;

  getmaxyx(win, max_h, max_w);
  draw_count = max_h - 2; /* usable rows inside border */

  if (state->plugin_count == 0) {
    wattron(win, A_DIM | COLOR_PAIR(CP_ACCENT));
    mvwprintw(win, 1, 2, "No plugins found");
    wattroff(win, A_DIM | COLOR_PAIR(CP_ACCENT));
    return;
  }

  /* adjust scroll to keep selected visible */
  if (state->plugin_selected < state->plugin_scroll) {
    state->plugin_scroll = state->plugin_selected;
  }
  if (state->plugin_selected >= state->plugin_scroll + draw_count) {
    state->plugin_scroll = state->plugin_selected - draw_count + 1;
  }

  for (i = 0; i < draw_count && (state->plugin_scroll + i) < state->plugin_count; i++) {
    int idx = state->plugin_scroll + i;
    _Bool is_selected = (idx == state->plugin_selected);
    _Bool is_active = (state->plugin_loaded &&
      strcmp(state->active_plugin_key, state->plugin_names[idx]) == 0);
    int label_max = max_w - 6;

    if (is_selected) {
      wattron(win, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
      mvwprintw(win, i + 1, 1, " %s %.*s",
        is_active ? "●" : "▸",
        label_max > 0 ? label_max : 0,
        state->plugin_names[idx]);
      wattroff(win, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
    } else if (is_active) {
      wattron(win, COLOR_PAIR(CP_SUCCESS) | A_BOLD);
      mvwprintw(win, i + 1, 1, " * %.*s",
        label_max > 0 ? label_max : 0,
        state->plugin_names[idx]);
      wattroff(win, COLOR_PAIR(CP_SUCCESS) | A_BOLD);
    } else {
      wattron(win, COLOR_PAIR(CP_NORMAL));
      mvwprintw(win, i + 1, 1, "   %.*s",
        label_max > 0 ? label_max : 0,
        state->plugin_names[idx]);
      wattroff(win, COLOR_PAIR(CP_NORMAL));
    }
  }
}
