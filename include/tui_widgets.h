#ifndef ___TUI_WIDGETS_H___
#define ___TUI_WIDGETS_H___

// standard library headers
#include <ncurses.h>
#include <stddef.h>

// internal library headers
#include "tui.h"

// macro defs
#define WIDGET_PROGRESS_BAR_WIDTH (30)


void widget_draw_spinner(WINDOW *win, int y, int x, int frame);
void widget_draw_progress(WINDOW *win, int y, int x, float pct);
void widget_draw_log_entry(WINDOW *win, int y, TUILogEntry_t *entry, int max_w);
void widget_draw_modal(TUIState_t *state, const char *title, const char *msg);
int  widget_draw_input_modal(TUIState_t *state, const char *title, const char *prompt, char *buf, size_t len);
void widget_draw_menu(WINDOW *win, const char **items, int count, int selected, int start_y, int start_x);
void widget_draw_logo(WINDOW *win, int start_y, int start_x);
void widget_draw_gecko_logo(WINDOW *win, int start_y, int start_x);
void widget_draw_bordered_window(WINDOW *win, const char *title, int color_pair);
void widget_draw_plugin_list(WINDOW *win, TUIState_t *state);


#endif /* ___TUI_WIDGETS_H___ */
