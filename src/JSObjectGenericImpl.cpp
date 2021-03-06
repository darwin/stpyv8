#include "JSObjectGenericImpl.h"
#include "JSException.h"
#include "Wrapping.h"
#include "JSObject.h"
#include "Logging.h"
#include "Printing.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectGenericImplLogger), __VA_ARGS__)

static void ensureAttrExistsOrThrow(v8x::LockedIsolatePtr& v8_isolate,
                                    v8::Local<v8::Object> v8_this,
                                    v8::Local<v8::String> v8_name) {
  TRACE("ensureAttrExistsOrThrow v8_name={} v8_this={}", v8_name, v8_this);
  assert(v8_isolate->InContext());
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);

  auto hasName = v8_this->Has(v8_context, v8_name).FromMaybe(false);
  if (!hasName) {
    auto v8_proto_str = v8_this->ObjectProtoToString(v8_context).ToLocalChecked();
    auto v8_proto_utf = v8x::toUTF(v8_isolate, v8_proto_str);
    auto v8_name_utf = v8x::toUTF(v8_isolate, v8_name);
    auto msg = fmt::format("'{}' object has no attribute '{}'", *v8_proto_utf, *v8_name_utf);
    throw JSException(msg, PyExc_AttributeError);
  }
}

py::object JSObjectGenericGetAttr(const JSObject& self, const py::object& py_key) {
  TRACE("JSObjectGenericGetAttr {} name={}", SELF, py_key);
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_try_catch = v8x::withAutoTryCatch(v8_isolate);

  auto v8_attr_name = v8x::toString(v8_isolate, py_key);
  auto v8_this = self.ToV8(v8_isolate);
  ensureAttrExistsOrThrow(v8_isolate, v8_this, v8_attr_name);
  auto v8_attr_value = v8_this->Get(v8_context, v8_attr_name).ToLocalChecked();

  auto py_result = wrap(v8_isolate, v8_attr_value, v8_this);
  TRACE("JSObjectGenericObjectGetAttr {} => {}", SELF, py_result);
  return py_result;
}

void JSObjectGenericSetAttr(const JSObject& self, const py::object& py_key, const py::object& py_obj) {
  TRACE("JSObjectGenericSetAttr {} name={} py_obj={}", SELF, py_key, py_obj);
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_try_catch = v8x::withAutoTryCatch(v8_isolate);

  auto v8_attr_name = v8x::toString(v8_isolate, py_key);
  auto v8_attr_obj = wrap(std::move(py_obj));

  auto v8_this = self.ToV8(v8_isolate);
  v8_this->Set(v8_context, v8_attr_name, v8_attr_obj).Check();
}

void JSObjectGenericDelAttr(const JSObject& self, const py::object& py_key) {
  TRACE("JSObjectGenericDelAttr {} name={}", SELF, py_key);
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_try_catch = v8x::withAutoTryCatch(v8_isolate);

  auto v8_attr_name = v8x::toString(v8_isolate, py_key);
  auto v8_this = self.ToV8(v8_isolate);
  ensureAttrExistsOrThrow(v8_isolate, v8_this, v8_attr_name);

  v8_this->Delete(v8_context, v8_attr_name).Check();
}

bool JSObjectGenericContains(const JSObject& self, const py::object& py_key) {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_try_catch = v8x::withAutoTryCatch(v8_isolate);

  auto v8_this = self.ToV8(v8_isolate);
  bool result = v8_this->Has(v8_context, v8x::toString(v8_isolate, py_key)).ToChecked();
  TRACE("JSObjectGenericContains {} py_key={} => {}", SELF, py_key, result);
  return result;
}

py::str JSObjectGenericStr(const JSObject& self) {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_this = self.ToV8(v8_isolate);
  auto py_result = [&] {
    if (v8_this.IsEmpty()) {
      return py::str("<EMPTY>");
    } else {
      auto v8_context = v8x::getCurrentContext(v8_isolate);
      auto v8_str = v8_this->ToString(v8_context).ToLocalChecked();
      auto v8_utf = v8x::toUTF(v8_isolate, v8_str);
      return py::str(*v8_utf);
    }
  }();

  TRACE("JSObjectGenericStr {} => {}", SELF, py_result);
  return py_result;
}

py::str JSObjectGenericRepr(const JSObject& self) {
  std::ostringstream ss;
  self.Dump(ss);
  auto str = fmt::format("JSObject[{}] {}", self.GetRoles(), ss.str());
  py::str py_result(str);
  TRACE("JSObjectGenericRepr {} => {}", SELF, py_result);
  return py_result;
}
