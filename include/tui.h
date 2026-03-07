#ifndef ___TUI_H___
#define ___TUI_H___

#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif
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
#define TUI_MAX_PLUGINS (16)

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
  TUI_SECTION_STATUS_LOG,
  TUI_SECTION_PLUGINS
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
  long                timestamp;
  long                duration_ms;
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
  WINDOW              *win_plugins;
  WINDOW              *win_nav;
  WINDOW              *win_modal;
  _Bool               op_executed;
  /* plugin panel state */
  int                 plugin_selected;
  int                 plugin_scroll;
  int                 plugin_count;
  char                plugin_names[TUI_MAX_PLUGINS][BUF_LEN_XS];
  char                plugin_paths[TUI_MAX_PLUGINS][BUF_LEN_M];
  _Bool               plugin_loaded;
  char                active_plugin_key[BUF_LEN_XS];
  _Bool               plugins_discovered;
} TUIState_t;


void tui_init(TUIState_t *state);
void tui_run(TUIState_t *state, int argc, char **argv);
void tui_shutdown(TUIState_t *state);
TUIState_t *get_tui_state(void);

void tui_push_log(TUIState_t *state, TUILogLevel_t level, const char *fmt, ...);
void tui_push_op_log(TUIState_t *state, TUILogLevel_t level, const char *fmt, ...);


#endif /* ___TUI_H___ */
