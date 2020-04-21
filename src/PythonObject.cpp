#include "PythonObject.h"
#include "JSObject.h"
#include "JSUndefined.h"
#include "JSNull.h"
#include "JSException.h"
#include "Isolate.h"
#include "PythonDateTime.h"
#include "Tracer.h"
#include "Hospital.h"
#include "Eternals.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kPythonObjectLogger), __VA_ARGS__)

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

static v8::Eternal<v8::Private> privateAPIForType(v8::IsolateRef v8_isolate) {
  return v8u::createEternalPrivateAPI(v8_isolate, "exc_type");
}

static v8::Eternal<v8::Private> privateAPIForValue(v8::IsolateRef v8_isolate) {
  return v8u::createEternalPrivateAPI(v8_isolate, "exc_value");
}

void CPythonObject::ThrowIf(const v8::IsolateRef& v8_isolate, const py::error_already_set& py_ex) {
  TRACE("CPythonObject::ThrowIf");
  auto py_gil = pyu::withGIL();

  auto v8_scope = v8u::withScope(v8_isolate);

  py::object py_type(py_ex.type());
  py::object py_value(py_ex.value());

  auto raw_type = py_type.ptr();
  auto raw_value = py_value.ptr();

  //  TODO: investigate: shall we call normalize, pybind does not call it
  //  PyErr_NormalizeException(&raw_exc, &raw_val, &raw_trb);

  std::string msg;

  if (py::hasattr(py_value, "args")) {
    auto py_args = py_value.attr("args");
    if (py::isinstance<py::tuple>(py_args)) {
      auto py_args_tuple = py::cast<py::tuple>(py_args);
      auto it = py_args_tuple.begin();
      while (it != py_args_tuple.end()) {
        auto py_arg = *it;
        if (py::isinstance<py::str>(py_arg)) {
          msg += py::cast<py::str>(py_arg);
        }
        it++;
      }
    }
  } else if (py::hasattr(py_value, "message")) {
    auto py_msg = py_value.attr("message");
    if (py::isinstance<py::str>(py_msg)) {
      msg += py::cast<py::str>(py_msg);
    }
  } else if (raw_value) {
    // TODO: use pybind
    if (PyBytes_CheckExact(raw_value)) {
      msg = PyBytes_AS_STRING(raw_value);
    } else if (PyTuple_CheckExact(raw_value)) {
      for (int i = 0; i < PyTuple_GET_SIZE(raw_value); i++) {
        auto raw_item = PyTuple_GET_ITEM(raw_value, i);

        if (raw_item && PyBytes_CheckExact(raw_item)) {
          msg = PyBytes_AS_STRING(raw_item);
          break;
        }
      }
    }
  }

  v8::Local<v8::Value> v8_error;

  if (PyErr_GivenExceptionMatches(raw_type, PyExc_IndexError)) {
    v8_error = v8::Exception::RangeError(v8u::toString(v8_isolate, msg));
  } else if (PyErr_GivenExceptionMatches(raw_type, PyExc_AttributeError)) {
    v8_error = v8::Exception::ReferenceError(v8u::toString(v8_isolate, msg));
  } else if (PyErr_GivenExceptionMatches(raw_type, PyExc_SyntaxError)) {
    v8_error = v8::Exception::SyntaxError(v8u::toString(v8_isolate, msg));
  } else if (PyErr_GivenExceptionMatches(raw_type, PyExc_TypeError)) {
    v8_error = v8::Exception::TypeError(v8u::toString(v8_isolate, msg));
  } else {
    v8_error = v8::Exception::Error(v8u::toString(v8_isolate, msg));
  }

  if (v8_error->IsObject()) {
    auto v8_error_object = v8_error.As<v8::Object>();
    auto v8_context = v8_isolate->GetCurrentContext();

    auto v8_type_api = lookupEternal<v8::Private>(v8_isolate, CEternals::kJSExceptionType, privateAPIForType);
    auto v8_exc_type_external = v8::External::New(v8_isolate, raw_type);
    v8_error_object->SetPrivate(v8_context, v8_type_api, v8_exc_type_external);

    auto v8_value_api = lookupEternal<v8::Private>(v8_isolate, CEternals::kJSExceptionValue, privateAPIForValue);
    auto v8_exc_value_external = v8::External::New(v8_isolate, raw_value);
    v8_error_object->SetPrivate(v8_context, v8_value_api, v8_exc_value_external);

    // this must match Py_DECREFs below !!!
    Py_INCREF(raw_type);
    Py_INCREF(raw_value);

    hospitalizePatient(v8_error_object, [](v8::Local<v8::Object> v8_patient) {
      // note we cannot capture anything in this lambda, this would not match hospitalizePatient signature
      TRACE("doing cleanup of v8_error {}", v8_patient);
      auto v8_isolate = v8_patient->GetIsolate();
      auto v8_context = v8_isolate->GetCurrentContext();

      auto v8_type_api = lookupEternal<v8::Private>(v8_isolate, CEternals::kJSExceptionType, privateAPIForType);
      auto v8_type_val = v8_patient->GetPrivate(v8_context, v8_type_api).ToLocalChecked();
      assert(v8_type_val->IsExternal());
      auto raw_type = static_cast<PyObject*>(v8_type_val.As<v8::External>()->Value());
      assert(raw_type);

      auto v8_value_api = lookupEternal<v8::Private>(v8_isolate, CEternals::kJSExceptionValue, privateAPIForValue);
      auto v8_value_val = v8_patient->GetPrivate(v8_context, v8_value_api).ToLocalChecked();
      assert(v8_value_val->IsExternal());
      auto raw_value = static_cast<PyObject*>(v8_value_val.As<v8::External>()->Value());
      assert(raw_value);

      // this must match Py_INCREFs above !!!
      Py_DECREF(raw_type);
      Py_DECREF(raw_value);
    });
  }

  v8_isolate->ThrowException(v8_error);
}

#pragma clang diagnostic pop

void CPythonObject::SetupObjectTemplate(const v8::IsolateRef& v8_isolate,
                                        v8::Local<v8::ObjectTemplate> v8_object_template) {
  TRACE("CPythonObject::SetupObjectTemplate");
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_handler_config =
      v8::NamedPropertyHandlerConfiguration(NamedGetter, NamedSetter, NamedQuery, NamedDeleter, NamedEnumerator);

  v8_object_template->SetHandler(v8_handler_config);
  v8_object_template->SetIndexedPropertyHandler(IndexedGetter, IndexedSetter, IndexedQuery, IndexedDeleter,
                                                IndexedEnumerator);
  v8_object_template->SetCallAsFunctionHandler(CallWrapperAsFunction);
}

v8::Local<v8::ObjectTemplate> CPythonObject::CreateObjectTemplate(const v8::IsolateRef& v8_isolate) {
  TRACE("CPythonObject::CreateObjectTemplate");
  auto v8_scope = v8u::withEscapableScope(v8_isolate);
  auto v8_class = v8::ObjectTemplate::New(v8_isolate);
  SetupObjectTemplate(v8_isolate, v8_class);
  return v8_scope.Escape(v8_class);
}

v8::Local<v8::ObjectTemplate> CPythonObject::GetCachedObjectTemplateOrCreate(const v8::IsolateRef& v8_isolate) {
  TRACE("CPythonObject::GetCachedObjectTemplateOrCreate");
  auto v8_scope = v8u::withEscapableScope(v8_isolate);
  auto v8_object_template =
      lookupEternal<v8::ObjectTemplate>(v8_isolate, CEternals::kJSObjectTemplate, [](v8::IsolateRef v8_isolate) {
        auto v8_born_template = CreateObjectTemplate(v8_isolate);
        return v8::Eternal<v8::ObjectTemplate>(v8_isolate, v8_born_template);
      });
  return v8_scope.Escape(v8_object_template);
}

v8::Local<v8::Value> CPythonObject::Wrap(py::handle py_handle) {
  TRACE("CPythonObject::Wrap py_handle={}", py_handle);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withEscapableScope(v8_isolate);

  auto v8_object = lookupTracedWrapper(v8_isolate, py_handle.ptr());
  if (!v8_object.IsEmpty()) {
    return v8_scope.Escape(v8_object);
  }

  auto v8_value = WrapInternal(py_handle);
  return v8_scope.Escape(v8_value);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

v8::Local<v8::Value> CPythonObject::WrapInternal(py::handle py_handle) {
  TRACE("CPythonObject::WrapInternal py_handle={}", py_handle);
  auto v8_isolate = v8u::getCurrentIsolate();
  assert(v8_isolate->InContext());
  auto v8_scope = v8u::withEscapableScope(v8_isolate);
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);
  auto py_gil = pyu::withGIL();

  if (v8u::executionTerminating(v8_isolate)) {
    return v8::Undefined(v8_isolate);
  }

  if (py_handle.ptr() == Py_JSNull) {
    return v8::Null(v8_isolate);
  }
  if (py_handle.ptr() == Py_JSUndefined) {
    return v8::Undefined(v8_isolate);
  }
  if (py::isinstance<py::bool_>(py_handle)) {
    auto py_bool = py::cast<py::bool_>(py_handle);
    if (py_bool) {
      return v8::True(v8_isolate);
    } else {
      return v8::False(v8_isolate);
    }
  }
  if (py::isinstance<CJSObject>(py_handle)) {
    auto object = py::cast<CJSObjectPtr>(py_handle);
    assert(object.get());

    if (object->Object().IsEmpty()) {
      throw CJSException("Refer to a null object", PyExc_AttributeError);
    }

    return v8_scope.Escape(object->Object());
  }

  v8::Local<v8::Value> v8_result;

  // TODO: replace this with pybind code
  if (PyLong_CheckExact(py_handle.ptr())) {
    v8_result = v8::Integer::New(v8_isolate, PyLong_AsLong(py_handle.ptr()));
  } else if (PyBool_Check(py_handle.ptr())) {
    v8_result = v8::Boolean::New(v8_isolate, py::cast<py::bool_>(py_handle));
  } else if (PyBytes_CheckExact(py_handle.ptr()) || PyUnicode_CheckExact(py_handle.ptr())) {
    v8_result = v8u::toString(py_handle);
  } else if (PyFloat_CheckExact(py_handle.ptr())) {
    v8_result = v8::Number::New(v8_isolate, py::cast<py::float_>(py_handle));
  } else if (isExactDateTime(py_handle) || isExactDate(py_handle)) {
    tm ts = {0};
    int ms = 0;
    getPythonDateTime(py_handle, ts, ms);
    v8_result = v8::Date::New(v8_isolate->GetCurrentContext(), (static_cast<double>(mktime(&ts))) * 1000 + ms / 1000)
                    .ToLocalChecked();
  } else if (isExactTime(py_handle)) {
    tm ts = {0};
    int ms = 0;
    getPythonTime(py_handle, ts, ms);
    v8_result = v8::Date::New(v8_isolate->GetCurrentContext(), (static_cast<double>(mktime(&ts))) * 1000 + ms / 1000)
                    .ToLocalChecked();
  } else {
    auto v8_object_template = GetCachedObjectTemplateOrCreate(v8_isolate);
    auto v8_object_instance = v8_object_template->NewInstance(v8_isolate->GetCurrentContext()).ToLocalChecked();
    assert(!v8_object_instance.IsEmpty());
    traceWrapper(py_handle.ptr(), v8_object_instance);
    v8_result = v8_object_instance;
  }

  if (v8_result.IsEmpty()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  return v8_scope.Escape(v8_result);
}

#pragma clang diagnostic pop