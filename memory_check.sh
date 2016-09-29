#!/bin/bash
#

make -C src
valgrind --leak-check=yes --trace-children=yes --track-origins=yes ./src/hs_leader ./test/test.txt
