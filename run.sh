#!/usr/bin/env bash
export ICP_ROOT=$HOME/QAT
export LD_LIBRARY_PATH=$PWD/install/lib:$ICP_ROOT/build:$LD_LIBRARY_PATH
pushd ./install/bin
./qat_modexp_validation
popd 
