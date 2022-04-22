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

NS_WTP_BEGIN
struct WTSTickStruct;
struct WTSBarStruct;
struct WTSOrdDtlStruct;
struct WTSOrdQueStruct;
struct WTSTransStruct;
NS_WTP_END

USING_NS_WTP;

//////////////////////////////////////////////////////////////////////////
//��չParser�ص�����
static const WtUInt32	EVENT_PARSER_INIT = 1;	//Parser��ʼ��
static const WtUInt32	EVENT_PARSER_CONNECT = 2;	//Parser����
static const WtUInt32	EVENT_PARSER_DISCONNECT = 3;	//Parser�Ͽ�����
static const WtUInt32	EVENT_PARSER_RELEASE = 4;	//Parser�ͷ�

typedef void(PORTER_FLAG *FuncParserEvtCallback)(WtUInt32 evtId, const char* id);
typedef void(PORTER_FLAG *FuncParserSubCallback)(const char* id, const char* fullCode, bool isForSub);


//////////////////////////////////////////////////////////////////////////
//��չDumper�ص�����
typedef bool(PORTER_FLAG *FuncDumpBars)(const char* id, const char* stdCode, const char* period, WTSBarStruct* bars, WtUInt32 count);
typedef bool(PORTER_FLAG *FuncDumpTicks)(const char* id, const char* stdCode, WtUInt32 uDate, WTSTickStruct* ticks, WtUInt32 count);
typedef bool(PORTER_FLAG *FuncDumpOrdQue)(const char* id, const char* stdCode, WtUInt32 uDate, WTSOrdQueStruct* items, WtUInt32 count);
typedef bool(PORTER_FLAG *FuncDumpOrdDtl)(const char* id, const char* stdCode, WtUInt32 uDate, WTSOrdDtlStruct* items, WtUInt32 count);
typedef bool(PORTER_FLAG *FuncDumpTrans)(const char* id, const char* stdCode, WtUInt32 uDate, WTSTransStruct* items, WtUInt32 count);
