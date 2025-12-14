#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <stdint.h>
#include "include/storage_local.h"
#include "include/config_parser.h"
#include "include/plugin_context.h"
#include "include/storage.h"

/** -------------------------------------------------------------------------------------
 * Overview (local fs write):
 * write_open ->  opens temp file, initializes LocalFSState_t.fd.
 * write_chunk -> writes chunks of data to the open file descriptor, updates bytes_transferred.
 * write_close -> fsync, close, rename temp → final (all-or-nothing semantics).
 * write_abort -> cleanup temp file if an error occurs.
 ----------------------------------------------------------------------------------------
 */


/**
 * mkdir_p -  ensure directory exists (create parents as needed)
 * @path: the path on which to create a new directory
 * Return: int
 */
int mkdir_p(const char *path) {
  if (!path || path[0] == '\0') return -1;
  char tmp[BUF_LEN_S];
  size_t len;

  strncpy(tmp, path, sizeof(tmp));
  tmp[sizeof(tmp)-1] = '\0';
  len = strlen(tmp);
  if (len == 0) return -1;

  if (tmp[len-1] == '/') tmp[len-1] = '\0';

  for (char *p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir(tmp, 0755) != 0) {
        if (errno != EEXIST) return -1;
      }
      *p = '/';
    }
  }
  if (mkdir(tmp, 0755) != 0) {
    if (errno != EEXIST) return -1;
  }

  return 0;
}

int fsync_parent_dir(const char *path) {
  char dirpath[BUF_LEN_S];
  int dfd = -1 , r = 0;
  char *last_slash = NULL;

  strncpy(dirpath, path, sizeof(dirpath));
  dirpath[sizeof(dirpath)-1] = '\0';
  last_slash = strrchr(dirpath, '/');

  if (!last_slash) {
    dfd = open(".", O_DIRECTORY | O_RDONLY);
    if (dfd < 0) return -1;
    int r = fsync(dfd);
    close(dfd);

    return r;
  }

  if (last_slash == dirpath) {
    last_slash[1] = '\0';
  } else {
    *last_slash = '\0';
  }

  dfd = open(dirpath, O_DIRECTORY | O_RDONLY);

  if (dfd < 0) return -1;
  r = fsync(dfd); close(dfd);

  return r;
}

/*
 * local_fs__write_open - set's up the starting state for writing file chunks
 *    open a temp file for writing. Creates parent dirs if necessary.
 *    ctx->state must be LocalFSState_t*.
 * @ctx: storage context for the local protocol
 * @rel_path: the relative path of the file
 * @err: the error message to propagate (in the event of a fail)
 * return: StorageStatus_t
 *
 */
StorageStatus_t local_fs__write_open(const StorageContext_t *ctx, const char *rel_path, StorageErrorMessage_t *err) {
  if (!ctx || !ctx->state || !rel_path) {
    set_err((const char **)err, BUF_LEN_S, "invalid arguments violating invariant");
    return STORAGE_WRITE_FAILED;
  }

  LocalFSState_t *s = (LocalFSState_t *)ctx->state;
  char parent_dir[BUF_LEN_S], *base_dir = NULL;
  AppConfig_t *cfg = *get_app_config_handle();

  base_dir = cfg->storage->backend->backend.local.base_dir;

  if (join_path(base_dir, rel_path, s->final_path, sizeof(s->final_path)) != 0) {
    set_err((const char **)err, BUF_LEN_XS, "path too long");
    return STORAGE_WRITE_FAILED;
  }

  strncpy(parent_dir, s->final_path, sizeof(parent_dir));
  parent_dir[sizeof(parent_dir)-1] = '\0';
  char *last_slash = strrchr(parent_dir, '/');

  if (last_slash) {
    *last_slash = '\0';
    if (mkdir_p(parent_dir) != 0 && errno != EEXIST) {
      set_err((const char **)err, BUF_LEN_S, "failed to create parent directories for %s: %s", s->final_path, strerror(errno));

      return STORAGE_MKDIR_FAILED;
    }
  }

  pid_t pid = getpid();
  if (snprintf(s->tmp_path, sizeof(s->tmp_path), "%s.%d", s->final_path, (int)pid) >= (int)sizeof(s->tmp_path)) {
    set_err((const char **)err, BUF_LEN_XS, "tmp path too long");
    return STORAGE_WRITE_FAILED;
  }

  int fd = open(s->tmp_path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  if (fd < 0) {
    set_err((const char **)err, BUF_LEN_S, "failed to open tmp file %s: %s", s->tmp_path, strerror(errno));
    return STORAGE_WRITE_FAILED;
  }

  s->fd = fd;
  s->bytes_transferred = 0;

  return STORAGE_OK;
}

/*
 * local_fs__write_chunk - write file in chunks using an in-memory buffer for each chunk
 * @ctx: storage context for the local protocol
 * @buf: the buffer holding the chunk contents
 * @err: the error message to propagate (in the event of a fail)
 * return: StorageStatus_t
 */
StorageStatus_t local_fs__write_chunk(const StorageContext_t *ctx, const void *buf, size_t len,
  StorageErrorMessage_t *err) {
  if (!ctx || !ctx->state || !buf) {
    set_err((const char **)err, BUF_LEN_XS, "invalid arguments violating invariant!");

    return STORAGE_WRITE_FAILED;
  }

  LocalFSState_t *s = (LocalFSState_t *)ctx->state;
  ssize_t w = 0;

  if (s->fd < 0) {
    set_err((const char **)err, BUF_LEN_XS, "no open file descriptor");
    return STORAGE_WRITE_FAILED;
  }

  const uint8_t *p = (const uint8_t *)buf;
  size_t remaining = len;

  while (remaining > 0) {
    w = write(s->fd, p, remaining);

    if (w < 0) {
      if (errno == EINTR) continue;
      set_err((const char **)err, BUF_LEN_S, "write failed: %s", strerror(errno));
      return STORAGE_WRITE_FAILED;
    }

    p += w; remaining -= (size_t)w;
    s->bytes_transferred += (size_t)w;
  }

  return STORAGE_OK;
}

/*
* local_fs__write_close - finalize the chunked writes: fsync file, close, fsync parent dir, rename tmp -> final
* @ctx: storage context for the local protocol
* @tmp_path_override: the path
* @err: the error message to propagate (in the event of a fail)
* return: StorageStatus_t
*/
StorageStatus_t local_fs__write_close(const StorageContext_t *ctx, StorageErrorMessage_t *err) {
  // tmp_path_override and final_path_override are passed from caller (storage_write_stream)
  if (!ctx || !ctx->state) {
    set_err((const char **)err, BUF_LEN_XS, "invalid args to %s", __func__);
    return STORAGE_WRITE_FAILED;
  }

  LocalFSState_t *s = (LocalFSState_t *)ctx->state;

  // If fd is open, fsync + close
  if (s->fd >= 0) {
    if (fsync(s->fd) != 0) {
      set_err((const char **)err, BUF_LEN_S, "fsync(tmp) failed: %s", strerror(errno));
      // fallthrough to abort cleanup below
      close(s->fd);
      s->fd = -1;
      unlink(s->tmp_path);

      return STORAGE_WRITE_FAILED;
    }

    if (close(s->fd) != 0) {
      set_err((const char **)err, BUF_LEN_S, "close(tmp) failed: %s", strerror(errno));
      s->fd = -1;
      unlink(s->tmp_path);

      return STORAGE_WRITE_FAILED;
    }
    s->fd = -1;
  }

  // fsync parent directory to ensure rename durability
  if (fsync_parent_dir(s->final_path) != 0) {
    // Not fatal on all filesystems, but warn
    set_err((const char **)err, BUF_LEN_S, "fsync(parent dir) failed: %s", strerror(errno));
    // Attempt rename anyway — if it fails, we report an error
  }

  // Do atomic rename
  if (rename(s->tmp_path, s->final_path) != 0) {
    set_err((const char **)err, BUF_LEN_S, "rename(%s -> %s) failed: %s", s->tmp_path, s->final_path, strerror(errno));
    unlink(s->tmp_path);

    return STORAGE_WRITE_FAILED;
  }

  // success
  s->bytes_transferred = 0;
  s->tmp_path[0] = '\0';
  s->final_path[0] = '\0';

  return STORAGE_OK;
}

/*
* local_fs__write_close -  close and unlink tmp file on failed atomic writes
* @ctx: storage context for the local protocol
* @tmp_path_override: the path to the temporary file used to overwrite the orignial
* @err: the error message to propagate (in the event of a fail)
* return: StorageStatus_t
*/
StorageStatus_t local_fs__write_abort(const StorageContext_t *ctx, const char *tmp_path_override, StorageErrorMessage_t *err) {
  if (!ctx || !ctx->state || !tmp_path_override) {
    set_err((const char **)err, BUF_LEN_XS, "invalid arguments violating invariant");
    return STORAGE_UNKNOWN_ERR;
  }

  LocalFSState_t *s = (LocalFSState_t *)ctx->state;
  if (s->fd >= 0) {
    close(s->fd);
    s->fd = -1;
  }

  if (unlink(tmp_path_override) != 0) {
    if (errno != ENOENT) {
      set_err((const char **)err, BUF_LEN_S, "failed to unlink tmp file %s: %s", tmp_path_override, strerror(errno));
      return STORAGE_UNKNOWN_ERR;
    }
  }

  s->tmp_path[0] = '\0';
  s->final_path[0] = '\0';
  s->bytes_transferred = 0;

  return STORAGE_OK;
}

/*
 * local_fs__read_file -  read file and write into a sink callback:
 * @ctx: storage context for the local protocol
 * @rel_path: the name of the file to read
 * @sink: a stateful callback function that stores the read chunks and processes it independently
 * @local_state: the state for the callback function
 * @err: the error message to propagate (in the event of a fail)
 * return: StorageStatus_t
 */
StorageStatus_t local_fs__read_file(const StorageContext_t *ctx, const char *rel_path, StorageDataSource sink,
  void *sink_userdata, StorageErrorMessage_t *err) {
  // if (!ctx || !ctx->state || !rel_path || !sink) { // currently handled in caller
  //   set_err((const char **)err, BUF_LEN_XS, "invalid args");
  //   return STORAGE_READ_FAILED;
  // }

  AppConfig_t *cfg = *get_app_config_handle();
  char full[BUF_LEN_S];
  const char *base_dir = cfg->storage->backend->backend.local.base_dir;
  uint8_t buffer[DEFAULT_STREAM_BUFFER_SIZE];
  int fd;
  ssize_t r;

  if (join_path(base_dir, rel_path, full, sizeof(full)) != 0) {
    set_err((const char **)err, BUF_LEN_XS, "path too long");
    return STORAGE_READ_FAILED;
  }

  fd = open(full, O_RDONLY | O_CLOEXEC);

  if (fd < 0) {
    set_err((const char **)err, BUF_LEN_S, "open(%s): %s", full, strerror(errno));
    return STORAGE_READ_FAILED;
  }

  for (;;) {
    r = read(fd, buffer, sizeof(buffer));

    if (r < 0) {
      if (errno == EINTR) continue;
      set_err((const char **)err, BUF_LEN_S, "read: %s", strerror(errno));
      close(fd);

      return STORAGE_READ_FAILED;
    }

    if (r == 0) break; // EOF

    StorageStatus_t sstat = STORAGE_OK;
    ssize_t sent = sink(sink_userdata, buffer, (size_t)r, &sstat);

    if (sstat != STORAGE_OK || sent < 0) {
      set_err((const char **)err, BUF_LEN_XS, "sink callback failed to write");
      close(fd);

      return sstat != STORAGE_OK ? sstat : STORAGE_READ_FAILED;
    }

    if ((size_t)sent != (size_t)r) {
      set_err((const char **)err, BUF_LEN_XS, "partial sink write");
      close(fd);

      return STORAGE_READ_FAILED;
    }
  }

  close(fd);

  return STORAGE_OK;
}

StorageStatus_t local_fs__mkdir(const StorageContext_t *ctx, const char *rel_path, StorageErrorMessage_t *err) {
  if (!ctx || !ctx->state || !rel_path) {
    set_err((const char **)err, BUF_LEN_XS, "invalid arguments violating invariant");
    return STORAGE_MKDIR_FAILED;
  }
  LocalFSState_t *s = (LocalFSState_t *)ctx->state;
  char full[BUF_LEN_S], *base_dir = NULL;
  AppConfig_t *cfg = *get_app_config_handle();

  base_dir = cfg->storage->backend->backend.local.base_dir;

  if (join_path(base_dir, rel_path, full, sizeof(full)) != 0) {
    set_err((const char **)err, BUF_LEN_XS, "path too long");
    return STORAGE_MKDIR_FAILED;
  }
  if (mkdir_p(full) != 0) {
    if (errno != EEXIST) {
      set_err((const char **)err, BUF_LEN_S, "mkdir_p(%s) failed: %s", full, strerror(errno));
      return STORAGE_MKDIR_FAILED;
    }
  }
  return STORAGE_OK;
}

StorageStatus_t local_fs__delete_file(const StorageContext_t *ctx, const char *rel_path, StorageErrorMessage_t *err) {
  if (!ctx || !ctx->state || !rel_path) {
    set_err((const char **)err, BUF_LEN_XS, "invalid arguments violating invariant");
    return STORAGE_FILE_NOT_FOUND;
  }
  LocalFSState_t *s = (LocalFSState_t *)ctx->state;
  char full[BUF_LEN_S], *base_dir = NULL;
  AppConfig_t *cfg = *get_app_config_handle();

  base_dir = cfg->storage->backend->backend.local.base_dir;
  if (join_path(base_dir, rel_path, full, sizeof(full)) != 0) {
    set_err((const char **)err, BUF_LEN_XS, "path too long");
    return STORAGE_FILE_NOT_FOUND;
  }
  if (unlink(full) != 0) {
    if (errno == ENOENT) return STORAGE_FILE_NOT_FOUND;
    set_err((const char **)err, BUF_LEN_S, "unlink(%s) failed: %s", full, strerror(errno));
    return STORAGE_UNKNOWN_ERR;
  }

  return STORAGE_OK;
}

_Bool local_fs__file_exists(const StorageContext_t *const ctx, const char *rel_path, StorageErrorMessage_t *err) {
  if (!ctx || !ctx->state || !rel_path) {
    return false;
  }

  AppConfig_t *cfg = *get_app_config_handle();

  const LocalFSState_t *s = (const LocalFSState_t *)ctx->state;
  char full[BUF_LEN_S], *base_dir = NULL;
  base_dir = cfg->storage->backend->backend.local.base_dir;
  if (join_path(base_dir, rel_path, full, sizeof(full)) != 0) {
    return false;
  }
  struct stat st;
  if (stat(full, &st) != 0) return false;
  return S_ISREG(st.st_mode);
}
