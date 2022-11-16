#!/bin/bash

HEQATLIB_ROOT_DIR=$(pwd)
export HEQATLIB_ROOT_DIR
ICP_ROOT=$("$PWD"/scripts/auto_find_qat_install.sh)
export ICP_ROOT
