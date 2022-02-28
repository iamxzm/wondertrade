from wondertrader/base


WORKDIR /root/

RUN chmod 777 /tmp
RUN apt update &&  apt install --reinstall -y ca-certificates && rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get install -y zlib1g-dev
RUN apt-get update && apt-get install -y cmake libssl-dev libsasl2-dev

COPY ./mongo-c-driver-1.20.0 ./mongo-c-driver-1.20.0
COPY ./mongo-cxx-driver-r3.6.6 ./mongo-cxx-driver-r3.6.6
COPY ./install_mongo.sh ./install_mongo.sh
COPY ./install_mongocxx.sh ./install_mongocxx.sh
COPY ./mydeps /home/mydeps
RUN ./install_mongo.sh
RUN ./install_mongocxx.sh

RUN git clone http://xiongzhimin:xiongzhimin@192.168.214.112:7070/bitbucket/scm/lhjy/lsqt_trade_core_v2.git
