#include "Printing.h"
#include "JSContext.h"
#include "JSEngine.h"
#include "JSScript.h"
#include "JSException.h"
#include "JSObject.h"
#include "JSStackTrace.h"
#include "JSStackFrame.h"

std::string printCoerced(const std::wstring& v) {
  try {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(v);
  } catch (...) {
    return "{std::wstring conversion error}";
  }
}

std::string printCoerced(v8::Isolate* v) {
  return fmt::format("v8::Isolate* {}", static_cast<void*>(v));
}

std::ostream& operator<<(std::ostream& os, const JSStackTrace& v) {
  v.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, const JSException& v) {
  os << "JSError: " << v.what();
  return os;
}

std::ostream& operator<<(std::ostream& os, const JSObject& v) {
  v.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, const JSContext& v) {
  v.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, const JSEngine& v) {
  v.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, const JSScript& v) {
  v.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, const JSStackFrame& v) {
  v.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, PyObject* v) {
  if (!v) {
    return os << "PyObject 0x0";
  }
  return os << fmt::format("PyObject {} [#{}] {}", static_cast<const void*>(v), Py_REFCNT(v), py::handle(v));
}

std::ostream& operator<<(std::ostream& os, PyTypeObject* v) {
  return operator<<(os, reinterpret_cast<PyObject*>(v));
}

std::ostream& operator<<(std::ostream& os, const SafePrinter<PyObject*>& wv) {
  auto& v = wv.m_v;
  if (!v) {
    return os << "PyObject 0x0";
  }
  return os << fmt::format("PyObject {} [#{}]", static_cast<const void*>(v), Py_REFCNT(v));
}

namespace v8 {

template <typename T>
static std::ostream& dumpLocalPrefix(std::ostream& os, const char* label, const Local<T>& v) {
  return os << fmt::format("{} {}", label, static_cast<void*>(*v));
}

template <typename T, typename F>
static std::ostream& printLocalChecked(std::ostream& os, const Local<T>& v, const char* label, F&& f) {
  dumpLocalPrefix(os, label, v);
  if (v.IsEmpty()) {
    os << "{EMPTY}";
  } else {
    os << f();
  }
  return os;
}

template <typename T>
static std::ostream& printLocalChecked(std::ostream& os, const Local<T>& v, const char* label) {
  return printLocalChecked(os, v, label, [] { return ""; });
}

std::ostream& printLocalValue(std::ostream& os, const Local<Value>& v) {
  if (v.IsEmpty()) {
    return os << "{EMPTY}";
  }

  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_context = v8_isolate->GetEnteredOrMicrotaskContext();
  if (v8_context.IsEmpty()) {
    return os << "{NO CONTEXT}";
  }

  auto v8_str = v->ToDetailString(v8_context);
  if (v8_str.IsEmpty()) {
    return os << "{N/A}";
  } else {
    return os << *v8x::toUTF(v8_isolate, v8_str.ToLocalChecked());
  }
}

std::ostream& operator<<(std::ostream& os, const Local<Private>& v) {
  return printLocalChecked(os, v, "v8::Context");
}

std::ostream& operator<<(std::ostream& os, const Local<Context>& v) {
  return printLocalChecked(os, v, "v8::Context", [&] { return fmt::format("global={}", v->Global()); });
}

std::ostream& operator<<(std::ostream& os, const Local<Script>& v) {
  return printLocalChecked(os, v, "v8::Script");
}

std::ostream& operator<<(std::ostream& os, const Local<ObjectTemplate>& v) {
  return printLocalChecked(os, v, "v8::ObjectTemplate");
}

std::ostream& operator<<(std::ostream& os, const Local<Message>& v) {
  return printLocalChecked(os, v, "v8::Message", [&] { return fmt::format("'{}'", v->Get()); });
}

std::ostream& operator<<(std::ostream& os, const Local<StackFrame>& v) {
  return printLocalChecked(os, v, "v8::StackFrame", [&] {
    return fmt::format("ScriptId={} Script={}", v->GetScriptId(), v->GetScriptNameOrSourceURL());
  });
}

std::ostream& operator<<(std::ostream& os, const Local<StackTrace>& v) {
  return printLocalChecked(os, v, "v8::StackFrame", [&] { return fmt::format("FrameCount={}", v->GetFrameCount()); });
}

std::ostream& operator<<(std::ostream& os, const TryCatch& v) {
  return os << fmt::format("v8::TryCatch Message='{}'", v.Message());
}

}  // namespace v8

namespace v8x {

std::ostream& operator<<(std::ostream& os, const ProtectedIsolatePtr& v) {
  return os << fmt::format("v8x::ProtectedIsolatePtr {}", static_cast<void*>(v.giveMeRawIsolateAndTrustMe()));
}

}  // namespace v8x

namespace pybind11 {

std::ostream& operator<<(std::ostream& os, const error_already_set& v) {
  return os << fmt::format("py::error_already_set[type={} what={}]", v.type(), v.what());
}

}  // namespace pybind11
