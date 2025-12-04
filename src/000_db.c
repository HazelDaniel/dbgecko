#include "include/plugin_context.h"


PluginState_t *create_plugin_state_from_type(DBType_t db_type) {
  PluginState_t *state = (PluginState_t *)calloc(1, sizeof(PluginState_t));

  if (!state) return NULL;

  state->persistent.db_type = db_type;
  state->persistent.connected = false;
  state->transient.dump_pipe = NULL;
  state->transient.restore_pipe = NULL;
  state->transient.buffer_size = DEFAULT_STREAM_BUFFER_SIZE;
  state->transient.buffer = (char *)malloc(state->transient.buffer_size);

  if (!state->transient.buffer) {
    free(state);

    return NULL;
  }

  return state;
}

void reset_plugin_transient(PluginState_t *state) {
  if (!state) return;

  if (state->transient.dump_pipe) {
    fclose(state->transient.dump_pipe);
    state->transient.dump_pipe = NULL;
  }

  if (state->transient.restore_pipe) {
    fclose(state->transient.restore_pipe);
    state->transient.restore_pipe = NULL;
  }

  if (state->transient.buffer) {
    free(state->transient.buffer);
    state->transient.buffer = NULL;
  }

  state->transient.buffer_size = 0;
}

void destroy_plugin_state(PluginState_t *state) {
  if (!state) return;

  switch (state->persistent.db_type) {
    case DB_TYPE_MONGO:
      if (state->persistent.db_handle.mongo_client) {
        // mongoc_client_destroy(state->persistent.db_handle.mongo_client);
        state->persistent.db_handle.mongo_client = NULL;
      }
      break;
    case DB_TYPE_POSTGRES:
      if (state->persistent.db_handle.pg_conn) {
        // PQfinish(state->persistent.db_handle.pg_conn);
        state->persistent.db_handle.pg_conn = NULL;
      }
      break;
    case DB_TYPE_MYSQL:
      if (state->persistent.db_handle.mysql_conn) {
        // mysql_close(state->persistent.db_handle.mysql_conn);
        state->persistent.db_handle.mysql_conn = NULL;
      }
      break;
    default:
      break;
  }

  reset_plugin_transient(state);
  free(state);
}
