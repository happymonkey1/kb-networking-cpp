#!/usr/bin/env bash

cmake --build ${KB_NETWORKING_ROOT}/build --config Release -j --clean-first --target kb-networking kb-networking-cli
