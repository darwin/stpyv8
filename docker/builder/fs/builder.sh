#!/usr/bin/env bash

set -e -o pipefail

export NAGA_WORK_DIR="/naga-work"
export NAGA_LDFLAGS="-L/usr/lib -L/usr/lib/x86_64-linux-gnu"
export NAGA_INCLUDES="-I/usr/include -I/usr/include/x86_64-linux-gnu"
export NAGA_EXAMPLES_EXPECTATIONS_DIR=
export NAGA_EXAMPLES_RESULTS_DIR=

cd "/naga"

if [[ "$1" == "build" ]]; then
  shift
  set -x
  ./scripts/prepare-deps.sh
  ./scripts/prepare-v8.sh
  ./scripts/build.sh "$@"
elif [[ "$1" == "test" ]]; then
  shift
  ./scripts/test.sh "$@"
elif [[ "$1" == "test-examples" ]]; then
  shift
  ./scripts/test-examples.sh "$@"
elif [[ "$1" == "enter" ]]; then
  shift
  "$@"
else
  echo "don't know what to do"
  echo "got: $*"
fi
