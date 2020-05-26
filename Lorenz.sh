#!/bin/bash

./bin/gen -deploy many-tasks.cpp many-tasks.dsg.cpp
./bin/skel -i many-tasks.cpp -s many-tasks.dsg.cpp
