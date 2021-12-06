/*!
 * \file DataManager.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief ���ݹ���������
 */
#pragma once

#include "../Includes/IDataWriter.h"
#include "../Share/StdUtils.hpp"
#include "../Share/BoostMappingFile.hpp"

NS_OTP_BEGIN
class WTSTickData;
class WTSOrdQueData;
class WTSOrdDtlData;
class WTSTransData;
class WTSVariant;
NS_OTP_END

USING_NS_OTP;

class WTSBaseDataMgr;
class StateMonitor;
class UDPCaster;

class DataManager : public IDataWriterSink
{
public:
	DataManager();
	~DataManager();

public:
	bool init(WTSVariant* params, WTSBaseDataMgr* bdMgr, StateMonitor* stMonitor, UDPCaster* caster = NULL);
	void release();

	bool writeTick(WTSTickData* curTick, bool bNeedSlice = true);

	bool writeOrderQueue(WTSOrdQueData* curOrdQue);

	bool writeOrderDetail(WTSOrdDtlData* curOrdDetail);

	bool writeTransaction(WTSTransData* curTrans);

	void transHisData(const char* sid);
	
	bool isSessionProceeded(const char* sid);

	WTSTickData* getCurTick(const char* code, const char* exchg = "");

public:
	//////////////////////////////////////////////////////////////////////////
	//IDataWriterSink
	virtual IBaseDataMgr* getBDMgr() override;

	virtual bool canSessionReceive(const char* sid) override;

	virtual void broadcastTick(WTSTickData* curTick) override;

	virtual void broadcastOrdQue(WTSOrdQueData* curOrdQue) override;

	virtual void broadcastOrdDtl(WTSOrdDtlData* curOrdDtl) override;

	virtual void broadcastTrans(WTSTransData* curTrans) override;

	virtual CodeSet* getSessionComms(const char* sid) override;

	virtual uint32_t getTradingDate(const char* pid) override;

	/*
	*	�������ģ�����־
	*	@ll			��־����
	*	@message	��־����
	*/
	virtual void outputWriterLog(WTSLogLevel ll, const char* format, ...) override;

private:
	IDataWriter*		_writer;
	FuncDeleteWriter	_remover;
	WTSBaseDataMgr*		_bd_mgr;
	StateMonitor*		_state_mon;
	UDPCaster*			_udp_caster;
};

