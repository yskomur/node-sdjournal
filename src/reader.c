#include "common.h"
#include <stdlib.h>
#include <stdio.h>

// ── Finalizer: GC sırasında sd_journal_close çağrılır ────────────────────────
static void reader_finalize(napi_env env, void *data, void *hint) {
  (void)env; (void)hint;
  JournalReader *r = (JournalReader *)data;
  if (r->j) sd_journal_close(r->j);
  free(r);
}

// ── Yardımcı: args[0]'dan JournalReader* çek ─────────────────────────────────
static JournalReader *unwrap(napi_env env, napi_callback_info info) {
  size_t argc = 8;
  napi_value args[8];
  napi_value this_val;
  napi_get_cb_info(env, info, &argc, args, &this_val, NULL);

  JournalReader *r;
  napi_unwrap(env, this_val, (void **)&r);
  return r;
}

// ── Reader.open(flags) → Reader nesnesi ──────────────────────────────────────
static napi_value Open(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

  int32_t flags = 0;
  if (argc > 0) napi_get_value_int32(env, args[0], &flags);

  JournalReader *r = calloc(1, sizeof(JournalReader));
  int rc = sd_journal_open(&r->j, flags);
  if (rc < 0) { free(r); return throw_errno(env, rc, "sd_journal_open"); }

  // Boş bir JS nesnesi yarat ve C pointer'ı wrap et
  napi_value obj;
  NAPI_CALL(env, napi_create_object(env, &obj));
  NAPI_CALL(env, napi_wrap(env, obj, r, reader_finalize, NULL, NULL));
  return obj;
}

// ── reader.close() ────────────────────────────────────────────────────────────
static napi_value Close(napi_env env, napi_callback_info info) {
  napi_value this_val;
  napi_get_cb_info(env, info, NULL, NULL, &this_val, NULL);
  JournalReader *r;
  napi_unwrap(env, this_val, (void **)&r);
  if (r->j) { sd_journal_close(r->j); r->j = NULL; }
  napi_value undef; napi_get_undefined(env, &undef);
  return undef;
}

// ── reader.seekHead() / seekTail() ───────────────────────────────────────────
static napi_value SeekHead(napi_env env, napi_callback_info info) {
  JournalReader *r = unwrap(env, info);
  int rc = sd_journal_seek_head(r->j);
  if (rc < 0) return throw_errno(env, rc, "sd_journal_seek_head");
  napi_value undef; napi_get_undefined(env, &undef); return undef;
}

static napi_value SeekTail(napi_env env, napi_callback_info info) {
  JournalReader *r = unwrap(env, info);
  int rc = sd_journal_seek_tail(r->j);
  if (rc < 0) return throw_errno(env, rc, "sd_journal_seek_tail");
  napi_value undef; napi_get_undefined(env, &undef); return undef;
}

// ── reader.seekCursor(cursor) ─────────────────────────────────────────────────
static napi_value SeekCursor(napi_env env, napi_callback_info info) {
  size_t argc = 1; napi_value args[1]; napi_value this_val;
  napi_get_cb_info(env, info, &argc, args, &this_val, NULL);
  JournalReader *r; napi_unwrap(env, this_val, (void **)&r);

  size_t len; napi_get_value_string_utf8(env, args[0], NULL, 0, &len);
  char *cursor = malloc(len + 1);
  napi_get_value_string_utf8(env, args[0], cursor, len + 1, &len);
  int rc = sd_journal_seek_cursor(r->j, cursor);
  free(cursor);
  if (rc < 0) return throw_errno(env, rc, "sd_journal_seek_cursor");
  napi_value undef; napi_get_undefined(env, &undef); return undef;
}

// ── reader.seekTime(usec) ─────────────────────────────────────────────────────
static napi_value SeekTime(napi_env env, napi_callback_info info) {
  size_t argc = 1; napi_value args[1]; napi_value this_val;
  napi_get_cb_info(env, info, &argc, args, &this_val, NULL);
  JournalReader *r; napi_unwrap(env, this_val, (void **)&r);

  int64_t usec; napi_get_value_int64(env, args[0], &usec);
  int rc = sd_journal_seek_realtime_usec(r->j, (uint64_t)usec);
  if (rc < 0) return throw_errno(env, rc, "sd_journal_seek_realtime_usec");
  napi_value undef; napi_get_undefined(env, &undef); return undef;
}

// ── reader.next() → bool ─────────────────────────────────────────────────────
static napi_value Next(napi_env env, napi_callback_info info) {
  JournalReader *r = unwrap(env, info);
  int rc = sd_journal_next(r->j);
  if (rc < 0) return throw_errno(env, rc, "sd_journal_next");
  napi_value result; napi_get_boolean(env, rc > 0, &result);
  return result;
}

// ── reader.previous() → bool ─────────────────────────────────────────────────
static napi_value Previous(napi_env env, napi_callback_info info) {
  JournalReader *r = unwrap(env, info);
  int rc = sd_journal_previous(r->j);
  if (rc < 0) return throw_errno(env, rc, "sd_journal_previous");
  napi_value result; napi_get_boolean(env, rc > 0, &result);
  return result;
}

// ── reader.getEntry() → { fields, cursor, timestamp } ────────────────────────
static napi_value GetEntry(napi_env env, napi_callback_info info) {
  JournalReader *r = unwrap(env, info);

  napi_value entry, fields;
  napi_create_object(env, &entry);
  napi_create_object(env, &fields);

  // Tüm field'ları iterate et
  sd_journal_restart_data(r->j);
  const void *data; size_t length;
  while (sd_journal_enumerate_data(r->j, &data, &length) > 0) {
    const char *s = (const char *)data;
    const char *eq = memchr(s, '=', length);
    if (!eq) continue;

    size_t klen = (size_t)(eq - s);
    size_t vlen = length - klen - 1;

    napi_value key_js, val_js;
    napi_create_string_utf8(env, s,      klen, &key_js);
    napi_create_string_utf8(env, eq + 1, vlen, &val_js);
    napi_set_property(env, fields, key_js, val_js);
  }

  // Cursor
  char *cursor = NULL;
  if (sd_journal_get_cursor(r->j, &cursor) >= 0) {
    napi_value cursor_js;
    napi_create_string_utf8(env, cursor, NAPI_AUTO_LENGTH, &cursor_js);
    napi_set_named_property(env, entry, "cursor", cursor_js);
    free(cursor);
  }

  // Realtime timestamp (microseconds)
  uint64_t usec;
  if (sd_journal_get_realtime_usec(r->j, &usec) >= 0) {
    napi_value ts_js;
    napi_create_int64(env, (int64_t)usec, &ts_js);
    napi_set_named_property(env, entry, "realtimeUsec", ts_js);
  }

  napi_set_named_property(env, entry, "fields", fields);
  return entry;
}

// ── reader.addMatch(str) ──────────────────────────────────────────────────────
static napi_value AddMatch(napi_env env, napi_callback_info info) {
  size_t argc = 1; napi_value args[1]; napi_value this_val;
  napi_get_cb_info(env, info, &argc, args, &this_val, NULL);
  JournalReader *r; napi_unwrap(env, this_val, (void **)&r);

  size_t len; napi_get_value_string_utf8(env, args[0], NULL, 0, &len);
  char *match = malloc(len + 1);
  napi_get_value_string_utf8(env, args[0], match, len + 1, &len);
  int rc = sd_journal_add_match(r->j, match, len);
  free(match);
  if (rc < 0) return throw_errno(env, rc, "sd_journal_add_match");
  napi_value undef; napi_get_undefined(env, &undef); return undef;
}

// ── reader.flushMatches() ─────────────────────────────────────────────────────
static napi_value FlushMatches(napi_env env, napi_callback_info info) {
  JournalReader *r = unwrap(env, info);
  sd_journal_flush_matches(r->j);
  napi_value undef; napi_get_undefined(env, &undef); return undef;
}

// ── reader.wait(timeoutUsec) → number (NOP=0 / APPEND=1 / INVALIDATE=2) ──────
static napi_value Wait(napi_env env, napi_callback_info info) {
  size_t argc = 1; napi_value args[1]; napi_value this_val;
  napi_get_cb_info(env, info, &argc, args, &this_val, NULL);
  JournalReader *r; napi_unwrap(env, this_val, (void **)&r);

  int64_t timeout_usec = -1; // UINT64_MAX → indefinite
  if (argc > 0) napi_get_value_int64(env, args[0], &timeout_usec);

  int rc = sd_journal_wait(r->j, (uint64_t)timeout_usec);
  if (rc < 0) return throw_errno(env, rc, "sd_journal_wait");
  napi_value result; napi_create_int32(env, rc, &result);
  return result;
}

// ── Export ────────────────────────────────────────────────────────────────────
napi_value reader_init(napi_env env, napi_value exports) {
  // open() factory fonksiyon olarak export edilir
  napi_value fn;

#define EXPORT_FN(name, fn_ptr) \
  napi_create_function(env, NULL, 0, fn_ptr, NULL, &fn); \
  napi_set_named_property(env, exports, name, fn);

  EXPORT_FN("open",         Open)
  EXPORT_FN("_close",       Close)
  EXPORT_FN("_seekHead",    SeekHead)
  EXPORT_FN("_seekTail",    SeekTail)
  EXPORT_FN("_seekCursor",  SeekCursor)
  EXPORT_FN("_seekTime",    SeekTime)
  EXPORT_FN("_next",        Next)
  EXPORT_FN("_previous",    Previous)
  EXPORT_FN("_getEntry",    GetEntry)
  EXPORT_FN("_addMatch",    AddMatch)
  EXPORT_FN("_flushMatches",FlushMatches)
  EXPORT_FN("_wait",        Wait)

  return exports;
}
