#!/bin/bash

docker run -it --rm --name esp32 -v $PWD/src:/home/esp32/project registry.hottis.de/dockerized/build-env-esp32:4.3.0 bash

