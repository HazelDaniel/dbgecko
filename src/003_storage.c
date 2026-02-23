#include <libssh/sftp.h>
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <stdint.h>
#include "include/storage_sftp.h"
#include "include/config_parser.h"
#include "include/plugin_context.h"
#include "include/storage.h"

// fixing ubsan error (for some reason, it couldn't detect the O_DIRECTORY exported from fcntl.h)
#ifndef O_DIRECTORY
#define O_DIRECTORY (0x10000)
#endif

#define set_error_from_sftp(s, err)\
{\
  char *err_buf = NULL;\
  if (sftp_get_error(s->session) == 0)\
  set_err((const char **)err, BUF_LEN_S, "%s failed: %s", __func__, ssh_get_error(err_buf));\
else\
  set_err((const char **)err, BUF_LEN_S, "%s failed: unknown", __func__);\
}

/** -------------------------------------------------------------------------------------
 * Overview (local fs write):
 * write_open ->  opens temp file, initializes LocalFSState_t.fd.
 * write_chunk -> writes chunks of data to the open file descriptor, updates bytes_transferred.
 * write_close -> fsync, close, rename temp → final (all-or-nothing semantics).
 * write_abort -> cleanup temp file if an error occurs.
 ----------------------------------------------------------------------------------------
 */

 static int fsync_parent_dir(SFTPState_t *s);
 int sftp_mkdir_p(SFTPState_t *s, const char *path);


/*
 * sftp__write_open - set's up the starting state for writing file chunks
 *    open a temp file for writing. Creates parent dirs if necessary.
 *    ctx->state must be LocalFSState_t*.
 * @ctx: storage context for the sftp protocol
 * @rel_path: the relative path of the file
 * @err: the error message to propagate (in the event of a fail)
 * return: StorageStatus_t
 *
 */
 StorageStatus_t sftp__write_open(const StorageContext_t *ctx, const char *rel_path, StorageErrorMessage_t *err) {
   if (!ctx || !ctx->state || !rel_path) {
     set_err((const char **)err, BUF_LEN_XS, "invalid args to %s", __func__);
     return STORAGE_WRITE_FAILED;
   }

   SFTPState_t *s = (SFTPState_t *)ctx->state;
   AppConfig_t *cfg = *get_app_config_handle();
   const char *base_dir = cfg->storage->base_dir;
   char parent_dir[BUF_LEN_S];

   if (join_path(base_dir, rel_path, s->final_path, sizeof(s->final_path)) != 0) {
     set_err((const char **)err, BUF_LEN_XS, "path too long");

     return STORAGE_WRITE_FAILED;
   }

   pid_t pid = getpid();

   if (snprintf(s->tmp_path, sizeof(s->tmp_path), "%s.%d", s->final_path, (int)pid) >= (int)sizeof(s->tmp_path)) {
     set_err((const char **)err, BUF_LEN_XS, "tmp path too long");

     return STORAGE_WRITE_FAILED;
   }

   strncpy(parent_dir, s->final_path, sizeof(parent_dir));
   parent_dir[sizeof(parent_dir)-1] = '\0';
   char *last_slash = strrchr(parent_dir, '/');

   if (last_slash) {
     *last_slash = '\0';
     if (sftp_mkdir_p(s, parent_dir) != 0) {
       set_error_from_sftp(s, err);

       return STORAGE_MKDIR_FAILED;
     }
   }

   s->file = sftp_open(s->session, s->tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

   if (!s->file) {
     set_error_from_sftp(s, err);

     return STORAGE_WRITE_FAILED;
   }

   s->byte_transferred = 0;

   return STORAGE_OK;
 }

/*
 * sftp__write_chunk - write file in chunks using an in-memory buffer for each chunk
 * @ctx: storage context for the sftp protocol
 * @buf: the buffer holding the chunk contents
 * @err: the error message to propagate (in the event of a fail)
 * return: StorageStatus_t
 */
 StorageStatus_t sftp__write_chunk(const StorageContext_t *ctx, const void *buf, size_t len, StorageErrorMessage_t *err) {
   if (!ctx || !ctx->state || !buf) {
     set_err((const char **)err, BUF_LEN_XS, "invalid arguments violating invariant!");
     return STORAGE_WRITE_FAILED;
   }

   SFTPState_t *s = (SFTPState_t *)ctx->state;

   if (!s->file) {
     set_err((const char **)err, BUF_LEN_XS, "no open file for write on sftp instance");
     return STORAGE_WRITE_FAILED;
   }

   const uint8_t *p = (const uint8_t *)buf;
   size_t remaining = len;

   while (remaining > 0) {
     ssize_t w = sftp_write(s->file, p, remaining);

     if (w < 0) {
       set_error_from_sftp(s, err);
       return STORAGE_WRITE_FAILED;
     }

     p += (size_t)w;
     remaining -= (size_t)w;
     s->byte_transferred += (size_t)w;
   }

   return STORAGE_OK;
 }

/*
* sftp__write_close - finalize the chunked writes: fsync file, close, fsync parent dir, rename tmp -> final
* @ctx: storage context for the sftp protocol
* @tmp_path_override: the path
* @err: the error message to propagate (in the event of a fail)
* return: StorageStatus_t
*/
StorageStatus_t sftp__write_close(const StorageContext_t *ctx, StorageErrorMessage_t *err) {
  if (!ctx || !ctx->state) {
    set_err((const char **)err, BUF_LEN_XS, "invalid args to %s", __func__);
    return STORAGE_WRITE_FAILED;
  }

  SFTPState_t *s = (SFTPState_t *)ctx->state;

  if (s->file) {
    if (sftp_fsync(s->file) != SSH_OK) {
      set_error_from_sftp(s, err); sftp_close(s->file); s->file = NULL;
      sftp_unlink(s->session, s->tmp_path);

      return STORAGE_WRITE_FAILED;
    }

    if (sftp_close(s->file) != SSH_OK) {
      set_error_from_sftp(s, err); s->file = NULL;
      sftp_unlink(s->session, s->tmp_path);

      return STORAGE_WRITE_FAILED;
    }

    s->file = NULL;
  }

  if (sftp_rename(s->session, s->tmp_path, s->final_path) != SSH_OK) {
    set_error_from_sftp(s, err);
    sftp_unlink(s->session, s->tmp_path);

    return STORAGE_WRITE_FAILED;
  }

  s->byte_transferred = 0;
  s->tmp_path[0] = '\0';
  s->final_path[0] = '\0';

  return STORAGE_OK;
}

/*
* sftp__write_abort -  close and unlink tmp file on failed atomic writes
* @ctx: storage context for the local protocol
* @tmp_path_override: the path to the temporary file used to overwrite the orignial
* @err: the error message to propagate (in the event of a fail)
* return: StorageStatus_t
*/
StorageStatus_t sftp__write_abort(const StorageContext_t *ctx, const char *tmp_path_override, StorageErrorMessage_t *err) {
  if (!ctx || !ctx->state || !tmp_path_override) {
    set_err((const char **)err, BUF_LEN_XS, "invalid arguments to %s", __func__);
    return STORAGE_UNKNOWN_ERR;
  }

  SFTPState_t *s = (SFTPState_t *)ctx->state;

  if (s->file) {
    sftp_close(s->file);
    s->file = NULL;
  }

  if (sftp_unlink(s->session, tmp_path_override) != SSH_OK) {
    set_error_from_sftp(s, err);
    return STORAGE_UNKNOWN_ERR;
  }

  s->tmp_path[0] = '\0';
  s->final_path[0] = '\0';
  s->byte_transferred = 0;

  return STORAGE_OK;
}

/*
 * sftp__read_file -  read file and write into a sink callback:
 * @ctx: storage context for the local protocol
 * @rel_path: the name of the file to read
 * @sink: a stateful callback function that stores the read chunks and processes it independently
 * @local_state: the state for the callback function
 * @err: the error message to propagate (in the event of a fail)
 * return: StorageStatus_t
 */
StorageStatus_t sftp__read_file(const StorageContext_t *ctx, const char *rel_path, StorageDataSource sink,
  void *sink_userdata, StorageErrorMessage_t *err) {
  AppConfig_t *cfg = *get_app_config_handle();
  char full[BUF_LEN_S];
  SFTPState_t *s = (SFTPState_t *)ctx->state;
  const char *base_dir = cfg->storage->base_dir;
  uint8_t buffer[DEFAULT_STREAM_BUFFER_SIZE];
  sftp_file fd;
  ssize_t r;

  if (join_path(base_dir, rel_path, full, sizeof(full)) != 0) {
    set_err((const char **)err, BUF_LEN_XS, "path too long");
    return STORAGE_READ_FAILED;
  }

  fd = sftp_open(s->session, full, O_RDONLY, 0);

  if (!fd) {
    set_error_from_sftp(s, err);
    return STORAGE_READ_FAILED;
  }

  for (;;) {
    r = sftp_read(fd, buffer, sizeof(buffer));

    if (r < 0) {
      set_error_from_sftp(s, err);

      return STORAGE_READ_FAILED;
    }

    if (r == 0) break; // EOF

    StorageStatus_t sstat = STORAGE_OK;
    ssize_t sent = sink(sink_userdata, buffer, (size_t)r, &sstat);

    if (sstat != STORAGE_OK || sent < 0) {
      set_err((const char **)err, BUF_LEN_XS, "sink callback failed to write");
      sftp_close(fd);

      return sstat != STORAGE_OK ? sstat : STORAGE_READ_FAILED;
    }

    if ((size_t)sent != (size_t)r) {
      set_err((const char **)err, BUF_LEN_XS, "partial sink write");
      sftp_close(fd);

      return STORAGE_READ_FAILED;
    }
  }

  sftp_close(fd);

  return STORAGE_OK;
}

StorageStatus_t sftp__mkdir(const StorageContext_t *ctx, const char *rel_path, StorageErrorMessage_t *err) {
  if (!ctx || !ctx->state || !rel_path) {
    set_err((const char **)err, BUF_LEN_XS, "invalid arguments to %s", __func__);
    return STORAGE_MKDIR_FAILED;
  }

  SFTPState_t *s = (SFTPState_t *)ctx->state;
  char full[BUF_LEN_S], *base_dir = NULL;
  AppConfig_t *cfg = *get_app_config_handle();
  char *err_buff = NULL;
  int rc, sftp_err;

  base_dir = cfg->storage->base_dir;

  if (join_path(base_dir, rel_path, full, sizeof(full)) != 0) {
    set_err((const char **)err, BUF_LEN_XS, "path too long");
    return STORAGE_MKDIR_FAILED;
  }


  rc = sftp_mkdir(s->session, full, 0755);

  if (rc != SSH_OK) {
    int e = sftp_get_error(s->session);
    if (e != SSH_FX_FILE_ALREADY_EXISTS) {
      set_error_from_sftp(s, err);
      return STORAGE_MKDIR_FAILED;
    }
  }

  return STORAGE_OK;
}

StorageStatus_t sftp__delete_file(const StorageContext_t *ctx, const char *rel_path, StorageErrorMessage_t *err) {
  if (!ctx || !ctx->state || !rel_path) {
    set_err((const char **)err, BUF_LEN_XS, "invalid arguments to %s", __func__);
    return STORAGE_FILE_NOT_FOUND;
  }

  SFTPState_t *s = (SFTPState_t *)ctx->state;
  char full[BUF_LEN_S], *base_dir = NULL;
  AppConfig_t *cfg = *get_app_config_handle();
  char *err_buf = NULL;
  int sftp_err;

  base_dir = cfg->storage->base_dir;
  if (join_path(base_dir, rel_path, full, sizeof(full)) != 0) {
    set_err((const char **)err, BUF_LEN_XS, "path too long");

    return STORAGE_FILE_NOT_FOUND;
  }

  if (sftp_unlink(s->session, full) != SSH_OK) {
    int e = sftp_get_error(s->session);
    if (e == SSH_FX_NO_SUCH_FILE) return STORAGE_FILE_NOT_FOUND;
    set_error_from_sftp(s, err);

    return STORAGE_UNKNOWN_ERR;
  }

  return STORAGE_OK;
}

_Bool sftp__file_exists(const StorageContext_t *const ctx, const char *rel_path, StorageErrorMessage_t *err) {
  if (!ctx || !ctx->state || !rel_path) {
    set_err((const char **)err, BUF_LEN_XS, "invalid arguments to %s", __func__);
    return false;
  }

  AppConfig_t *cfg = *get_app_config_handle();

  const SFTPState_t *s = (const SFTPState_t *)ctx->state;
  char full[BUF_LEN_S], *base_dir = NULL;
  sftp_attributes st;

  base_dir = cfg->storage->base_dir;

  if (join_path(base_dir, rel_path, full, sizeof(full)) != 0) return false;
  if (!(st = sftp_stat(s->session, full))) return false;
  if (sftp_get_error(s->session) == SSH_FX_NO_SUCH_FILE) return false;

  sftp_attributes_free(st);

  return true;
}

/**
 * fsync_parent_dir - best effort approach to sync parent directory of the final path
 * @s: the sftp state
 * Returns: 0 indicating success, -1 otherwise
 */
static int fsync_parent_dir(SFTPState_t *s) {
  char dirpath[BUF_LEN_S];
  sftp_file dfd = NULL;
  int r = 0, r_ = 0;
  char *last_slash = NULL;

  if (!s) return -1;

  char *path = s->final_path;

  strncpy(dirpath, path, sizeof(dirpath));
  dirpath[sizeof(dirpath)-1] = '\0';
  last_slash = strrchr(dirpath, '/');

  if (!last_slash) {
    dfd = sftp_open(s->session, ".", O_DIRECTORY | O_RDONLY, 0644);
    if (!dfd) return -1;
    int r = sftp_fsync(dfd);
    sftp_close(dfd);

    return r;
  }

  if (last_slash == dirpath) {
    last_slash[1] = '\0';
  } else {
    *last_slash = '\0';
  }

  dfd = sftp_open(s->session, dirpath, O_DIRECTORY | O_RDONLY, 0644);

  if (!dfd) return -1;
  r = sftp_fsync(dfd);
  r_ = sftp_close(dfd);

  return r < r_ ? r : r_;
}

int sftp_mkdir_p(SFTPState_t *s, const char *path) {
  char tmp[BUF_LEN_S];
  char *p = NULL;

  strncpy(tmp, path, sizeof(tmp));
  tmp[sizeof(tmp) - 1] = '\0';

  for (p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';

      if (sftp_mkdir(s->session, tmp, 0755) != SSH_OK) {
        int e = sftp_get_error(s->session);
        if (e != SSH_FX_FILE_ALREADY_EXISTS) {
          // const char *err = ssh_get_error(s->parent_state);
          // printf("err is %s\n", err);
          // puts("--------------------couldn't mkdir recursively and the file doesn't exist with sftp");
          return -1;
        }
      }

      *p = '/';
    }
  }

  if (sftp_mkdir(s->session, tmp, 0755) != SSH_OK) {
    int e = sftp_get_error(s->session);
    if (e != SSH_FX_FILE_ALREADY_EXISTS) {
      // puts("--------------------couldn't mkdir and the file doesn't exist with sftp");
      return -1;
    }
  }

  return 0;
}

#undef set_error_from_sftp
