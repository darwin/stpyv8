#include "_precompile.h"

#include "JSException.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSExceptionLogger), __VA_ARGS__)

static void translateJavascriptException(const CJSException& e) {
  TRACE("translateJavascriptException");
  auto py_gil = pyu::withGIL();

  if (e.GetType()) {
    PyErr_SetString(e.GetType(), e.what());
  } else {
    auto v8_isolate = v8u::getCurrentIsolate();
    auto v8_scope = v8u::withScope(v8_isolate);

    if (!e.Exception().IsEmpty() && e.Exception()->IsObject()) {
      auto v8_ex = e.Exception()->ToObject(v8_isolate->GetCurrentContext()).ToLocalChecked();

      auto v8_ex_type_key = v8::String::NewFromUtf8(v8_isolate, "exc_type").ToLocalChecked();
      auto v8_ex_type_api = v8::Private::ForApi(v8_isolate, v8_ex_type_key);
      auto v8_ex_type_val = v8_ex->GetPrivate(v8_isolate->GetCurrentContext(), v8_ex_type_api);

      auto v8_ex_value_key = v8::String::NewFromUtf8(v8_isolate, "exc_value").ToLocalChecked();
      auto v8_ex_value_api = v8::Private::ForApi(v8_isolate, v8_ex_value_key);
      auto v8_ex_value_val = v8_ex->GetPrivate(v8_isolate->GetCurrentContext(), v8_ex_value_api);

      if (!v8_ex_type_val.IsEmpty() && !v8_ex_value_val.IsEmpty()) {
        auto v8_ex_type = v8_ex_type_val.ToLocalChecked();
        PyObject* raw_type = nullptr;
        if (v8_ex_type->IsExternal()) {
          auto type_val = v8::Local<v8::External>::Cast(v8_ex_type)->Value();
          raw_type = static_cast<PyObject*>(type_val);
        }
        auto v8_ex_value = v8_ex_value_val.ToLocalChecked();
        PyObject* raw_value = nullptr;
        if (v8_ex_value->IsExternal()) {
          auto value_val = v8::Local<v8::External>::Cast(v8_ex_value)->Value();
          raw_value = static_cast<PyObject*>(value_val);
        }

        // TODO: remove manual refcounting
        if (raw_type && raw_value) {
          PyErr_SetObject(raw_type, raw_value);
          Py_DECREF(raw_type);
          Py_DECREF(raw_value);
          return;
        }
        if (raw_type) {
          Py_DECREF(raw_type);
        }
        if (raw_value) {
          Py_DECREF(raw_value);
        }
      }
    }

    // TODO: revisit if we can do better with pybind
    // Boost Python doesn't support inheriting from Python class,
    // so, just use some workaround to throw our custom exception
    //
    // http://www.language-binding.net/pyplusplus/troubleshooting_guide/exceptions/exceptions.html

    auto m = py::module::import("_STPyV8");
    auto py_error_class = m.attr("JSError");
    auto py_error_instance = py_error_class(e);
    py_error_instance.inc_ref();
    PyErr_SetObject(py_error_class.ptr(), py_error_instance.ptr());
  }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "performance-unnecessary-value-param"
// TODO: raise question in pybind issues
// clang-tidy suggests "const std::exception_ptr& p" signature but register_exception_translator won't accept it
static void translateException(std::exception_ptr p) {
  TRACE("translateException");
  try {
    if (p) {
      std::rethrow_exception(p);
    }
  } catch (const CJSException& e) {
    translateJavascriptException(e);
  }
}
#pragma clang diagnostic pop

void CJSException::Expose(const py::module& py_module) {
  TRACE("CJSException::Expose py_module={}", py_module);

  py::register_exception_translator(&translateException);

  // clang-format off
  py::class_<CJSException>(py_module, "_JSError")
      .def("__str__", &CJSException::ToPythonStr)

      .def_property_readonly("name", &CJSException::GetName,
                             "The exception name.")
      .def_property_readonly("message", &CJSException::GetMessage,
                             "The exception message.")
      .def_property_readonly("scriptName", &CJSException::GetScriptName,
                             "The script name which throw the exception.")
      .def_property_readonly("lineNum", &CJSException::GetLineNumber, "The line number of error statement.")
      .def_property_readonly("startPos", &CJSException::GetStartPosition,
                             "The start position of error statement in the script.")
      .def_property_readonly("endPos", &CJSException::GetEndPosition,
                             "The end position of error statement in the script.")
      .def_property_readonly("startCol", &CJSException::GetStartColumn,
                             "The start column of error statement in the script.")
      .def_property_readonly("endCol", &CJSException::GetEndColumn,
                             "The end column of error statement in the script.")
      .def_property_readonly("sourceLine", &CJSException::GetSourceLine,
                             "The source line of error statement.")
      .def_property_readonly("stackTrace", &CJSException::GetStackTrace,
                             "The stack trace of error statement.")
      .def("print_tb", &CJSException::PrintCallStack,
           py::arg("file") = py::none(),
           "Print the stack trace of error statement.");
  // clang-format on
}

CJSException::CJSException(const v8::IsolateRef& v8_isolate, const v8::TryCatch& v8_try_catch, PyObject* raw_type)
    : std::runtime_error(Extract(v8_isolate, v8_try_catch)), m_v8_isolate(v8_isolate), m_raw_type(raw_type) {
  TRACE("CJSException::CJSException {} v8_isolate={} v8_try_catch={} raw_type={}", THIS,
        isolateref_printer{m_v8_isolate}, v8_try_catch, raw_object_printer{raw_type});
  auto v8_scope = v8u::withScope(m_v8_isolate);

  m_v8_exception.Reset(m_v8_isolate, v8_try_catch.Exception());

  auto stack_trace = v8_try_catch.StackTrace(v8u::getCurrentIsolate()->GetCurrentContext());
  if (!stack_trace.IsEmpty()) {
    m_v8_stack.Reset(m_v8_isolate, stack_trace.ToLocalChecked());
  }

  m_v8_message.Reset(m_v8_isolate, v8_try_catch.Message());
}

CJSException::CJSException(v8::IsolateRef v8_isolate, const std::string& msg, PyObject* raw_type) noexcept
    : std::runtime_error(msg), m_v8_isolate(std::move(v8_isolate)), m_raw_type(raw_type) {
  TRACE("CJSException::CJSException {} v8_isolate={} msg='{}' raw_type={}", THIS, isolateref_printer{m_v8_isolate}, msg,
        raw_object_printer{raw_type});
}

CJSException::CJSException(const std::string& msg, PyObject* raw_type) noexcept
    : std::runtime_error(msg), m_v8_isolate(v8u::getCurrentIsolate()), m_raw_type(raw_type) {
  TRACE("CJSException::CJSException {} msg='{}' raw_type={}", THIS, msg, raw_object_printer{raw_type});
}

CJSException::CJSException(const CJSException& ex) noexcept
    : std::runtime_error(ex.what()), m_v8_isolate(ex.m_v8_isolate), m_raw_type(ex.m_raw_type) {
  TRACE("CJSException::CJSException {} ex={}", THIS, ex);
  auto v8_scope = v8u::withScope(m_v8_isolate);

  m_v8_exception.Reset(m_v8_isolate, ex.Exception());
  m_v8_stack.Reset(m_v8_isolate, ex.Stack());
  m_v8_message.Reset(m_v8_isolate, ex.Message());
}

CJSException::~CJSException() noexcept {
  TRACE("CJSException::~CJSException {}", THIS);
  if (!m_v8_exception.IsEmpty()) {
    m_v8_exception.Reset();
  }
  if (!m_v8_message.IsEmpty()) {
    m_v8_message.Reset();
  }
}

std::string CJSException::GetName() {
  TRACE("CJSException::GetName {}", THIS);
  if (m_v8_exception.IsEmpty()) {
    return std::string();
  }

  assert(m_v8_isolate->InContext());

  auto v8_scope = v8u::withScope(m_v8_isolate);

  v8::String::Utf8Value msg(m_v8_isolate, v8::Local<v8::String>::Cast(
                                              Exception()
                                                  ->ToObject(m_v8_isolate->GetCurrentContext())
                                                  .ToLocalChecked()
                                                  ->Get(m_v8_isolate->GetCurrentContext(),
                                                        v8::String::NewFromUtf8(m_v8_isolate, "name").ToLocalChecked())
                                                  .ToLocalChecked()));
  auto result = std::string(*msg, msg.length());
  TRACE("CJSException::GetName {} => {}", THIS, result);
  return result;
}

std::string CJSException::GetMessage() {
  TRACE("CJSException::GetMessage {}", THIS);
  if (m_v8_exception.IsEmpty()) {
    return std::string();
  }

  assert(m_v8_isolate->InContext());

  auto v8_scope = v8u::withScope(m_v8_isolate);

  v8::String::Utf8Value msg(
      m_v8_isolate,
      v8::Local<v8::String>::Cast(Exception()
                                      ->ToObject(m_v8_isolate->GetCurrentContext())
                                      .ToLocalChecked()
                                      ->Get(m_v8_isolate->GetCurrentContext(),
                                            v8::String::NewFromUtf8(m_v8_isolate, "message").ToLocalChecked())
                                      .ToLocalChecked()));

  auto result = std::string(*msg, msg.length());
  TRACE("CJSException::GetMessage {} => {}", THIS, result);
  return result;
}

std::string CJSException::GetScriptName() {
  TRACE("CJSException::GetScriptName {}", THIS);
  assert(m_v8_isolate->InContext());

  auto v8_scope = v8u::withScope(m_v8_isolate);

  if (m_v8_message.IsEmpty() || Message()->GetScriptResourceName().IsEmpty() ||
      Message()->GetScriptResourceName()->IsUndefined()) {
    return std::string();
  }

  v8::String::Utf8Value name(m_v8_isolate, Message()->GetScriptResourceName());

  auto result = std::string(*name, name.length());
  TRACE("CJSException::GetScriptName {} => {}", THIS, result);
  return result;
}

int CJSException::GetLineNumber() {
  assert(m_v8_isolate->InContext());
  auto v8_scope = v8u::withScope(m_v8_isolate);
  auto result = m_v8_message.IsEmpty() ? 1 : Message()->GetLineNumber(m_v8_isolate->GetCurrentContext()).ToChecked();
  TRACE("CJSException::GetLineNumber {} => {}", THIS, result);
  return result;
}

int CJSException::GetStartPosition() {
  assert(m_v8_isolate->InContext());
  auto v8_scope = v8u::withScope(m_v8_isolate);
  auto result = m_v8_message.IsEmpty() ? 1 : Message()->GetStartPosition();
  TRACE("CJSException::GetStartPosition {} => {}", THIS, result);
  return result;
}

int CJSException::GetEndPosition() {
  assert(m_v8_isolate->InContext());
  auto v8_scope = v8u::withScope(m_v8_isolate);
  auto result = m_v8_message.IsEmpty() ? 1 : Message()->GetEndPosition();
  TRACE("CJSException::GetEndPosition {} => {}", THIS, result);
  return result;
}

int CJSException::GetStartColumn() {
  assert(m_v8_isolate->InContext());
  auto v8_scope = v8u::withScope(m_v8_isolate);
  auto result = m_v8_message.IsEmpty() ? 1 : Message()->GetStartColumn();
  TRACE("CJSException::GetStartColumn {} => {}", THIS, result);
  return result;
}

int CJSException::GetEndColumn() {
  TRACE("CJSException::GetEndColumn {}", THIS);
  assert(m_v8_isolate->InContext());
  auto v8_scope = v8u::withScope(m_v8_isolate);
  auto result = m_v8_message.IsEmpty() ? 1 : Message()->GetEndColumn();
  TRACE("CJSException::GetEndColumn {} => {}", THIS, result);
  return result;
}

std::string CJSException::GetSourceLine() {
  TRACE("CJSException::GetSourceLine {}", THIS);
  assert(m_v8_isolate->InContext());

  auto v8_scope = v8u::withScope(m_v8_isolate);

  if (m_v8_message.IsEmpty() || Message()->GetSourceLine(m_v8_isolate->GetCurrentContext()).IsEmpty()) {
    return std::string();
  }

  auto v8_line = Message()->GetSourceLine(m_v8_isolate->GetCurrentContext()).ToLocalChecked();
  v8::String::Utf8Value line(m_v8_isolate, v8_line);
  auto result = std::string(*line, line.length());
  TRACE("CJSException::GetSourceLine {} => {}", THIS, result);
  return result;
}

std::string CJSException::GetStackTrace() {
  TRACE("CJSException::GetStackTrace {}", THIS);
  assert(m_v8_isolate->InContext());

  auto v8_scope = v8u::withScope(m_v8_isolate);

  if (m_v8_stack.IsEmpty()) {
    return std::string();
  }

  v8::String::Utf8Value stack(m_v8_isolate, v8::Local<v8::String>::Cast(Stack()));
  auto result = std::string(*stack, stack.length());
  TRACE("CJSException::GetStackTrace {} => {}", THIS, result);
  return result;
}

std::string CJSException::Extract(const v8::IsolateRef& v8_isolate, const v8::TryCatch& v8_try_catch) {
  TRACE("CJSException::Extract v8_isolate={} v8_try_catch={}", isolateref_printer{v8_isolate}, v8_try_catch);
  assert(v8_isolate->InContext());

  auto v8_scope = v8u::withScope(v8_isolate);

  std::ostringstream oss;

  v8::String::Utf8Value msg(v8_isolate, v8_try_catch.Exception());

  if (*msg) {
    oss << std::string(*msg, msg.length());
  }

  v8::Local<v8::Message> message = v8_try_catch.Message();

  if (!message.IsEmpty()) {
    oss << " ( ";

    if (!message->GetScriptResourceName().IsEmpty() && !message->GetScriptResourceName()->IsUndefined()) {
      v8::String::Utf8Value name(v8_isolate, message->GetScriptResourceName());

      oss << std::string(*name, name.length());
    }

    oss << " @ " << message->GetLineNumber(v8_isolate->GetCurrentContext()).ToChecked() << " : "
        << message->GetStartColumn() << " ) ";

    if (!message->GetSourceLine(v8_isolate->GetCurrentContext()).IsEmpty()) {
      v8::String::Utf8Value line(v8_isolate, message->GetSourceLine(v8_isolate->GetCurrentContext()).ToLocalChecked());

      oss << " -> " << std::string(*line, line.length());
    }
  }

  return oss.str();
}

struct SupportedError {
  const char* m_name;
  PyObject* m_raw_type;
};

static SupportedError g_supported_errors[] = {{"RangeError", PyExc_IndexError},
                                              {"ReferenceError", PyExc_ReferenceError},
                                              {"SyntaxError", PyExc_SyntaxError},
                                              {"TypeError", PyExc_TypeError}};

void CJSException::ThrowIf(const v8::IsolateRef& v8_isolate, const v8::TryCatch& v8_try_catch) {
  TRACE("CJSException::ThrowIf v8_isolate={} v8_try_catch={}", isolateref_printer{v8_isolate}, v8_try_catch);
  auto v8_scope = v8u::withScope(v8_isolate);
  if (!v8_try_catch.HasCaught() || !v8_try_catch.CanContinue()) {
    return;
  }

  PyObject* raw_type = nullptr;
  auto v8_ex = v8_try_catch.Exception();

  if (v8_ex->IsObject()) {
    auto v8_context = v8_isolate->GetCurrentContext();
    auto v8_exc_obj = v8_ex->ToObject(v8_context).ToLocalChecked();
    auto v8_name = v8::String::NewFromUtf8(v8_isolate, "name").ToLocalChecked();

    if (v8_exc_obj->Has(v8_context, v8_name).ToChecked()) {
      auto v8_name_val = v8_exc_obj->Get(v8_isolate->GetCurrentContext(), v8_name).ToLocalChecked();
      auto v8_uft = v8u::toUTF(v8_isolate, v8_name_val.As<v8::String>());

      for (auto& error : g_supported_errors) {
        if (strncasecmp(error.m_name, *v8_uft, v8_uft.length()) == 0) {
          raw_type = error.m_raw_type;
        }
      }
    }
  }

  throw CJSException(v8_isolate, v8_try_catch, raw_type);
}

void CJSException::PrintCallStack(py::object py_file) {
  TRACE("CJSException::PrintCallStack {} py_file={}", THIS, py_file);
  auto py_gil = pyu::withGIL();

  // TODO: move this into utility function
  auto raw_file = py_file.is_none() ? PySys_GetObject("stdout") : py_file.ptr();
  auto fd = PyObject_AsFileDescriptor(raw_file);

  Message()->PrintCurrentStackTrace(m_v8_isolate, fdopen(fd, "w+"));
}

py::object CJSException::ToPythonStr() const {
  std::stringstream ss;
  ss << *this;
  auto result = py::cast(ss.str());
  TRACE("CJSException::ToPythonStr {} => {}", THIS, result);
  return result;
}

v8::Local<v8::Value> CJSException::Exception() const {
  auto result = v8::Local<v8::Value>::New(m_v8_isolate, m_v8_exception);
  TRACE("CJSException::Exception {} => {}", THIS, result);
  return result;
}

v8::Local<v8::Value> CJSException::Stack() const {
  auto result = v8::Local<v8::Value>::New(m_v8_isolate, m_v8_stack);
  TRACE("CJSException::Stack {} => {}", THIS, result);
  return result;
}

v8::Local<v8::Message> CJSException::Message() const {
  auto result = v8::Local<v8::Message>::New(m_v8_isolate, m_v8_message);
  TRACE("CJSException::Message {} => {}", THIS, result);
  return result;
}

PyObject* CJSException::GetType() const {
  TRACE("CJSException::GetType {} => {}", THIS, raw_object_printer{m_raw_type});
  return m_raw_type;
}
