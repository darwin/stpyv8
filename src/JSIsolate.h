#ifndef NAGA_JSISOLATE_H_
#define NAGA_JSISOLATE_H_

#include "Base.h"
#include "V8XProtectedIsolate.h"
#include "V8XLockedIsolate.h"
#include "V8XIsolateLockerHolder.h"

// JSIsolate is our wrapper of v8::Isolate which provides Python interface for exposed JSIsolate object.
//
// JSIsolate instances are typically created from Python side by calling exposed JSIsolate().
// Pybind machinery creates them as shared pointers and C++ should pass them around as shared pointers (JSIsolatePtr).
// If you happen to receive a naked v8::Isolate* you can obtain associated wrapper via JSIsolate::FromV8().
// Note that isolates not created by our wrapper are considered foreign isolates and FromV8 will throw.
// If you have a JSIsolate instance you can get to naked v8::Isolate* by calling JSIsolate::ToV8().
// Calls to FromV8/ToV8 should be cheap.
//
// JSIsolate holds several helper data structures where we keep track of some C++/Python objects associated
// with JS objects living in the isolate. It is important to properly dispose these resources before the isolate
// goes away. See the destructor. Just to refresh: JSIsolate is de-allocated when last smart pointer holder drops it.
// Please note that both Python side and C++ side can hold it (smart pointers from live Python JSIsolate objects managed
// by pybind and JSIsolatePtr in our codebase in C++). So for V8 isolate to be let go all users have to drop
// reference to its JSIsolate wrapper which will call JSIsolate destructor which will dispose all resources and
// finally dispose the isolate in V8.

class JSIsolate : public std::enable_shared_from_this<JSIsolate> {
  using LockerLevelStack = std::stack<int>;
  v8x::ProtectedIsolatePtr m_v8_isolate;
  std::unique_ptr<JSTracer> m_tracer;
  std::unique_ptr<JSHospital> m_hospital;
  std::unique_ptr<JSEternals> m_eternals;
  v8x::IsolateLockerHolder m_locker_holder;
  v8x::SharedIsolateLockerPtr m_exposed_locker;
  int m_exposed_locker_level;
  LockerLevelStack m_exposed_locker_levels;

 public:
  JSIsolate();
  ~JSIsolate();

  JSTracer& Tracer() const;
  JSHospital& Hospital() const;
  JSEternals& Eternals() const;

  static SharedJSIsolatePtr FromV8(v8::Isolate* v8_isolate);
  [[nodiscard]] v8x::LockedIsolatePtr ToV8();

  SharedJSStackTracePtr GetCurrentStackTrace(
      int frame_limit,
      v8::StackTrace::StackTraceOptions v8_options = v8::StackTrace::kOverview) const;

  static py::object GetCurrent();

  void Enter() const;
  void Leave() const;
  bool Locked() const;

  void Lock();
  void Unlock();
  void UnlockAll();
  void RelockAll();
  int LockLevel() const;

  py::object GetEnteredOrMicrotaskContext() const;
  py::object GetCurrentContext() const;
  py::bool_ InContext() const;
};

#endif