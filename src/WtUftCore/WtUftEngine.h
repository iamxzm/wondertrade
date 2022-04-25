/*!
 * \file WtHftEngine.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#pragma once

#include <queue>
#include <functional>
#include <stdint.h>

#include "ParserAdapter.h"

#include "../Includes/FasterDefs.h"
#include "../Includes/RiskMonDefs.h"

#include "../Share/StdUtils.hpp"
#include "../Share/DLLHelper.hpp"

#include "../Share/BoostFile.hpp"

#include "../Includes/IUftStraCtx.h"

NS_WTP_BEGIN
class WTSSessionInfo;
class WTSCommodityInfo;
class WTSContractInfo;

class IBaseDataMgr;
class IHotMgr;

class WTSVariant;

class WTSTickData;
struct WTSBarStruct;
class WTSTickSlice;
class WTSKlineSlice;
class WTSPortFundInfo;

class WtUftDtMgr;
class TraderAdapterMgr;

class EventNotifier;

typedef std::function<void()>	TaskItem;


class WTSVariant;
class WtUftRtTicker;

typedef std::shared_ptr<IUftStraCtx> UftContextPtr;

class WtUftEngine : public IParserStub
{
public:
	WtUftEngine();
	virtual ~WtUftEngine();

public:
	inline void set_adapter_mgr(TraderAdapterMgr* mgr) { _adapter_mgr = mgr; }

	void set_date_time(uint32_t curDate, uint32_t curTime, uint32_t curSecs = 0, uint32_t rawTime = 0);

	void set_trading_date(uint32_t curTDate);

	inline uint32_t get_date() { return _cur_date; }
	inline uint32_t get_min_time() { return _cur_time; }
	inline uint32_t get_raw_time() { return _cur_raw_time; }
	inline uint32_t get_secs() { return _cur_secs; }
	inline uint32_t get_trading_date() { return _cur_tdate; }

	inline IBaseDataMgr*		get_basedata_mgr() { return _base_data_mgr; }
	WTSSessionInfo*		get_session_info(const char* sid, bool isCode = false);
	WTSCommodityInfo*	get_commodity_info(const char* stdCode);
	WTSContractInfo*	get_contract_info(const char* stdCode);

	WTSTickData*	get_last_tick(uint32_t sid, const char* stdCode);
	WTSTickSlice*	get_tick_slice(uint32_t sid, const char* stdCode, uint32_t count);
	WTSKlineSlice*	get_kline_slice(uint32_t sid, const char* stdCode, const char* period, uint32_t count, uint32_t times = 1, uint64_t etime = 0);

	void sub_tick(uint32_t sid, const char* code);

	double get_cur_price(const char* stdCode);

public:
	void init(WTSVariant* cfg, IBaseDataMgr* bdMgr, WtUftDtMgr* dataMgr);

	void run(bool bAsync = false);

	void on_tick(const char* stdCode, WTSTickData* curTick);

	void on_bar(const char* stdCode, const char* period, uint32_t times, WTSBarStruct* newBar);

	void on_init(){}

	void on_session_begin();

	void on_session_end();

	virtual void handle_push_quote(WTSTickData* newTick) override;
	virtual void handle_push_order_detail(WTSOrdDtlData* curOrdDtl) override;
	virtual void handle_push_order_queue(WTSOrdQueData* curOrdQue) override;
	virtual void handle_push_transaction(WTSTransData* curTrans) override;

public:
	WTSOrdQueSlice* get_order_queue_slice(uint32_t sid, const char* stdCode, uint32_t count);
	WTSOrdDtlSlice* get_order_detail_slice(uint32_t sid, const char* stdCode, uint32_t count);
	WTSTransSlice* get_transaction_slice(uint32_t sid, const char* stdCode, uint32_t count);

public:
	void on_minute_end(uint32_t curDate, uint32_t curTime);

	void addContext(UftContextPtr ctx);

	UftContextPtr	getContext(uint32_t id);

	void sub_order_queue(uint32_t sid, const char* stdCode);
	void sub_order_detail(uint32_t sid, const char* stdCode);
	void sub_transaction(uint32_t sid, const char* stdCode);

private:
	uint32_t		_cur_date;	//��ǰ����
	uint32_t		_cur_time;		//��ǰʱ��, ��1������ʱ��, ����0900, ���ʱ���1��������0901, _cur_timeҲ����0901, �����Ϊ��CTA���淽��
	uint32_t		_cur_raw_time;	//��ǰ��ʵʱ��
	uint32_t		_cur_secs;	//��ǰ����, ��������
	uint32_t		_cur_tdate;	//��ǰ������

	IBaseDataMgr*	_base_data_mgr;	//�������ݹ�����
	WtUftDtMgr*		_data_mgr;		//���ݹ�����

	//By Wesley @ 2022.02.07
	//tick���ݶ����first��contextid��second�Ƕ���ѡ�0-ԭʼ���ģ�1-ǰ��Ȩ��2-��Ȩ
	typedef faster_hashset<uint32_t> SubList;
	typedef faster_hashmap<LongKey, SubList>	StraSubMap;
	StraSubMap		_tick_sub_map;	//tick���ݶ��ı�
	StraSubMap		_ordque_sub_map;	//ί�ж��ж��ı�
	StraSubMap		_orddtl_sub_map;	//ί����ϸ���ı�
	StraSubMap		_trans_sub_map;		//�ɽ���ϸ���ı�
	StraSubMap		_bar_sub_map;	//K�����ݶ��ı�	

	TraderAdapterMgr*	_adapter_mgr;

	typedef faster_hashmap<uint32_t, UftContextPtr> ContextMap;
	ContextMap		_ctx_map;

	WtUftRtTicker*	_tm_ticker;
	WTSVariant*		_cfg;	
};

NS_WTP_END