#include "JSObjectUtils.h"
#include "Logging.h"
#include "V8XUtils.h"
#include "JSEternals.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

static auto createConstructorString(v8x::LockedIsolatePtr& v8_isolate) {
  return v8x::createEternalString(v8_isolate, "constructor");
}

static auto createCLJSTypeString(v8x::LockedIsolatePtr& v8_isolate) {
  return v8x::createEternalString(v8_isolate, "cljs$lang$type");
}

bool isCLJSType(v8::Local<v8::Object> v8_obj) {
  if (v8_obj.IsEmpty()) {
    return false;
  }

  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);

  auto v8_ctor_key = lookupEternal<v8::String>(v8_isolate, JSEternals::kConstructorString, createConstructorString);
  auto v8_ctor = v8_obj->Get(v8_context, v8_ctor_key);

  if (v8_ctor.IsEmpty()) {
    return false;
  }

  auto v8_ctor_val = v8_ctor.ToLocalChecked();
  if (!v8_ctor_val->IsObject()) {
    return false;
  }

  auto v8_ctor_obj = v8_ctor_val.As<v8::Object>();
  auto v8_cljs_key = lookupEternal<v8::String>(v8_isolate, JSEternals::kCLJSLangTypeString, createCLJSTypeString);
  auto v8_cljs_val = v8_ctor_obj->Get(v8_context, v8_cljs_key).ToLocalChecked();

  return !(v8_cljs_val.IsEmpty() || !v8_cljs_val->IsBoolean());

  // note:
  // we should cast cljs_val to object and check cljs_obj->BooleanValue()
  // but we assume existence of property means true value
}