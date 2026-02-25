#ifndef ___TUI_H___
#define ___TUI_H___

// standard library headers
#include <ncurses.h>
#include <stddef.h>
#include <stdbool.h>

// internal library headers
#include "globals.h"

// macro defs
#define TUI_MAX_LOG_ENTRIES (128)
#define TUI_LOG_MSG_LEN (BUF_LEN_S)
#define TUI_MAX_MENU_ITEMS (16)
#define TUI_SPINNER_FRAMES (10)
#define TUI_DEFAULT_CONFIG_PATH_FMT ("%s/.config/dbgecko/config.yml")
#define TUI_INPUT_BUF_LEN (BUF_LEN_S)

typedef enum {
  LOG_INFO,
  LOG_WARN,
  LOG_ERROR
} TUILogLevel_t;

typedef enum {
  TUI_SCREEN_STARTUP,
  TUI_SCREEN_OPERATION
} TUIScreen_t;

typedef enum {
  TUI_SECTION_MENU,
  TUI_SECTION_OP_LOG,
  TUI_SECTION_STATUS_LOG
} TUISection_t;

typedef enum {
  TUI_OP_BACKUP,
  TUI_OP_RESTORE,
  TUI_OP_LIST,
  TUI_OP_VERIFY,
  TUI_OP_INFO,
  TUI_OP_QUIT,
  TUI_OP_COUNT
} TUIOperation_t;

typedef struct TUILogEntry {
  TUILogLevel_t       level;
  char                message[TUI_LOG_MSG_LEN];
} TUILogEntry_t;

typedef struct TUILogRing {
  TUILogEntry_t       entries[TUI_MAX_LOG_ENTRIES];
  size_t              head;
  size_t              count;
} TUILogRing_t;

typedef struct TUIState {
  TUIScreen_t         screen;
  TUISection_t        active_section;
  int                 menu_selected;
  int                 op_log_scroll;
  int                 status_log_scroll;
  _Bool               config_loaded;
  _Bool               modal_active;
  _Bool               running;
  int                 spinner_frame;
  float               progress;
  _Bool               progress_measurable;
  char                op_status_text[BUF_LEN_S];
  TUILogRing_t        status_log;
  TUILogRing_t        op_log;
  WINDOW              *win_main;
  WINDOW              *win_header;
  WINDOW              *win_op_log;
  WINDOW              *win_status_log;
  WINDOW              *win_modal;
} TUIState_t;


void tui_init(TUIState_t *state);
void tui_run(TUIState_t *state, int argc, char **argv);
void tui_shutdown(TUIState_t *state);

void tui_push_log(TUIState_t *state, TUILogLevel_t level, const char *fmt, ...);
void tui_push_op_log(TUIState_t *state, TUILogLevel_t level, const char *fmt, ...);


#endif /* ___TUI_H___ */
