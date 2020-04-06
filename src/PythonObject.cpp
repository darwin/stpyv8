#include "PythonObject.h"
#include "JSObject.h"
#include "JSObjectNull.h"
#include "JSObjectUndefined.h"
#include "JSObjectArray.h"
#include "Isolate.h"
#include "PythonDateTime.h"
#include "Tracer.h"
#include "PythonGIL.h"
#include "PythonExceptionGuard.h"

// static std::ostream& operator<<(std::ostream& os, const CJSObject& obj) {
//  obj.Dump(os);
//
//  return os;
//}

static std::ostream& operator<<(std::ostream& os, const CJSObjectPtr& obj) {
  obj->Dump(os);
  return os;
}

// void CPythonObject::ThrowIf(v8::Isolate* isolate) {
//  CPythonGIL python_gil;
//
//  assert(PyErr_Occurred());
//
//  v8::HandleScope handle_scope(isolate);
//
//  PyObject *exc, *val, *trb;
//
//  PyErr_Fetch(&exc, &val, &trb);
//  PyErr_NormalizeException(&exc, &val, &trb);
//
//  py::object type(py::handle<>(py::allow_null(exc))), value(py::handle<>(py::allow_null(val)));
//
//  if (trb)
//    py::decref(trb);
//
//  std::string msg;
//
//  if (PyObject_HasAttrString(value.ptr(), "args")) {
//    py::object args = value.attr("args");
//
//    if (PyTuple_Check(args.ptr())) {
//      for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(args.ptr()); i++) {
//        py::extract<const std::string> extractor(args[i]);
//
//        if (extractor.check())
//          msg += extractor();
//      }
//    }
//  } else if (PyObject_HasAttrString(value.ptr(), "message")) {
//    py::extract<const std::string> extractor(value.attr("message"));
//
//    if (extractor.check())
//      msg = extractor();
//  } else if (val) {
//    if (PyBytes_CheckExact(val)) {
//      msg = PyBytes_AS_STRING(val);
//    } else if (PyTuple_CheckExact(val)) {
//      for (int i = 0; i < PyTuple_GET_SIZE(val); i++) {
//        PyObject* item = PyTuple_GET_ITEM(val, i);
//
//        if (item && PyBytes_CheckExact(item)) {
//          msg = PyBytes_AS_STRING(item);
//          break;
//        }
//      }
//    }
//  }
//
//  v8::Local<v8::Value> error;
//
//  if (PyErr_GivenExceptionMatches(type.ptr(), PyExc_IndexError)) {
//    error = v8::Exception::RangeError(
//        v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
//  } else if (PyErr_GivenExceptionMatches(type.ptr(), PyExc_AttributeError)) {
//    error = v8::Exception::ReferenceError(
//        v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
//  } else if (PyErr_GivenExceptionMatches(type.ptr(), PyExc_SyntaxError)) {
//    error = v8::Exception::SyntaxError(
//        v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
//  } else if (PyErr_GivenExceptionMatches(type.ptr(), PyExc_TypeError)) {
//    error = v8::Exception::TypeError(
//        v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
//  } else {
//    error = v8::Exception::Error(
//        v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
//  }
//
//  if (error->IsObject()) {
//    v8::Local<v8::Context> ctxt = isolate->GetCurrentContext();
//
//    v8::Local<v8::String> key_type = v8::String::NewFromUtf8(isolate, "exc_type").ToLocalChecked();
//    v8::Local<v8::Private> privateKey_type = v8::Private::ForApi(isolate, key_type);
//
//    v8::Local<v8::String> key_value = v8::String::NewFromUtf8(isolate, "exc_value").ToLocalChecked();
//    v8::Local<v8::Private> privateKey_value = v8::Private::ForApi(isolate, key_value);
//
//    error->ToObject(ctxt).ToLocalChecked()->SetPrivate(
//        ctxt, privateKey_type, v8::External::New(isolate, ObjectTracer::Trace(error, new py::object(type)).Object()));
//    error->ToObject(ctxt).ToLocalChecked()->SetPrivate(
//        ctxt, privateKey_value, v8::External::New(isolate, ObjectTracer::Trace(error, new
//        py::object(value)).Object()));
//  }
//
//  isolate->ThrowException(error);
//}

void CPythonObject::ThrowIf(v8::Isolate* v8_isolate, const pb::error_already_set& e) {
  CPythonGIL python_gil;
  //  assert(PyErr_Occurred());

  auto v8_scope = v8u::getScope(v8_isolate);

  //  PyObject* raw_exc;
  //  PyObject* raw_val;
  //  PyObject* raw_trb;
  //
  //  PyErr_Fetch(&raw_exc, &raw_val, &raw_trb);
  //  // TODO: investigate: don't we optionally get new references here?
  //  PyErr_NormalizeException(&raw_exc, &raw_val, &raw_trb);

  //  pb::object py_type(pb::reinterpret_steal<pb::object>(raw_exc));
  //  pb::object py_value(pb::reinterpret_steal<pb::object>(raw_val));
  //  pb::object py_trb(pb::reinterpret_steal<pb::object>(raw_trb));

  pb::object py_type(e.type());
  pb::object py_value(e.value());
  pb::object py_trb(e.trace());

  PyObject* raw_val = py_value.ptr();

  //  TODO: investigate: shall we call normalize, pybind does not call it
  //  PyErr_NormalizeException(&raw_exc, &raw_val, &raw_trb);

  std::string msg;

  if (pb::hasattr(py_value, "args")) {
    auto py_args = py_value.attr("args");
    if (pb::isinstance<pb::tuple>(py_args)) {
      auto py_args_tuple = pb::cast<pb::tuple>(py_args);
      auto it = py_args_tuple.begin();
      while (it != py_args_tuple.end()) {
        auto py_arg = *it;
        if (pb::isinstance<pb::str>(py_arg)) {
          msg += pb::cast<pb::str>(py_arg);
        }
        it++;
      }
    }
  } else if (pb::hasattr(py_value, "message")) {
    auto py_msg = py_value.attr("message");
    if (pb::isinstance<pb::str>(py_msg)) {
      msg += pb::cast<pb::str>(py_msg);
    }
  } else if (raw_val) {
    // TODO: use pybind
    if (PyBytes_CheckExact(raw_val)) {
      msg = PyBytes_AS_STRING(raw_val);
    } else if (PyTuple_CheckExact(raw_val)) {
      for (int i = 0; i < PyTuple_GET_SIZE(raw_val); i++) {
        PyObject* raw_item = PyTuple_GET_ITEM(raw_val, i);

        if (raw_item && PyBytes_CheckExact(raw_item)) {
          msg = PyBytes_AS_STRING(raw_item);
          break;
        }
      }
    }
  }

  v8::Local<v8::Value> v8_error;

  if (PyErr_GivenExceptionMatches(py_type.ptr(), PyExc_IndexError)) {
    auto v8_msg =
        v8::String::NewFromUtf8(v8_isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked();
    v8_error = v8::Exception::RangeError(v8_msg);
  } else if (PyErr_GivenExceptionMatches(py_type.ptr(), PyExc_AttributeError)) {
    auto v8_msg =
        v8::String::NewFromUtf8(v8_isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked();
    v8_error = v8::Exception::ReferenceError(v8_msg);
  } else if (PyErr_GivenExceptionMatches(py_type.ptr(), PyExc_SyntaxError)) {
    auto v8_msg =
        v8::String::NewFromUtf8(v8_isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked();
    v8_error = v8::Exception::SyntaxError(v8_msg);
  } else if (PyErr_GivenExceptionMatches(py_type.ptr(), PyExc_TypeError)) {
    auto v8_msg =
        v8::String::NewFromUtf8(v8_isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked();
    v8_error = v8::Exception::TypeError(v8_msg);
  } else {
    auto v8_msg =
        v8::String::NewFromUtf8(v8_isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked();
    v8_error = v8::Exception::Error(v8_msg);
  }

  if (v8_error->IsObject()) {
    auto v8_context = v8_isolate->GetCurrentContext();

    auto v8_exc_type_key = v8::String::NewFromUtf8(v8_isolate, "exc_type").ToLocalChecked();
    auto v8_exc_value_key = v8::String::NewFromUtf8(v8_isolate, "exc_value").ToLocalChecked();

    auto v8_exc_type_api = v8::Private::ForApi(v8_isolate, v8_exc_type_key);
    auto v8_exc_value_api = v8::Private::ForApi(v8_isolate, v8_exc_value_key);

//    auto py_exc_type_ptr = ObjectTracer2::Trace(v8_error, py_type.ptr()).Object();
//    auto py_exc_value_ptr = ObjectTracer2::Trace(v8_error, py_value.ptr()).Object();

    Py_INCREF(py_type.ptr());
    Py_INCREF(py_value.ptr());

    auto v8_exc_type_external = v8::External::New(v8_isolate, py_type.ptr());
    auto v8_exc_value_external = v8::External::New(v8_isolate, py_value.ptr());

    v8_error->ToObject(v8_context).ToLocalChecked()->SetPrivate(v8_context, v8_exc_type_api, v8_exc_type_external);
    v8_error->ToObject(v8_context).ToLocalChecked()->SetPrivate(v8_context, v8_exc_value_api, v8_exc_value_external);
  }

  v8_isolate->ThrowException(v8_error);
}

void CPythonObject::NamedGetter(v8::Local<v8::Name> v8_prop_name, const v8::PropertyCallbackInfo<v8::Value>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  if (v8_prop_name->IsSymbol()) {
    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }

  auto result = withPythonExceptionGuard<v8::Local<v8::Value>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());

    auto v8_utf_name = v8u::toUtf8Value(v8_isolate, v8_prop_name.As<v8::String>());

    // TODO: use pybind
    if (PyGen_Check(py_obj.ptr())) {
      return v8::Undefined(v8_isolate).As<v8::Value>();
    }

    if (!*v8_utf_name) {
      return v8::Undefined(v8_isolate).As<v8::Value>();
    }

    pb::object py_val;
    try {
      py_val = pb::getattr(py_obj, *v8_utf_name);
    } catch (const pb::error_already_set& e) {
      if (e.matches(PyExc_AttributeError)) {
        // TODO: revisit this, what is the difference between mapping and hasattr?
        if (PyMapping_Check(py_obj.ptr()) && PyMapping_HasKeyString(py_obj.ptr(), *v8_utf_name)) {
          auto raw_item = PyMapping_GetItemString(py_obj.ptr(), *v8_utf_name);
          pb::object py_result(pb::reinterpret_steal<pb::object>(raw_item));
          return Wrap(py_result);
        }
        return v8::Local<v8::Value>();
      } else {
        throw e;
      }
    }

    if (PyObject_TypeCheck(py_val.ptr(), &PyProperty_Type)) {
      auto getter = py_val.attr("fget");

      if (getter.is_none()) {
        throw CJSException("unreadable attribute", PyExc_AttributeError);
      }

      py_val = getter();
    }

    return Wrap(py_val);
  });

  auto result_handle = value_or(result, [&]() { return v8::Undefined(v8_isolate); });
  v8_info.GetReturnValue().Set(result_handle);
}

//
// void CPythonObject::NamedGetter(v8::Local<v8::Name> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
//  auto isolate = info.GetIsolate();
//  if (prop->IsSymbol()) {
//    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
//    info.GetReturnValue().Set(v8::Undefined(isolate));
//    return;
//  }
//  v8::HandleScope handle_scope(isolate);
//
//  if (v8u::executionTerminating(isolate)) {
//    info.GetReturnValue().Set(v8::Undefined(isolate));
//    return;
//  }
//
//  auto result = withPythonExceptionGuard<v8::Local<v8::Value>>(isolate, [&]() {
//    CPythonGIL python_gil;
//
//    py::object obj = CJSObject::Wrap(info.Holder());
//
//    v8::String::Utf8Value name(isolate, v8::Local<v8::String>::Cast(prop));
//
//    if (PyGen_Check(obj.ptr())) {
//      return v8::Undefined(isolate).As<v8::Value>();
//    }
//
//    if (*name == nullptr) {
//      return v8::Undefined(isolate).As<v8::Value>();
//    }
//
//    PyObject* value = PyObject_GetAttrString(obj.ptr(), *name);
//
//    if (!value) {
//      if (PyErr_Occurred()) {
//        if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
//          PyErr_Clear();
//        } else {
//          py::throw_error_already_set();
//        }
//      }
//
//      if (PyMapping_Check(obj.ptr()) && PyMapping_HasKeyString(obj.ptr(), *name)) {
//        py::object result(py::handle<>(PyMapping_GetItemString(obj.ptr(), *name)));
//
//        if (!result.is_none()) {
//          return Wrap(result);
//        }
//      }
//
//      return v8::Local<v8::Value>();
//    }
//
//    py::object attr = py::object(py::handle<>(value));
//
//    if (PyObject_TypeCheck(attr.ptr(), &PyProperty_Type)) {
//      py::object getter = attr.attr("fget");
//
//      if (getter.is_none()) {
//        throw CJavascriptException("unreadable attribute", PyExc_AttributeError);
//      }
//
//      attr = getter();
//    }
//
//    return Wrap(attr);
//  });
//
//  auto result_handle = value_or(result, [&]() { return v8::Undefined(isolate); });
//  info.GetReturnValue().Set(result_handle);
//}

void CPythonObject::NamedSetter(v8::Local<v8::Name> v8_prop_name,
                                v8::Local<v8::Value> v8_value,
                                const v8::PropertyCallbackInfo<v8::Value>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  if (v8_prop_name->IsSymbol()) {
    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Value>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());

    v8::String::Utf8Value v8_utf_name(v8_isolate, v8_prop_name);
    auto py_val = CJSObject::Wrap(v8_value);
    bool found = pb::hasattr(py_obj, *v8_utf_name);

    // TODO: review this after learning about __watchpoints__
    if (pb::hasattr(py_obj, "__watchpoints__")) {
      pb::dict py_watchpoints(py_obj.attr("__watchpoints__"));
      pb::str py_prop_name(*v8_utf_name, v8_utf_name.length());
      if (py_watchpoints.contains(py_prop_name)) {
        auto py_watchhandler = py_watchpoints[py_prop_name];
        auto py_attr = found ? (pb::object)py_obj.attr(*v8_utf_name) : pb::none();
        py_val = py_watchhandler(py_prop_name, py_attr, py_val);
      }
    }

    // TODO: revisit
    if (!found && PyMapping_Check(py_obj.ptr())) {
      PyMapping_SetItemString(py_obj.ptr(), *v8_utf_name, py_val.ptr());
    } else {
      if (found) {
        auto py_name_attr = py_obj.attr(*v8_utf_name);

        if (PyObject_TypeCheck(py_name_attr.ptr(), &PyProperty_Type)) {
          auto setter = py_name_attr.attr("fset");

          if (setter.is_none()) {
            throw CJSException("can't set attribute", PyExc_AttributeError);
          }

          setter(py_val);
          return v8_value;
        }
      }

      py_obj.attr(*v8_utf_name) = py_val;
    }

    return v8_value;
  });

  auto v8_final_result = value_or(v8_result, [&]() { return v8::Undefined(v8_isolate); });
  v8_info.GetReturnValue().Set(v8_final_result);
}

// void CPythonObject::NamedSetter(v8::Local<v8::Name> prop,
//                                v8::Local<v8::Value> value,
//                                const v8::PropertyCallbackInfo<v8::Value>& info) {
//  auto isolate = info.GetIsolate();
//  if (prop->IsSymbol()) {
//    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
//    info.GetReturnValue().Set(v8::Undefined(isolate));
//    return;
//  }
//  v8::HandleScope handle_scope(isolate);
//
//  if (v8u::executionTerminating(isolate)) {
//    info.GetReturnValue().Set(v8::Undefined(isolate));
//    return;
//  }
//
//  auto result = withPythonExceptionGuard<v8::Local<v8::Value>>(isolate, [&]() {
//    CPythonGIL python_gil;
//
//    py::object obj = CJSObject::Wrap(info.Holder());
//
//    v8::String::Utf8Value name(isolate, prop);
//    py::object newval = CJSObject::Wrap(value);
//
//    bool found = 1 == PyObject_HasAttrString(obj.ptr(), *name);
//
//    if (PyObject_HasAttrString(obj.ptr(), "__watchpoints__")) {
//      py::dict watchpoints(obj.attr("__watchpoints__"));
//      py::str propname(*name, name.length());
//
//      if (watchpoints.has_key(propname)) {
//        py::object watchhandler = watchpoints.get(propname);
//        newval = watchhandler(propname, found ? obj.attr(propname) : py::object(), newval);
//      }
//    }
//
//    if (!found && PyMapping_Check(obj.ptr())) {
//      PyMapping_SetItemString(obj.ptr(), *name, newval.ptr());
//    } else {
//      if (found) {
//        py::object attr = obj.attr(*name);
//
//        if (PyObject_TypeCheck(attr.ptr(), &PyProperty_Type)) {
//          py::object setter = attr.attr("fset");
//
//          if (setter.is_none()) {
//            throw CJavascriptException("can't set attribute", PyExc_AttributeError);
//          }
//
//          setter(newval);
//          return value;
//        }
//      }
//      obj.attr(*name) = newval;
//    }
//
//    return value;
//  });
//
//  auto result_handle = value_or(result, [&]() { return v8::Undefined(isolate); });
//  info.GetReturnValue().Set(result_handle);
//}

void CPythonObject::NamedQuery(v8::Local<v8::Name> v8_prop_name, const v8::PropertyCallbackInfo<v8::Integer>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  if (v8_prop_name->IsSymbol()) {
    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
    v8_info.GetReturnValue().Set(v8::Local<v8::Integer>());
    return;
  }
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Integer>());
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Integer>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());
    v8::String::Utf8Value name(v8_isolate, v8_prop_name);

    // TODO: rewrite using pybind
    bool exists = PyGen_Check(py_obj.ptr()) || PyObject_HasAttrString(py_obj.ptr(), *name) ||
                  (PyMapping_Check(py_obj.ptr()) && PyMapping_HasKeyString(py_obj.ptr(), *name));

    if (exists) {
      return v8::Integer::New(v8_isolate, v8::None);
    } else {
      return v8::Local<v8::Integer>();
    }
  });

  auto v8_final_result = value_or(v8_result, [&]() { return v8::Local<v8::Integer>(); });
  v8_info.GetReturnValue().Set(v8_final_result);
}

// void CPythonObject::NamedQuery(v8::Local<v8::Name> prop, const v8::PropertyCallbackInfo<v8::Integer>& info) {
//  auto isolate = info.GetIsolate();
//  if (prop->IsSymbol()) {
//    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
//    info.GetReturnValue().Set(v8::Local<v8::Integer>());
//    return;
//  }
//  v8::HandleScope handle_scope(isolate);
//
//  if (v8u::executionTerminating(isolate)) {
//    info.GetReturnValue().Set(v8::Local<v8::Integer>());
//    return;
//  }
//
//  auto result = withPythonExceptionGuard<v8::Local<v8::Integer>>(isolate, [&]() {
//    CPythonGIL python_gil;
//
//    py::object obj = CJSObject::Wrap(info.Holder());
//
//    v8::String::Utf8Value name(isolate, prop);
//
//    bool exists = PyGen_Check(obj.ptr()) || PyObject_HasAttrString(obj.ptr(), *name) ||
//        (PyMapping_Check(obj.ptr()) && PyMapping_HasKeyString(obj.ptr(), *name));
//
//    if (exists) {
//      return v8::Integer::New(isolate, v8::None);
//    } else {
//      return v8::Local<v8::Integer>();
//    }
//  });
//
//  auto result_handle = value_or(result, [&]() { return v8::Local<v8::Integer>(); });
//  info.GetReturnValue().Set(result_handle);
//}

void CPythonObject::NamedDeleter(v8::Local<v8::Name> v8_prop_name,
                                 const v8::PropertyCallbackInfo<v8::Boolean>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  if (v8_prop_name->IsSymbol()) {
    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
    v8_info.GetReturnValue().Set(v8::Local<v8::Boolean>());
    return;
  }
  v8::HandleScope handle_scope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Boolean>());
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Boolean>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());
    v8::String::Utf8Value name(v8_isolate, v8_prop_name);

    // TODO: rewrite using pybind
    if (!PyObject_HasAttrString(py_obj.ptr(), *name) && PyMapping_Check(py_obj.ptr()) &&
        PyMapping_HasKeyString(py_obj.ptr(), *name)) {
      return v8::Boolean::New(v8_isolate, -1 != PyMapping_DelItemString(py_obj.ptr(), *name));
    } else {
      auto py_name_attr = py_obj.attr(*name);

      if (PyObject_HasAttrString(py_obj.ptr(), *name) && PyObject_TypeCheck(py_name_attr.ptr(), &PyProperty_Type)) {
        auto py_deleter = py_name_attr.attr("fdel");

        if (py_deleter.is_none()) {
          throw CJSException("can't delete attribute", PyExc_AttributeError);
        }
        auto py_result = py_deleter();
        auto py_bool_result = pb::cast<pb::bool_>(py_result);
        return v8::Boolean::New(v8_isolate, py_bool_result);
      } else {
        auto result = -1 != PyObject_DelAttrString(py_obj.ptr(), *name);
        return v8::Boolean::New(v8_isolate, result);
      }
    }
  });

  auto result_handle = value_or(v8_result, [&]() { return v8::Local<v8::Boolean>(); });
  v8_info.GetReturnValue().Set(result_handle);
}

// void CPythonObject::NamedDeleter(v8::Local<v8::Name> prop, const v8::PropertyCallbackInfo<v8::Boolean>& info) {
//  auto isolate = info.GetIsolate();
//  if (prop->IsSymbol()) {
//    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
//    info.GetReturnValue().Set(v8::Local<v8::Boolean>());
//    return;
//  }
//  v8::HandleScope handle_scope(isolate);
//
//  if (v8u::executionTerminating(isolate)) {
//    info.GetReturnValue().Set(v8::Local<v8::Boolean>());
//    return;
//  }
//
//  auto result = withPythonExceptionGuard<v8::Local<v8::Boolean>>(isolate, [&]() {
//    CPythonGIL python_gil;
//
//    py::object obj = CJSObject::Wrap(info.Holder());
//
//    v8::String::Utf8Value name(isolate, prop);
//
//    if (!PyObject_HasAttrString(obj.ptr(), *name) && PyMapping_Check(obj.ptr()) &&
//        PyMapping_HasKeyString(obj.ptr(), *name)) {
//      return v8::Boolean::New(isolate, -1 != PyMapping_DelItemString(obj.ptr(), *name));
//    } else {
//      py::object attr = obj.attr(*name);
//
//      if (PyObject_HasAttrString(obj.ptr(), *name) && PyObject_TypeCheck(attr.ptr(), &PyProperty_Type)) {
//        py::object deleter = attr.attr("fdel");
//
//        if (deleter.is_none())
//          throw CJavascriptException("can't delete attribute", PyExc_AttributeError);
//
//        return v8::Boolean::New(isolate, py::extract<bool>(deleter()));
//      } else {
//        return v8::Boolean::New(isolate, -1 != PyObject_DelAttrString(obj.ptr(), *name));
//      }
//    }
//  });
//
//  auto result_handle = value_or(result, [&]() { return v8::Local<v8::Boolean>(); });
//  info.GetReturnValue().Set(result_handle);
//}

void CPythonObject::NamedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  v8::HandleScope handle_scope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Array>());
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Array>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());

    pb::list keys;
    bool filter_name = false;

    if (PySequence_Check(py_obj.ptr())) {
      return v8::Local<v8::Array>();
    } else if (PyMapping_Check(py_obj.ptr())) {
      keys = pb::reinterpret_steal<pb::list>(PyMapping_Keys(py_obj.ptr()));
    } else if (PyGen_CheckExact(py_obj.ptr())) {
      auto py_iter(pb::reinterpret_steal<pb::object>(PyObject_GetIter(py_obj.ptr())));

      PyObject* raw_item = nullptr;

      while (nullptr != (raw_item = PyIter_Next(py_iter.ptr()))) {
        keys.append(pb::reinterpret_steal<pb::object>(raw_item));
      }
    } else {
      keys = pb::reinterpret_steal<pb::list>(PyObject_Dir(py_obj.ptr()));
      filter_name = true;
    }

    Py_ssize_t len = PyList_GET_SIZE(keys.ptr());
    v8::Local<v8::Array> v8_array = v8::Array::New(v8_isolate, len);
    for (Py_ssize_t i = 0; i < len; i++) {
      PyObject* raw_item = PyList_GET_ITEM(keys.ptr(), i);

      if (filter_name && PyBytes_CheckExact(raw_item)) {
        std::string name(pb::reinterpret_borrow<pb::str>(raw_item));

        // FIXME: Are there any methods to avoid such a dirty work?
        if (name.find("__", 0) == 0 && name.rfind("__", name.size() - 2)) {
          continue;
        }
      }

      auto py_item = Wrap(pb::reinterpret_borrow<pb::object>(raw_item));
      auto v8_i = v8::Uint32::New(v8_isolate, i);
      auto res = v8_array->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), v8_i, py_item);
      res.Check();
    }
    return v8_array;
  });

  auto result_handle = value_or(v8_result, [&]() { return v8::Local<v8::Array>(); });
  v8_info.GetReturnValue().Set(result_handle);
}

// void CPythonObject::NamedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info) {
//  auto isolate = info.GetIsolate();
//  v8::HandleScope handle_scope(isolate);
//
//  if (v8u::executionTerminating(isolate)) {
//    info.GetReturnValue().Set(v8::Local<v8::Array>());
//    return;
//  }
//
//  auto result = withPythonExceptionGuard<v8::Local<v8::Array>>(isolate, [&]() {
//    CPythonGIL python_gil;
//
//    py::object obj = CJSObject::Wrap(info.Holder());
//
//    py::list keys;
//    bool filter_name = false;
//
//    if (PySequence_Check(obj.ptr())) {
//      return v8::Local<v8::Array>();
//    } else if (PyMapping_Check(obj.ptr())) {
//      keys = py::list(py::handle<>(PyMapping_Keys(obj.ptr())));
//    } else if (PyGen_CheckExact(obj.ptr())) {
//      py::object iter(py::handle<>(PyObject_GetIter(obj.ptr())));
//
//      PyObject* item = NULL;
//
//      while (NULL != (item = PyIter_Next(iter.ptr()))) {
//        keys.append(py::object(py::handle<>(item)));
//      }
//    } else {
//      keys = py::list(py::handle<>(PyObject_Dir(obj.ptr())));
//      filter_name = true;
//    }
//
//    Py_ssize_t len = PyList_GET_SIZE(keys.ptr());
//    v8::Local<v8::Array> result = v8::Array::New(isolate, len);
//    for (Py_ssize_t i = 0; i < len; i++) {
//      PyObject* item = PyList_GET_ITEM(keys.ptr(), i);
//
//      if (filter_name && PyBytes_CheckExact(item)) {
//        py::str name(py::handle<>(py::borrowed(item)));
//
//        // FIXME Are there any methods to avoid such a dirty work?
//
//        if (name.startswith("__") && name.endswith("__")) {
//          continue;
//        }
//      }
//
//      auto res = result->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), v8::Uint32::New(isolate, i),
//                             Wrap(py::object(py::handle<>(py::borrowed(item)))));
//      res.Check();
//    }
//    return result;
//  });
//
//  auto result_handle = value_or(result, [&]() { return v8::Local<v8::Array>(); });
//  info.GetReturnValue().Set(result_handle);
//}

void CPythonObject::IndexedGetter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Value>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());

    if (PyGen_Check(py_obj.ptr())) {
      return v8::Undefined(v8_isolate).As<v8::Value>();
    }

    if (PySequence_Check(py_obj.ptr())) {
      if ((Py_ssize_t)index < PySequence_Size(py_obj.ptr())) {
        auto ret(pb::reinterpret_steal<pb::object>(PySequence_GetItem(py_obj.ptr(), index)));
        return Wrap(ret);
      } else {
        return v8::Undefined(v8_isolate).As<v8::Value>();
      }
    }

    if (PyMapping_Check(py_obj.ptr())) {
      char buf[65];

      snprintf(buf, sizeof(buf), "%d", index);

      PyObject* raw_value = PyMapping_GetItemString(py_obj.ptr(), buf);

      if (!raw_value) {
        pb::int_ py_index(index);
        raw_value = PyObject_GetItem(py_obj.ptr(), py_index.ptr());
      }

      if (raw_value) {
        return Wrap(pb::reinterpret_steal<pb::object>(raw_value));
      } else {
        return v8::Undefined(v8_isolate).As<v8::Value>();
      }
    }

    return v8::Undefined(v8_isolate).As<v8::Value>();
  });

  auto result_handle = value_or(v8_result, [&]() { return v8::Undefined(v8_isolate); });
  v8_info.GetReturnValue().Set(result_handle);
}

// void CPythonObject::IndexedGetter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info) {
//  auto isolate = info.GetIsolate();
//  v8::HandleScope handle_scope(isolate);
//
//  if (v8u::executionTerminating(isolate)) {
//    info.GetReturnValue().Set(v8::Undefined(isolate));
//    return;
//  }
//
//  auto result = withPythonExceptionGuard<v8::Local<v8::Value>>(isolate, [&]() {
//    CPythonGIL python_gil;
//
//    py::object obj = CJSObject::Wrap(info.Holder());
//
//    if (PyGen_Check(obj.ptr())) {
//      return v8::Undefined(isolate).As<v8::Value>();
//    }
//
//    if (PySequence_Check(obj.ptr())) {
//      if ((Py_ssize_t)index < PySequence_Size(obj.ptr())) {
//        py::object ret(py::handle<>(PySequence_GetItem(obj.ptr(), index)));
//
//        return Wrap(ret);
//      } else {
//        return v8::Undefined(isolate).As<v8::Value>();
//      }
//    }
//
//    if (PyMapping_Check(obj.ptr())) {
//      char buf[65];
//
//      snprintf(buf, sizeof(buf), "%d", index);
//
//      PyObject* value = PyMapping_GetItemString(obj.ptr(), buf);
//
//      if (!value) {
//        py::long_ key(index);
//
//        value = PyObject_GetItem(obj.ptr(), key.ptr());
//      }
//
//      if (value) {
//        return Wrap(py::object(py::handle<>(value)));
//      } else {
//        return v8::Undefined(isolate).As<v8::Value>();
//      }
//    }
//
//    return v8::Undefined(isolate).As<v8::Value>();
//  });
//
//  auto result_handle = value_or(result, [&]() { return v8::Undefined(isolate); });
//  info.GetReturnValue().Set(result_handle);
//}

void CPythonObject::IndexedSetter(uint32_t index,
                                  v8::Local<v8::Value> v8_value,
                                  const v8::PropertyCallbackInfo<v8::Value>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Value>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());

    if (PySequence_Check(py_obj.ptr())) {
      if (PySequence_SetItem(py_obj.ptr(), index, CJSObject::Wrap(v8_value).ptr()) < 0) {
        auto v8_msg = v8::String::NewFromUtf8(v8_isolate, "fail to set indexed value").ToLocalChecked();
        auto v8_ex = v8::Exception::Error(v8_msg);
        v8_isolate->ThrowException(v8_ex);
      }
    } else if (PyMapping_Check(py_obj.ptr())) {
      char buf[65];

      snprintf(buf, sizeof(buf), "%d", index);

      if (PyMapping_SetItemString(py_obj.ptr(), buf, CJSObject::Wrap(v8_value).ptr()) < 0) {
        auto v8_msg = v8::String::NewFromUtf8(v8_isolate, "fail to set named value").ToLocalChecked();
        auto v8_ex = v8::Exception::Error(v8_msg);
        v8_isolate->ThrowException(v8_ex);
      }
    }

    return v8_value;
  });

  auto v8_final_result = value_or(v8_result, [&]() { return v8::Undefined(v8_isolate); });
  v8_info.GetReturnValue().Set(v8_final_result);
}

// void CPythonObject::IndexedSetter(uint32_t index,
//                                  v8::Local<v8::Value> value,
//                                  const v8::PropertyCallbackInfo<v8::Value>& info) {
//  auto isolate = info.GetIsolate();
//  v8::HandleScope handle_scope(isolate);
//
//  if (v8u::executionTerminating(isolate)) {
//    info.GetReturnValue().Set(v8::Undefined(isolate));
//    return;
//  }
//
//  auto result = withPythonExceptionGuard<v8::Local<v8::Value>>(isolate, [&]() {
//    CPythonGIL python_gil;
//
//    py::object obj = CJSObject::Wrap(info.Holder());
//
//    if (PySequence_Check(obj.ptr())) {
//      if (PySequence_SetItem(obj.ptr(), index, CJSObject::Wrap(value).ptr()) < 0)
//        isolate->ThrowException(
//            v8::Exception::Error(v8::String::NewFromUtf8(isolate, "fail to set indexed value").ToLocalChecked()));
//    } else if (PyMapping_Check(obj.ptr())) {
//      char buf[65];
//
//      snprintf(buf, sizeof(buf), "%d", index);
//
//      if (PyMapping_SetItemString(obj.ptr(), buf, CJSObject::Wrap(value).ptr()) < 0)
//        isolate->ThrowException(
//            v8::Exception::Error(v8::String::NewFromUtf8(isolate, "fail to set named value").ToLocalChecked()));
//    }
//
//    return value;
//  });
//
//  auto result_handle = value_or(result, [&]() { return v8::Undefined(isolate); });
//  info.GetReturnValue().Set(result_handle);
//}

void CPythonObject::IndexedQuery(uint32_t index, const v8::PropertyCallbackInfo<v8::Integer>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Integer>());
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Integer>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());

    if (PyGen_Check(py_obj.ptr())) {
      return v8::Integer::New(v8_isolate, v8::ReadOnly);
    }

    if (PySequence_Check(py_obj.ptr())) {
      if ((Py_ssize_t)index < PySequence_Size(py_obj.ptr())) {
        return v8::Integer::New(v8_isolate, v8::None);
      } else {
        return v8::Local<v8::Integer>();
      }
    }

    if (PyMapping_Check(py_obj.ptr())) {
      // TODO: revisit this
      char buf[65];
      snprintf(buf, sizeof(buf), "%d", index);

      auto py_index = pb::int_(index);
      if (PyMapping_HasKeyString(py_obj.ptr(), buf) || PyMapping_HasKey(py_obj.ptr(), py_index.ptr())) {
        return v8::Integer::New(v8_isolate, v8::None);
      } else {
        return v8::Local<v8::Integer>();
      }
    }

    return v8::Local<v8::Integer>();
  });

  auto v8_final_result = value_or(v8_result, [&]() { return v8::Local<v8::Integer>(); });
  v8_info.GetReturnValue().Set(v8_final_result);
}

// void CPythonObject::IndexedQuery(uint32_t index, const v8::PropertyCallbackInfo<v8::Integer>& info) {
//  auto isolate = info.GetIsolate();
//  v8::HandleScope handle_scope(isolate);
//
//  if (v8u::executionTerminating(isolate)) {
//    info.GetReturnValue().Set(v8::Local<v8::Integer>());
//    return;
//  }
//
//  auto result = withPythonExceptionGuard<v8::Local<v8::Integer>>(isolate, [&]() {
//    CPythonGIL python_gil;
//
//    py::object obj = CJSObject::Wrap(info.Holder());
//
//    if (PyGen_Check(obj.ptr())) {
//      return v8::Integer::New(isolate, v8::ReadOnly);
//    }
//
//    if (PySequence_Check(obj.ptr())) {
//      if ((Py_ssize_t)index < PySequence_Size(obj.ptr())) {
//        return v8::Integer::New(isolate, v8::None);
//      } else {
//        return v8::Local<v8::Integer>();
//      }
//    }
//
//    if (PyMapping_Check(obj.ptr())) {
//      // TODO: revisit this
//      char buf[65];
//      snprintf(buf, sizeof(buf), "%d", index);
//
//      if (PyMapping_HasKeyString(obj.ptr(), buf) || PyMapping_HasKey(obj.ptr(), py::long_(index).ptr())) {
//        return v8::Integer::New(isolate, v8::None);
//      } else {
//        return v8::Local<v8::Integer>();
//      }
//    }
//
//    return v8::Local<v8::Integer>();
//  });
//
//  auto result_handle = value_or(result, [&]() { return v8::Local<v8::Integer>(); });
//  info.GetReturnValue().Set(result_handle);
//}

void CPythonObject::IndexedDeleter(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Boolean>());
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Boolean>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());

    if (PySequence_Check(py_obj.ptr()) && (Py_ssize_t)index < PySequence_Size(py_obj.ptr())) {
      auto result = 0 <= PySequence_DelItem(py_obj.ptr(), index);
      return v8::Boolean::New(v8_isolate, result);
    }
    if (PyMapping_Check(py_obj.ptr())) {
      char buf[65];

      snprintf(buf, sizeof(buf), "%d", index);
      auto result = PyMapping_DelItemString(py_obj.ptr(), buf) == 0;
      return v8::Boolean::New(v8_isolate, result);
    }
    return v8::Local<v8::Boolean>();
  });

  auto v8_final_result = value_or(v8_result, [&]() { return v8::Local<v8::Boolean>(); });
  v8_info.GetReturnValue().Set(v8_final_result);
}

// void CPythonObject::IndexedDeleter(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean>& info) {
//  auto isolate = info.GetIsolate();
//  v8::HandleScope handle_scope(isolate);
//
//  if (v8u::executionTerminating(isolate)) {
//    info.GetReturnValue().Set(v8::Local<v8::Boolean>());
//    return;
//  }
//
//  auto result = withPythonExceptionGuard<v8::Local<v8::Boolean>>(isolate, [&]() {
//    CPythonGIL python_gil;
//
//    py::object obj = CJSObject::Wrap(info.Holder());
//
//    if (PySequence_Check(obj.ptr()) && (Py_ssize_t)index < PySequence_Size(obj.ptr())) {
//      auto result = 0 <= PySequence_DelItem(obj.ptr(), index);
//      return v8::Boolean::New(isolate, result);
//    }
//    if (PyMapping_Check(obj.ptr())) {
//      char buf[65];
//
//      snprintf(buf, sizeof(buf), "%d", index);
//      auto result = PyMapping_DelItemString(obj.ptr(), buf) == 0;
//      return v8::Boolean::New(isolate, result);
//    }
//    return v8::Local<v8::Boolean>();
//  });
//
//  auto result_handle = value_or(result, [&]() { return v8::Local<v8::Boolean>(); });
//  info.GetReturnValue().Set(result_handle);
//}

void CPythonObject::IndexedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Array>());
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Array>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());
    Py_ssize_t len = PySequence_Check(py_obj.ptr()) ? PySequence_Size(py_obj.ptr()) : 0;
    auto v8_array = v8::Array::New(v8_isolate, len);
    auto v8_context = v8_isolate->GetCurrentContext();

    for (Py_ssize_t i = 0; i < len; i++) {
      auto v8_i = v8::Integer::New(v8_isolate, i);
      v8_array->Set(v8_context, v8_i, v8_i).Check();
    }
    return v8_array;
  });

  auto v8_final_result = value_or(v8_result, [&]() { return v8::Local<v8::Array>(); });
  v8_info.GetReturnValue().Set(v8_final_result);
}

// void CPythonObject::IndexedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info) {
//  auto isolate = info.GetIsolate();
//  v8::HandleScope handle_scope(isolate);
//
//  if (v8u::executionTerminating(isolate)) {
//    info.GetReturnValue().Set(v8::Local<v8::Array>());
//    return;
//  }
//
//  auto result = withPythonExceptionGuard<v8::Local<v8::Array>>(isolate, [&]() {
//    CPythonGIL python_gil;
//
//    py::object obj = CJSObject::Wrap(info.Holder());
//    Py_ssize_t len = PySequence_Check(obj.ptr()) ? PySequence_Size(obj.ptr()) : 0;
//    v8::Local<v8::Array> result = v8::Array::New(isolate, len);
//    v8::Local<v8::Context> context = isolate->GetCurrentContext();
//
//    for (Py_ssize_t i = 0; i < len; i++) {
//      result->Set(context, v8::Integer::New(isolate, i), v8::Integer::New(isolate, i)).Check();
//    }
//
//    return result;
//  });
//
//  auto result_handle = value_or(result, [&]() { return v8::Local<v8::Array>(); });
//  info.GetReturnValue().Set(result_handle);
//}

void CPythonObject::Caller(const v8::FunctionCallbackInfo<v8::Value>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Value>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    pb::object py_self;

    if (!v8_info.Data().IsEmpty() && v8_info.Data()->IsExternal()) {
      auto v8_field = v8_info.Data().As<v8::External>();
      auto raw_self = static_cast<PyObject*>(v8_field->Value());
      py_self = pb::reinterpret_borrow<pb::object>(raw_self);
    } else {
      py_self = CJSObject::Wrap(v8_info.This());
    }

    pb::object py_result;

    switch (v8_info.Length()) {
      // clang-format off
      case 0: {
        py_result = py_self();
        break;
      }
      case 1: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]));
        break;
      }
      case 2: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]),
                      CJSObject::Wrap(v8_info[1]));
        break;
      }
      case 3: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]),
                      CJSObject::Wrap(v8_info[1]),
                      CJSObject::Wrap(v8_info[2]));
        break;
      }
      case 4: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]),
                      CJSObject::Wrap(v8_info[1]),
                      CJSObject::Wrap(v8_info[2]),
                      CJSObject::Wrap(v8_info[3]));
        break;
      }
      case 5: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]),
                      CJSObject::Wrap(v8_info[1]),
                      CJSObject::Wrap(v8_info[2]),
                      CJSObject::Wrap(v8_info[3]),
                      CJSObject::Wrap(v8_info[4]));
        break;
      }
      case 6: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]),
                      CJSObject::Wrap(v8_info[1]),
                      CJSObject::Wrap(v8_info[2]),
                      CJSObject::Wrap(v8_info[3]),
                      CJSObject::Wrap(v8_info[4]),
                      CJSObject::Wrap(v8_info[5]));
        break;
      }
      case 7: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]),
                      CJSObject::Wrap(v8_info[1]),
                      CJSObject::Wrap(v8_info[2]),
                      CJSObject::Wrap(v8_info[3]),
                      CJSObject::Wrap(v8_info[4]),
                      CJSObject::Wrap(v8_info[5]),
                      CJSObject::Wrap(v8_info[6]));
        break;
      }
      case 8: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]),
                      CJSObject::Wrap(v8_info[1]),
                      CJSObject::Wrap(v8_info[2]),
                      CJSObject::Wrap(v8_info[3]),
                      CJSObject::Wrap(v8_info[4]),
                      CJSObject::Wrap(v8_info[5]),
                      CJSObject::Wrap(v8_info[6]),
                      CJSObject::Wrap(v8_info[7]));
        break;
      }
      case 9: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]),
                      CJSObject::Wrap(v8_info[1]),
                      CJSObject::Wrap(v8_info[2]),
                      CJSObject::Wrap(v8_info[3]),
                      CJSObject::Wrap(v8_info[4]),
                      CJSObject::Wrap(v8_info[5]),
                      CJSObject::Wrap(v8_info[6]),
                      CJSObject::Wrap(v8_info[7]),
                      CJSObject::Wrap(v8_info[8]));
        break;
      }
      case 10: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]),
                      CJSObject::Wrap(v8_info[1]),
                      CJSObject::Wrap(v8_info[2]),
                      CJSObject::Wrap(v8_info[3]),
                      CJSObject::Wrap(v8_info[4]),
                      CJSObject::Wrap(v8_info[5]),
                      CJSObject::Wrap(v8_info[6]),
                      CJSObject::Wrap(v8_info[7]),
                      CJSObject::Wrap(v8_info[8]),
                      CJSObject::Wrap(v8_info[9]));
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

  auto v8_final_result = value_or(v8_result, [&]() { return v8::Undefined(v8_isolate); });
  v8_info.GetReturnValue().Set(v8_final_result);
}

void CPythonObject::SetupObjectTemplate(v8::Isolate* isolate, v8::Local<v8::ObjectTemplate> clazz) {
  v8::HandleScope handle_scope(isolate);

  clazz->SetInternalFieldCount(1);
  clazz->SetHandler(
      v8::NamedPropertyHandlerConfiguration(NamedGetter, NamedSetter, NamedQuery, NamedDeleter, NamedEnumerator));
  clazz->SetIndexedPropertyHandler(IndexedGetter, IndexedSetter, IndexedQuery, IndexedDeleter, IndexedEnumerator);
  clazz->SetCallAsFunctionHandler(Caller);
}

v8::Local<v8::ObjectTemplate> CPythonObject::CreateObjectTemplate(v8::Isolate* isolate) {
  v8::EscapableHandleScope handle_scope(isolate);

  v8::Local<v8::ObjectTemplate> clazz = v8::ObjectTemplate::New(isolate);

  SetupObjectTemplate(isolate, clazz);

  return handle_scope.Escape(clazz);
}

v8::Local<v8::ObjectTemplate> CPythonObject::GetCachedObjectTemplateOrCreate(v8::Isolate* isolate) {
  v8::EscapableHandleScope handle_scope(isolate);
  // retrieve cached object template from the isolate
  auto template_ptr = isolate->GetData(kJSObjectTemplate);
  if (!template_ptr) {
    // it hasn't been created yet = > go create it and cache it in the isolate
    auto template_val = CreateObjectTemplate(isolate);
    auto eternal_val_ptr = new v8::Eternal<v8::ObjectTemplate>(isolate, template_val);
    assert(isolate->GetNumberOfDataSlots() > kJSObjectTemplate);
    isolate->SetData(kJSObjectTemplate, eternal_val_ptr);
    template_ptr = eternal_val_ptr;
  }
  // convert raw pointer to pointer to eternal handle
  auto template_eternal_val = static_cast<v8::Eternal<v8::ObjectTemplate>*>(template_ptr);
  assert(template_eternal_val);
  // retrieve local handle from eternal handle
  auto template_val = template_eternal_val->Get(isolate);
  return handle_scope.Escape(template_val);
}

// bool CPythonObject::IsWrapped(v8::Local<v8::Object> v8_obj) {
//  if (v8_obj->InternalFieldCount() > 0) {
//    auto f = v8_obj->GetInternalField(0);
//    std::cerr << "IS WRAPPED1? " << f << "\n";
//    return !f.IsEmpty() && !f->IsUndefined();
//  }
//  return false;
//}

bool CPythonObject::IsWrapped2(v8::Local<v8::Object> v8_obj) {
  return v8_obj->InternalFieldCount() > 0;
}

// py::object CPythonObject::GetWrapper(v8::Local<v8::Object> v8_obj) {
//  v8::Isolate* isolate = v8::Isolate::GetCurrent();
//  v8::HandleScope handle_scope(isolate);
//
//  v8::Local<v8::External> payload = v8::Local<v8::External>::Cast(v8_obj->GetInternalField(0));
//
//  return *static_cast<py::object*>(payload->Value());
//}

pb::object CPythonObject::GetWrapper2(v8::Local<v8::Object> v8_obj) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_val = v8_obj->GetInternalField(0);
  assert(!v8_val.IsEmpty());
  auto v8_payload = v8_val.As<v8::External>();
  auto raw_obj = static_cast<PyObject*>(v8_payload->Value());
  return pb::reinterpret_borrow<pb::object>(raw_obj);
}

void CPythonObject::Dispose(v8::Local<v8::Value> value) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  if (value->IsObject()) {
    v8::MaybeLocal<v8::Object> objMaybe = value->ToObject(isolate->GetCurrentContext());

    if (objMaybe.IsEmpty()) {
      return;
    }

    v8::Local<v8::Object> obj = objMaybe.ToLocalChecked();

    // TODO: revisit this
    if (IsWrapped2(obj)) {
      Py_DECREF(CPythonObject::GetWrapper2(obj).ptr());
    }
  }
}

// v8::Local<v8::Value> CPythonObject::Wrap(py::object obj) {
//  v8::EscapableHandleScope handle_scope(v8::Isolate::GetCurrent());
//
//  v8::Local<v8::Value> value;
//
//  value = ObjectTracer::FindCache(obj);
//
//  if (value.IsEmpty())
//
//    value = WrapInternal(obj);
//
//  return handle_scope.Escape(value);
//}

v8::Local<v8::Value> CPythonObject::Wrap(pb::handle py_obj) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::openEscapableScope(v8_isolate);

  auto value = ObjectTracer2::FindCache(py_obj.ptr());
  if (value.IsEmpty()) {
    value = WrapInternal2(py_obj);
  }
  return v8_scope.Escape(value);
}

// v8::Local<v8::Value> CPythonObject::WrapInternal(py::object obj) {
//  v8::Isolate* isolate = v8::Isolate::GetCurrent();
//  assert(isolate->InContext());
//
//  v8::EscapableHandleScope handle_scope(isolate);
//  v8::TryCatch try_catch(isolate);
//
//  CPythonGIL python_gil;
//
//  if (v8u::executionTerminating(isolate)) {
//    return v8::Undefined(isolate);
//  }
//
//  if (obj.is_none())
//    return v8::Null(isolate);
//  if (obj.ptr() == Py_True)
//    return v8::True(isolate);
//  if (obj.ptr() == Py_False)
//    return v8::False(isolate);
//
//  py::extract<CJSObject&> extractor(obj);
//
//  if (extractor.check()) {
//    CJSObject& jsobj = extractor();
//
//    assert(0);
//
//    //    if (dynamic_cast<CJSObjectNull*>(&jsobj))
//    //      return v8::Null(isolate);
//    //    if (dynamic_cast<CJSObjectUndefined*>(&jsobj))
//    //      return v8::Undefined(isolate);
//    //
//    //    if (jsobj.Object().IsEmpty()) {
//    //      auto pLazyArray = dynamic_cast<CJSObjectArray*>(&jsobj);
//    //      if (pLazyArray) {
//    //        pLazyArray->LazyConstructor();
//    //      }
//    //    }
//
//    if (jsobj.Object().IsEmpty()) {
//      throw CJavascriptException("Refer to a null object", PyExc_AttributeError);
//    }
//
//    py::object* object = new py::object(obj);
//
//    ObjectTracer::Trace(jsobj.Object(), object);
//
//    return handle_scope.Escape(jsobj.Object());
//  }
//
//  v8::Local<v8::Value> result;
//
//  if (PyLong_CheckExact(obj.ptr())) {
//    result = v8::Integer::New(isolate, PyLong_AsLong(obj.ptr()));
//  } else if (PyBool_Check(obj.ptr())) {
//    result = v8::Boolean::New(isolate, py::extract<bool>(obj));
//  } else if (PyBytes_CheckExact(obj.ptr()) || PyUnicode_CheckExact(obj.ptr())) {
//    result = v8u::toString(obj);
//  } else if (PyFloat_CheckExact(obj.ptr())) {
//    result = v8::Number::New(isolate, py::extract<double>(obj));
//  } else if (isExactDateTime(obj) || isExactDate(obj)) {
//    tm ts = {0};
//    int ms = 0;
//    getPythonDateTime(obj, ts, ms);
//    result = v8::Date::New(isolate->GetCurrentContext(), ((double)mktime(&ts)) * 1000 + ms / 1000).ToLocalChecked();
//  } else if (isExactTime(obj)) {
//    tm ts = {0};
//    int ms = 0;
//    getPythonTime(obj, ts, ms);
//    result = v8::Date::New(isolate->GetCurrentContext(), ((double)mktime(&ts)) * 1000 + ms / 1000).ToLocalChecked();
//  } else if (PyCFunction_Check(obj.ptr()) || PyFunction_Check(obj.ptr()) || PyMethod_Check(obj.ptr()) ||
//             PyType_CheckExact(obj.ptr())) {
//    v8::Local<v8::FunctionTemplate> func_tmpl = v8::FunctionTemplate::New(isolate);
//    py::object* object = new py::object(obj);
//
//    func_tmpl->SetCallHandler(Caller, v8::External::New(isolate, object));
//
//    if (PyType_Check(obj.ptr())) {
//      v8::Local<v8::String> cls_name =
//          v8::String::NewFromUtf8(isolate, py::extract<const char*>(obj.attr("__name__"))()).ToLocalChecked();
//
//      func_tmpl->SetClassName(cls_name);
//    }
//
//    result = func_tmpl->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();
//
//    if (!result.IsEmpty())
//      ObjectTracer::Trace(result, object);
//  } else {
//    auto template_val = GetCachedObjectTemplateOrCreate(isolate);
//    auto instance = template_val->NewInstance(isolate->GetCurrentContext());
//    if (!instance.IsEmpty()) {
//      py::object* object = new py::object(obj);
//
//      v8::Local<v8::Object> realInstance = instance.ToLocalChecked();
//
//      std::cerr << "WRAPPING1 " << object << "\n";
//      realInstance->SetInternalField(0, v8::External::New(isolate, object));
//
//      ObjectTracer::Trace(instance.ToLocalChecked(), object);
//
//      result = realInstance;
//    }
//  }
//
//  if (result.IsEmpty())
//    CJavascriptException::ThrowIf(isolate, try_catch);
//
//  return handle_scope.Escape(result);
//}

v8::Local<v8::Value> CPythonObject::WrapInternal2(pb::handle py_obj) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  assert(v8_isolate->InContext());
  auto v8_scope = v8u::openEscapableScope(v8_isolate);
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

  CPythonGIL python_gil;

  if (v8u::executionTerminating(v8_isolate)) {
    return v8::Undefined(v8_isolate);
  }

  if (py_obj.is_none()) {
    return v8::Null(v8_isolate);
  }
  if (pb::isinstance<pb::bool_>(py_obj)) {
    auto py_bool = pb::cast<pb::bool_>(py_obj);
    if (py_bool) {
      return v8::True(v8_isolate);
    } else {
      return v8::False(v8_isolate);
    }
  }

  if (pb::isinstance<CJSObjectNull>(py_obj)) {
    return v8::Null(v8_isolate);
  }

  if (pb::isinstance<CJSObjectUndefined>(py_obj)) {
    return v8::Undefined(v8_isolate);
  }

  if (pb::isinstance<CJSObject>(py_obj)) {
    auto obj = pb::cast<CJSObjectPtr>(py_obj);
    assert(obj.get());
    obj->LazyInit();

    if (obj->Object().IsEmpty()) {
      throw CJSException("Refer to a null object", PyExc_AttributeError);
    }

    //ObjectTracer2::Trace(obj->Object(), py_obj.ptr());
    return v8_scope.Escape(obj->Object());
  }

  v8::Local<v8::Value> v8_result;

  // TODO: replace this with pybind code
  if (PyLong_CheckExact(py_obj.ptr())) {
    v8_result = v8::Integer::New(v8_isolate, PyLong_AsLong(py_obj.ptr()));
  } else if (PyBool_Check(py_obj.ptr())) {
    v8_result = v8::Boolean::New(v8_isolate, pb::cast<pb::bool_>(py_obj));
  } else if (PyBytes_CheckExact(py_obj.ptr()) || PyUnicode_CheckExact(py_obj.ptr())) {
    v8_result = v8u::toString(py_obj);
  } else if (PyFloat_CheckExact(py_obj.ptr())) {
    v8_result = v8::Number::New(v8_isolate, pb::cast<pb::float_>(py_obj));
  } else if (isExactDateTime(py_obj) || isExactDate(py_obj)) {
    tm ts = {0};
    int ms = 0;
    getPythonDateTime(py_obj, ts, ms);
    v8_result =
        v8::Date::New(v8_isolate->GetCurrentContext(), ((double)mktime(&ts)) * 1000 + ms / 1000).ToLocalChecked();
  } else if (isExactTime(py_obj)) {
    tm ts = {0};
    int ms = 0;
    getPythonTime(py_obj, ts, ms);
    v8_result =
        v8::Date::New(v8_isolate->GetCurrentContext(), ((double)mktime(&ts)) * 1000 + ms / 1000).ToLocalChecked();
  } else if (PyCFunction_Check(py_obj.ptr()) || PyFunction_Check(py_obj.ptr()) || PyMethod_Check(py_obj.ptr()) ||
             PyType_CheckExact(py_obj.ptr())) {
    auto func_tmpl = v8::FunctionTemplate::New(v8_isolate);

    // NOTE: tracker will keep the object alive
    ObjectTracer2::Trace(v8_result, py_obj.ptr());
    //Py_INCREF(py_obj.ptr());
    func_tmpl->SetCallHandler(Caller, v8::External::New(v8_isolate, py_obj.ptr()));

    if (PyType_Check(py_obj.ptr())) {
      auto py_name_attr = py_obj.attr("__name__");
      // TODO: we should do it safer here, if __name__ is not string
      auto v8_cls_name = v8u::toString(py_name_attr);
      func_tmpl->SetClassName(v8_cls_name);
    }
    v8_result = func_tmpl->GetFunction(v8_isolate->GetCurrentContext()).ToLocalChecked();
//    if (!v8_result.IsEmpty()) {
//      ObjectTracer2::Trace(v8_result, py_obj.ptr());
//    }
  } else {
    auto template_val = GetCachedObjectTemplateOrCreate(v8_isolate);
    auto instance = template_val->NewInstance(v8_isolate->GetCurrentContext());
    if (!instance.IsEmpty()) {
      auto realInstance = instance.ToLocalChecked();
      // NOTE: tracker will keep the object alive
      ObjectTracer2::Trace(instance.ToLocalChecked(), py_obj.ptr());
      //Py_INCREF(py_obj.ptr());
      realInstance->SetInternalField(0, v8::External::New(v8_isolate, py_obj.ptr()));
      v8_result = realInstance;
    }
  }

  if (v8_result.IsEmpty()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  return v8_scope.Escape(v8_result);
}