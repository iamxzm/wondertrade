#!/bin/bash

cd /root/lsqt_trade_core_v2/src/BackTest
#当前目录
name=$(ls)
echo 'current directory is:   ' $(pwd)
echo 'has file:'$name

#将策略代码文件内容移动到backtest.cpp中
for n in $name
do
   if [[ $n =~ ^([0-9]+) ]] && [[ $n =~ ".cpp"$ ]];then
      echo '策略文件名称为----------'$n
      mv -f $n backtest.cpp
      break
   fi

done

# 编译
cd ../
./build_debug.sh
#执行
cd ./build_Debug/bin/WtTester
./BackTest >>nohup.out
