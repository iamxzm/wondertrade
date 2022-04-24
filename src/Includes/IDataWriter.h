/*!
 * \file IDataWriter.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief ������ؽӿڶ���
 */
#pragma once
#include <stdint.h>
#include "WTSTypes.h"
#include "FasterDefs.h"

NS_WTP_BEGIN
class WTSTickData;
class WTSOrdQueData;
class WTSOrdDtlData;
class WTSTransData;
class WTSVariant;
class IBaseDataMgr;
struct WTSBarStruct;
struct WTSTickStruct;
struct WTSOrdDtlStruct;
struct WTSOrdQueStruct;
struct WTSTransStruct;

class IDataWriterSink
{
public:
	
	virtual IBaseDataMgr* getBDMgr() = 0;

	virtual bool canSessionReceive(const char* sid) = 0;

	virtual void broadcastTick(WTSTickData* curTick) = 0;

	virtual void broadcastOrdQue(WTSOrdQueData* curOrdQue) = 0;

	virtual void broadcastOrdDtl(WTSOrdDtlData* curOrdDtl) = 0;

	virtual void broadcastTrans(WTSTransData* curTrans) = 0;

	virtual CodeSet* getSessionComms(const char* sid) = 0;

	virtual uint32_t getTradingDate(const char* pid) = 0;

	/*
	*	�������ģ�����־
	*	@ll			��־����
	*	@message	��־����
	*/
	virtual void outputLog(WTSLogLevel ll, const char* message) = 0;
};

class IHisDataDumper
{
public:
	virtual bool dumpHisBars(const char* stdCode, const char* period, WTSBarStruct* bars, uint32_t count) = 0;
	virtual bool dumpHisTicks(const char* stdCode, uint32_t uDate, WTSTickStruct* ticks, uint32_t count) = 0;

	virtual bool dumpHisOrdQue(const char* stdCode, uint32_t uDate, WTSOrdQueStruct* items, uint32_t count) { return false; }
	virtual bool dumpHisOrdDtl(const char* stdCode, uint32_t uDate, WTSOrdDtlStruct* items, uint32_t count) { return false; }
	virtual bool dumpHisTrans(const char* stdCode, uint32_t uDate, WTSTransStruct* items, uint32_t count) { return false; }
};

typedef faster_hashmap<ShortKey, IHisDataDumper*> ExtDumpers;

/*
 *	������ؽӿ�
 */
class IDataWriter
{
public:
	IDataWriter():_sink(NULL){}

	virtual bool init(WTSVariant* params, IDataWriterSink* sink) { _sink = sink; return true; }

	virtual void release() = 0;

	void	add_ext_dumper(const char* id, IHisDataDumper* dumper) { _dumpers[id] = dumper; }

public:
	virtual bool writeTick(WTSTickData* curTick, uint32_t procFlag) = 0;

	virtual bool writeOrderQueue(WTSOrdQueData* curOrdQue) { return false; }

	virtual bool writeOrderDetail(WTSOrdDtlData* curOrdDetail) { return false; }

	virtual bool writeTransaction(WTSTransData* curTrans) { return false; }

	virtual void transHisData(const char* sid) {}

	virtual bool isSessionProceeded(const char* sid) { return true; }

	virtual WTSTickData* getCurTick(const char* code, const char* exchg = "") = 0;

protected:
	ExtDumpers			_dumpers;
	IDataWriterSink*	_sink;
};

NS_WTP_END


//��ȡIDataWriter�ĺ���ָ������
typedef wtp::IDataWriter* (*FuncCreateWriter)();
typedef void(*FuncDeleteWriter)(wtp::IDataWriter* &writer);