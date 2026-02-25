#include <stdbool.h>
#include <stdlib.h>
#include "include/globals.h"
#include "include/tui.h"

int main(int argc, char **argv)
{
  const char *headless = getenv("DBGECKO_HEADLESS");
  TUIState_t state;

  /* headless mode: skip TUI for CI / scripted environments */
  if (headless && headless[0] == '1') {
    return EXEC_SUCCESS;
  }

  tui_init(&state);
  tui_run(&state, argc, argv);
  tui_shutdown(&state);

  return EXEC_SUCCESS;
}
