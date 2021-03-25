#!/bin/bash

VERSION=`git rev-parse --short=8 HEAD`

cat - > ./src/main/version.c <<EOF
#include <stdint.h>
const uint32_t APP_VERSION = 0x${VERSION};
EOF


