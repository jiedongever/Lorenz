#!/bin/bash

./bin/gen -deploy Lorenz.cpp Lorenz.dsg.cpp
./bin/skel -i Lorenz.cpp -s Lorenz.dsg.cpp
