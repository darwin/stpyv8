#include "_precompile.h"

#include "Unlocker.h"
#include "PythonAllowThreadsGuard.h"
#include "Isolate.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kLockingLogger), __VA_ARGS__)

void CUnlocker::Expose(const py::module& py_module) {
  TRACE("CUnlocker::Expose py_module={}", py_module);
  // clang-format off
  py::class_<CUnlocker>(py_module, "JSUnlocker")
      .def(py::init<>())
      .def("entered", &CUnlocker::IsEntered)
      .def("enter", &CUnlocker::Enter)
      .def("leave", &CUnlocker::Leave);
  // clang-format on
}

bool CUnlocker::IsEntered() {
  auto result = static_cast<bool>(m_v8_unlocker.get());
  TRACE("CUnlocker::IsEntered {} => {}", THIS, result);
  return result;
}

void CUnlocker::Enter() {
  TRACE("CUnlocker::Enter {}", THIS);
  withPythonAllowThreadsGuard([&]() {
    auto v8_isolate = v8u::getCurrentIsolate();
    m_isolate = CIsolate::FromV8(v8_isolate);
    m_v8_unlocker = std::make_unique<v8::Unlocker>(v8_isolate);
  });
}

void CUnlocker::Leave() {
  TRACE("CUnlocker::Leave {}", THIS);
  withPythonAllowThreadsGuard([&]() {
    m_v8_unlocker.reset();
    m_isolate.reset();
  });
}
