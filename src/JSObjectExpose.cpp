#include "JSObject.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

void exposeJSObject(py::module py_module) {
  TRACE("exposeJSObject py_module={}", py_module);
  // clang-format off
  py::class_<CJSObject, CJSObjectPtr>(py_module, "JSObject")
      .def("__getattr__", &CJSObjectAPI::PythonGetAttr)
      .def("__setattr__", &CJSObjectAPI::PythonSetAttr)
      .def("__delattr__", &CJSObjectAPI::PythonDelAttr)

      .def("__hash__", &CJSObjectAPI::PythonIdentityHash)
      // TODO: I'm not sure about this, revisit
      .def("clone", &CJSObjectAPI::PythonClone,
           "Clone the object.")
      .def("__dir__", &CJSObjectAPI::PythonGetAttrList)

      // Emulating dict object
      // TODO: I'm not sure about this, revisit
      .def("keys", &CJSObjectAPI::PythonGetAttrList,
           "Get a list of the object attributes.")

      .def("__getitem__", &CJSObjectAPI::PythonGetItem)
      .def("__setitem__", &CJSObjectAPI::PythonSetItem)
      .def("__delitem__", &CJSObjectAPI::PythonDelItem)

      .def("__contains__", &CJSObjectAPI::PythonContains)
      .def("__len__", &CJSObjectAPI::PythonLength)

      .def("__int__", &CJSObjectAPI::PythonInt)
      .def("__float__", &CJSObjectAPI::PythonFloat)
      .def("__str__", &CJSObjectAPI::PythonStr)
      .def("__repr__", &CJSObjectAPI::PythonRepr)
      .def("__bool__", &CJSObjectAPI::PythonBool)

      .def("__eq__", &CJSObjectAPI::PythonEquals)
      .def("__ne__", &CJSObjectAPI::PythonNotEquals)

      .def_static("create", &CJSObjectAPI::PythonCreateWithArgs,
                  py::arg("constructor"),
                  py::arg("arguments") = py::tuple(),
                  py::arg("propertiesObject") = py::dict(),
                  "Creates a new object with the specified prototype object and properties.")

      .def_static("hasJSArrayRole", [](const CJSObjectPtr &obj) {
          return obj->HasRole(CJSObjectBase::Roles::JSArray);
      })
      .def_static("hasJSFunctionRole", [](const CJSObjectPtr &obj) {
          return obj->HasRole(CJSObjectBase::Roles::JSFunction);
      })
      .def_static("hasCLJSObjectRole", [](const CJSObjectPtr &obj) {
          return obj->HasRole(CJSObjectBase::Roles::CLJSObject);
      })

      .def("__call__", &CJSObjectAPI::PythonCallWithArgs)
      .def("apply", &CJSObjectAPI::PythonApply,
           py::arg("self"),
           py::arg("args") = py::list(),
           py::arg("kwds") = py::dict(),
           "Performs a function call using the parameters.")
      .def("invoke", &CJSObjectAPI::PythonInvoke,
           py::arg("args") = py::list(),
           py::arg("kwds") = py::dict(),
           "Performs a binding method call using the parameters.")

// TODO: revisit this, there is a clash with normal attribute lookups
//      .def("setName", &CJSObjectAPI::PythonSetName)
//
//      .def_property("name", &CJSObjectAPI::PythonGetName, &CJSObjectAPI::PythonSetName,
//                    "The name of function")

      .def_property_readonly("linenum", &CJSObjectAPI::PythonLineNumber,
                             "The line number of function in the script")
      .def_property_readonly("colnum", &CJSObjectAPI::PythonColumnNumber,
                             "The column number of function in the script")
      .def_property_readonly("resname", &CJSObjectAPI::PythonResourceName,
                             "The resource name of script")
      .def_property_readonly("inferredname", &CJSObjectAPI::PythonInferredName,
                             "Name inferred from variable or property assignment of this function")
      .def_property_readonly("lineoff", &CJSObjectAPI::PythonLineOffset,
                             "The line offset of function in the script")
      .def_property_readonly("coloff", &CJSObjectAPI::PythonColumnOffset,
                             "The column offset of function in the script")

    // TODO:      .def("__iter__", &CJSObjectArray::begin, &CJSObjectArray::end)
      ;
  // clang-format on
}