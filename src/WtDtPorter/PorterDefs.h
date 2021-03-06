/*!
 * \file PorterDefs.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#pragma once

#include <stdint.h>
#include "../Includes/WTSTypes.h"

NS_OTP_BEGIN
struct WTSTickStruct;
NS_OTP_END

USING_NS_OTP;

#ifdef _WIN32
#	define PORTER_FLAG _cdecl
#else
#	define PORTER_FLAG __attribute__((_cdecl))
#endif

typedef unsigned long		WtUInt32;
typedef unsigned long long	WtUInt64;
typedef const char*			WtString;

//////////////////////////////////////////////////////////////////////////
//扩展Parser回调函数
static const WtUInt32	EVENT_PARSER_INIT = 1;	//Parser初始化
static const WtUInt32	EVENT_PARSER_CONNECT = 2;	//Parser连接
static const WtUInt32	EVENT_PARSER_DISCONNECT = 3;	//Parser断开连接
static const WtUInt32	EVENT_PARSER_RELEASE = 4;	//Parser释放

typedef void(PORTER_FLAG *FuncParserEvtCallback)(WtUInt32 evtId, const char* id);
typedef void(PORTER_FLAG *FuncParserSubCallback)(const char* id, const char* fullCode, bool isForSub);
