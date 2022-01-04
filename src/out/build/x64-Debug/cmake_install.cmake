# Install script for directory: D:/work/lsqt_trade_core_v2/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "D:/work/lsqt_trade_core_v2/src/out/install/x64-Debug")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WTSUtils/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WTSTools/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WtCtaStraFact/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/CTPLoader/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/CTPOptLoader/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/MiniLoader/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/ParserCTP/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/ParserCTPMini/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/ParserCTPOpt/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/ParserFemas/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/ParseriTap/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/ParserXTP/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WtDataWriter/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WtDtCore/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WtDtHelper/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WtDtPorter/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WtDtServo/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/QuoteFactory/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WtBtCore/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WtBtPorter/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WtBtRunner/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/ParserUDP/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/TraderCTP/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/TraderCTPMini/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/TraderCTPOpt/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/TraderFemas/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/TraderMocker/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/TraderXTP/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/TraderiTap/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WtExeFact/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WtRiskMonFact/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WtDataReader/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WtMsgQue/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WtCore/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WtPorter/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WtExecMon/cmake_install.cmake")
  include("D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/WtRunner/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "D:/work/lsqt_trade_core_v2/src/out/build/x64-Debug/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
