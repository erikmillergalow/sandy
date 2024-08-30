#!/bin/bash

set -x

rm sandy
gcc main.c -o sandy -Iinclude -lGL -ldl -lm -lX11 -lasound -lXi -lXcursor
./sandy
