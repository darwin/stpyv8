#include <iostream>

//Python includes
#include <boost/python/raw_function.hpp>
#include <descrobject.h>
#include <datetime.h>

//V8 includes
#include "libplatform/libplatform.h"

//Our includes
#include "Wrapper.h"

#define TERMINATE_EXECUTION_CHECK(returnValue) \
  if(v8::Isolate::GetCurrent()->IsExecutionTerminating()) { \
    ::PyErr_Clear(); \
    ::PyErr_SetString(PyExc_RuntimeError, "execution is terminating"); \
    return returnValue; \
  }


void CWrapper::Expose(void)
{
  //TODO port me
  std::cout << "Stub" << std::endl;
}


// =====================================================================
// CPythonObject Start
// =====================================================================

void CPythonObject::ThrowIf(v8::Isolate* isolate)
{
  CPythonGIL python_gil;

  assert(PyErr_OCCURRED());

  v8::HandleScope handle_scope(isolate);

  PyObject *exc, *val, *trb;

  ::PyErr_Fetch(&exc, &val, &trb);
  ::PyErr_NormalizeException(&exc, &val, &trb);

  py::object type(py::handle<>(py::allow_null(exc))),
             value(py::handle<>(py::allow_null(val)));

  if (trb) py::decref(trb);

  std::string msg;

  if (::PyObject_HasAttrString(value.ptr(), "args"))
  {
    py::object args = value.attr("args");

    if (PyTuple_Check(args.ptr()))
    {
      for (Py_ssize_t i=0; i<PyTuple_GET_SIZE(args.ptr()); i++)
      {
        py::extract<const std::string> extractor(args[i]);

        if (extractor.check()) msg += extractor();
      }
    }
  }
  else if (::PyObject_HasAttrString(value.ptr(), "message"))
  {
    py::extract<const std::string> extractor(value.attr("message"));

    if (extractor.check()) msg = extractor();
  }
  else if (val)
  {
    if (PyBytes_CheckExact(val))
    {
      msg = PyBytes_AS_STRING(val);
    }
    else if (PyTuple_CheckExact(val))
    {
      for (int i=0; i<PyTuple_GET_SIZE(val); i++)
      {
        PyObject *item = PyTuple_GET_ITEM(val, i);

        if (item && PyBytes_CheckExact(item))
        {
          msg = PyBytes_AS_STRING(item);
          break;
        }
      }
    }
  }

  v8::Handle<v8::Value> error;

  if (::PyErr_GivenExceptionMatches(type.ptr(), ::PyExc_IndexError))
  {
    error = v8::Exception::RangeError(v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
  }
  else if (::PyErr_GivenExceptionMatches(type.ptr(), ::PyExc_AttributeError))
  {
    error = v8::Exception::ReferenceError(v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
  }
  else if (::PyErr_GivenExceptionMatches(type.ptr(), ::PyExc_SyntaxError))
  {
    error = v8::Exception::SyntaxError(v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
  }
  else if (::PyErr_GivenExceptionMatches(type.ptr(), ::PyExc_TypeError))
  {
    error = v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
  }
  else
  {
    error = v8::Exception::Error(v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
  }

  if (error->IsObject())
  {
    // FIXME How to trace the lifecycle of exception? and when to delete those object in the hidden value?

  /* TODO port me
  #ifdef SUPPORT_TRACE_EXCEPTION_LIFECYCLE
    error->ToObject()->SetHiddenValue(v8::String::NewFromUtf8(isolate, "exc_type"),
                                      v8::External::New(isolate, ObjectTracer::Trace(error, new py::object(type)).Object()));
    error->ToObject()->SetHiddenValue(v8::String::NewFromUtf8(isolate, "exc_value"),
                                      v8::External::New(isolate, ObjectTracer::Trace(error, new py::object(value)).Object()));
  #else
    error->ToObject()->SetHiddenValue(v8::String::NewFromUtf8(isolate, "exc_type"),
                                      v8::External::New(isolate, new py::object(type)));
    error->ToObject()->SetHiddenValue(v8::String::NewFromUtf8(isolate, "exc_value"),
                                      v8::External::New(isolate, new py::object(value)));
  #endif
  */
  }

  isolate->ThrowException(error);
}

#define _TERMINATE_CALLBACK_EXECUTION_CHECK(returnValue) \
  if(v8::Isolate::GetCurrent()->IsExecutionTerminating()) { \
    ::PyErr_Clear(); \
    ::PyErr_SetString(PyExc_RuntimeError, "execution is terminating"); \
    info.GetReturnValue().Set(returnValue); \
    return; \
  }

#define TRY_HANDLE_EXCEPTION(value) _TERMINATE_CALLBACK_EXECUTION_CHECK(value) \
                                    BEGIN_HANDLE_PYTHON_EXCEPTION \
                                    {
#define END_HANDLE_EXCEPTION(value) } \
                                    END_HANDLE_PYTHON_EXCEPTION \
                                    info.GetReturnValue().Set(value);

#define CALLBACK_RETURN(value) do { info.GetReturnValue().Set(value); return; } while(0);

CPythonObject::CPythonObject()
{
}

CPythonObject::~CPythonObject()
{
}

void CPythonObject::NamedGetter(v8::Local<v8::Name> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()))

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  v8::String::Utf8Value name(info.GetIsolate(), prop);

  if (PyGen_Check(obj.ptr())) CALLBACK_RETURN(v8::Undefined(info.GetIsolate()));

  PyObject *value = ::PyObject_GetAttrString(obj.ptr(), *name);

  if (!value)
  {
    if (_PyErr_OCCURRED())
    {
      if (::PyErr_ExceptionMatches(::PyExc_AttributeError))
      {
        ::PyErr_Clear();
      }
      else
      {
        py::throw_error_already_set();
      }
    }

    if (::PyMapping_Check(obj.ptr()) &&
        ::PyMapping_HasKeyString(obj.ptr(), *name))
    {
      py::object result(py::handle<>(::PyMapping_GetItemString(obj.ptr(), *name)));

      if (!result.is_none()) CALLBACK_RETURN(Wrap(result));
    }

    CALLBACK_RETURN(v8::Handle<v8::Value>());
  }

  py::object attr = py::object(py::handle<>(value));

/* TODO port me
#ifdef SUPPORT_PROPERTY
  if (PyObject_TypeCheck(attr.ptr(), &::PyProperty_Type))
  {
    py::object getter = attr.attr("fget");

    if (getter.is_none())
      throw CJavascriptException("unreadable attribute", ::PyExc_AttributeError);

    attr = getter();
  }
#endif
*/

  CALLBACK_RETURN(Wrap(attr));

  END_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()))

}

void CPythonObject::NamedSetter(v8::Local<v8::Name> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()))

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  v8::String::Utf8Value name(info.GetIsolate(), prop);
  py::object newval = CJavascriptObject::Wrap(value);

  bool found = 1 == ::PyObject_HasAttrString(obj.ptr(), *name);

  if (::PyObject_HasAttrString(obj.ptr(), "__watchpoints__"))
  {
    py::dict watchpoints(obj.attr("__watchpoints__"));
    py::str propname(*name, name.length());

    if (watchpoints.has_key(propname))
    {
      py::object watchhandler = watchpoints.get(propname);

      newval = watchhandler(propname, found ? obj.attr(propname) : py::object(), newval);
    }
  }

  if (!found && ::PyMapping_Check(obj.ptr()))
  {
    ::PyMapping_SetItemString(obj.ptr(), *name, newval.ptr());
  }
  else
  {
    /* TODO port me
  #ifdef SUPPORT_PROPERTY
    if (found)
    {
      py::object attr = obj.attr(*name);

      if (PyObject_TypeCheck(attr.ptr(), &::PyProperty_Type))
      {
        py::object setter = attr.attr("fset");

        if (setter.is_none())
          throw CJavascriptException("can't set attribute", ::PyExc_AttributeError);

        setter(newval);

        CALLBACK_RETURN(value);
      }
    }
  #endif
  */
    obj.attr(*name) = newval;
  }

  CALLBACK_RETURN(value);

  END_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()));
}

void CPythonObject::NamedQuery(v8::Local<v8::Name> prop, const v8::PropertyCallbackInfo<v8::Integer>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Handle<v8::Integer>())

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  v8::String::Utf8Value name(info.GetIsolate(), prop);

  bool exists = PyGen_Check(obj.ptr()) || ::PyObject_HasAttrString(obj.ptr(), *name) ||
                (::PyMapping_Check(obj.ptr()) && ::PyMapping_HasKeyString(obj.ptr(), *name));

  if (exists) CALLBACK_RETURN(v8::Integer::New(info.GetIsolate(), v8::None));

  END_HANDLE_EXCEPTION(v8::Handle<v8::Integer>())
}

void CPythonObject::NamedDeleter(v8::Local<v8::Name> prop, const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Handle<v8::Boolean>())

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  v8::String::Utf8Value name(info.GetIsolate(), prop);

  if (!::PyObject_HasAttrString(obj.ptr(), *name) &&
      ::PyMapping_Check(obj.ptr()) &&
      ::PyMapping_HasKeyString(obj.ptr(), *name))
  {
    CALLBACK_RETURN(-1 != ::PyMapping_DelItemString(obj.ptr(), *name));
  }
  else
  {
    /* TODO port me
  #ifdef SUPPORT_PROPERTY
    py::object attr = obj.attr(*name);

    if (::PyObject_HasAttrString(obj.ptr(), *name) &&
        PyObject_TypeCheck(attr.ptr(), &::PyProperty_Type))
    {
      py::object deleter = attr.attr("fdel");

      if (deleter.is_none())
        throw CJavascriptException("can't delete attribute", ::PyExc_AttributeError);

      CALLBACK_RETURN(py::extract<bool>(deleter()));
    }
    else
    {
      CALLBACK_RETURN(-1 != ::PyObject_DelAttrString(obj.ptr(), *name));
    }
  #else
  */
    CALLBACK_RETURN(-1 != ::PyObject_DelAttrString(obj.ptr(), *name));
    /* TODO port me
  #endif
  */
  }

  END_HANDLE_EXCEPTION(v8::Handle<v8::Boolean>())
}

#pragma GCC diagnostic ignored "-Wunused-result"

void CPythonObject::NamedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Handle<v8::Array>())

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  py::list keys;
  bool filter_name = false;

  if (::PySequence_Check(obj.ptr()))
  {
    CALLBACK_RETURN(v8::Handle<v8::Array>());
  }
  else if (::PyMapping_Check(obj.ptr()))
  {
    keys = py::list(py::handle<>(PyMapping_Keys(obj.ptr())));
  }
  else if (PyGen_CheckExact(obj.ptr()))
  {
    py::object iter(py::handle<>(::PyObject_GetIter(obj.ptr())));

    PyObject *item = NULL;

    while (NULL != (item = ::PyIter_Next(iter.ptr())))
    {
      keys.append(py::object(py::handle<>(item)));
    }
  }
  else
  {
    keys = py::list(py::handle<>(::PyObject_Dir(obj.ptr())));
    filter_name = true;
  }

  Py_ssize_t len = PyList_GET_SIZE(keys.ptr());
  v8::Handle<v8::Array> result = v8::Array::New(info.GetIsolate(), len);

  if (len > 0)
  {
    for (Py_ssize_t i=0; i<len; i++)
    {
      PyObject *item = PyList_GET_ITEM(keys.ptr(), i);

      if (filter_name && PyBytes_CheckExact(item))
      {
        py::str name(py::handle<>(py::borrowed(item)));

        // FIXME Are there any methods to avoid such a dirty work?

        if (name.startswith("__") && name.endswith("__"))
          continue;
      }

      result->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), v8::Uint32::New(info.GetIsolate(), i), Wrap(py::object(py::handle<>(py::borrowed(item)))));
    }

    CALLBACK_RETURN(result);
  }

  END_HANDLE_EXCEPTION(v8::Handle<v8::Array>())
}

void CPythonObject::IndexedGetter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()));

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  if (PyGen_Check(obj.ptr())) CALLBACK_RETURN(v8::Undefined(info.GetIsolate()));

  if (::PySequence_Check(obj.ptr()))
  {
    if ((Py_ssize_t) index < ::PySequence_Size(obj.ptr()))
    {
      py::object ret(py::handle<>(::PySequence_GetItem(obj.ptr(), index)));

      CALLBACK_RETURN(Wrap(ret));
    }
  }
  else if (::PyMapping_Check(obj.ptr()))
  {
    char buf[65];

    snprintf(buf, sizeof(buf), "%d", index);

    PyObject *value = ::PyMapping_GetItemString(obj.ptr(), buf);

    if (!value)
    {
      py::long_ key(index);

      value = ::PyObject_GetItem(obj.ptr(), key.ptr());
    }

    if (value)
    {
      CALLBACK_RETURN(Wrap(py::object(py::handle<>(value))));
    }
  }

  END_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()))
}

void CPythonObject::IndexedSetter(uint32_t index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()));

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  if (::PySequence_Check(obj.ptr()))
  {
    if (::PySequence_SetItem(obj.ptr(), index, CJavascriptObject::Wrap(value).ptr()) < 0)
      info.GetIsolate()->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(info.GetIsolate(), "fail to set indexed value").ToLocalChecked()));
  }
  else if (::PyMapping_Check(obj.ptr()))
  {
    char buf[65];

    snprintf(buf, sizeof(buf), "%d", index);

    if (::PyMapping_SetItemString(obj.ptr(), buf, CJavascriptObject::Wrap(value).ptr()) < 0)
      info.GetIsolate()->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(info.GetIsolate(), "fail to set named value").ToLocalChecked()));
  }

  CALLBACK_RETURN(value);

  END_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()))
}

void CPythonObject::IndexedQuery(uint32_t index, const v8::PropertyCallbackInfo<v8::Integer>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Handle<v8::Integer>());

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  if (PyGen_Check(obj.ptr())) CALLBACK_RETURN(v8::Integer::New(info.GetIsolate(), v8::ReadOnly));

  if (::PySequence_Check(obj.ptr()))
  {
    if ((Py_ssize_t) index < ::PySequence_Size(obj.ptr()))
    {
      CALLBACK_RETURN(v8::Integer::New(info.GetIsolate(), v8::None));
    }
  }
  else if (::PyMapping_Check(obj.ptr()))
  {
    char buf[65];

    snprintf(buf, sizeof(buf), "%d", index);

    if (::PyMapping_HasKeyString(obj.ptr(), buf) ||
        ::PyMapping_HasKey(obj.ptr(), py::long_(index).ptr()))
    {
      CALLBACK_RETURN(v8::Integer::New(info.GetIsolate(), v8::None));
    }
  }

  END_HANDLE_EXCEPTION(v8::Handle<v8::Integer>())
}

void CPythonObject::IndexedDeleter(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Handle<v8::Boolean>());

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  if (::PySequence_Check(obj.ptr()) && (Py_ssize_t) index < ::PySequence_Size(obj.ptr()))
  {
    CALLBACK_RETURN(0 <= ::PySequence_DelItem(obj.ptr(), index));
  }
  else if (::PyMapping_Check(obj.ptr()))
  {
    char buf[65];

    snprintf(buf, sizeof(buf), "%d", index);

    CALLBACK_RETURN(PyMapping_DelItemString(obj.ptr(), buf) == 0);
  }

  END_HANDLE_EXCEPTION(v8::Handle<v8::Boolean>())
}

void CPythonObject::IndexedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Handle<v8::Array>());

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  Py_ssize_t len = ::PySequence_Check(obj.ptr()) ? ::PySequence_Size(obj.ptr()) : 0;

  v8::Handle<v8::Array> result = v8::Array::New(info.GetIsolate(), len);

  for (Py_ssize_t i=0; i<len; i++)
  {
    result->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), v8::Uint32::New(info.GetIsolate(), i), v8::Int32::New(info.GetIsolate(), i)->ToString(v8::Isolate::GetCurrent()->GetCurrentContext()).ToLocalChecked());
  }

  CALLBACK_RETURN(result);

  END_HANDLE_EXCEPTION(v8::Handle<v8::Array>())
}

#define GEN_ARG(z, n, data) CJavascriptObject::Wrap(info[n])
#define GEN_ARGS(count) BOOST_PP_ENUM(count, GEN_ARG, NULL)

#define GEN_CASE_PRED(r, state) \
  BOOST_PP_NOT_EQUAL( \
    BOOST_PP_TUPLE_ELEM(2, 0, state), \
    BOOST_PP_INC(BOOST_PP_TUPLE_ELEM(2, 1, state)) \
  ) \
  /**/

#define GEN_CASE_OP(r, state) \
  ( \
    BOOST_PP_INC(BOOST_PP_TUPLE_ELEM(2, 0, state)), \
    BOOST_PP_TUPLE_ELEM(2, 1, state) \
  ) \
  /**/

#define GEN_CASE_MACRO(r, state) \
  case BOOST_PP_TUPLE_ELEM(2, 0, state): { \
    result = self(GEN_ARGS(BOOST_PP_TUPLE_ELEM(2, 0, state))); \
    break; \
  } \
  /**/

void CPythonObject::Caller(const v8::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()));

  CPythonGIL python_gil;

  py::object self;

  if (!info.Data().IsEmpty() && info.Data()->IsExternal())
  {
    v8::Handle<v8::External> field = v8::Handle<v8::External>::Cast(info.Data());

    self = *static_cast<py::object *>(field->Value());
  }
  else
  {
    self = CJavascriptObject::Wrap(info.This());
  }

  py::object result;

  switch (info.Length())
  {
    BOOST_PP_FOR((0, 10), GEN_CASE_PRED, GEN_CASE_OP, GEN_CASE_MACRO)
  default:
    info.GetIsolate()->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(info.GetIsolate(), "too many arguments").ToLocalChecked()));

    CALLBACK_RETURN(v8::Undefined(info.GetIsolate()));
  }

  CALLBACK_RETURN(Wrap(result));

  END_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()))
}

void CPythonObject::SetupObjectTemplate(v8::Isolate *isolate, v8::Handle<v8::ObjectTemplate> clazz)
{
  v8::HandleScope handle_scope(isolate);

  clazz->SetInternalFieldCount(1);
  clazz->SetHandler(v8::NamedPropertyHandlerConfiguration(NamedGetter, NamedSetter, NamedQuery, NamedDeleter, NamedEnumerator));
  clazz->SetIndexedPropertyHandler(IndexedGetter, IndexedSetter, IndexedQuery, IndexedDeleter, IndexedEnumerator);
  clazz->SetCallAsFunctionHandler(Caller);
}

v8::Handle<v8::ObjectTemplate> CPythonObject::CreateObjectTemplate(v8::Isolate *isolate)
{
  v8::EscapableHandleScope handle_scope(isolate);

  v8::Local<v8::ObjectTemplate> clazz = v8::ObjectTemplate::New(isolate);

  SetupObjectTemplate(isolate, clazz);

  return handle_scope.Escape(clazz);
}

bool CPythonObject::IsWrapped(v8::Handle<v8::Object> obj)
{
  return obj->InternalFieldCount() == 1;
}

py::object CPythonObject::Unwrap(v8::Handle<v8::Object> obj)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  v8::Handle<v8::External> payload = v8::Handle<v8::External>::Cast(obj->GetInternalField(0));

  return *static_cast<py::object *>(payload->Value());
}

void CPythonObject::Dispose(v8::Handle<v8::Value> value)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  if (value->IsObject())
  {
    v8::MaybeLocal<v8::Object> objMaybe = value->ToObject(v8::Isolate::GetCurrent()->GetCurrentContext());

    if (objMaybe.IsEmpty()) 
    {
      return;
    }

    v8::Handle<v8::Object> obj = objMaybe.ToLocalChecked();

    if (IsWrapped(obj))
    {
      Py_DECREF(CPythonObject::Unwrap(obj).ptr());
    }
  }
}

v8::Handle<v8::Value> CPythonObject::Wrap(py::object obj)
{
  v8::EscapableHandleScope handle_scope(v8::Isolate::GetCurrent());

  v8::Local<v8::Value> value;

  //TODO port me
  //#ifdef SUPPORT_TRACE_LIFECYCLE
  //value = ObjectTracer::FindCache(obj);
  //
  //if (value.IsEmpty())
  //#endif

  value = WrapInternal(obj);

  return handle_scope.Escape(value);
}

v8::Handle<v8::Value> CPythonObject::WrapInternal(py::object obj)
{
  assert(v8::Isolate::GetCurrent()->InContext());

  v8::EscapableHandleScope handle_scope(v8::Isolate::GetCurrent());

  v8::TryCatch try_catch(v8::Isolate::GetCurrent());

  CPythonGIL python_gil;

  TERMINATE_EXECUTION_CHECK(v8::Undefined(v8::Isolate::GetCurrent()))

  if (obj.is_none()) return v8::Null(v8::Isolate::GetCurrent());
  if (obj.ptr() == Py_True) return v8::True(v8::Isolate::GetCurrent());
  if (obj.ptr() == Py_False) return v8::False(v8::Isolate::GetCurrent());

  py::extract<CJavascriptObject&> extractor(obj);

  if (extractor.check())
  {
    CJavascriptObject& jsobj = extractor();
    
    if (dynamic_cast<CJavascriptNull *>(&jsobj)) return v8::Null(v8::Isolate::GetCurrent());
    if (dynamic_cast<CJavascriptUndefined *>(&jsobj)) return v8::Undefined(v8::Isolate::GetCurrent());
    
    if (jsobj.Object().IsEmpty())
    {
      ILazyObject *pLazyObject = dynamic_cast<ILazyObject *>(&jsobj);

      if (pLazyObject) pLazyObject->LazyConstructor();
    }

    if (jsobj.Object().IsEmpty())
    {
      //throw CJavascriptException("Refer to a null object", ::PyExc_AttributeError);
      throw std::string("TODO port CJavascriptException");
    }

    //TODO port me
    //#ifdef SUPPORT_TRACE_LIFECYCLE
    //py::object *object = new py::object(obj);
    //
    //ObjectTracer::Trace(jsobj.Object(), object);
    //#endif 

    return handle_scope.Escape(jsobj.Object());
  }

  v8::Local<v8::Value> result;

#if PY_MAJOR_VERSION < 3
  if (PyInt_CheckExact(obj.ptr()))
  {
    result = v8::Integer::New(v8::Isolate::GetCurrent(), ::PyInt_AsLong(obj.ptr()));
  }
  else
#endif
    if (PyLong_CheckExact(obj.ptr()))
    {
      result = v8::Integer::New(v8::Isolate::GetCurrent(), ::PyLong_AsLong(obj.ptr()));
    }
    else if (PyBool_Check(obj.ptr()))
    {
      result = v8::Boolean::New(v8::Isolate::GetCurrent(), py::extract<bool>(obj));
    }
    else if (PyBytes_CheckExact(obj.ptr()) ||
        PyUnicode_CheckExact(obj.ptr()))
    {
      result = ToString(obj);
    }
    else if (PyFloat_CheckExact(obj.ptr()))
    {
      result = v8::Number::New(v8::Isolate::GetCurrent(), py::extract<double>(obj));
    }
    else if (PyDateTime_CheckExact(obj.ptr()) || PyDate_CheckExact(obj.ptr()))
    {
      tm ts = { 0 };

      ts.tm_year = PyDateTime_GET_YEAR(obj.ptr()) - 1900;
      ts.tm_mon = PyDateTime_GET_MONTH(obj.ptr()) - 1;
      ts.tm_mday = PyDateTime_GET_DAY(obj.ptr());
      ts.tm_hour = PyDateTime_DATE_GET_HOUR(obj.ptr());
      ts.tm_min = PyDateTime_DATE_GET_MINUTE(obj.ptr());
      ts.tm_sec = PyDateTime_DATE_GET_SECOND(obj.ptr());
      ts.tm_isdst = -1;

      int ms = PyDateTime_DATE_GET_MICROSECOND(obj.ptr());

      result = v8::Date::New(v8::Isolate::GetCurrent()->GetCurrentContext(), ((double) mktime(&ts)) * 1000 + ms / 1000).ToLocalChecked();
    }
    else if (PyTime_CheckExact(obj.ptr()))
    {
      tm ts = { 0 };

      ts.tm_hour = PyDateTime_TIME_GET_HOUR(obj.ptr()) - 1;
      ts.tm_min = PyDateTime_TIME_GET_MINUTE(obj.ptr());
      ts.tm_sec = PyDateTime_TIME_GET_SECOND(obj.ptr());

      int ms = PyDateTime_TIME_GET_MICROSECOND(obj.ptr());

      result = v8::Date::New(v8::Isolate::GetCurrent()->GetCurrentContext(), ((double) mktime(&ts)) * 1000 + ms / 1000).ToLocalChecked();
    }
    else if (PyCFunction_Check(obj.ptr()) || PyFunction_Check(obj.ptr()) ||
        PyMethod_Check(obj.ptr()) || PyType_Check(obj.ptr()))
    {
      v8::Handle<v8::FunctionTemplate> func_tmpl = v8::FunctionTemplate::New(v8::Isolate::GetCurrent());
      py::object *object = new py::object(obj);

      func_tmpl->SetCallHandler(Caller, v8::External::New(v8::Isolate::GetCurrent(), object));

      if (PyType_Check(obj.ptr()))
      {
        v8::Handle<v8::String> cls_name = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), py::extract<const char *>(obj.attr("__name__"))()).ToLocalChecked();

        func_tmpl->SetClassName(cls_name);
      }

      result = func_tmpl->GetFunction(v8::Isolate::GetCurrent()->GetCurrentContext()).ToLocalChecked();
      
      /* TODO port me
      #ifdef SUPPORT_TRACE_LIFECYCLE
      if (!result.IsEmpty()) ObjectTracer::Trace(result, object);
      #endif
      */
    }
    else
    {
      static v8::Persistent<v8::ObjectTemplate> s_template(v8::Isolate::GetCurrent(), CreateObjectTemplate(v8::Isolate::GetCurrent()));

      v8::MaybeLocal<v8::Object> instance = v8::Local<v8::ObjectTemplate>::New(
          v8::Isolate::GetCurrent(), s_template)->NewInstance(
          v8::Isolate::GetCurrent()->GetCurrentContext());

      if (!instance.IsEmpty())
      {
        py::object *object = new py::object(obj);

        v8::Handle<v8::Object> realInstance = instance.ToLocalChecked();
        realInstance->SetInternalField(0, v8::External::New(v8::Isolate::GetCurrent(), object));

        //TODO port me
        //#ifdef SUPPORT_TRACE_LIFECYCLE
        //  ObjectTracer::Trace(instance, object);
        //#endif
        result = realInstance;
//TODO add test
//std::cout << "Is is wrapped? " << CPythonObject::IsWrapped(realInstance) << std::endl;
//py::object un = CPythonObject::Unwrap(realInstance);
//std::cout << "Unwrapped python object is this many bytes: " << sizeof(un) << std::endl;
      }

    }

  //TODO port me
  //if (result.IsEmpty()) CJavascriptException::ThrowIf(v8::Isolate::GetCurrent(), try_catch);

  return handle_scope.Escape(result);
}




// =====================================================================
// CPythonObject End 
// =====================================================================

// =====================================================================
// CJavascriptObject Start 
// =====================================================================

py::object CJavascriptObject::Wrap(v8::Handle<v8::Value> value, v8::Handle<v8::Object> self)
{
  assert(v8::Isolate::GetCurrent()->InContext());

  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  if (value.IsEmpty() || value->IsNull() || value->IsUndefined()) 
  {
    return py::object();
  }
  if (value->IsTrue())
  {
    return py::object(py::handle<>(py::borrowed(Py_True)));
  }
  if (value->IsFalse())
  {
    return py::object(py::handle<>(py::borrowed(Py_False)));
  }

  if (value->IsInt32())
  { 
    return py::object(value->Int32Value(v8::Isolate::GetCurrent()->GetCurrentContext()).ToChecked());
  }
  if (value->IsString())
  {
    v8::String::Utf8Value str(v8::Isolate::GetCurrent(), v8::Handle<v8::String>::Cast(value));

    return py::str(*str, str.length());
  }
  if (value->IsStringObject())
  {
    v8::String::Utf8Value str(v8::Isolate::GetCurrent(), value.As<v8::StringObject>()->ValueOf());

    return py::str(*str, str.length());
  }
  if (value->IsBoolean())
  {
    return py::object(py::handle<>(py::borrowed(value->BooleanValue(v8::Isolate::GetCurrent()) ? Py_True : Py_False)));
  }
  if (value->IsBooleanObject())
  {
    return py::object(py::handle<>(py::borrowed(value.As<v8::BooleanObject>()->BooleanValue(v8::Isolate::GetCurrent()) ? Py_True : Py_False)));
  }
  if (value->IsNumber())
  {
    return py::object(py::handle<>(::PyFloat_FromDouble(value->NumberValue(v8::Isolate::GetCurrent()->GetCurrentContext()).ToChecked())));
  }
  if (value->IsNumberObject())
  {
    return py::object(py::handle<>(::PyFloat_FromDouble(value.As<v8::NumberObject>()->NumberValue(v8::Isolate::GetCurrent()->GetCurrentContext()).ToChecked())));
  }
  if (value->IsDate())
  {
    double n = v8::Handle<v8::Date>::Cast(value)->NumberValue(v8::Isolate::GetCurrent()->GetCurrentContext()).ToChecked();

    time_t ts = (time_t) floor(n / 1000);

    tm *t = localtime(&ts);

    return py::object(py::handle<>(::PyDateTime_FromDateAndTime(
      t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec,
      ((long long) floor(n)) % 1000 * 1000)));
  }

  return Wrap(value->ToObject(v8::Isolate::GetCurrent()->GetCurrentContext()).ToLocalChecked(), self);
}

py::object CJavascriptObject::Wrap(v8::Handle<v8::Object> obj, v8::Handle<v8::Object> self)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  if (obj.IsEmpty())
  {
    return py::object();
  }
  else if (obj->IsArray())
  {
    v8::Handle<v8::Array> array = v8::Handle<v8::Array>::Cast(obj);

    return Wrap(new CJavascriptArray(array));
  }
  else if (CPythonObject::IsWrapped(obj))
  {
    return CPythonObject::Unwrap(obj);
  }
  else if (obj->IsFunction())
  {
    return Wrap(new CJavascriptFunction(self, v8::Handle<v8::Function>::Cast(obj)));
  }

  return Wrap(new CJavascriptObject(obj));
}

py::object CJavascriptObject::Wrap(CJavascriptObject *obj)
{
  CPythonGIL python_gil;

  TERMINATE_EXECUTION_CHECK(py::object())

  return py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CJavascriptObject>(CJavascriptObjectPtr(obj))));
}

// =====================================================================
// CJavascriptObject End 
// =====================================================================

void CJavascriptArray::LazyConstructor(void)
{
  //Stubbed
}


static v8::Isolate* GetNewIsolate()
{
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
    v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  v8::Isolate* isolate = v8::Isolate::New(create_params);
  return isolate;
}

std::string toString(v8::Isolate* isolate, v8::Handle<v8::Value> v)
{
  auto name = v->ToString(isolate->GetCurrentContext()).ToLocalChecked();
  int length = name->Utf8Length(isolate);
  char* buffer = new char[length + 1]{};
  name->WriteUtf8(isolate, buffer);
  return(std::string(buffer));
}

std::string heavyToString(v8::Isolate* isolate, v8::Persistent<v8::Value>* p)
{
  v8::HandleScope handle_scope(isolate); 
  v8::Local<v8::Value> v = p->Get(isolate);

  auto name = v->ToString(isolate->GetCurrentContext()).ToLocalChecked();
  int length = name->Utf8Length(isolate);
  char* buffer = new char[length + 1]{};
  name->WriteUtf8(isolate, buffer);
  return(std::string(buffer));
}



py::object run_python_code(const char *code)
{
  py::object main_module = py::import("__main__");
  py::object main_namespace = main_module.attr("__dict__");
  py::object result = py::exec(code, main_namespace);
  return main_namespace["result"];
}

int main___test_wrapping_python_code(int argc, char* argv[])
{
  /*
     const char *code =
     "import datetime\n"
     "result = datetime.time()\n"
     "print(result)";

  const char *code =
    "def foo():\n"
    "  print('foo called')\n"
    "\n"
    "result = foo\n"
    "print(result)";

     */
     const char *code =
    "result = ['a','b','c']\n"
    "print(result)";


  Py_Initialize();
  PyDateTime_IMPORT;

  py::object pyvar = run_python_code(code); 

  v8::V8::InitializeICUDefaultLocation(argv[0]);
  v8::V8::InitializeExternalStartupData(argv[0]);
  std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();
  v8::Isolate* isolate = GetNewIsolate();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = v8::Context::New(isolate);
  v8::Context::Scope context_scope(context);

  v8::Handle<v8::Value> v = CPythonObject::Wrap(pyvar);
  std::cout << "Wrapped it" << std::endl;
  std::cout << "Got a value back of size " << sizeof(v) << std::endl;
  CPythonObject::Dispose(v);
  return 0;
}

v8::Local<v8::Value> run_javascript(v8::Isolate* isolate, v8::Local<v8::Context> context, const char *code)
{
  v8::Local<v8::String> source =
    v8::String::NewFromUtf8(isolate, code, v8::NewStringType::kNormal)
    .ToLocalChecked();

  v8::Local<v8::Script> script =
    v8::Script::Compile(context, source).ToLocalChecked();

  return script->Run(context).ToLocalChecked();
}

int main(int argc, char* argv[])
{
  //const char* code = "['apples','oranges']";
  //const char* code = "x = { 'a' : 'b' }";
  //const char* code = "fn = function foo(baz) { console.write(baz); }";
  const char* code = "date1 = new Date('December 17, 1995 03:24:00');";

  Py_Initialize();
  PyDateTime_IMPORT;

  py::class_<CJavascriptObject, boost::noncopyable>("JSObject", py::no_init)
    ;

  py::class_<CJavascriptArray, py::bases<CJavascriptObject>, boost::noncopyable>("JSArray", py::no_init)
    .def(py::init<py::object>())
    ;

  py::objects::class_value_wrapper<boost::shared_ptr<CJavascriptObject>,
    py::objects::make_ptr_instance<CJavascriptObject,
    py::objects::pointer_holder<boost::shared_ptr<CJavascriptObject>,CJavascriptObject> > >();

  v8::V8::InitializeICUDefaultLocation(argv[0]);
  v8::V8::InitializeExternalStartupData(argv[0]);
  std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();
  v8::Isolate* isolate = GetNewIsolate();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = v8::Context::New(isolate);
  v8::Context::Scope context_scope(context);

  v8::Handle<v8::Value> result = run_javascript(isolate, context, code);
  v8::String::Utf8Value utf8(isolate, result);
  printf("script result: %s\n", *utf8);

  v8::Local<v8::ObjectTemplate> templ = v8::ObjectTemplate::New(isolate);
  v8::Local<v8::Object> dummy = templ->NewInstance(context).ToLocalChecked();
  CJavascriptObject wrapper(dummy);

  py::object pyres = wrapper.Wrap(result);
  std::cout << "Python result size: " << sizeof(pyres) << std::endl;
}


