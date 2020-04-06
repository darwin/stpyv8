#include "Engine.h"
#include "Exception.h"
#include "JSObject.h"
#include "PythonAllowThreadsGuard.h"

void CEngine::Expose(const pb::module& m) {
  // clang-format off
  pb::class_<CEngine>(m, "JSEngine", "JSEngine is a backend Javascript engine.")
      .def(pb::init<>(),
           "Create a new script engine instance.")
      .def_property_readonly_static(
          "version", [](const pb::object&) { return CEngine::GetVersion(); },
          "Get the V8 engine version.")

      .def_property_readonly_static(
          "dead", [](const pb::object&) { return CEngine::IsDead(); },
          "Check if V8 is dead and therefore unusable.")

      .def_static("setFlags", &CEngine::SetFlags,
                  "Sets V8 flags from a string.")

      .def_static("terminateAllThreads", &CEngine::TerminateAllThreads,
                  "Forcefully terminate the current thread of JavaScript execution.")

      .def_static(
          "dispose", []() { return v8::V8::Dispose(); },
          "Releases any resources used by v8 and stops any utility threads "
          "that may be running. Note that disposing v8 is permanent, "
          "it cannot be reinitialized.")

      .def_static(
          "lowMemory", []() { v8::Isolate::GetCurrent()->LowMemoryNotification(); },
          "Optional notification that the system is running low on memory.")

          /*
              .def("setMemoryLimit", &CEngine::SetMemoryLimit, (pb::arg("max_young_space_size") = 0,
                                                                pb::arg("max_old_space_size") = 0,
                                                                pb::arg("max_executable_size") = 0),
                   "Specifies the limits of the runtime's memory use."
                   "You must set the heap size before initializing the VM"
                   "the size cannot be adjusted after the VM is initialized.")
              .staticmethod("setMemoryLimit")
          */

      .def_static("setStackLimit", &CEngine::SetStackLimit,
                  pb::arg("stack_limit_size") = 0,
                  "Uses the address of a local variable to determine the stack top now."
                  "Given a size, returns an address that is that far from the current top of stack.")

          /*
              .def("setMemoryAllocationCallback", &MemoryAllocationManager::SetCallback,
                                                  (pb::arg("callback"),
                                                   pb::arg("space") = v8::kObjectSpaceAll,
                                                   pb::arg("action") = v8::kAllocationActionAll),
                                                  "Enables the host application to provide a mechanism to be notified "
                                                  "and perform custom logging when V8 Allocates Executable Memory.")
              .staticmethod("setMemoryAllocationCallback")
          */

      .def("compile", &CEngine::Compile,
           pb::arg("source"),
           pb::arg("name") = std::string(),
           pb::arg("line") = -1,
           pb::arg("col") = -1)
      .def("compile", &CEngine::CompileW,
           pb::arg("source"),
           pb::arg("name") = std::wstring(),
           pb::arg("line") = -1,
           pb::arg("col") = -1);

  pb::class_<CScript, CScriptPtr>(m, "JSScript", "JSScript is a compiled JavaScript script.")
      .def_property_readonly("source", &CScript::GetSource,
                             "the source code")

      .def("run", &CScript::Run,
           "Execute the compiled code.");
  // clang-format on
}

bool CEngine::IsDead() {
  return v8::Isolate::GetCurrent()->IsDead();
}

void CEngine::TerminateAllThreads() {
  v8::Isolate::GetCurrent()->TerminateExecution();
}

void CEngine::ReportFatalError(const char* location, const char* message) {
  std::cerr << "<" << location << "> " << message << std::endl;
}

void CEngine::ReportMessage(v8::Local<v8::Message> v8_message, [[maybe_unused]] v8::Local<v8::Value> v8_data) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_context = v8_isolate->GetCurrentContext();

  v8::String::Utf8Value filename(v8_isolate, v8_message->GetScriptResourceName());
  int line_number = v8_message->GetLineNumber(v8_context).ToChecked();
  v8::String::Utf8Value source_line(v8_isolate, v8_message->GetSourceLine(v8_context).ToLocalChecked());

  std::cerr << *filename << ":" << line_number << " -> " << *source_line << std::endl;
}

bool CEngine::SetMemoryLimit(int max_young_space_size,
                             int max_old_space_size,
                             [[maybe_unused]] int max_executable_size) {
  v8::ResourceConstraints limit;

  if (max_young_space_size) {
    limit.set_max_young_generation_size_in_bytes(max_young_space_size);
  }

  if (max_old_space_size) {
    limit.set_max_old_generation_size_in_bytes(max_old_space_size);
  }
  // TODO: should this be code range size instead?
  // if (max_executable_size) limit.set_max_executable_size(max_executable_size);

  // TODO: - memory limits are now only settable on isolate creation
  // return v8::SetResourceConstraints(v8::Isolate::GetCurrent(), &limit);
  return false;
}

void CEngine::SetStackLimit(uintptr_t stack_limit_size) {
  // This function uses a local stack variable to determine the isolate's
  // stack limit
  uint32_t here;
  uintptr_t stack_limit = reinterpret_cast<uintptr_t>(&here) - stack_limit_size;

  // If the size is very large and the stack is near the bottom of memory
  // then the calculation above may wrap around and give an address that is
  // above the (downwards-growing) stack. In such case we alert the user
  // that the new stack limit is not going to be set and return
  if (stack_limit > reinterpret_cast<uintptr_t>(&here)) {
    std::cerr << "[ERROR] Attempted to set a stack limit greater than available memory" << std::endl;
    return;
  }

  v8::Isolate::GetCurrent()->SetStackLimit(stack_limit);
}

CScriptPtr CEngine::InternalCompile(v8::Local<v8::String> v8_src, v8::Local<v8::Value> v8_name, int line, int col) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  v8::Persistent<v8::String> script_source(m_v8_isolate, v8_src);

  v8::MaybeLocal<v8::Script> script;
  v8::Local<v8::String> source = v8::Local<v8::String>::New(m_v8_isolate, script_source);

  withPythonAllowThreadsGuard([&]() {
    if (line >= 0 && col >= 0) {
      v8::ScriptOrigin script_origin(v8_name, v8::Integer::New(m_v8_isolate, line),
                                     v8::Integer::New(m_v8_isolate, col));
      script = v8::Script::Compile(context, source, &script_origin);
    } else {
      v8::ScriptOrigin script_origin(v8_name);
      script = v8::Script::Compile(context, source, &script_origin);
    }
  });

  if (script.IsEmpty()) {
    CJavascriptException::ThrowIf(m_v8_isolate, try_catch);
  }

  return CScriptPtr(new CScript(m_v8_isolate, *this, script_source, script.ToLocalChecked()));
}

pb::object CEngine::ExecuteScript(v8::Local<v8::Script> v8_script) const {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

  v8::MaybeLocal<v8::Value> v8_result;

  withPythonAllowThreadsGuard([&]() { v8_result = v8_script->Run(v8_context); });

  if (!v8_result.IsEmpty()) {
    return CJSObject::Wrap(v8_result.ToLocalChecked());
  } else {
    if (v8_try_catch.HasCaught()) {
      if (!v8_try_catch.CanContinue() && PyErr_Occurred()) {
        throw pb::error_already_set();
      }

      CJavascriptException::ThrowIf(m_v8_isolate, v8_try_catch);
    }
    v8_result = v8::Null(m_v8_isolate);
  }

  return CJSObject::Wrap(v8_result.ToLocalChecked());
}

void CEngine::SetFlags(const std::string& flags) {
  v8::V8::SetFlagsFromString(flags.c_str(), flags.size());
}

const char* CEngine::GetVersion() {
  return v8::V8::GetVersion();
}

CEngine::CEngine(v8::Isolate* v8_isolate) : m_v8_isolate(v8_isolate) {
  if (!m_v8_isolate) {
    m_v8_isolate = v8::Isolate::GetCurrent();
  }
}

CScriptPtr CEngine::Compile(const std::string& src, const std::string& name, int line, int col) {
  auto v8_scope = v8u::getScope(m_v8_isolate);
  return InternalCompile(v8u::toString(src), v8u::toString(name), line, col);
}

CScriptPtr CEngine::CompileW(const std::wstring& src, const std::wstring& name, int line, int col) {
  auto v8_scope = v8u::getScope(m_v8_isolate);
  return InternalCompile(v8u::toString(src), v8u::toString(name), line, col);
}

std::string CScript::GetSource() const {
  v8::HandleScope handle_scope(m_isolate);

  v8::String::Utf8Value source(m_isolate, Source());

  return std::string(*source, source.length());
}

pb::object CScript::Run() {
  auto v8_scope = v8u::getScope(m_isolate);
  return m_engine.ExecuteScript(Script());
}
