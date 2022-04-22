
#1. 确定CMake的最低版本需求
CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)

#2. 确定工程名
PROJECT(TraderCTP LANGUAGES CXX)
SET(CMAKE_CXX_STANDARD 11)


IF (WIN32)
	IF("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
		SET(INCS $ENV{MyDepends141}/include)
		SET(LNKS $ENV{MyDepends141}/lib/x64)
	ELSE()
		SET(INCS $ENV{MyDepends141}/include)
		SET(LNKS $ENV{MyDepends141}/lib/x86)
	ENDIF()
ELSE (UNIX)
	SET (MyDeps "/home/mydeps")
	SET(INCS ${MyDeps}/include)
	SET(LNKS ${MyDeps}/lib)
	SET(INC_1 "/usr/local/include/bsoncxx")
	SET(INC_2 "/usr/local/include/bsoncxx/v_noabi")
	SET(INC_3 "/usr/local/include/mongocxx")
	SET(INC_4 "/usr/local/include/mongocxx/v_noabi")
	SET(MONGOCXX "/usr/local//lib")
	SET(BSONCXX "/usr/local/lib")
ENDIF ()

SET(SRC  
	${PROJECT_SOURCE_DIR}/TraderCTP.cpp
	${PROJECT_SOURCE_DIR}/TraderCTP.h
)

SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../build_${CMAKE_BUILD_TYPE}/bin)

INCLUDE_DIRECTORIES(${INCS} ${INC_1} ${INC_2} ${INC_3} ${INC_4})
LINK_DIRECTORIES(${LNKS} ${MONGOCXX} ${BSONCXX})
ADD_LIBRARY(TraderCTP SHARED ${SRC})

SET(LIBS
	boost_thread
	boost_filesystem
	mongocxx
	bsoncxx
)
TARGET_LINK_LIBRARIES(TraderCTP ${LIBS})

IF (WIN32)
ELSE (UNIX)
	SET_TARGET_PROPERTIES(TraderCTP PROPERTIES
		CXX_VISIBILITY_PRESET hidden
		C_VISIBILITY_PRESET hidden
		VISIBILITY_INLINES_HIDDEN 1)
ENDIF ()

