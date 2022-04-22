#!/bin/bash

cd ./mongo-cxx-driver-r3.6.6/
cd  ./build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
make && make install
cd ..
cd ..
