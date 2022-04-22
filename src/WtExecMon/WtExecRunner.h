#pragma once

#include "WtSimpDataMgr.h"

#include "../WtCore/WtExecMgr.h"
#include "../WtCore/TraderAdapter.h"
#include "../WtCore/ParserAdapter.h"
#include "../WtCore/ActionPolicyMgr.h"

#include "../WTSTools/WTSHotMgr.h"
#include "../WTSTools/WTSBaseDataMgr.h"

NS_OTP_BEGIN
class WTSVariant;
NS_OTP_END

USING_NS_OTP;

class WtExecRunner : public IParserStub, public IExecuterStub
{
public:
	WtExecRunner();

	/*
	 *	��ʼ��
	 */
	bool init(const char* logCfg = "logcfgexec.json", bool isFile = true);

	bool config(const char* cfgFile, bool isFile = true);

	void run(bool bAsync = false);

	void release();

	void setPosition(const char* stdCode, double targetPos);

	bool addExeFactories(const char* folder);

	IBaseDataMgr*	get_bd_mgr() { return &_bd_mgr; }

	IHotMgr* get_hot_mgr() { return &_hot_mgr; }

	WTSSessionInfo* get_session_info(const char* sid, bool isCode = true);

	//////////////////////////////////////////////////////////////////////////
	/// <summary>
	/// ����ʵʱ��������
	/// </summary>
	/// <param name="curTick">���µ�tick����</param>
	/// <param name="isHot">�Ƿ���������Լ����</param>
	virtual void handle_push_quote(WTSTickData* curTick, uint32_t hotFlag = 0) override;

	///////////////////////////////////////////////////////////////////////////
	//IExecuterStub �ӿ�
	virtual uint64_t get_real_time() override;
	virtual WTSCommodityInfo* get_comm_info(const char* stdCode) override;
	virtual WTSSessionInfo* get_sess_info(const char* stdCode) override;
	virtual IHotMgr* get_hot_mon() override { return &_hot_mgr; }
	virtual uint32_t get_trading_day() override;

private:
	bool initTraders(WTSVariant* cfgTrader);
	bool initParsers(WTSVariant* cfgParser);
	bool initExecuters();
	bool initDataMgr();
	bool initActionPolicy();

private:
	TraderAdapterMgr	_traders;
	ParserAdapterMgr	_parsers;
	WtExecuterFactory	_exe_factory;
	WtExecuterMgr		_exe_mgr;

	WTSVariant*			_config;

	WtSimpDataMgr		_data_mgr;

	WTSBaseDataMgr		_bd_mgr;
	WTSHotMgr			_hot_mgr;
	ActionPolicyMgr		_act_policy;
};

