#ifndef ___PLUGIN_CONTEXT_H___
#define ___PLUGIN_CONTEXT_H___


// external library headers

// standard library headers
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//internal library headers
#include "globals.h"

//macro defs
#define DEFAULT_STREAM_BUFFER_SIZE (BUF_LEN * BUF_LEN) // 1 MB


typedef struct mongoc_client_t MongoClient_t;
typedef struct PGconn PGconn_t;
typedef struct MYSQL MYSQL_t;

/**
 * Transient resources allocated per operation (backup/restore/...)
 * @dump_pipe: stdout pipe from backup command
 * @dump_pipe: stdin pipe from restore command
 * @buffer: the buffer shared between command - a temporary in-memory storage for R-W chunks
 * @buffer_size: the size of buffer held
 */
typedef struct PluginTransientState {
  FILE           *dump_pipe;
  FILE           *restore_pipe;
  char           *buffer;
  size_t         buffer_size;
} PluginTransientState_t;

/**
 * Persistent resources kept across plugin lifetime
 * @db_type: the database type
 * @db_handle: the handle that's owned to us after connection
 * @connected: a status flag
 */
typedef struct PluginPersistentState {
  DBType_t                        db_type;
  union {
    MongoClient_t    *mongo_client;
    PGconn_t         *pg_conn;
    MYSQL_t          *mysql_conn;
  }                               db_handle;
  _Bool                           connected;
} PluginPersistentState_t;

/**
 * Overall plugin state (persistent + transient)
 */
typedef struct PluginState {
  PluginPersistentState_t  persistent;
  PluginTransientState_t   transient;
} PluginState_t;


PluginState_t *create_plugin_state_from_type(DBType_t db_type);

void reset_plugin_transient_state(PluginState_t *state);
void destroy_plugin_state(PluginState_t *state);


#endif /* ___PLUGIN_CONTEXT_H___ */
