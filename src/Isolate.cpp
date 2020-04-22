#include "Isolate.h"
#include "Tracer.h"
#include "Hospital.h"
#include "Eternals.h"
#include "JSStackTrace.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kIsolateLogger), __VA_ARGS__)

const int kSelfDataSlotIndex = 0;

CIsolatePtr CIsolate::FromV8(const v8::IsolateRef& v8_isolate) {
  // FromV8 may be called only on isolates created by our constructor
  assert(v8_isolate->GetNumberOfDataSlots() > kSelfDataSlotIndex);
  auto isolate_ptr = static_cast<CIsolate*>(v8_isolate->GetData(kSelfDataSlotIndex));
  assert(isolate_ptr);
  TRACE("CIsolate::FromV8 v8_isolate={} => {}", P$(v8_isolate), (void*)isolate_ptr);
  return isolate_ptr->shared_from_this();
}

CIsolate::CIsolate() : m_v8_isolate(v8u::createIsolate()) {
  TRACE("CIsolate::CIsolate {}", THIS);
  m_eternals = std::make_unique<CEternals>(m_v8_isolate);
  m_tracer = std::make_unique<CTracer>();
  m_hospital = std::make_unique<CHospital>(m_v8_isolate);
  m_v8_isolate->SetData(kSelfDataSlotIndex, this);
}

CIsolate::~CIsolate() {
  TRACE("CIsolate::~CIsolate {}", THIS);

  // isolate could be entered, we cannot dispose unless we exit it completely
  int defensive_counter = 0;
  while (m_v8_isolate->IsInUse()) {
    Leave();
    if (++defensive_counter > 100) {
      break;
    }
  }

  // hospital has to die and do cleanup before we call dispose
  m_hospital.reset();

  // tracer has to die and do cleanup before we call dispose
  m_tracer.reset();

  m_eternals.reset();

  Dispose();
  TRACE("CIsolate::~CIsolate {} [COMPLETED]", THIS);
}

CJSStackTracePtr CIsolate::GetCurrentStackTrace(int frame_limit, v8::StackTrace::StackTraceOptions v8_options) {
  TRACE("CIsolate::GetCurrentStackTrace {} frame_limit={} v8_options={:#x}", THIS, frame_limit, v8_options);
  return CJSStackTrace::GetCurrentStackTrace(m_v8_isolate, frame_limit, v8_options);
}

py::object CIsolate::GetCurrent() {
  TRACE("CIsolate::GetCurrent");
  // here we don't want to call our v8u::getCurrentIsolate, which returns not_null<>)
  auto v8_nullable_isolate = v8::Isolate::GetCurrent();
  if (!v8_nullable_isolate || !v8_nullable_isolate->IsInUse()) {
    TRACE("CIsolate::GetCurrent => None");
    return py::none();
  }

  v8::IsolateRef v8_isolate(v8_nullable_isolate);
  return py::cast(FromV8(v8_isolate));
}

bool CIsolate::IsLocked() {
  auto result = v8::Locker::IsLocked(m_v8_isolate);
  TRACE("CIsolate::IsLocked {} => {}", THIS, result);
  return result;
}

void CIsolate::Enter() {
  TRACE("CIsolate::Enter {}", THIS);
  m_v8_isolate->Enter();
}

void CIsolate::Leave() {
  TRACE("CIsolate::Leave {}", THIS);
  m_v8_isolate->Exit();
}

void CIsolate::Dispose() {
  TRACE("CIsolate::Dispose {}", THIS);
  m_v8_isolate->Dispose();
}

CTracer* CIsolate::Tracer() {
  TRACE("CIsolate::Tracer {} => {}", THIS, (void*)m_tracer.get());
  return m_tracer.get();
}

CHospital* CIsolate::Hospital() {
  TRACE("CIsolate::Hospital {} => {}", THIS, (void*)m_hospital.get());
  return m_hospital.get();
}

CEternals* CIsolate::Eternals() {
  TRACE("CIsolate::Eternals {} => {}", THIS, (void*)m_eternals.get());
  return m_eternals.get();
}
