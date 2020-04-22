#include "JSObject.h"
#include "JSException.h"
#include "PythonThreads.h"
#include "PythonObject.h"
#include "Wrapping.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

py::object CJSObjectAPI::GetAttr(const py::object& py_key) const {
  py::object py_result;
  if (HasRole(Roles::Array)) {
    throw CJSException("__getattr__ not implemented for JSObjects with Array role", PyExc_AttributeError);
  } else if (HasRole(Roles::Array)) {
    py_result = m_cljs_impl.GetAttr(py_key);
  } else {
    py_result = m_generic_impl.GetAttr(py_key);
  }
  TRACE("CJSObjectAPI::GetAttr {} => {}", THIS, py_result);
  return py_result;
}

void CJSObjectAPI::SetAttr(const py::object& py_key, const py::object& py_obj) const {
  if (HasRole(Roles::Array)) {
    throw CJSException("__setattr__ not implemented for JSObjects with Array role", PyExc_AttributeError);
  } else {
    m_generic_impl.SetAttr(py_key, py_obj);
  }
}

void CJSObjectAPI::DelAttr(const py::object& py_key) const {
  if (HasRole(Roles::Array)) {
    throw CJSException("__delattr__ not implemented for JSObjects with Array role", PyExc_AttributeError);
  } else {
    m_generic_impl.DelAttr(py_key);
  }
}

py::list CJSObjectAPI::Dir() const {
  TRACE("CJSObjectAPI::Dir {}", THIS);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  v8u::checkContext(v8_isolate);

  auto py_gil = pyu::withGIL();

  py::list attrs;

  if (v8u::executionTerminating(v8_isolate)) {
    return attrs;
  }

  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto props = Object()->GetPropertyNames(v8_context).ToLocalChecked();

  for (size_t i = 0; i < props->Length(); i++) {
    auto v8_i = v8::Integer::New(v8_isolate, i);
    auto v8_prop = props->Get(v8_context, v8_i).ToLocalChecked();
    attrs.append(wrap(v8_isolate, v8_prop));
  }

  if (v8_try_catch.HasCaught()) {
    CJSException::HandleTryCatch(v8_isolate, v8_try_catch);
  }

  TRACE("CJSObjectAPI::Dir {} => {}", THIS, attrs);
  return attrs;
}

py::object CJSObjectAPI::GetItem(const py::object& py_key) const {
  py::object py_result;
  if (HasRole(Roles::Array)) {
    py_result = m_array_impl.GetItem(py_key);
  } else if (HasRole(Roles::CLJS)) {
    py_result = m_cljs_impl.GetItem(py_key);
  } else {
    // TODO: do robust arg checking here
    py_result = m_generic_impl.GetAttr(py::cast<py::str>(py_key));
  }
  TRACE("CJSObjectAPI::GetItem {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObjectAPI::SetItem(const py::object& py_key, const py::object& py_value) const {
  if (HasRole(Roles::Array)) {
    return m_array_impl.SetItem(py_key, py_value);
  } else {
    // TODO: do robust arg checking here
    m_generic_impl.SetAttr(py::cast<py::str>(py_key), py_value);
    return py::none();
  }
}

py::object CJSObjectAPI::DelItem(const py::object& py_key) const {
  if (HasRole(Roles::Array)) {
    return m_array_impl.DelItem(py_key);
  } else {
    // TODO: do robust arg checking here
    m_generic_impl.DelAttr(py::cast<py::str>(py_key));
    return py::none();
  }
}

bool CJSObjectAPI::Contains(const py::object& py_key) const {
  bool result;
  if (HasRole(Roles::Array)) {
    result = m_array_impl.Contains(py_key);
  } else {
    result = m_generic_impl.Contains(py_key);
  }
  TRACE("CJSObjectAPI::Contains {} => {}", THIS, result);
  return result;
}

int CJSObjectAPI::Hash() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  auto result = Object()->GetIdentityHash();
  TRACE("CJSObjectAPI::Hash {} => {}", THIS, result);
  return result;
}

CJSObjectPtr CJSObjectAPI::Clone() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  auto result = std::make_shared<CJSObject>(Object()->Clone());
  TRACE("CJSObjectAPI::Clone {} => {}", THIS, result);
  return result;
}

bool CJSObjectAPI::EQ(const CJSObjectPtr& other) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Context> context = v8_isolate->GetCurrentContext();

  auto result = other.get() && Object()->Equals(context, other->Object()).ToChecked();
  TRACE("CJSObjectAPI::EQ {} other={} => {}", THIS, other, result);
  return result;
}

bool CJSObjectAPI::NE(const CJSObjectPtr& other) const {
  return !EQ(other);
}

py::object CJSObjectAPI::Int() const {
  TRACE("CJSObjectAPI::ToPythonInt {}", THIS);
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  if (m_v8_obj.IsEmpty()) {
    throw CJSException("argument must be a string or a number, not 'NoneType'", PyExc_TypeError);
  }

  auto val = Object()->Int32Value(v8_context).ToChecked();
  auto py_result = py::cast(val);
  TRACE("CJSObjectAPI::Int {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObjectAPI::Float() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  if (m_v8_obj.IsEmpty()) {
    throw CJSException("argument must be a string or a number, not 'NoneType'", PyExc_TypeError);
  }

  auto val = Object()->NumberValue(v8_context).ToChecked();
  auto py_result = py::cast(val);
  TRACE("CJSObjectAPI::Float {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObjectAPI::Bool() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  bool val = false;
  if (!m_v8_obj.IsEmpty()) {
    val = Object()->BooleanValue(v8_isolate);
  }

  auto py_result = py::cast(val);
  TRACE("CJSObjectAPI::Bool {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObjectAPI::Str() const {
  py::object py_result;
  if (HasRole(Roles::CLJS)) {
    py_result = m_cljs_impl.Str();
  } else {
    std::stringstream ss;
    ss << *this;
    py_result = py::cast(ss.str());
  }

  TRACE("CJSObjectAPI::Str {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObjectAPI::Repr() const {
  py::object py_result;
  if (HasRole(Roles::CLJS)) {
    py_result = m_cljs_impl.Repr();
  } else {
    auto s = fmt::format("JSObject[{}]", m_roles);
    py_result = py::cast(s);
  }

  TRACE("CJSObjectAPI::Repr {} => {}", THIS, py_result);
  return py_result;
}

size_t CJSObjectAPI::Len() const {
  size_t result;
  if (HasRole(Roles::Array)) {
    result = m_array_impl.Length();
  } else if (HasRole(Roles::CLJS)) {
    result = m_cljs_impl.Length();
  } else {
    result = 0;
  }

  TRACE("CJSObjectAPI::Len {} => {}", THIS, result);
  return result;
}

py::object CJSObjectAPI::Call(const py::args& py_args, const py::kwargs& py_kwargs) {
  py::object py_result;
  if (HasRole(Roles::Function)) {
    py_result = m_function_impl.Call(py_args, py_kwargs);
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::Call {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObjectAPI::CreateWithArgs(const CJSObjectPtr& proto, const py::tuple& py_args, const py::dict& py_kwds) {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  if (proto->Object().IsEmpty()) {
    throw CJSException("Object prototype may only be an Object", PyExc_TypeError);
  }

  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto fn = proto->Object().As<v8::Function>();
  auto args_count = py_args.size();
  std::vector<v8::Local<v8::Value>> v8_params(args_count);

  for (size_t i = 0; i < args_count; i++) {
    v8_params[i] = CPythonObject::Wrap(py_args[i]);
  }

  v8::Local<v8::Object> v8_result;

  withAllowedPythonThreads(
      [&]() { v8_result = fn->NewInstance(v8_context, v8_params.size(), v8_params.data()).ToLocalChecked(); });

  if (v8_result.IsEmpty()) {
    CJSException::HandleTryCatch(v8_isolate, v8_try_catch);
  }

  auto it = py_kwds.begin();
  while (it != py_kwds.end()) {
    auto py_key = it->first;
    auto py_val = it->second;
    auto v8_key = v8u::toString(py_key);
    auto v8_val = CPythonObject::Wrap(py_val);
    v8_result->Set(v8_context, v8_key, v8_val).Check();
    it++;
  }

  return wrap(v8_isolate, v8_result);
}

py::object CJSObjectAPI::Apply(const py::object& py_self, const py::list& py_args, const py::dict& py_kwds) {
  py::object py_result;
  if (HasRole(Roles::Function)) {
    py_result = m_function_impl.Apply(py_self, py_args, py_kwds);
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::Apply {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObjectAPI::Invoke(const py::list& py_args, const py::dict& py_kwds) {
  py::object py_result;
  if (HasRole(Roles::Function)) {
    py_result = m_function_impl.Call(py_args, py_kwds);
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::Invoke {} => {}", THIS, py_result);
  return py_result;
}

std::string CJSObjectAPI::GetName() const {
  std::string result;
  if (HasRole(Roles::Function)) {
    result = m_function_impl.GetName();
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::GetName {} => {}", THIS, result);
  return result;
}

void CJSObjectAPI::SetName(const std::string& name) {
  TRACE("CJSObjectAPI::SetName {} name={}", THIS, name);
  if (HasRole(Roles::Function)) {
    m_function_impl.SetName(name);
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }
}

int CJSObjectAPI::LineNumber() const {
  int result;
  if (HasRole(Roles::Function)) {
    result = m_function_impl.GetLineNumber();
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::LineNumber {} => {}", THIS, result);
  return result;
}

int CJSObjectAPI::ColumnNumber() const {
  int result;
  if (HasRole(Roles::Function)) {
    result = m_function_impl.GetColumnNumber();
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::ColumnNumber {} => {}", THIS, result);
  return result;
}

std::string CJSObjectAPI::ResourceName() const {
  std::string result;
  if (HasRole(Roles::Function)) {
    result = m_function_impl.GetResourceName();
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::ResourceName {} => {}", THIS, result);
  return result;
}

std::string CJSObjectAPI::InferredName() const {
  std::string result;
  if (HasRole(Roles::Function)) {
    result = m_function_impl.GetInferredName();
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::InferredName {} => {}", THIS, result);
  return result;
}

int CJSObjectAPI::LineOffset() const {
  int result;
  if (HasRole(Roles::Function)) {
    result = m_function_impl.GetLineOffset();
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::LineOffset {} => {}", THIS, result);
  return result;
}

int CJSObjectAPI::ColumnOffset() const {
  int result;
  if (HasRole(Roles::Function)) {
    result = m_function_impl.GetColumnOffset();
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::ColumnOffset {} => {}", THIS, result);
  return result;
}