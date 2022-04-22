#!/bin/bash

cd ./mongo-c-driver-1.20.0
cd build
cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..
make && make install
cd ..
cd ..

