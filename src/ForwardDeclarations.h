#pragma once

#include "Base.h"

class CJSPlatform;
class CJSContext;
class CJSEngine;
class CJSIsolate;
class CJSScript;
class CJSStackTrace;
class CJSStackFrame;
class CJSException;
class CTracer;
class CJSHospital;
class CJSEternals;
class CJSObject;
class CJSObjectBase;
class CJSObjectGenericImpl;
class CJSObjectFunctionImpl;
class CJSObjectArrayImpl;
class CJSObjectCLJSImpl;
class CJSObjectAPI;

typedef std::shared_ptr<CJSContext> CContextPtr;
typedef std::shared_ptr<CJSIsolate> CIsolatePtr;
typedef std::shared_ptr<CJSScript> CScriptPtr;
typedef std::shared_ptr<CJSStackTrace> CJSStackTracePtr;
typedef std::shared_ptr<CJSStackFrame> CJSStackFramePtr;
typedef std::shared_ptr<CJSException> CJSExceptionPtr;
typedef std::shared_ptr<CJSObject> CJSObjectPtr;

namespace v8 {

typedef gsl::not_null<v8::Isolate*> IsolateRef;

}

namespace spdlog {

class logger;

}

typedef gsl::not_null<spdlog::logger*> LoggerRef;