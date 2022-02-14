/*!
 * \file HftMocker.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#pragma once
#include <queue>
#include <sstream>
#include <math.h>

#include "HisDataReplayer.h"

#include "../Includes/FasterDefs.h"
#include "../Includes/IHftStraCtx.h"
#include "../Includes/HftStrategyDefs.h"

#include "../Share/StdUtils.hpp"
#include "../Share/DLLHelper.hpp"

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/database.hpp>

class HisDataReplayer;

class HftMocker : public IDataSink, public IHftStraCtx
{
public:
	HftMocker(HisDataReplayer* replayer, const char* name);
	virtual ~HftMocker();

public:
	//////////////////////////////////////////////////////////////////////////
	//IDataSink
	virtual void	handle_tick(const char* stdCode, WTSTickData* curTick) override;
	virtual void	handle_order_queue(const char* stdCode, WTSOrdQueData* curOrdQue) override;
	virtual void	handle_order_detail(const char* stdCode, WTSOrdDtlData* curOrdDtl) override;
	virtual void	handle_transaction(const char* stdCode, WTSTransData* curTrans) override;

	virtual void	handle_bar_close(const char* stdCode, const char* period, uint32_t times, WTSBarStruct* newBar) override;
	virtual void	handle_schedule(uint32_t uDate, uint32_t uTime) override;

	virtual void	handle_init() override;
	virtual void	handle_session_begin(uint32_t curTDate) override;
	virtual void	handle_session_end(uint32_t curTDate) override;

	virtual void	handle_replay_done() override;

	virtual void	on_tick_updated(const char* stdCode, WTSTickData* newTick) override;
	virtual void	on_ordque_updated(const char* stdCode, WTSOrdQueData* newOrdQue) override;
	virtual void	on_orddtl_updated(const char* stdCode, WTSOrdDtlData* newOrdDtl) override;
	virtual void	on_trans_updated(const char* stdCode, WTSTransData* newTrans) override;

	//////////////////////////////////////////////////////////////////////////
	//IHftStraCtx
	virtual void on_tick(const char* stdCode, WTSTickData* newTick) override;

	virtual void on_order_queue(const char* stdCode, WTSOrdQueData* newOrdQue) override;

	virtual void on_order_detail(const char* stdCode, WTSOrdDtlData* newOrdDtl) override;

	virtual void on_transaction(const char* stdCode, WTSTransData* newTrans) override;

	virtual uint32_t id() override;

	virtual void on_init() override;

	virtual void on_bar(const char* stdCode, const char* period, uint32_t times, WTSBarStruct* newBar) override;

	virtual void on_session_begin(uint32_t curTDate) override;

	virtual void on_session_end(uint32_t curTDate) override;

	virtual bool stra_cancel(uint32_t localid) override;

	virtual OrderIDs stra_cancel(const char* stdCode, bool isBuy, double qty = 0) override;

	virtual OrderIDs stra_buy(const char* stdCode, double price, double qty, const char* userTag) override;

	virtual OrderIDs stra_sell(const char* stdCode, double price, double qty, const char* userTag) override;

	virtual WTSCommodityInfo* stra_get_comminfo(const char* stdCode) override;

	virtual WTSKlineSlice* stra_get_bars(const char* stdCode, const char* period, uint32_t count) override;

	virtual WTSTickSlice* stra_get_ticks(const char* stdCode, uint32_t count) override;

	virtual WTSOrdDtlSlice*	stra_get_order_detail(const char* stdCode, uint32_t count) override;

	virtual WTSOrdQueSlice*	stra_get_order_queue(const char* stdCode, uint32_t count) override;

	virtual WTSTransSlice*	stra_get_transaction(const char* stdCode, uint32_t count) override;

	virtual WTSTickData* stra_get_last_tick(const char* stdCode) override;

	virtual double stra_get_position(const char* stdCode) override;

	virtual double stra_get_position_profit(const char* stdCode) override;

	virtual double stra_get_undone(const char* stdCode) override;

	virtual double stra_get_price(const char* stdCode) override;

	virtual uint32_t stra_get_date() override;

	virtual uint32_t stra_get_time() override;

	virtual uint32_t stra_get_secs() override;

	virtual void stra_sub_ticks(const char* stdCode) override;

	virtual void stra_sub_order_queues(const char* stdCode) override;

	virtual void stra_sub_order_details(const char* stdCode) override;

	virtual void stra_sub_transactions(const char* stdCode) override;

	virtual void stra_log_info(const char* fmt, ...) override;
	virtual void stra_log_debug(const char* fmt, ...) override;
	virtual void stra_log_error(const char* fmt, ...) override;

	virtual void stra_save_user_data(const char* key, const char* val) override;

	virtual const char* stra_load_user_data(const char* key, const char* defVal = "") override;

	//////////////////////////////////////////////////////////////////////////
	virtual void on_trade(uint32_t localid, std::string instid,const char* stdCode, bool isBuy, double vol, double price, const char* userTag);

	virtual void on_order(uint32_t localid, const char* stdCode, bool isBuy, double totalQty, double leftQty, double price, bool isCanceled, const char* userTag);

	virtual void on_channel_ready();

	virtual void on_entrust(uint32_t localid, const char* stdCode, bool bSuccess, const char* message, const char* userTag);

public:
	bool	init_hft_factory(WTSVariant* cfg);
	void	install_hook();
	void	enable_hook(bool bEnabled = true);
	void	step_tick();
	void	set_initacc(double money);

private:
	typedef std::function<void()> Task;
	void	postTask(Task task);
	void	procTask();

	bool	procOrder(uint32_t localid, std::string instid);

	void	do_set_position(const char* stdCode, std::string instid, double qty, double price = 0.0, const char* userTag = "");
	void	update_dyn_profit(const char* stdCode, WTSTickData* newTick);

	void	dump_outputs();
	inline void	log_trade(const char* stdCode, bool isLong, bool isOpen, uint64_t curTime, double price, double qty, double fee, const char* userTag);
	inline void	log_close(const char* stdCode, bool isLong, uint64_t openTime, double openpx, uint64_t closeTime, double closepx, double qty,
		double profit, double maxprofit, double maxloss, double totalprofit, const char* enterTag, const char* exitTag);

	void set_dayaccount(const char* stdCode, WTSTickData* newTick, bool bEmitStrategy = true);

private:
	HisDataReplayer*	_replayer;

	double		init_money = 100000;			//初始资金
	double      _balance = 0;					//今总资产
	double		_total_money = init_money;		//剩余资金
	double		_static_balance = init_money;	//期初资产
	double		_close_price = 0;				//昨结算价
	double		_settlepx;						//今结算价
	double        _used_margin = 0;				//占用保证金
	double        _margin_rate = 0.5;			//保证金比例
	uint64_t		_cur_multiplier = 100;		//当前合约乘数

	std::string _per_strategy_id;

	double		_day_profit = 0;
	double		_total_profit = 0;				//策略收益
	double		_benchmark_rate_of_return = 0;	//基准收益率
	double		_benchmark_cumulative_rate = 0;
	double		_strategy_cumulative_rate = 0;	//策略累计收益率
	double		_daily_rate_of_return = 0;		//策略收益率
	double		_abnormal_rate_of_return = 0;	//日超额收益率
	int			_win_or_lose_flag;

	bool			_new_trade_day = true;
	bool		_dayacc_insert_flag = true;
	bool		_changepos = true;
	uint32_t    _traderday = 0;
	uint32_t	_pretraderday;
	uint32_t	_firstday = 0;
	double		_firstprice = 0;
	
	double		_total_closeprofit = 0;

	bool			_use_newpx;
	uint32_t		_error_rate;

	typedef faster_hashmap<std::string, double> PriceMap;
	PriceMap		_price_map;


	typedef struct _StraFactInfo
	{
		std::string		_module_path;
		DllHandle		_module_inst;
		IHftStrategyFact*	_fact;
		FuncCreateHftStraFact	_creator;
		FuncDeleteHftStraFact	_remover;

		_StraFactInfo()
		{
			_module_inst = NULL;
			_fact = NULL;
		}

		~_StraFactInfo()
		{
			if (_fact)
				_remover(_fact);
		}
	} StraFactInfo;
	StraFactInfo	_factory;

	HftStrategy*	_strategy;

	StdThreadPtr		_thrd;
	StdUniqueMutex		_mtx;
	std::queue<Task>	_tasks;
	bool				_stopped;

	StdRecurMutex		_mtx_control;

	typedef struct _OrderInfo
	{
		bool	_isBuy;
		char	_code[32];
		double	_price;
		double	_total;
		double	_left;
		char	_usertag[32];
		
		uint32_t	_localid;

		_OrderInfo()
		{
			memset(this, 0, sizeof(_OrderInfo));
		}

	} OrderInfo;
	typedef faster_hashmap<uint32_t, OrderInfo> Orders;
	StdRecurMutex	_mtx_ords;
	Orders			_orders;

	typedef WTSHashMap<std::string> CommodityMap;
	CommodityMap*	_commodities;

	//用户数据
	typedef faster_hashmap<std::string, std::string> StringHashMap;
	StringHashMap	_user_datas;
	bool			_ud_modified;

	typedef struct _DetailInfo
	{
		bool		_long;
		double		_price;
		double		_volume;
		uint64_t	_opentime;
		uint32_t	_opentdate;
		double		_max_profit;
		double		_max_loss;
		double		_profit;
		char		_usertag[32];
		double		_margin;

		_DetailInfo()
		{
			memset(this, 0, sizeof(_DetailInfo));
		}
	} DetailInfo;

	typedef struct _PosInfo
	{
		double		_volume;
		double		_closeprofit;
		double		_dynprofit;

		std::vector<DetailInfo> _details;

		_PosInfo()
		{
			_volume = 0;
			_closeprofit = 0;
			_dynprofit = 0;
		}
	} PosInfo;

	void insert_his_position(DetailInfo dInfo, PosInfo pInfo, double fee, 
		std::string exch_id, std::string inst_id, uint64_t curTime);
	void insert_his_trades(DetailInfo dInfo, PosInfo pInfo, double fee,
		std::string exch_id, std::string inst_id, uint64_t curTime, int offset);
	void insert_his_trade(DetailInfo dInfo, PosInfo pInfo, double fee,
		std::string exch_id, std::string inst_id, uint64_t curTime, int offset);
	typedef faster_hashmap<std::string, PosInfo> PositionMap;
	PositionMap		_pos_map;

	std::stringstream	_trade_logs;
	std::stringstream	_close_logs;
	std::stringstream	_fund_logs;
	std::stringstream	_sig_logs;

	typedef struct _StraFundInfo
	{
		double	_total_profit;
		double	_total_dynprofit;
		double	_total_fees;

		_StraFundInfo()
		{
			memset(this, 0, sizeof(_StraFundInfo));
		}
	} StraFundInfo;

	StraFundInfo		_fund_info;

protected:
	uint32_t		_context_id;

	StdUniqueMutex	_mtx_calc;
	StdCondVariable	_cond_calc;
	bool			_has_hook;		//这是人为控制是否启用钩子
	bool			_hook_valid;	//这是根据是否是异步回测模式而确定钩子是否可用
	std::atomic<bool>	_resumed;	//临时变量，用于控制状态
};

