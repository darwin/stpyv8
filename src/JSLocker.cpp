#include "JSLocker.h"
#include "JSIsolate.h"
#include "PythonThreads.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSLockingLogger), __VA_ARGS__)

bool CJSLocker::IsEntered() const {
  auto result = static_cast<bool>(m_v8_locker.get());
  TRACE("CJSLocker::IsEntered {} => {}", THIS, result);
  return result;
}

void CJSLocker::Enter() {
  TRACE("CJSLocker::Enter {}", THIS);
  withAllowedPythonThreads([&]() {
    auto v8_isolate = v8u::getCurrentIsolate();
    m_isolate = CJSIsolate::FromV8(v8_isolate);
    m_v8_locker = std::make_unique<v8::Locker>(v8_isolate);
  });
}

void CJSLocker::Leave() {
  TRACE("CJSLocker::Leave {}", THIS);
  withAllowedPythonThreads([&]() {
    m_v8_locker.reset();
    m_isolate.reset();
  });
}

bool CJSLocker::IsLocked() {
  auto result = v8::Locker::IsLocked(v8u::getCurrentIsolate());
  TRACE("CJSLocker::IsLocked => {}", result);
  return result;
}

bool CJSLocker::IsActive() {
  auto result = v8::Locker::IsActive();
  TRACE("CJSLocker::IsActive => {}", result);
  return result;
}
