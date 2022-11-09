#!/bin/bash

# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

export HEQATLIB_INSTALL_DIR=$HEQATLIB_ROOT_DIR/install
ICP_ROOT=$("$HEQATLIB_ROOT_DIR"/scripts/auto_find_qat_install.sh)
export ICP_ROOT
export LD_LIBRARY_PATH=$HEQATLIB_INSTALL_DIR/lib:$ICP_ROOT/build:$LD_LIBRARY_PATH

pushd "$HEQATLIB_INSTALL_DIR"/bin || exit

for app in test_*; do
  echo "*****************************************************************"
  echo "* [START]            RUNNING TEST SAMPLE $app                   *"
  echo "*****************************************************************"
  ./"$app"
  echo "*****************************************************************"
  echo "* [STOP]             RUNNING TEST SAMPLE $app                   *"
  echo "*****************************************************************"
done

popd || exit
