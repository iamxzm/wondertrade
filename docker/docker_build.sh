#!/bin/bash

#! 创建一个镜像
docker build -t docker-cpp -f Dockerfile --no-cache .

#! 查询docker id
# docker images
#! 创建一个容器
# docker container run -it -d  --network=bridge --shm-size 4G --label lsqt-c-strategy-container --name=docker-cpp 258feeba0e43 /bin/bash
#! docker create --name cpp_env1 --label lsqt-c-strategy-container docker-cpp

