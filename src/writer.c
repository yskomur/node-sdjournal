#include "common.h"
#include <stdio.h>
#include <stdlib.h>

// ── journal.print(priority, message) ─────────────────────────────────────────
static napi_value Print(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

  int32_t priority;
  NAPI_CALL(env, napi_get_value_int32(env, args[0], &priority));

  size_t msg_len;
  NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], NULL, 0, &msg_len));
  char *msg = malloc(msg_len + 1);
  NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], msg, msg_len + 1, &msg_len));

  int rc = sd_journal_print(priority, "%s", msg);
  free(msg);
  if (rc < 0) return throw_errno(env, rc, "sd_journal_print");

  napi_value result;
  NAPI_CALL(env, napi_get_undefined(env, &result));
  return result;
}

// ── journal.send({ MESSAGE, PRIORITY, ...fields }) ───────────────────────────
// sd_journal_sendv() ile structured log gönderir
static napi_value Send(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

  napi_value keys;
  NAPI_CALL(env, napi_get_property_names(env, args[0], &keys));

  uint32_t key_count;
  NAPI_CALL(env, napi_get_array_length(env, keys, &key_count));

  struct iovec *iov = malloc(key_count * sizeof(struct iovec));
  char **bufs       = malloc(key_count * sizeof(char *));
  uint32_t iov_count = 0;

  for (uint32_t i = 0; i < key_count; i++) {
    napi_value key_val, val_val;
    napi_get_element(env, keys, i, &key_val);
    napi_get_property(env, args[0], key_val, &val_val);

    size_t klen, vlen;
    napi_get_value_string_utf8(env, key_val, NULL, 0, &klen);
    napi_get_value_string_utf8(env, val_val, NULL, 0, &vlen);

    char *buf = malloc(klen + 1 + vlen + 1);
    napi_get_value_string_utf8(env, key_val, buf, klen + 1, &klen);
    buf[klen] = '=';
    napi_get_value_string_utf8(env, val_val, buf + klen + 1, vlen + 1, &vlen);

    bufs[iov_count]          = buf;
    iov[iov_count].iov_base  = buf;
    iov[iov_count].iov_len   = klen + 1 + vlen;
    iov_count++;
  }

  int rc = sd_journal_sendv(iov, (int)iov_count);
  for (uint32_t i = 0; i < iov_count; i++) free(bufs[i]);
  free(bufs);
  free(iov);

  if (rc < 0) return throw_errno(env, rc, "sd_journal_sendv");

  napi_value result;
  NAPI_CALL(env, napi_get_undefined(env, &result));
  return result;
}

napi_value writer_init(napi_env env, napi_value exports) {
  napi_value fn_print, fn_send;
  napi_create_function(env, NULL, 0, Print, NULL, &fn_print);
  napi_create_function(env, NULL, 0, Send,  NULL, &fn_send);
  napi_set_named_property(env, exports, "print", fn_print);
  napi_set_named_property(env, exports, "send",  fn_send);
  return exports;
}
