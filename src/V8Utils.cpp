#include "V8Utils.h"
#include "PythonUtils.h"
#include "JSException.h"

namespace v8u {

std::optional<v8::Local<v8::String>> toStringDirectly(py::handle obj) {
  if (PyUnicode_CheckExact(obj.ptr())) {
    auto py_bytes = PyUnicode_AsUTF8String(obj.ptr());  // may be NULL
    auto bytes_obj = py::reinterpret_steal<py::object>(py_bytes);
    return pythonBytesObjectToString(bytes_obj.ptr());
  }

  if (PyBytes_CheckExact(obj.ptr())) {
    return pythonBytesObjectToString(obj.ptr());
  }

  // please note that returning nullopt is different than
  // returning v8::Local<v8::String> with empty value inside
  // and that is different than returning v8::Local<v8::String> with empty string "" inside
  return std::nullopt;
}

v8::Local<v8::String> toString(py::handle py_str) {
  // first try to convert python string object directly if possible
  auto v8_str = toStringDirectly(py_str);
  if (v8_str) {
    return *v8_str;
  }

  // alternatively convert it to string representation and try again
  auto py_computed_str = PyObject_Str(py_str.ptr());  // may be NULL
  auto py_computed_obj = py::reinterpret_steal<py::object>(py_computed_str);
  v8_str = toStringDirectly(py_computed_obj);
  if (v8_str) {
    return *v8_str;
  }

  // all attempts failed, return empty value
  return v8::Local<v8::String>();
}

v8::Local<v8::String> toString(const std::string& str) {
  auto isolate = v8::Isolate::GetCurrent();
  return v8::String::NewFromUtf8(isolate, str.c_str(), v8::NewStringType::kNormal, str.size()).ToLocalChecked();
}

v8::Local<v8::String> toString(const std::wstring& str) {
  auto raw_unicode = PyUnicode_FromWideChar(str.c_str(), str.size());  // may be NULL
  auto py_obj = py::reinterpret_steal<py::object>(raw_unicode);
  auto v8_str = toStringDirectly(py_obj);
  if (v8_str) {
    return *v8_str;
  }

  // all attempts failed, return empty value
  return v8::Local<v8::String>();
}

v8::String::Utf8Value toUtf8Value(v8::Isolate* v8_isolate, v8::Local<v8::String> v8_string) {
  return v8::String::Utf8Value(v8_isolate, v8_string);
}

void checkContext(v8::Isolate* v8_isolate) {
  auto scope = openScope(v8_isolate);
  if (v8_isolate->GetCurrentContext().IsEmpty()) {
    throw CJSException(v8_isolate, "Javascript object out of context", PyExc_UnboundLocalError);
  }
}

bool executionTerminating(v8::Isolate* v8_isolate) {
  if (!v8_isolate->IsExecutionTerminating()) {
    return false;
  }

  PyErr_Clear();
  PyErr_SetString(PyExc_RuntimeError, "execution is terminating");
  return true;
}

v8::HandleScope openScope(v8::Isolate* v8_isolate) {
  return v8::HandleScope(v8_isolate);
}

v8::EscapableHandleScope openEscapableScope(v8::Isolate* v8_isolate) {
  return v8::EscapableHandleScope(v8_isolate);
}

v8::TryCatch openTryCatch(v8::Isolate* v8_isolate) {
  return v8::TryCatch(v8_isolate);
}

}  // namespace v8u