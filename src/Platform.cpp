#include "Platform.h"

#define TRACE(...) (SPDLOG_LOGGER_TRACE(getLogger(kPlatformLogger), __VA_ARGS__))

std::unique_ptr<v8::Platform> CPlatform::m_v8_platform;
bool CPlatform::m_inited = false;

void CPlatform::Expose(const py::module& py_module) {
  TRACE("CPlatform::Expose py_module={}", py_module);
  // clang-format off
  py::class_<CPlatform, CPlatformPtr>(py_module, "JSPlatform", "JSPlatform allows the V8 platform to be initialized")
      .def(py::init<std::string>(),
           py::arg("argv") = std::string())
      .def("init", &CPlatform::Init,
           "Initializes the platform");
  // clang-format on
}

void CPlatform::Init() {
  TRACE("CPlatform::Init {}", THIS);
  if (m_inited) {
    TRACE("CPlatform::Init {} => [already inited]", THIS);
    return;
  }

  // https://v8.dev/docs/i18n#embedding-v8
  v8::V8::InitializeICU();
  v8::V8::InitializeExternalStartupData(m_argv.c_str());

  m_v8_platform = v8::platform::NewDefaultPlatform();

  v8::V8::InitializePlatform(m_v8_platform.get());
  v8::V8::Initialize();

  m_inited = true;
}

CPlatform::CPlatform(std::string argv) : m_argv(std::move(argv)) {
  TRACE("CPlatform::CPlatform {} argv={}", THIS, m_argv);
}
