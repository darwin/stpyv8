#!/usr/bin/env python

import os
import shutil

# noinspection PyUnresolvedReferences
from distutils.command.build_ext import build_ext
from distutils.command.install import install

try:
    from setuptools import setup, Extension
except ImportError:
    from distutils.core import setup, Extension

NAGA_V8_GIT_TAG = os.environ.get('NAGA_V8_GIT_TAG')
if NAGA_V8_GIT_TAG is None:
    raise Exception("NAGA_V8_GIT_TAG is not defined in your environment")

NAGA_VERSION = NAGA_V8_GIT_TAG

NAGA_GN_OUT_DIR = os.environ.get('NAGA_GN_OUT_DIR')
if NAGA_GN_OUT_DIR is None:
    raise Exception("NAGA_GN_OUT_DIR is not defined in your environment")


# here we override standard Extension build,
# to simply check for existence and copy existing pre-built binary
class BuildExtCmd(build_ext):

    def get_input_path(self):
        build_config = "debug" if self.debug else "release"
        return os.path.join(NAGA_GN_OUT_DIR, NAGA_VERSION, build_config, "libnaga.so")

    def get_output_path(self):
        name = self.extensions[0].name
        return self.get_ext_fullpath(name)

    def build_extension(self, ext):
        # this is intentionally a no-op
        return

    def run(self):
        # the extension has only name and should be empty so nothing should be happening here
        # anyways, we let normal build_ext run and then finish the work ourselves
        build_ext.run(self)

        input_binary = self.get_input_path()
        output_binary = self.get_output_path()

        if not os.path.isfile(input_binary):
            msg = "Expected pre-compiled file '{}' does not exists. Follow build steps.".format(input_binary)
            raise Exception(msg)

        output_parent_dir = os.path.abspath(os.path.join(output_binary, os.pardir))
        print(output_parent_dir)
        os.makedirs(output_parent_dir, exist_ok=True)
        assert (not os.path.isdir(output_binary))
        shutil.copy2(input_binary, output_binary)
        print("copied '{}' to '{}'\n".format(input_binary, output_binary))


class InstallCmd(install):
    skip_build = False

    def run(self):
        # TODO: why is this here? explain
        self.skip_build = True
        install.run(self)


naga_ext = Extension(name="naga_native", sources=[])
naga_packages = ["naga", "naga.aux", "naga.toolkit"]

setup(name="naga",
      version=NAGA_VERSION,
      description="Python Wrapper for Google V8 Engine",
      platforms="x86",
      author="Flier Lu, Philip Syme, Angelo Dell'Aera, Antonin Hildebrand",
      url="https://github.com/darwin/naga",
      license="Apache License 2.0",
      packages=naga_packages,
      ext_modules=[naga_ext],
      classifiers=[
          "Development Status :: 4 - Beta",
          "Environment :: Plugins",
          "Intended Audience :: Developers",
          "Intended Audience :: System Administrators",
          "License :: OSI Approved :: Apache Software License",
          "Natural Language :: English",
          "Operating System :: POSIX",
          "Programming Language :: C++",
          "Programming Language :: Python",
          "Topic :: Internet",
          "Topic :: Internet :: WWW/HTTP",
          "Topic :: Software Development",
          "Topic :: Software Development :: Libraries :: Python Modules",
          "Topic :: Utilities",
      ],
      cmdclass=dict(
          build_ext=BuildExtCmd,
          install=InstallCmd),
      )
