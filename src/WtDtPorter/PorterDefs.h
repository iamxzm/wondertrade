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
//��չParser�ص�����
static const WtUInt32	EVENT_PARSER_INIT = 1;	//Parser��ʼ��
static const WtUInt32	EVENT_PARSER_CONNECT = 2;	//Parser����
static const WtUInt32	EVENT_PARSER_DISCONNECT = 3;	//Parser�Ͽ�����
static const WtUInt32	EVENT_PARSER_RELEASE = 4;	//Parser�ͷ�

typedef void(PORTER_FLAG *FuncParserEvtCallback)(WtUInt32 evtId, const char* id);
typedef void(PORTER_FLAG *FuncParserSubCallback)(const char* id, const char* fullCode, bool isForSub);
