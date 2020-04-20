#pragma once

#include "Base.h"

class CJSObjectArrayImpl {
 public:
  CJSObjectBase& m_base;

  size_t Length() const;
  py::object GetItem(const py::object& py_key) const;
  py::object SetItem(const py::object& py_key, const py::object& py_value) const;
  py::object DelItem(const py::object& py_key) const;
  bool Contains(const py::object& py_key) const;
};