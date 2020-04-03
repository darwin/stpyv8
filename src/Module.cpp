#include "Base.h"
#include "Context.h"
#include "Engine.h"
#include "Exception.h"
#include "Locker.h"
#include "JSObject.h"

BOOST_PYTHON_MODULE(_STPyV8) {
  CJavascriptException::Expose();
  CJSObject::Expose();
  CContext::Expose();
  CEngine::Expose();
  CLocker::Expose();
}
