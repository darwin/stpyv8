#pragma once

#include "Base.h"
#include "JSObject.h"

class CJavascriptUndefined : public CJSObject {
 public:
  bool nonzero() const { return false; }
  const std::string str() const { return "undefined"; }
};