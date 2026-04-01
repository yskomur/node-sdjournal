#include "common.h"

static napi_value Init(napi_env env, napi_value exports) {
  writer_init(env, exports);
  reader_init(env, exports);
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
