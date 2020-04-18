#pragma once

#include "Base.h"
#include "JSObject.h"

class CJSObjectArray : public CJSObject {
 public:
  class ArrayIterator {
    CJSObjectArray* m_array_ptr;
    size_t m_idx;

   public:
    ArrayIterator(CJSObjectArray* array, size_t idx) : m_array_ptr(array), m_idx(idx) {}

    void increment() { m_idx++; }
    [[nodiscard]] bool equal(ArrayIterator const& other) const {
      return m_array_ptr == other.m_array_ptr && m_idx == other.m_idx;
    }
    [[nodiscard]] py::object dereference() const { return m_array_ptr->GetItem(py::int_(m_idx)); }
  };

  explicit CJSObjectArray(v8::Local<v8::Array> v8_array);

  size_t Length();

  py::object GetItem(py::object py_key);
  py::object SetItem(py::object py_key, py::object py_value);
  py::object DelItem(py::object py_key);
  bool Contains(const py::object& py_key);

  ArrayIterator begin() { return {this, 0}; }
  ArrayIterator end() { return {this, Length()}; }
};