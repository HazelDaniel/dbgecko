#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>
#include "include/config_parser.h"

void print_storage_backend_config(StorageBackendConfig_t *bck) {
  if (!bck) return;
  char *left_pad_2 = "\t\t";

  printf("\t");

  switch (bck->kind) {
    case PTC_S3:
      puts("s3:");
      break;
    case PTC_SSH:
      puts("ssh:");
      break;
    case PTC_SFTP:
      puts("sftp:");
      break;
    default:
      puts("");
      return;
  }

  printf("%s{", left_pad_2);
  if (bck->kind == PTC_S3) {
    printf("%sendpoint: %s, ", " ", bck->backend.s3.endpoint);
    printf("%sregion: %s, ", " ", bck->backend.s3.region);
    printf("%saccess_key: %s, ", " ", bck->backend.s3.access_key);
    printf("%ssecret_key: %s, ", " ", bck->backend.s3.secret_key);
    printf("%suse_ssl: %s, ", " ", bck->backend.s3.use_ssl ? "true" : "false");
    printf("%spath_style: %s, ", " ", bck->backend.s3.path_style);
    printf("%ssession_token: %s, ", " ", bck->backend.s3.session_token);
    printf("%smax_retries: %zu, ", " ", bck->backend.s3.max_retries);
    printf("%stimeout_seconds: %zu, ", " ", bck->backend.s3.timeout_seconds);
    printf("%smultipart_threshold_mb: %zu, ", " ", bck->backend.s3.multipart_threshold_mb);
    printf("%smultipart_chunk_mb: %zu, ", " ", bck->backend.s3.multipart_chunk_mb);
  } else if (bck->kind == PTC_SFTP) {
    printf("%susername: %s, ", " ", bck->backend.sftp.username);
    printf("%sport: %zu, ", " ", bck->backend.sftp.port);
    printf("%shost: %s, ", " ", bck->backend.sftp.host);
    printf("%stimeout_seconds: %zu, ", " ", bck->backend.sftp.timeout_seconds);
    printf("%smax_retries: %zu, ", " ", bck->backend.sftp.max_retries);
    printf("%sprivate_key: %s, ", " ", bck->backend.sftp.private_key);
  } else if (bck->kind == PTC_SSH) {
    printf("%susername: %s, ", " ", bck->backend.ssh.username);
    printf("%sport: %zu, ", " ", bck->backend.ssh.port);
    printf("%shost: %s, ", " ", bck->backend.ssh.host);
    printf("%stimeout_seconds: %zu, ", " ", bck->backend.ssh.timeout_seconds);
    printf("%smax_retries: %zu, ", " ", bck->backend.ssh.max_retries);
    printf("%sprivate_key: %s, ", " ", bck->backend.ssh.private_key);
    printf("%sverify_known_hosts: %s, ", " ", bck->backend.ssh.verify_known_hosts ? "true" : "false");
  }
  printf("}\n");
}

void print_app_config(AppConfig_t *cfg) {
  if (!cfg) return;
  puts("db:");
  printf("\t timeout_seconds: %li\n", cfg->db->timeout_seconds);
  printf("\t type: %s\n", cfg->db->type);
  printf("\t backup_mode: %s\n", cfg->db->backup_mode);
  printf("\t uri: %s\n", cfg->db->uri);
  printf("\t online: %s\n", cfg->db->online ? "true" : "false");

  puts("runtime:");
  printf("\t log_level: %li\n", cfg->runtime->log_level);
  printf("\t tmp_dir: %s\n", cfg->runtime->temp_dir);
  printf("\t thread_count: %li\n", cfg->runtime->thread_count);

  puts("storage:");
  printf("\t compression: %s\n", cfg->storage->compression);
  printf("\t key_path: %s\n", cfg->storage->encryption_key_path);
  printf("\t output_path: %s\n", cfg->storage->output_path);
  printf("\t remote_target: %s\n", cfg->storage->remote_target);
  print_storage_backend_config(cfg->storage->backend);

  puts("platform:");
  printf("\t version: %.2f\n", cfg->platform->version);

  puts("plugin:");
  printf("\t dir: %s\n", cfg->plugin->dir);
  printf("\t path: %s\n", cfg->plugin->path);
}

int assign_yaml_parsed_value(config_section_t section, const char *key,
  const char *value, AppConfig_t *cfg, ConfigParserError_t *err) {
  long val = 0L;
  double val_float = 0.0f;
  // TODO: validate the config values

  if (section == SECTION_DB) {
    if (strcmp(key, "type") == 0) strncpy(cfg->db->type, value, BUF_LEN_XS);
    else if (strcmp(key, "uri") == 0) strncpy(cfg->db->uri, value, BUF_LEN_S);
    else if (strcmp(key, "backup_mode") == 0) strncpy(cfg->db->backup_mode, value, BUF_LEN_XS);
    else if (strcmp(key, "online") == 0 ) {
      if (strcmp(value, "true") == 0) cfg->db->online = true;
      else if (strcmp(value, "false") == 0) cfg->db->online = false;
    } else if (strcmp(key, "timeout_seconds") == 0) {
      val = strtol(value, NULL, 10);

      if (val <= 0) {
        err->code = CONFIG_VALIDATION_ERROR;
        snprintf(err->message, sizeof(err->message), "db->timeout_seconds must be > 0");

        return -1;
      }

      cfg->db->timeout_seconds = (int)val;
    } else {
      err->code = CONFIG_VALIDATION_ERROR;
      snprintf(err->message, sizeof(err->message), "Unknown db key: %s", key);

      return -1;
    }
  } else if (section == SECTION_STORAGE) {
    if (strcmp(key, "output_path") == 0) strncpy(cfg->storage->output_path, value, BUF_LEN_S);
    else if (strcmp(key, "compression") == 0) strncpy(cfg->storage->compression, value, BUF_LEN_XS);
    else if (strcmp(key, "remote_target") == 0) strncpy(cfg->storage->remote_target, value, BUF_LEN_XS);
    else if (strcmp(key, "encryption_key_path") == 0) strncpy(cfg->storage->encryption_key_path, value, BUF_LEN_S);
    else {
      err->code = CONFIG_VALIDATION_ERROR;
      snprintf(err->message, sizeof(err->message), "Unknown storage key: %s", key);

      return -1;
    }
  } else if (section == SECTION_PLATFORM) {
    if (strcmp(key, "version") == 0) {
      char *end = NULL;
      val_float = strtof(value, &end);
      if (*end != '\0') {
        err->code = CONFIG_VALIDATION_ERROR;
        snprintf(err->message, sizeof(err->message), "Invalid platform version: %.f%s", val_float, end);

        return -1;
      }
      cfg->platform->version = val_float;
    } else {
      err->code = CONFIG_VALIDATION_ERROR;
      snprintf(err->message, sizeof(err->message), "Unknown platform key: %s", key);

      return -1;
    }
  } else if (section == SECTION_PLUGIN) {
    if (strcmp(key, "dir") == 0) {
       strncpy(cfg->plugin->dir, value, BUF_LEN_S);
    } else if (strcmp(key, "path") == 0) {
       strncpy(cfg->plugin->path, value, BUF_LEN_M);
    } else {
      err->code = CONFIG_VALIDATION_ERROR;
      snprintf(err->message, sizeof(err->message), "Unknown plugin key: %s", key);

      return -1;
    }
  } else if (section == SECTION_RUNTIME) {
    if (strcmp(key, "log_level") == 0) {
      val = strtol(value, NULL, 10);
      cfg->runtime->log_level = (int)val;
    } else if (strcmp(key, "thread_count") == 0) {
      val = strtol(value, NULL, 10);
      cfg->runtime->thread_count = (int)val;
    } else if (strcmp(key, "tmp_dir") == 0) {
      strncpy(cfg->runtime->temp_dir, value, BUF_LEN_S);
    } else {
      err->code = CONFIG_VALIDATION_ERROR;
      snprintf(err->message, sizeof(err->message), "Unknown runtime key: %s", key);

      return -1;
    }
  } else if (section == SECTION_CHILD_SSH) {
    cfg->storage->backend->kind = PTC_SSH;
    if (strcmp(key, "private_key") == 0) {
      strncpy(cfg->storage->backend->backend.ssh.private_key, value, BUF_LEN_S);
    } else if (strcmp(key, "host") == 0) {
      strncpy(cfg->storage->backend->backend.ssh.host, value, BUF_LEN_XS - 1);
    } else if (strcmp(key, "port") == 0) {
      val = strtol(value, NULL, 10);
      cfg->storage->backend->backend.ssh.port = (size_t)val;
    } else if (strcmp(key, "max_retries") == 0) {
      val = strtol(value, NULL, 10);
      cfg->storage->backend->backend.ssh.max_retries = (size_t)val;
    } else if (strcmp(key, "timeout_seconds") == 0) {
      val = strtol(value, NULL, 10);
      cfg->storage->backend->backend.ssh.timeout_seconds = (size_t)val;
    } else if (strcmp(key, "verify_known_hosts") == 0 ) {
      if (strcmp(value, "true") == 0) cfg->storage->backend->backend.ssh.verify_known_hosts = true;
      else if (strcmp(value, "false") == 0) cfg->storage->backend->backend.ssh.verify_known_hosts = false;
    } else if (strcmp(key, "username") == 0) {
      strncpy(cfg->storage->backend->backend.ssh.username, value, BUF_LEN_S);
    } else {
      err->code = CONFIG_VALIDATION_ERROR;
      snprintf(err->message, sizeof(err->message), "Unknown ssh configuration key for storage: %s", key);

      return -1;
    }
  } else if (section == SECTION_CHILD_S3) {
    cfg->storage->backend->kind = PTC_S3;
    if (strcmp(key, "endpoint") == 0) {
      strncpy(cfg->storage->backend->backend.s3.endpoint, value, BUF_LEN_S);
    } else if (strcmp(key, "region") == 0) {
      strncpy(cfg->storage->backend->backend.s3.region, value, BUF_LEN_XS);
    } else if (strcmp(key, "access_key") == 0) {
      strncpy(cfg->storage->backend->backend.s3.access_key, value, BUF_LEN_S);
    } else if (strcmp(key, "secret_key") == 0) {
      strncpy(cfg->storage->backend->backend.s3.secret_key, value, BUF_LEN_S);
    } else if (strcmp(key, "use_ssl") == 0 ) {
      if (strcmp(value, "true") == 0) cfg->storage->backend->backend.s3.use_ssl = true;
      else if (strcmp(value, "false") == 0) cfg->storage->backend->backend.s3.use_ssl = false;
    } else if (strcmp(key, "path_style") == 0) {
      strncpy(cfg->storage->backend->backend.s3.path_style, value, BUF_LEN_XS);
    } else if (strcmp(key, "session_token") == 0) {
      strncpy(cfg->storage->backend->backend.s3.session_token, value, BUF_LEN_S);
    } else if (strcmp(key, "max_retries") == 0) {
      val = strtol(value, NULL, 10);
      cfg->storage->backend->backend.s3.max_retries = (size_t)val;
    } else if (strcmp(key, "timeout_seconds") == 0) {
      val = strtol(value, NULL, 10);
      cfg->storage->backend->backend.s3.timeout_seconds = (size_t)val;
    } else if (strcmp(key, "multipart_threshold_mb") == 0) {
      val = strtol(value, NULL, 10);
      cfg->storage->backend->backend.s3.multipart_threshold_mb = (size_t)val;
    } else if (strcmp(key, "multipart_chunk_mb") == 0) {
      val = strtol(value, NULL, 10);
      cfg->storage->backend->backend.s3.multipart_chunk_mb = (size_t)val;
    } else {
      err->code = CONFIG_VALIDATION_ERROR;
      snprintf(err->message, sizeof(err->message), "Unknown s3 configuration key for storage: %s", key);

      return -1;
    }
  } else if (section == SECTION_CHILD_SFTP) {
    cfg->storage->backend->kind = PTC_SFTP;
    if (strcmp(key, "private_key") == 0) {
      strncpy(cfg->storage->backend->backend.sftp.private_key, value, BUF_LEN_S);
    } else if (strcmp(key, "host") == 0) {
      strncpy(cfg->storage->backend->backend.sftp.host, value, BUF_LEN_XS - 1);
    } else if (strcmp(key, "port") == 0) {
      val = strtol(value, NULL, 10);
      cfg->storage->backend->backend.sftp.port = (size_t)val;
    } else if (strcmp(key, "max_retries") == 0) {
      val = strtol(value, NULL, 10);
      cfg->storage->backend->backend.sftp.max_retries = (size_t)val;
    } else if (strcmp(key, "timeout_seconds") == 0) {
      val = strtol(value, NULL, 10);
      cfg->storage->backend->backend.sftp.timeout_seconds = (size_t)val;
    } else if (strcmp(key, "username") == 0) {
      strncpy(cfg->storage->backend->backend.sftp.username, value, BUF_LEN_S);
    } else {
      err->code = CONFIG_VALIDATION_ERROR;
      snprintf(err->message, sizeof(err->message), "Unknown sftp configuration key for storage: %s", key);

      return -1;
    }
  }

  return 0;
}

char *get_storage_protocol_text(RemoteStorageProtocol_t ptc) {
  switch (ptc) {
    case PTC_S3:
      return "s3";
    case PTC_SFTP:
      return "sftp";
    case PTC_SSH:
      return "ssh";
    default: return "unknown";
  }
}
