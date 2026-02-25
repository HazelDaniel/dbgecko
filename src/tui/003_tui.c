#include <string.h>
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

  wattron(win, COLOR_PAIR(color) | A_BOLD);
  mvwprintw(win, y, 2, "%s", prefix);
  wattroff(win, A_BOLD);

  wattron(win, COLOR_PAIR(CP_NORMAL));
  wprintw(win, " %.*s", max_w - 8, entry->message);
  wattroff(win, COLOR_PAIR(CP_NORMAL));

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
      mvwprintw(win, start_y + i * 2, start_x, " ▸ %s", items[i]);
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
  cursor_pos = 0;
  memset(buf, 0, len);

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

void widget_draw_logo(WINDOW *win, int start_y, int start_x) {
  wattron(win, COLOR_PAIR(CP_LOGO) | A_BOLD);

  mvwprintw(win, start_y,     start_x, "     ____  ____   ____           _         ");
  mvwprintw(win, start_y + 1, start_x, "    |  _ \\| __ ) / ___| ___  ___| | _____  ");
  mvwprintw(win, start_y + 2, start_x, "    | | | |  _ \\| |  _ / _ \\/ __| |/ / _ \\ ");
  mvwprintw(win, start_y + 3, start_x, "    | |_| | |_) | |_| |  __/ (__|   < (_) |");
  mvwprintw(win, start_y + 4, start_x, "    |____/|____/ \\____|\\___|\\___||_|\\_\\___/ ");

  wattroff(win, A_BOLD);

  wattron(win, COLOR_PAIR(CP_ACCENT) | A_DIM);
  mvwprintw(win, start_y + 6, start_x + 8, "Database Backup & Restore Utility");
  mvwprintw(win, start_y + 7, start_x + 14, "v%.1f", CURRENT_PLATFORM_VERSION);
  wattroff(win, COLOR_PAIR(CP_ACCENT) | A_DIM);

  wattroff(win, COLOR_PAIR(CP_LOGO));
}
