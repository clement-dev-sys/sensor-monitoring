#!/bin/bash

. ~/.espressif/v6.1/esp-idf/export.sh
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor