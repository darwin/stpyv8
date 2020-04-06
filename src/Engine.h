#pragma once

#include "Base.h"

class CEngine {
  v8::Isolate* m_v8_isolate;

 protected:
  CScriptPtr InternalCompile(v8::Local<v8::String> v8_src, v8::Local<v8::Value> v8_name, int line, int col);
  static void TerminateAllThreads();

 public:
  explicit CEngine(v8::Isolate* v8_isolate = nullptr);

  CScriptPtr Compile(const std::string& src, const std::string& name = std::string(), int line = -1, int col = -1);
  CScriptPtr CompileW(const std::wstring& src, const std::wstring& name = std::wstring(), int line = -1, int col = -1);

 public:
  static const char* GetVersion();
  static void SetStackLimit(uintptr_t stack_limit_size);

  [[nodiscard]] pb::object ExecuteScript(v8::Local<v8::Script> v8_script) const;

  static void SetFlags(const std::string& flags);

  static bool IsDead();

  static void Expose(const pb::module& py_module);
};