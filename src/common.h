#pragma once

#include <node_api.h>
#include <systemd/sd-journal.h>
#include <errno.h>
#include <string.h>

// ── Hata yönetimi ─────────────────────────────────────────────────────────────

#define NAPI_CALL(env, call)                                              \
  do {                                                                    \
    napi_status _s = (call);                                              \
    if (_s != napi_ok) {                                                  \
      const napi_extended_error_info *info;                               \
      napi_get_last_error_info((env), &info);                             \
      napi_throw_error((env), NULL, info->error_message);                 \
      return NULL;                                                        \
    }                                                                     \
  } while (0)

static inline napi_value throw_errno(napi_env env, int rc, const char *ctx) {
  char msg[256];
  snprintf(msg, sizeof(msg), "%s: %s", ctx, strerror(-rc));
  napi_throw_error(env, NULL, msg);
  return NULL;
}

// ── Reader nesnesi ────────────────────────────────────────────────────────────

typedef struct {
  sd_journal *j;
} JournalReader;

napi_value writer_init(napi_env env, napi_value exports);
napi_value reader_init(napi_env env, napi_value exports);
