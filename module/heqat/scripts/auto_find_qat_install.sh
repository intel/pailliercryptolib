#!/bin/bash

# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

# shellcheck disable=SC2143
for item in $(locate QAT/build); do [ -d "$item" ] && [ "$(echo "$item" | grep "$HOME")" ] && echo "${item%/*}"; done
