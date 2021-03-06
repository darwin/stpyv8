#include "V8XIsolateLockerHolder.h"
#include "Logging.h"
#include "Printing.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kIsolateLockingLogger), __VA_ARGS__)

namespace v8x {

IsolateLockerHolder::IsolateLockerHolder(v8::Isolate* v8_isolate) : m_v8_isolate(v8_isolate) {
  TRACE("IsolateLocker::IsolateLocker {} v8_isolate={}", THIS, P$(m_v8_isolate));
}

IsolateLockerHolder::~IsolateLockerHolder() {
  TRACE("IsolateLocker::~IsolateLocker {} v8_isolate={}", THIS, P$(m_v8_isolate));
}

SharedIsolateLockerPtr IsolateLockerHolder::CreateOrShareLocker() {
  // if locker already exists, just share it
  if (auto shared_locker = m_v8_weak_locker.lock()) {
    TRACE("IsolateLocker::CreateOrShareLocker {} v8_isolate={} sharing existing locker {}", THIS, P$(m_v8_isolate),
          (void*)shared_locker.get());
    return shared_locker;
  } else {
    // else create new locker inplace and share it
    auto v8_locker_ptr = new (m_v8_locker_storage.data()) LockerType(m_v8_isolate);
    auto new_shared_locker = SharedIsolateLockerPtr(v8_locker_ptr, DeleteInplaceLocker);
    TRACE("IsolateLocker::CreateOrShareLocker {} v8_isolate={} creating new locker {}", THIS, P$(m_v8_isolate),
          (void*)new_shared_locker.get());
    m_v8_weak_locker = new_shared_locker;
    return new_shared_locker;
  }
}

void IsolateLockerHolder::DeleteInplaceLocker(LockerType* p) {
  TRACE("IsolateLockerHolder::DeleteInplaceLocker locker={}", (void*)p);
  // just call the destructor
  // deallocation is not needed because we keep the buffer for future usage
  p->~LockerType();
}

LockedIsolatePtr IsolateLockerHolder::GetLockedIsolate() {
  return LockedIsolatePtr(m_v8_isolate, CreateOrShareLocker());
}

}  // namespace v8x