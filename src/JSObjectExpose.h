#pragma once

#include "Base.h"
#include "JSObjectBase.h"
#include "JSObjectGenericImpl.h"
#include "JSObjectFunctionImpl.h"
#include "JSObjectArrayImpl.h"
#include "JSObjectCLJSImpl.h"

void exposeJSObject(py::module py_module);