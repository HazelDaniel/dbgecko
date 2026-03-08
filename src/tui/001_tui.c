#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "include/tui.h"
#include "include/tui_widgets.h"
#include "include/config_parser.h"


/* color pair references (defined in 000_tui.c) */
#define CP_NORMAL    1
#define CP_ERROR     2
#define CP_ACCENT    4
#define CP_HIGHLIGHT 5
#define CP_BORDER    6
#define CP_LOGO      7

static const char *menu_items[] = {
  "  Backup Database  ",
  "  Restore Database ",
  "  List Backups     ",
  "  Verify Backup    ",
  "  System Info      ",
  "  Quit             "
};

static const int menu_count = TUI_OP_COUNT;


static _Bool try_load_config_from_path(TUIState_t *state, const char *path) {
  AppConfig_t **app_cfg = get_app_config_handle();
  ConfigParserError_t *cfg_err = NULL;
  ConfigParserStatus_t status;

  if (!path || !path[0]) return false;

  if (access(path, R_OK) != 0) {
    tui_push_log(state, LOG_ERROR, "Config file not accessible: %s", path);

    return false;
  }

  if (!*app_cfg) {
    DBConfig_t *cfg_db = init_db_config(DEFAULT_DB_TYPE, DEFAULT_DB_URI,
      DEFAULT_DB_BACKUP_MODE, DEFAULT_DB_TIMEOUT);
    StorageConfig_t *cfg_storage = init_storage_config(DEFAULT_STORAGE_OUTPUT_NAME,
      DEFAULT_STORAGE_COMPRESSION, DEFAULT_STORAGE_ENC_KEY_PATH, DEFAULT_STORAGE_REMOTE);
    RuntimeConfig_t *cfg_runtime = init_runtime_config(DEFAULT_RUNTIME_LOG_LEVEL,
      DEFAULT_RUNTIME_THREAD_COUNT, DEFAULT_RUNTIME_TMP_DIR, NULL, NULL);
    PlatformConfig_t *cfg_platform = init_platform_config(DEFAULT_PLATFORM_VERSION);
    PluginConfig_t *cfg_plugin = init_plugin_config(DEFAULT_PLUGIN_DIR_PATH, DEFAULT_PLUGIN_PATH);
    AppConfig_t *cfg = init_app_config(cfg_db, cfg_storage, cfg_runtime, cfg_platform, cfg_plugin);

    set_app_config(cfg);
  }

  status = config_load_file(path, *app_cfg, &cfg_err);

  if (status != CONFIG_OK) {
    if (cfg_err) {
      tui_push_log(state, LOG_ERROR, "Config error: %s", cfg_err->message);
      destroy_parser_error(&cfg_err);
    } else {
      tui_push_log(state, LOG_ERROR, "Unknown config parse error");
    }

    return false;
  }

  tui_push_log(state, LOG_INFO, "Configuration loaded: %s", path);

  return true;
}

_Bool tui_try_default_config(TUIState_t *state) {
  char default_path[BUF_LEN_S];
  const char *home = getenv("HOME");

  if (!home) return false;

  snprintf(default_path, sizeof(default_path), TUI_DEFAULT_CONFIG_PATH_FMT, home);

  if (access(default_path, R_OK) == 0) {
    return try_load_config_from_path(state, default_path);
  }

  return false;
}

_Bool tui_prompt_config_path(TUIState_t *state) {
  char path_buf[TUI_INPUT_BUF_LEN] = {0};
  int result;

  result = widget_draw_input_modal(state,
    " Configuration Required ",
    "Enter config file path: ",
    path_buf, sizeof(path_buf));

  if (result == 0 && path_buf[0]) {
    return try_load_config_from_path(state, path_buf);
  }

  return false;
}

void tui_draw_startup_screen(TUIState_t *state) {
  int max_y, max_x, logo_start_y, logo_start_x, menu_start_y, menu_start_x;
  int status_y;

  getmaxyx(stdscr, max_y, max_x);
  erase();

  /* draw brand name centered near top */
  logo_start_y = 2;
  logo_start_x = (max_x / 2) - 19;
  if (logo_start_x < 0) logo_start_x = 0;

  widget_draw_brand_name(stdscr, logo_start_y, logo_start_x);

  /* draw menu centered below brand */
  menu_start_y = logo_start_y + 11;
  menu_start_x = (max_x / 2) - 12;

  if (menu_start_x < 0) menu_start_x = 0;

  if (!state->config_loaded) {
    attron(COLOR_PAIR(CP_ERROR));
    mvprintw(menu_start_y, menu_start_x - 2, "[ No configuration loaded ]");
    attroff(COLOR_PAIR(CP_ERROR));
    menu_start_y += 2;

    attron(COLOR_PAIR(CP_ACCENT));
    mvprintw(menu_start_y, menu_start_x - 2, "Press [c] to load config file");
    attroff(COLOR_PAIR(CP_ACCENT));
    menu_start_y += 2;
  }

  widget_draw_menu(stdscr, menu_items, menu_count,
    state->menu_selected, menu_start_y, menu_start_x);

  /* draw keybinding hints at bottom */
  status_y = max_y - 2;

  attron(COLOR_PAIR(CP_BORDER) | A_DIM);
  mvhline(status_y - 1, 0, ACS_HLINE, max_x);
  attroff(A_DIM);

  mvprintw(status_y, 2, "[j/k] Navigate  [Enter] Select  [c] Load Config  [q] Quit");
  attroff(COLOR_PAIR(CP_BORDER));

  /* config status indicator */
  if (state->config_loaded) {
    attron(COLOR_PAIR(CP_ACCENT));
    mvprintw(0, max_x - 18, " Config: OK ");
    attroff(COLOR_PAIR(CP_ACCENT));
  } else {
    attron(COLOR_PAIR(CP_ERROR));
    mvprintw(0, max_x - 22, " Config: MISSING ");
    attroff(COLOR_PAIR(CP_ERROR));
  }

  refresh();
}

int tui_handle_startup_input(TUIState_t *state, int ch) {
  switch (ch) {
    case 'k':
    case KEY_UP:
      if (state->menu_selected > 0) {
        state->menu_selected--;
      }
      break;

    case 'j':
    case KEY_DOWN:
      if (state->menu_selected < menu_count - 1) {
        state->menu_selected++;
      }
      break;

    case '\n':
    case KEY_ENTER:
      if (state->menu_selected == TUI_OP_QUIT) {
        state->running = false;

        return 0;
      }

      if (!state->config_loaded) {
        tui_push_log(state, LOG_WARN, "Please load a configuration first");

        break;
      }

      if (state->menu_selected == TUI_OP_BACKUP || state->menu_selected == TUI_OP_RESTORE) {
        AppConfig_t *cfg = *get_app_config_handle();
        char path_buf[TUI_INPUT_BUF_LEN];
        strncpy(path_buf, cfg->storage->output_name, sizeof(path_buf));
        
        int res = widget_draw_input_modal(state, 
          state->menu_selected == TUI_OP_BACKUP ? " Backup Destination " : " Restore Source ",
          "Filename: ", path_buf, sizeof(path_buf));
          
        if (res == 0 && path_buf[0]) {
          strncpy(cfg->storage->output_name, path_buf, sizeof(cfg->storage->output_name));
          tui_push_log(state, LOG_INFO, "Target filename set to: %s", path_buf);
        } else {
          return -1; // Canceled
        }
      }

      state->screen = TUI_SCREEN_OPERATION;
      state->active_section = TUI_SECTION_OP_LOG;
      state->op_executed = false;
      tui_push_op_log(state, LOG_INFO, "Operation selected: %s",
        menu_items[state->menu_selected]);

      return state->menu_selected;

    case 'c':
      tui_prompt_config_path(state);
      break;

    case 'q':
      state->running = false;
      break;

    default:
      break;
  }

  return -1;
}
