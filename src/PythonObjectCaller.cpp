#include "PythonObject.h"
#include "JSObject.h"
#include "PythonExceptionGuard.h"
#include "Tracer.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kPythonObjectLogger), __VA_ARGS__)

void CPythonObject::CallWrapperAsFunction(const v8::FunctionCallbackInfo<v8::Value>& v8_info) {
  TRACE("CPythonObject::CallWrapperAsFunction v8_info={}", v8_info);
  // "this" should be some wrapper object wrapping some callable from python land
  auto v8_this = v8_info.This();

  // extract wrapped python object
  auto raw_object = lookupTracedObject(v8_this);
  assert(raw_object);

  // use it
  auto py_callable = py::reinterpret_borrow<py::object>(raw_object);
  CallPythonCallable(py_callable, v8_info);
}

void CPythonObject::CallPythonCallable(py::object py_fn, const v8::FunctionCallbackInfo<v8::Value>& v8_info) {
  TRACE("CPythonObject::CallPythonCallable py_fn={} v8_info={}", raw_object_printer{py_fn.ptr()}, v8_info);
  assert(PyCallable_Check(py_fn.ptr()));
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }

  auto v8_result = withPythonExceptionGuard(v8_isolate, [&]() {
    auto py_gil = pyu::withGIL();
    py::object py_result;

    switch (v8_info.Length()) {
      // clang-format off
      case 0: {
        py_result = py_fn();
        break;
      }
      case 1: {
        py_result = py_fn(CJSObject::Wrap(v8_isolate, v8_info[0]));
        break;
      }
      case 2: {
        py_result = py_fn(CJSObject::Wrap(v8_isolate, v8_info[0]),
                          CJSObject::Wrap(v8_isolate, v8_info[1]));
        break;
      }
      case 3: {
        py_result = py_fn(CJSObject::Wrap(v8_isolate, v8_info[0]),
                          CJSObject::Wrap(v8_isolate, v8_info[1]),
                          CJSObject::Wrap(v8_isolate, v8_info[2]));
        break;
      }
      case 4: {
        py_result = py_fn(CJSObject::Wrap(v8_isolate, v8_info[0]),
                          CJSObject::Wrap(v8_isolate, v8_info[1]),
                          CJSObject::Wrap(v8_isolate, v8_info[2]),
                          CJSObject::Wrap(v8_isolate, v8_info[3]));
        break;
      }
      case 5: {
        py_result = py_fn(CJSObject::Wrap(v8_isolate, v8_info[0]),
                          CJSObject::Wrap(v8_isolate, v8_info[1]),
                          CJSObject::Wrap(v8_isolate, v8_info[2]),
                          CJSObject::Wrap(v8_isolate, v8_info[3]),
                          CJSObject::Wrap(v8_isolate, v8_info[4]));
        break;
      }
      case 6: {
        py_result = py_fn(CJSObject::Wrap(v8_isolate, v8_info[0]),
                          CJSObject::Wrap(v8_isolate, v8_info[1]),
                          CJSObject::Wrap(v8_isolate, v8_info[2]),
                          CJSObject::Wrap(v8_isolate, v8_info[3]),
                          CJSObject::Wrap(v8_isolate, v8_info[4]),
                          CJSObject::Wrap(v8_isolate, v8_info[5]));
        break;
      }
      case 7: {
        py_result = py_fn(CJSObject::Wrap(v8_isolate, v8_info[0]),
                          CJSObject::Wrap(v8_isolate, v8_info[1]),
                          CJSObject::Wrap(v8_isolate, v8_info[2]),
                          CJSObject::Wrap(v8_isolate, v8_info[3]),
                          CJSObject::Wrap(v8_isolate, v8_info[4]),
                          CJSObject::Wrap(v8_isolate, v8_info[5]),
                          CJSObject::Wrap(v8_isolate, v8_info[6]));
        break;
      }
      case 8: {
        py_result = py_fn(CJSObject::Wrap(v8_isolate, v8_info[0]),
                          CJSObject::Wrap(v8_isolate, v8_info[1]),
                          CJSObject::Wrap(v8_isolate, v8_info[2]),
                          CJSObject::Wrap(v8_isolate, v8_info[3]),
                          CJSObject::Wrap(v8_isolate, v8_info[4]),
                          CJSObject::Wrap(v8_isolate, v8_info[5]),
                          CJSObject::Wrap(v8_isolate, v8_info[6]),
                          CJSObject::Wrap(v8_isolate, v8_info[7]));
        break;
      }
      case 9: {
        py_result = py_fn(CJSObject::Wrap(v8_isolate, v8_info[0]),
                          CJSObject::Wrap(v8_isolate, v8_info[1]),
                          CJSObject::Wrap(v8_isolate, v8_info[2]),
                          CJSObject::Wrap(v8_isolate, v8_info[3]),
                          CJSObject::Wrap(v8_isolate, v8_info[4]),
                          CJSObject::Wrap(v8_isolate, v8_info[5]),
                          CJSObject::Wrap(v8_isolate, v8_info[6]),
                          CJSObject::Wrap(v8_isolate, v8_info[7]),
                          CJSObject::Wrap(v8_isolate, v8_info[8]));
        break;
      }
      case 10: {
        py_result = py_fn(CJSObject::Wrap(v8_isolate, v8_info[0]),
                          CJSObject::Wrap(v8_isolate, v8_info[1]),
                          CJSObject::Wrap(v8_isolate, v8_info[2]),
                          CJSObject::Wrap(v8_isolate, v8_info[3]),
                          CJSObject::Wrap(v8_isolate, v8_info[4]),
                          CJSObject::Wrap(v8_isolate, v8_info[5]),
                          CJSObject::Wrap(v8_isolate, v8_info[6]),
                          CJSObject::Wrap(v8_isolate, v8_info[7]),
                          CJSObject::Wrap(v8_isolate, v8_info[8]),
                          CJSObject::Wrap(v8_isolate, v8_info[9]));
        break;
      }
        // clang-format on
      default: {
        auto v8_msg = v8::String::NewFromUtf8(v8_isolate, "too many arguments").ToLocalChecked();
        v8_isolate->ThrowException(v8::Exception::Error(v8_msg));
        return v8::Undefined(v8_isolate).As<v8::Value>();
      }
    }

    return Wrap(py_result);
  });

  auto v8_final_result = VALUE_OR_LAZY(v8_result, v8::Undefined(v8_isolate));
  v8_info.GetReturnValue().Set(v8_final_result);
}