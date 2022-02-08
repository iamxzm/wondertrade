/*!
 * \file HftMocker.cpp
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#include "HftMocker.h"
#include "WtHelper.h"

#include <stdarg.h>
#include <math.h>

#include <boost/filesystem.hpp>

#include "../Includes/WTSVariant.hpp"
#include "../Includes/WTSContractInfo.hpp"
#include "../Share/decimal.h"
#include "../Share/TimeUtils.hpp"
#include "../Share/StdUtils.hpp"
#include "../Share/StrUtil.hpp"

#include "../WTSTools/WTSLogger.h"


using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;
using bsoncxx::to_json;

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

using namespace mongocxx;

namespace rj = rapidjson;
using namespace std;

std::mutex c1_mtx_1{};

uint32_t makeLocalOrderID()
{
	static std::atomic<uint32_t> _auto_order_id{ 0 };
	if (_auto_order_id == 0)
	{
		uint32_t curYear = TimeUtils::getCurDate() / 10000 * 10000 + 101;
		_auto_order_id = (uint32_t)((TimeUtils::getLocalTimeNow() - TimeUtils::makeTime(curYear, 0)) / 1000 * 50);
	}

	return _auto_order_id.fetch_add(1);
}

std::vector<uint32_t> splitVolume(uint32_t vol)
{
	uint32_t minQty = 1;
	uint32_t maxQty = 100;
	uint32_t length = maxQty - minQty + 1;
	std::vector<uint32_t> ret;
	if (vol <= minQty)
	{
		ret.emplace_back(vol);
	}
	else
	{
		uint32_t left = vol;
		srand((uint32_t)time(NULL));
		while (left > 0)
		{
			uint32_t curVol = minQty + (uint32_t)rand() % length;

			if (curVol >= left)
				curVol = left;

			if (curVol == 0)
				continue;

			ret.emplace_back(curVol);
			left -= curVol;
		}
	}

	return ret;
}

uint32_t genRand(uint32_t maxVal = 10000)
{
	srand(TimeUtils::getCurMin());
	return rand() % maxVal;
}

inline uint32_t makeHftCtxId()
{
	static std::atomic<uint32_t> _auto_context_id{ 6000 };
	return _auto_context_id.fetch_add(1);
}

HftMocker::HftMocker(HisDataReplayer* replayer, const char* name)
	: IHftStraCtx(name)
	, _replayer(replayer)
	, _strategy(NULL)
	, _thrd(NULL)
	, _stopped(false)
	, _use_newpx(false)
	, _error_rate(0)
	, _has_hook(false)
	, _hook_valid(true)
	, _resumed(false)
{
	_commodities = CommodityMap::create();

	_context_id = makeHftCtxId();
}


HftMocker::~HftMocker()
{
	if(_strategy)
	{
		_factory._fact->deleteStrategy(_strategy);
	}

	_commodities->release();
}

void HftMocker::procTask()
{
	if (_tasks.empty())
	{
		return;
	}

	_mtx_control.lock();

	while (!_tasks.empty())
	{
		Task& task = _tasks.front();

		task();

		{
			std::unique_lock<std::mutex> lck(_mtx);
			_tasks.pop();
		}
	}

	_mtx_control.unlock();
}

void HftMocker::postTask(Task task)
{
	{
		std::unique_lock<std::mutex> lck(_mtx);
		_tasks.push(task);
		return;
	}

	if(_thrd == NULL)
	{
		_thrd.reset(new std::thread([this](){
			while (!_stopped)
			{
				if(_tasks.empty())
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					continue;
				}

				_mtx_control.lock();

				while(!_tasks.empty())
				{
					Task& task = _tasks.front();

					task();

					{
						std::unique_lock<std::mutex> lck(_mtx);
						_tasks.pop();
					}
				}

				_mtx_control.unlock();
			}
		}));
	}
}

bool HftMocker::init_hft_factory(WTSVariant* cfg)
{
	if (cfg == NULL)
		return false;

	const char* module = cfg->getCString("module");
	
	_use_newpx = cfg->getBoolean("use_newpx");
	_error_rate = cfg->getUInt32("error_rate");

	DllHandle hInst = DLLHelper::load_library(module);
	if (hInst == NULL)
		return false;

	FuncCreateHftStraFact creator = (FuncCreateHftStraFact)DLLHelper::get_symbol(hInst, "createStrategyFact");
	if (creator == NULL)
	{
		DLLHelper::free_library(hInst);
		return false;
	}

	_factory._module_inst = hInst;
	_factory._module_path = module;
	_factory._creator = creator;
	_factory._remover = (FuncDeleteHftStraFact)DLLHelper::get_symbol(hInst, "deleteStrategyFact");
	_factory._fact = _factory._creator();

	WTSVariant* cfgStra = cfg->get("strategy");
	if(cfgStra)
	{
		_strategy = _factory._fact->createStrategy(cfgStra->getCString("name"), "hft");
		_strategy->init(cfgStra->get("params"));
	}
	return true;
}

void HftMocker::handle_tick(const char* stdCode, WTSTickData* curTick)
{
	on_tick(stdCode, curTick);
}

void HftMocker::handle_order_detail(const char* stdCode, WTSOrdDtlData* curOrdDtl)
{
	on_order_detail(stdCode, curOrdDtl);
}

void HftMocker::handle_order_queue(const char* stdCode, WTSOrdQueData* curOrdQue)
{
	on_order_queue(stdCode, curOrdQue);
}

void HftMocker::handle_transaction(const char* stdCode, WTSTransData* curTrans)
{
	on_transaction(stdCode, curTrans);
}

void HftMocker::handle_bar_close(const char* stdCode, const char* period, uint32_t times, WTSBarStruct* newBar)
{
	on_bar(stdCode, period, times, newBar);
}

void HftMocker::handle_init()
{
	on_init();
	on_channel_ready();
}

void HftMocker::handle_schedule(uint32_t uDate, uint32_t uTime)
{
	//on_schedule(uDate, uTime);
}

void HftMocker::handle_session_begin(uint32_t curTDate)
{
	on_session_begin(curTDate);
}

void HftMocker::handle_session_end(uint32_t curTDate)
{
	on_session_end(curTDate);
}

void HftMocker::handle_replay_done()
{
	dump_outputs();

	this->on_bactest_end();
}

void HftMocker::on_bar(const char* stdCode, const char* period, uint32_t times, WTSBarStruct* newBar)
{
	if (_strategy)
		_strategy->on_bar(this, stdCode, period, times, newBar);
}

void HftMocker::enable_hook(bool bEnabled /* = true */)
{
	_hook_valid = bEnabled;

	WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Calculating hook %s", bEnabled ? "enabled" : "disabled");
}

void HftMocker::install_hook()
{
	_has_hook = true;

	WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "HFT hook installed");
}

void HftMocker::step_tick()
{
	if (!_has_hook)
		return;

	WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Notify calc thread, wait for calc done");
	while (!_resumed)
		_cond_calc.notify_all();

	{
		StdUniqueLock lock(_mtx_calc);
		_cond_calc.wait(_mtx_calc);
		WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Calc done notified");
		_resumed = false;
	}
}

void HftMocker::on_tick(const char* stdCode, WTSTickData* newTick)
{
	_price_map[stdCode] = newTick->price();
	{
		std::unique_lock<std::recursive_mutex> lck(_mtx_control);
	}

	//交易日
	if (_traderday < _replayer->get_trading_date())
	{
		_new_trade_day = true;
		_traderday = _replayer->get_trading_date();
	}

	_pretraderday = _replayer->getPrevTDate(stdCode, _traderday);
	if (_firstday == 0)
	{
		_firstday = _replayer->get_trading_date();
	}

	//昨结
	_close_price = newTick->presettle();
	//结算价
	_settlepx = newTick->price();

	std::string inst = "";
	auto barinstdate = _replayer->get_barinstdate();
	for (auto it = barinstdate->begin(); it != barinstdate->end(); it++)
	{
		bool quit = false;
		for (auto iit = it->second.begin(); iit != it->second.end(); iit++)
		{
			if (iit->second._s_date <= newTick->actiondate() && iit->second._e_date >= newTick->actiondate())
			{
				inst = iit->first;
				quit = true;
				break;
			}
		}
		if (quit) break;
	}

	update_dyn_profit(stdCode, newTick);

	procTask();
	
	if (!_orders.empty())
	{
		OrderIDs ids;
		for (auto it = _orders.begin(); it != _orders.end(); it++)
		{
			uint32_t localid = it->first;
			bool bNeedErase = procOrder(localid, inst);
			if (bNeedErase)
			{
				_changepos = true;  //持仓变化
				ids.emplace_back(localid);
			}
				
		}

		for(uint32_t localid : ids)
		{
			auto it = _orders.find(localid);
			_orders.erase(it);
		}
	}

	if (_has_hook && _hook_valid)
	{
		WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Waiting for resume notify");
		StdUniqueLock lock(_mtx_calc);
		_cond_calc.wait(_mtx_calc);
		WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Calc resumed");
		_resumed = true;
	}

	on_tick_updated(stdCode, newTick);

	if (_has_hook && _hook_valid)
	{
		WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Calc done, notify control thread");
		while (_resumed)
			_cond_calc.notify_all();
	}

	//持仓变化更新数据
	if (_changepos)
	{
		_changepos = false;
		set_dayaccount(stdCode, newTick);
	}
}

void HftMocker::on_tick_updated(const char* stdCode, WTSTickData* newTick)
{
	if (_strategy)
		_strategy->on_tick(this, stdCode, newTick);
}

void HftMocker::on_order_queue(const char* stdCode, WTSOrdQueData* newOrdQue)
{
	on_ordque_updated(stdCode, newOrdQue);
}

void HftMocker::on_ordque_updated(const char* stdCode, WTSOrdQueData* newOrdQue)
{
	if (_strategy)
		_strategy->on_order_queue(this, stdCode, newOrdQue);
}

void HftMocker::on_order_detail(const char* stdCode, WTSOrdDtlData* newOrdDtl)
{
	on_orddtl_updated(stdCode, newOrdDtl);
}

void HftMocker::on_orddtl_updated(const char* stdCode, WTSOrdDtlData* newOrdDtl)
{
	if (_strategy)
		_strategy->on_order_detail(this, stdCode, newOrdDtl);
}

void HftMocker::on_transaction(const char* stdCode, WTSTransData* newTrans)
{
	on_trans_updated(stdCode, newTrans);
}

void HftMocker::on_trans_updated(const char* stdCode, WTSTransData* newTrans)
{
	if (_strategy)
		_strategy->on_transaction(this, stdCode, newTrans);
}

uint32_t HftMocker::id()
{
	return _context_id;
}

void HftMocker::on_init()
{
	if (_strategy)
		_strategy->on_init(this);
}

void HftMocker::on_session_begin(uint32_t curTDate)
{

}

void HftMocker::on_session_end(uint32_t curTDate)
{
	uint32_t curDate = curTDate;// _replayer->get_trading_date();

	double total_profit = 0;
	double total_dynprofit = 0;

	for (auto it = _pos_map.begin(); it != _pos_map.end(); it++)
	{
		const char* stdCode = it->first.c_str();
		const PosInfo& pInfo = it->second;
		total_profit += pInfo._closeprofit;
		total_dynprofit += pInfo._dynprofit;
	}

	_fund_logs << StrUtil::printf("%d,%.2f,%.2f,%.2f,%.2f\n", curDate,
		_fund_info._total_profit, _fund_info._total_dynprofit,
		_fund_info._total_profit + _fund_info._total_dynprofit - _fund_info._total_fees, _fund_info._total_fees);
}

double HftMocker::stra_get_undone(const char* stdCode)
{
	double ret = 0;
	for (auto it = _orders.begin(); it != _orders.end(); it++)
	{
		const OrderInfo& ordInfo = it->second;
		if (strcmp(ordInfo._code, stdCode) == 0)
		{
			ret += ordInfo._left * ordInfo._isBuy ? 1 : -1;
		}
	}

	return ret;
}

bool HftMocker::stra_cancel(uint32_t localid)
{
	postTask([this, localid](){
		auto it = _orders.find(localid);
		if (it == _orders.end())
			return;

		StdLocker<StdRecurMutex> lock(_mtx_ords);
		OrderInfo& ordInfo = (OrderInfo&)it->second;
		ordInfo._left = 0;

		on_order(localid, ordInfo._code, ordInfo._isBuy, ordInfo._total, ordInfo._left, ordInfo._price, true, ordInfo._usertag);
		_orders.erase(it);
	});

	return true;
}

OrderIDs HftMocker::stra_cancel(const char* stdCode, bool isBuy, double qty /* = 0 */)
{
	OrderIDs ret;
	uint32_t cnt = 0;
	for (auto it = _orders.begin(); it != _orders.end(); it++)
	{
		const OrderInfo& ordInfo = it->second;
		if(ordInfo._isBuy == isBuy && strcmp(ordInfo._code, stdCode) == 0)
		{
			double left = ordInfo._left;
			stra_cancel(it->first);
			ret.emplace_back(it->first);
			cnt++;
			if (left < qty)
				qty -= left;
			else
				break;
		}
	}

	return ret;
}

otp::OrderIDs HftMocker::stra_buy(const char* stdCode, double price, double qty, const char* userTag)
{
	uint32_t localid = makeLocalOrderID();

	OrderInfo order;
	order._localid = localid;
	strcpy(order._code, stdCode);
	strcpy(order._usertag, userTag);
	order._isBuy = true;
	order._price = price;
	order._total = qty;
	order._left = qty;

	{
		_mtx_ords.lock();
		_orders[localid] = order;
		_mtx_ords.unlock();
	}

	postTask([this, localid](){
		const OrderInfo& ordInfo = _orders[localid];
		on_entrust(localid, ordInfo._code, true, "下单成功", ordInfo._usertag);
		//bool bNeedErase = procOrder(localid);
		//if(bNeedErase)
		//{
		//	auto it = _orders.find(localid);
		//	_orders.erase(it);
		//}
	});

	OrderIDs ids;
	ids.emplace_back(localid);
	return ids;
}

void HftMocker::on_order(uint32_t localid, const char* stdCode, bool isBuy, double totalQty, double leftQty, double price, bool isCanceled /* = false */, const char* userTag /* = "" */)
{
	if(_strategy)
		_strategy->on_order(this, localid, stdCode, isBuy, totalQty, leftQty, price, isCanceled, userTag);
}

void HftMocker::on_trade(uint32_t localid, std::string instid, const char* stdCode, bool isBuy, double vol, double price, const char* userTag/* = ""*/)
{
	if (_strategy)
		_strategy->on_trade(this, localid, stdCode, isBuy, vol, price, userTag);

	const PosInfo& posInfo = _pos_map[stdCode];
	double curPos = posInfo._volume + vol * (isBuy ? 1 : -1);
	do_set_position(stdCode, instid, curPos, price, userTag);
}

void HftMocker::on_entrust(uint32_t localid, const char* stdCode, bool bSuccess, const char* message, const char* userTag/* = ""*/)
{
	if (_strategy)
		_strategy->on_entrust(localid, bSuccess, message, userTag);
}

void HftMocker::on_channel_ready()
{
	if (_strategy)
		_strategy->on_channel_ready(this);
}

void HftMocker::update_dyn_profit(const char* stdCode, WTSTickData* newTick)
{
	auto it = _pos_map.find(stdCode);
	if (it != _pos_map.end())
	{
		PosInfo& pInfo = (PosInfo&)it->second;
		if (pInfo._volume == 0)
		{
			pInfo._dynprofit = 0;
		}
		else
		{
			bool isLong = decimal::gt(pInfo._volume, 0);
			double price = isLong ? newTick->bidprice(0) : newTick->askprice(0);

			WTSCommodityInfo* commInfo = _replayer->get_commodity_info(stdCode);
			double dynprofit = 0;
			for (auto pit = pInfo._details.begin(); pit != pInfo._details.end(); pit++)
			{
				
				DetailInfo& dInfo = *pit;
				dInfo._profit = dInfo._volume*(price - dInfo._price)*commInfo->getVolScale()*(dInfo._long ? 1 : -1);
				if (dInfo._profit > 0)
					dInfo._max_profit = max(dInfo._profit, dInfo._max_profit);
				else if (dInfo._profit < 0)
					dInfo._max_loss = min(dInfo._profit, dInfo._max_loss);

				dynprofit += dInfo._profit;
			}

			pInfo._dynprofit = dynprofit;
		}
	}
}


void HftMocker::set_dayaccount(const char* stdCode, WTSTickData* newTick, bool bEmitStrategy /* = true */)
{
	mongocxx::database mongodb = _replayer->_client["lsqt_db"];
	mongocxx::collection acccoll = mongodb["his_account"];
	mongocxx::collection daycoll = mongodb["day_account"];
	mongocxx::collection allcoll = mongodb["account"];

	bsoncxx::document::value position_doc = document{} << "test" << "INIT DOC" << finalize;

	int64_t curTime = _replayer->get_date() * 1000000 + _replayer->get_min_time() * 100 + _replayer->get_secs();

	double total_dynprofit = 0;
	for (auto v : _pos_map)
	{
		const PosInfo& pInfo = v.second;
		total_dynprofit += pInfo._dynprofit;
	}

	_fund_info._total_dynprofit = total_dynprofit;

	//计算期初权益
	if (_new_trade_day)
	{
		_static_balance = _total_money + _used_margin + _fund_info._total_fees - _fund_info._total_dynprofit - _fund_info._total_profit;
		_new_trade_day = false;
	}
	//今日资产 = 期初权益 + 持仓盈亏 + 平仓盈亏 - 手续费
	_balance = _static_balance + _fund_info._total_dynprofit + _total_closeprofit - _fund_info._total_fees;
	//当日盈亏
	_day_profit = _balance - _static_balance;
	//策略收益
	_total_profit = _balance - init_money;

	//收益率公式 = (当前净值/最初净值)
	_daily_rate_of_return = (_day_profit / _static_balance);
	if (isnan(_daily_rate_of_return) || !isfinite(_daily_rate_of_return))
	{
		_daily_rate_of_return = 0;
	}

	//基准收益率
	double benchmarkPrePrice = _close_price;		//cacheHandler.getClosePriceByDate(BENCHMARK_CODE, preTradeDay).doubleValue();  //昨收价
	double benchmarkEndPrice = _settlepx;			//cacheHandler.getClosePriceByDate(BENCHMARK_CODE, tradeDay).doubleValue(); //今收价

	_benchmark_rate_of_return = (benchmarkPrePrice / benchmarkEndPrice) - 1;
	if (isnan(_benchmark_rate_of_return) || !isfinite(_benchmark_rate_of_return) || _firstday == _replayer->get_trading_date())
	{
		_benchmark_rate_of_return = 0;
	}

	//基准累计收益率
	double firstPrice = 0;
	_benchmark_cumulative_rate = benchmarkEndPrice / firstPrice;
	if (!isfinite(_benchmark_cumulative_rate))
	{
		_benchmark_cumulative_rate = 0;
	}

	//日超额收益率
	_abnormal_rate_of_return = (_daily_rate_of_return + 1) / (_benchmark_rate_of_return + 1) - 1;
	if (isnan(_abnormal_rate_of_return) || !isfinite(_abnormal_rate_of_return))
	{
		_abnormal_rate_of_return = 0;
	}

	_win_or_lose_flag = _daily_rate_of_return > _benchmark_rate_of_return ? 1 : 0;

	//策略累计收益率
	bsoncxx::stdx::optional<bsoncxx::document::value> day_result = daycoll.find_one(make_document(kvp("accounts.314159.account_id", "314159"),
		kvp("trade_day", to_string(_pretraderday)),
		kvp("strategy_id", _name)));

	double preRate = 0;
	if (day_result)
	{
		bsoncxx::document::view view = day_result->view();
		bsoncxx::document::element msgs_ele = view["strategy_cumulative_rate"];
		if (msgs_ele && msgs_ele.type() == bsoncxx::type::k_double)
		{
			preRate = view["strategy_cumulative_rate"].get_double().value;
		}
	}
	else
	{
		preRate = 0;
	}
	if (_firstday == _replayer->get_trading_date())
	{
		_strategy_cumulative_rate = 0;
	}
	else
	{
		_strategy_cumulative_rate = (preRate + 1) * (_daily_rate_of_return + 1) - 1;	//(preRate + 1) * (currRate + 1) - 1;
	}

	//储存到mongo
	position_doc = document{} <<
		"position_profit" << 0.0 <<
		"available" << _total_money <<
		"frozen_premium" << 0.0 <<
		"close_profit" << _total_closeprofit <<
		"day_profit" << _day_profit <<
		"premium" << 0.0 <<
		"balance" << _balance <<
		"static_balance" << _static_balance <<
		"currency" << "CNY" <<
		"commission" << 0.0 <<
		"frozen_margin" << 0.0 <<
		"pre_balance" << _static_balance <<
		"benchmark_rate_of_return" << _benchmark_rate_of_return <<
		"benchmark_cumulative_rate" << _benchmark_cumulative_rate <<
		"strategy_cumulative_rate" << _strategy_cumulative_rate <<
		"float_profit" << 0.0 <<
		"timestamp" << curTime <<
		"margin" << _used_margin <<
		"risk_ratio" << 0.0 <<
		"trade_day" << to_string(_traderday) <<
		"frozen_commission" << 0.0 <<
		"abnormal_rate_of_return" << _abnormal_rate_of_return <<
		"daily_rate_of_return" << _daily_rate_of_return <<
		"win_or_lose_flag" << _win_or_lose_flag <<
		"strategy_id" << _name <<
		"deposit" << 0.0 <<
		"accounts" << open_document <<
		"314159" << open_document <<
		"position_profit" << 0.0 <<
		"margin" << _used_margin <<
		"risk_ratio" << 0.0 <<
		"frozen_commission" << 0.0 <<
		"frozen_premium" << 0.0 <<
		"available" << 0.0 <<
		"close_profit" << _total_closeprofit <<
		"account_id" << "314159" <<
		"premium" << 0.0 <<
		"static_balance" << _static_balance <<
		"balance" << _balance <<
		"deposit" << 0.0 <<
		"currency" << "rmb" <<
		"pre_balance" << 0.0 <<
		"commission" << 0.0 <<
		"frozen_margin" << 0.0 <<
		"float_profit" << 0.0 <<
		"withdraw" << 0.0 <<
		close_document <<
		close_document <<
		"withdraw" << 0.0 <<
		finalize;


	acccoll.insert_one(std::move(position_doc));

	//插入day acc
	if (_dayacc_insert_flag || _per_strategy_id != _name)
	{
		_per_strategy_id = _name;

		position_doc = document{} <<
			"position_profit" << 0.0 <<
			"available" << _total_money <<
			"frozen_premium" << 0.0 <<
			"close_profit" << _total_closeprofit <<
			"day_profit" << _day_profit <<
			"premium" << 0.0 <<
			"balance" << _balance <<
			"static_balance" << _static_balance <<
			"currency" << "CNY" <<
			"commission" << 0.0 <<
			"frozen_margin" << 0.0 <<
			"pre_balance" << _static_balance <<
			"benchmark_rate_of_return" << _benchmark_rate_of_return <<
			"benchmark_cumulative_rate" << _benchmark_cumulative_rate <<
			"strategy_cumulative_rate" << _strategy_cumulative_rate <<
			"float_profit" << 0.0 <<
			"timestamp" << curTime <<
			"margin" << _used_margin <<
			"risk_ratio" << 0.0 <<
			"trade_day" << to_string(_traderday) <<
			"frozen_commission" << 0.0 <<
			"abnormal_rate_of_return" << _abnormal_rate_of_return <<
			"daily_rate_of_return" << _daily_rate_of_return <<
			"win_or_lose_flag" << _win_or_lose_flag <<
			"strategy_id" << _name <<
			"total_deposit" << init_money <<
			"total_profit" << _total_profit <<
			"accounts" << open_document <<
			"314159" << open_document <<
			"position_profit" << 0.0 <<
			"margin" << _used_margin <<
			"risk_ratio" << 0.0 <<
			"frozen_commission" << 0.0 <<
			"frozen_premium" << 0.0 <<
			"available" << 0.0 <<
			"close_profit" << _total_closeprofit <<
			"account_id" << "314159" <<
			"premium" << 0.0 <<
			"static_balance" << _static_balance <<
			"balance" << _balance <<
			"deposit" << 0.0 <<
			"currency" << "rmb" <<
			"pre_balance" << 0.0 <<
			"commission" << 0.0 <<
			"frozen_margin" << 0.0 <<
			"float_profit" << 0.0 <<
			"withdraw" << 0.0 <<
			close_document <<
			close_document <<
			"total_withdraw" << 0.0 <<
			finalize;

		daycoll.insert_one(std::move(position_doc));
		_dayacc_insert_flag = false;
	}
	else //更新day acc
	{
		daycoll.update_one(
			make_document(kvp("trade_day", to_string(_traderday)), kvp("accounts.314159.account_id", "314159"), kvp("strategy_id", _name)),
			make_document(kvp("$set", make_document(kvp("available", _total_money),
				kvp("close_profit", _total_closeprofit),
				kvp("day_profit", _day_profit),
				kvp("balance", _balance),
				kvp("static_balance", _static_balance),
				kvp("benchmark_rate_of_return", _benchmark_rate_of_return),
				kvp("benchmark_cumulative_rate", _benchmark_cumulative_rate),
				kvp("strategy_cumulative_rate", _strategy_cumulative_rate),
				kvp("timestamp", curTime),
				kvp("margin", _used_margin),
				kvp("abnormal_rate_of_return", _abnormal_rate_of_return),
				kvp("daily_rate_of_return", _daily_rate_of_return),
				kvp("win_or_lose_flag", _win_or_lose_flag),
				kvp("total_profit", _total_profit),
				kvp("total_deposit", init_money),
				kvp("accounts.314159.margin", _used_margin),
				kvp("accounts.314159.static_balance", _static_balance),
				kvp("accounts.314159.balance", _balance),
				kvp("accounts.314159.close_profit", _total_closeprofit)
			))));
	}

	//accounts数据库
	bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = allcoll.find_one(make_document(kvp("accounts.314159.account_id", "314159"), kvp("strategy_id", _name)));
	if (maybe_result)
	{
		allcoll.update_one(
			make_document(kvp("accounts", "314159"), kvp("strategy_id", _name)),
			make_document(kvp("$set", make_document(kvp("available", _total_money),
				kvp("day_profit", _day_profit),
				kvp("close_profit", _total_closeprofit),
				kvp("balance", _balance),
				kvp("static_balance", _static_balance),
				kvp("benchmark_rate_of_return", _benchmark_rate_of_return),
				kvp("timestamp", curTime),
				kvp("margin", _used_margin),
				kvp("abnormal_rate_of_return", _abnormal_rate_of_return),
				kvp("daily_rate_of_return", _daily_rate_of_return),
				kvp("win_or_lose_flag", _win_or_lose_flag),
				kvp("accounts.314159.margin", _used_margin),
				kvp("accounts.314159.static_balance", _static_balance),
				kvp("accounts.314159.balance", _balance),
				kvp("accounts.314159.close_profit", _total_closeprofit)
			))));
	}
	else
	{
		allcoll.insert_one(
			document{} <<
			"position_profit" << 0.0 <<
			"available" << _total_money <<
			"frozen_premium" << 0.0 <<
			"close_profit" << _total_closeprofit <<
			"day_profit" << _day_profit <<
			"premium" << 0.0 <<
			"balance" << _balance <<
			"static_balance" << _static_balance <<
			"currency" << "CNY" <<
			"commission" << 0.0 <<
			"frozen_margin" << 0.0 <<
			"pre_balance" << _static_balance <<
			"float_profit" << 0.0 <<
			"timestamp" << curTime <<
			"margin" << _used_margin <<
			"risk_ratio" << 0.0 <<
			"trade_day" << to_string(_traderday) <<
			"frozen_commission" << 0.0 <<
			"strategy_id" << _name <<
			"deposit" << 0.0 <<
			"accounts" << open_document <<
			"314159" << open_document <<
			"position_profit" << 0.0 <<
			"margin" << _used_margin <<
			"risk_ratio" << 0.0 <<
			"frozen_commission" << 0.0 <<
			"frozen_premium" << 0.0 <<
			"available" << 0.0 <<
			"close_profit" << _total_closeprofit <<
			"account_id" << "314159" <<
			"premium" << 0.0 <<
			"static_balance" << _static_balance <<
			"balance" << _balance <<
			"deposit" << 0.0 <<
			"currency" << "rmb" <<
			"pre_balance" << 0.0 <<
			"commission" << 0.0 <<
			"frozen_margin" << 0.0 <<
			"float_profit" << 0.0 <<
			"withdraw" << 0.0 <<
			close_document <<
			close_document <<
			"withdraw" << 0.0 <<
			finalize
		);
	}
}


bool HftMocker::procOrder(uint32_t localid,std::string instid)
{
	auto it = _orders.find(localid);
	if (it == _orders.end())
		return false;

	StdLocker<StdRecurMutex> lock(_mtx_ords);
	OrderInfo& ordInfo = (OrderInfo&)it->second;

	//第一步,如果在撤单概率中,则执行撤单
	if(_error_rate>0 && genRand(10000)<=_error_rate)
	{
		on_order(localid, ordInfo._code, ordInfo._isBuy, ordInfo._total, ordInfo._left, ordInfo._price, true, ordInfo._usertag);
		stra_log_info("Random error order: %u", localid);
		return true;
	}
	else
	{
		on_order(localid, ordInfo._code, ordInfo._isBuy, ordInfo._total, ordInfo._left, ordInfo._price, false, ordInfo._usertag);
	}

	WTSTickData* curTick = stra_get_last_tick(ordInfo._code);
	if (curTick == NULL)
		return false;

	double curPx = curTick->price();
	double orderQty = ordInfo._isBuy ? curTick->askqty(0) : curTick->bidqty(0);	//看对手盘的数量
	curTick->release();
	if (!_use_newpx)
	{
		curPx = ordInfo._isBuy ? curTick->askprice(0) : curTick->bidprice(0);
		//if (curPx == 0.0)
		if(decimal::eq(curPx, 0.0))
			return false;
	}

	//如果没有成交条件,则退出逻辑
	if(!decimal::eq(ordInfo._price, 0.0))
	{
		if(ordInfo._isBuy && decimal::gt(curPx, ordInfo._price))
		{
			//买单,但是当前价大于限价,不成交
			return false;
		}

		if (!ordInfo._isBuy && decimal::lt(curPx, ordInfo._price))
		{
			//卖单,但是当前价小于限价,不成交
			return false;
		}
	}

	/*
	 *	下面就要模拟成交了
	 */
	double maxQty = min(orderQty, ordInfo._left);
	auto vols = splitVolume((uint32_t)maxQty);
	for(uint32_t curQty : vols)
	{
		on_trade(ordInfo._localid, instid, ordInfo._code, ordInfo._isBuy, curQty, curPx, ordInfo._usertag);

		ordInfo._left -= curQty;
		on_order(localid, ordInfo._code, ordInfo._isBuy, ordInfo._total, ordInfo._left, ordInfo._price, false, ordInfo._usertag);

		double curPos = stra_get_position(ordInfo._code);

		_sig_logs << _replayer->get_date() << "." << _replayer->get_raw_time() << "." << _replayer->get_secs() << ","
			<< (ordInfo._isBuy ? "+" : "-") << curQty << "," << curPos << "," << curPx << std::endl;
	}

	//if(ordInfo._left == 0)
	if(decimal::eq(ordInfo._left, 0.0))
	{
		return true;
	}

	return false;
}

otp::OrderIDs HftMocker::stra_sell(const char* stdCode, double price, double qty, const char* userTag)
{
	uint32_t localid = makeLocalOrderID();

	OrderInfo order;
	order._localid = localid;
	strcpy(order._code, stdCode);
	strcpy(order._usertag, userTag);
	order._isBuy = false;
	order._price = price;
	order._total = qty;
	order._left = qty;

	{
		StdLocker<StdRecurMutex> lock(_mtx_ords);
		_orders[localid] = order;
	}

	postTask([this, localid]() {
		const OrderInfo& ordInfo = _orders[localid];
		on_entrust(localid, ordInfo._code, true, "下单成功", ordInfo._usertag);
		//bool bNeedErase = procOrder(localid);
		//if (bNeedErase)
		//{
		//	auto it = _orders.find(localid);
		//	_orders.erase(it);
		//}
	});

	OrderIDs ids;
	ids.emplace_back(localid);
	return ids;
}

WTSCommodityInfo* HftMocker::stra_get_comminfo(const char* stdCode)
{
	return _replayer->get_commodity_info(stdCode);
}

WTSKlineSlice* HftMocker::stra_get_bars(const char* stdCode, const char* period, uint32_t count)
{
	std::string basePeriod = "";
	uint32_t times = 1;
	if (strlen(period) > 1)
	{
		basePeriod.append(period, 1);
		times = strtoul(period + 1, NULL, 10);
	}
	else
	{
		basePeriod = period;
	}

	return _replayer->get_kline_slice(stdCode, basePeriod.c_str(), count, times);
}

WTSTickSlice* HftMocker::stra_get_ticks(const char* stdCode, uint32_t count)
{
	return _replayer->get_tick_slice(stdCode, count);
}

WTSOrdQueSlice* HftMocker::stra_get_order_queue(const char* stdCode, uint32_t count)
{
	return _replayer->get_order_queue_slice(stdCode, count);
}

WTSOrdDtlSlice* HftMocker::stra_get_order_detail(const char* stdCode, uint32_t count)
{
	return _replayer->get_order_detail_slice(stdCode, count);
}

WTSTransSlice* HftMocker::stra_get_transaction(const char* stdCode, uint32_t count)
{
	return _replayer->get_transaction_slice(stdCode, count);
}

WTSTickData* HftMocker::stra_get_last_tick(const char* stdCode)
{
	return _replayer->get_last_tick(stdCode);
}

double HftMocker::stra_get_position(const char* stdCode)
{
	const PosInfo& pInfo = _pos_map[stdCode];
	return pInfo._volume;
}

double HftMocker::stra_get_position_profit(const char* stdCode)
{
	const PosInfo& pInfo = _pos_map[stdCode];
	return pInfo._dynprofit;
}

double HftMocker::stra_get_price(const char* stdCode)
{
	return _replayer->get_cur_price(stdCode);
}

uint32_t HftMocker::stra_get_date()
{
	return _replayer->get_date();
}

uint32_t HftMocker::stra_get_time()
{
	return _replayer->get_raw_time();
}

uint32_t HftMocker::stra_get_secs()
{
	return _replayer->get_secs();
}

void HftMocker::stra_sub_ticks(const char* stdCode)
{
	_replayer->sub_tick(_context_id, stdCode);
}

void HftMocker::stra_sub_order_queues(const char* stdCode)
{
	_replayer->sub_order_queue(_context_id, stdCode);
}

void HftMocker::stra_sub_order_details(const char* stdCode)
{
	_replayer->sub_order_detail(_context_id, stdCode);
}

void HftMocker::stra_sub_transactions(const char* stdCode)
{
	_replayer->sub_transaction(_context_id, stdCode);
}

void HftMocker::stra_log_info(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	WTSLogger::vlog_dyn("strategy", _name.c_str(), LL_INFO, fmt, args);
	va_end(args);
}

void HftMocker::stra_log_debug(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	WTSLogger::vlog_dyn("strategy", _name.c_str(), LL_DEBUG, fmt, args);
	va_end(args);
}

void HftMocker::stra_log_error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	WTSLogger::vlog_dyn("strategy", _name.c_str(), LL_ERROR, fmt, args);
	va_end(args);
}

const char* HftMocker::stra_load_user_data(const char* key, const char* defVal /*= ""*/)
{
	auto it = _user_datas.find(key);
	if (it != _user_datas.end())
		return it->second.c_str();

	return defVal;
}

void HftMocker::stra_save_user_data(const char* key, const char* val)
{
	_user_datas[key] = val;
	_ud_modified = true;
}

void HftMocker::dump_outputs()
{
	std::string folder = WtHelper::getOutputDir();
	folder += _name;
	folder += "/";
	boost::filesystem::create_directories(folder.c_str());

	std::string filename = folder + "trades.csv";
	std::string content = "code,time,direct,action,price,qty,fee,usertag\n";
	content += _trade_logs.str();
	StdFile::write_file_content(filename.c_str(), (void*)content.c_str(), content.size());

	filename = folder + "closes.csv";
	content = "code,direct,opentime,openprice,closetime,closeprice,qty,profit,maxprofit,maxloss,totalprofit,entertag,exittag\n";
	content += _close_logs.str();
	StdFile::write_file_content(filename.c_str(), (void*)content.c_str(), content.size());


	filename = folder + "funds.csv";
	content = "date,closeprofit,positionprofit,dynbalance,fee\n";
	content += _fund_logs.str();
	StdFile::write_file_content(filename.c_str(), (void*)content.c_str(), content.size());


	filename = folder + "signals.csv";
	content = "time, action, position, price\n";
	content += _sig_logs.str();
	StdFile::write_file_content(filename.c_str(), (void*)content.c_str(), content.size());
}

void HftMocker::log_trade(const char* stdCode, bool isLong, bool isOpen, uint64_t curTime, double price, double qty, double fee, const char* userTag/* = ""*/)
{
	_trade_logs << stdCode << "," << curTime << "," << (isLong ? "LONG" : "SHORT") << "," << (isOpen ? "OPEN" : "CLOSE")
		<< "," << price << "," << qty << "," << fee << "," << userTag << "\n";
}

void HftMocker::log_close(const char* stdCode, bool isLong, uint64_t openTime, double openpx, uint64_t closeTime, double closepx, double qty, double profit, double maxprofit, double maxloss,
	double totalprofit /* = 0 */, const char* enterTag/* = ""*/, const char* exitTag/* = ""*/)
{
	_close_logs << stdCode << "," << (isLong ? "LONG" : "SHORT") << "," << openTime << "," << openpx
		<< "," << closeTime << "," << closepx << "," << qty << "," << profit << "," << maxprofit << "," << maxloss << ","
		<< totalprofit << "," << enterTag << "," << exitTag << "\n";
}

void HftMocker::insert_his_position(DetailInfo dInfo, PosInfo pInfo, double fee, std::string exch_id, std::string inst_id, uint64_t curTime)
{
	auto db = _replayer->_client["lsqt_db"];
	auto _poscoll_1 = db["his_positions"];
	bsoncxx::document::value position_doc = document{} << finalize;
	std::string exch_inst = exch_id;
	exch_inst += "::";
	exch_inst += inst_id;
	if (dInfo._volume == 0)
	{
		return;
	}
	if (dInfo._long)
	{
		position_doc = document{} << "trade_day" << to_string(dInfo._opentdate) <<
			"strategy_id" << _name <<//
			"position" << open_document <<
			exch_inst << open_document <<
			"position_profit" << dInfo._profit <<
			"float_profit_short" << 0.0 <<
			"open_price_short" << 0.0 <<
			"volume_long_frozen_today" << 0 <<//
			"open_cost_long" << fee <<
			"position_price_short" << 0.0 <<
			"float_profit_long" << pInfo._dynprofit <<
			"open_price_long" << dInfo._price <<
			"exchange_id" << exch_id <<
			"volume_short_frozen_today" << 0 <<//
			"position_price_long" << dInfo._price <<
			"position_profit_long" << dInfo._profit <<
			"volume_short_today" << 0 <<
			"position_profit_short" << 0.0 <<
			"volume_long" << dInfo._volume <<
			"margin_short" << 0.0 <<//
			"volume_long_frozen_his" << 0 <<//
			"float_profit" << 0.0 <<//
			"open_cost_short" << 0.0 <<
			"margin" << 0.0 <<//
			"position_cost_short" << 0.0 <<//
			"volume_short_frozen_his" << 0 <<//
			"instrument_id" << inst_id <<
			"volume_short" << 0 <<
			"account_id" << "" <<
			"volume_long_today" << dInfo._volume <<
			"position_cost_long" << 0.0 <<//
			"volume_long_his" << 0 <<//
			"hedge_flag" << " " <<//
			"margin_long" << 0.0 <<//
			"volume_short_his" << 0 <<//
			"last_price" << 0.0 << //
			close_document <<
			close_document <<
			"timestamp" << _replayer->StringToDatetime(to_string(curTime)) * 1000 <<
			finalize;
	}
	else
	{
		position_doc = document{} << "trade_day" << to_string(dInfo._opentdate) <<
			"strategy_id" << _name <<//
			"position" << open_document <<
			exch_inst << open_document <<
			"position_profit" << dInfo._profit <<
			"float_profit_short" << pInfo._dynprofit <<
			"open_price_short" << dInfo._price <<
			"volume_long_frozen_today" << 0 <<//
			"open_cost_long" << 0.0 <<
			"position_price_short" << dInfo._price <<
			"float_profit_long" << 0.0 <<
			"open_price_long" << 0.0 <<
			"exchange_id" << exch_id <<
			"volume_short_frozen_today" << 0 <<//
			"position_price_long" << 0.0 <<
			"position_profit_long" << 0.0 <<
			"volume_short_today" << dInfo._volume <<
			"position_profit_short" << dInfo._profit <<
			"volume_long" << 0 <<
			"margin_short" << 0.0 <<//
			"volume_long_frozen_his" << 0 <<//
			"float_profit" << 0.0 <<//
			"open_cost_short" << fee <<
			"margin" << 0.0 <<//
			"position_cost_short" << 0.0 <<//
			"volume_short_frozen_his" << 0 <<//
			"instrument_id" << inst_id <<
			"volume_short" << dInfo._volume <<
			"account_id" << "" <<
			"volume_long_today" << 0.0 <<
			"position_cost_long" << fee <<//
			"volume_long_his" << 0 <<//
			"hedge_flag" << " " <<//
			"margin_long" << 0.0 <<//
			"volume_short_his" << 0 <<//
			"last_price" << 0.0 << //
			close_document <<
			close_document <<
			"timestamp" << _replayer->StringToDatetime(to_string(curTime)) * 1000 <<
			finalize;
	}
	c1_mtx_1.lock();
	auto result = _poscoll_1.insert_one(move(position_doc));
	bsoncxx::oid oid = result->inserted_id().get_oid().value;
	//std::cout << "insert one:" << oid.to_string() << std::endl;
	c1_mtx_1.unlock();

}

void HftMocker::insert_his_trades(DetailInfo dInfo, PosInfo pInfo, double fee, std::string exch_id, std::string inst_id, uint64_t curTime, int offset)
{
	auto db = _replayer->_client["lsqt_db"];
	auto _poscoll_1 = db["his_trades"];
	bsoncxx::document::value position_doc = document{} << finalize;
	std::string exch_inst = exch_id;
	exch_inst += "::";
	exch_inst += inst_id;
	if (dInfo._volume == 0)
	{
		return;
	}

	std::string off_set = "";
	if (offset == 1)
	{
		off_set = "P033_1";
	}
	else if (offset == 2)
	{
		off_set = "P033_2";
	}
	else if (offset == 3)
	{
		off_set = "P033_3";
	}
	if (dInfo._long)
	{
		position_doc = document{} << "exchange_trade_id" << "111111" <<
			"account_id" << "111111" <<
			"commission" << 0.0 <<
			"direction" << 1 <<
			"exchange_id" << exch_id <<
			"exchange_order_id" << "123456" <<
			"instrument_id" << inst_id <<
			"offset" << off_set <<
			"order_id" << "123456" <<
			"price" << dInfo._price <<
			"seqno" << 0 <<
			"strategy_id" << _name <<
			"trade_date_time" << _replayer->StringToDatetime(to_string(curTime)) * 1000 <<
			"volume" << dInfo._volume <<
			finalize;
	}
	else
	{
		position_doc = document{} << "exchange_trade_id" << "111111" <<
			"account_id" << "111111" <<
			"commission" << 0.0 <<
			"direction" << 2 <<
			"exchange_id" << exch_id <<
			"exchange_order_id" << "123456" <<
			"instrument_id" << inst_id <<
			"offset" << off_set <<
			"order_id" << "123456" <<
			"price" << dInfo._price <<
			"seqno" << 0 <<
			"strategy_id" << _name <<
			"trade_date_time" << _replayer->StringToDatetime(to_string(curTime)) * 1000 <<
			"volume" << dInfo._volume <<
			finalize;
	}
	c1_mtx_1.lock();
	auto result = _poscoll_1.insert_one(move(position_doc));
	bsoncxx::oid oid = result->inserted_id().get_oid().value;
	//std::cout << "insert one:" << oid.to_string() << std::endl;
	c1_mtx_1.unlock();
}

void HftMocker::insert_his_trade(DetailInfo dInfo, PosInfo pInfo, double fee, std::string exch_id, std::string inst_id, uint64_t curTime, int offset)
{
	auto db = _replayer->_client["lsqt_db"];
	auto _poscoll_1 = db["his_trade"];
	bsoncxx::document::value position_doc = document{} << finalize;
	std::string exch_inst = exch_id;
	exch_inst += "::";
	exch_inst += inst_id;
	if (dInfo._volume == 0)
	{
		return;
	}

	std::string off_set = "";
	if (offset == 1)
	{
		off_set = "P033_1";
	}
	else if (offset == 2)
	{
		off_set = "P033_2";
	}
	else if (offset == 3)
	{
		off_set = "P033_3";
	}
	position_doc = document{} << "trade_date_time" << _replayer->StringToDatetime(to_string(curTime)) * 1000 <<
		"offset" << off_set <<
		"seqno" << "" <<
		"exchange_trade_id" << "" <<
		"trading_day" << to_string(_replayer->get_trading_date()) <<
		"type" << "" <<
		"instrument_id" << inst_id <<
		"exchange_order_id" << "" <<
		"close_profit" << pInfo._closeprofit <<
		"volume" << dInfo._volume <<
		"exchange_id" << exch_id <<
		"account_id" << "" <<
		"price" << dInfo._price <<
		"strategy_id" << _name <<
		"commission" << "" <<
		"order_id" << "" <<
		"direction" << dInfo._long << finalize;

	c1_mtx_1.lock();
	auto result = _poscoll_1.insert_one(move(position_doc));
	bsoncxx::oid oid = result->inserted_id().get_oid().value;
	//std::cout << "insert one:" << oid.to_string() << std::endl;
	c1_mtx_1.unlock();
}

void HftMocker::do_set_position(const char* stdCode, std::string instid, double qty, double price /* = 0.0 */, const char* userTag /*= ""*/)
{
	PosInfo& pInfo = _pos_map[stdCode];
	double curPx = price;
	if (decimal::eq(price, 0.0))
		curPx = _price_map[stdCode];
	uint64_t curTm = (uint64_t)_replayer->get_date() * 1000000000 + (uint64_t)_replayer->get_min_time()*100000 + _replayer->get_secs();
	uint32_t curTDate = _replayer->get_trading_date();

	//手数相等则不用操作了
	if (decimal::eq(pInfo._volume, qty))
		return;

	stra_log_info("[%04u.%05u] %s position updated: %.0f -> %0.f", _replayer->get_min_time(), _replayer->get_secs(), stdCode, pInfo._volume, qty);

	WTSCommodityInfo* commInfo = _replayer->get_commodity_info(stdCode);

	std::string exchid = commInfo->getExchg();
	std::string exch_inst = exchid + "::";
	exch_inst += instid;

	//成交价
	double trdPx = curPx;

	double diff = qty - pInfo._volume;

	//保证金检测是否成交
	double tempfee = _replayer->calc_fee(stdCode, trdPx, abs(diff), 0);
	double tempmargin = _margin_rate * _cur_multiplier * _close_price * abs(diff);
	if (!decimal::gt(_total_money - (tempmargin + tempfee), 0))
	{
		WTSLogger::log_dyn("strategy", _name.c_str(), LL_WARN, "error:资金账户不足");
		return;
	}

	if (decimal::gt(pInfo._volume*diff, 0))//当前持仓和仓位变化方向一致, 增加一条明细, 增加数量即可
	{
		pInfo._volume = qty;

		DetailInfo dInfo;
		dInfo._long = decimal::gt(qty, 0);
		dInfo._price = trdPx;
		dInfo._volume = abs(diff);
		dInfo._opentime = curTm;
		dInfo._margin = _margin_rate * _cur_multiplier * _close_price * abs(diff);
		dInfo._opentdate = curTDate;
		strcpy(dInfo._usertag, userTag);
		pInfo._details.emplace_back(dInfo);

		double fee = _replayer->calc_fee(stdCode, trdPx, abs(diff), 0);
		_fund_info._total_fees += fee;


		//保证金计算
		_total_money -= dInfo._margin;
		_total_money -= fee;
		_used_margin += dInfo._margin;

		int offset = 1;
		if (_name != "")
		{
			insert_his_position(dInfo, pInfo, fee, exchid, instid, curTm);
			insert_his_trades(dInfo, pInfo, fee, exchid, instid, curTm, offset);
			insert_his_trade(dInfo, pInfo, fee, exchid, instid, curTm, offset);
		}


		log_trade(stdCode, dInfo._long, true, curTm, trdPx, abs(diff), fee, userTag);
	}
	else if(decimal::lt(pInfo._volume * diff, 0))
	{//持仓方向和仓位变化方向不一致,需要平仓
		double left = abs(diff);

		pInfo._volume = qty;
		if (decimal::eq(pInfo._volume, 0))
			pInfo._dynprofit = 0;
		uint32_t count = 0;
		for (auto it = pInfo._details.begin(); it != pInfo._details.end(); it++)
		{
			DetailInfo& dInfo = *it;
			double maxQty = min(dInfo._volume, left);
			if (decimal::eq(maxQty, 0))
				continue;

			double maxProf = dInfo._max_profit * maxQty / dInfo._volume;
			double maxLoss = dInfo._max_loss * maxQty / dInfo._volume;

			dInfo._volume -= maxQty;
			left -= maxQty;

			if (decimal::eq(dInfo._volume, 0))
				count++;

			double profit = (trdPx - dInfo._price) * maxQty * commInfo->getVolScale();
			if (!dInfo._long)
				profit *= -1;
			pInfo._closeprofit += profit;
			_total_closeprofit += profit;
			pInfo._dynprofit = pInfo._dynprofit*dInfo._volume / (dInfo._volume + maxQty);//浮盈也要做等比缩放
			_fund_info._total_profit += profit;

			double fee = _replayer->calc_fee(stdCode, trdPx, maxQty, dInfo._opentdate == curTDate ? 2 : 1);
			_fund_info._total_fees += fee;


			//释放保证金
			double cur_margin = _margin_rate * _cur_multiplier * _close_price * maxQty;

			_total_money += profit;
			_total_money -= fee;
			_total_money += cur_margin;
			_used_margin -= cur_margin;
			dInfo._margin -= cur_margin;

			int offset = 2;
			if (_name != "")
			{
				insert_his_position(dInfo, pInfo, fee, exchid, instid, curTm);
				insert_his_trades(dInfo, pInfo, fee, exchid, instid, curTm, offset);
				insert_his_trade(dInfo, pInfo, fee, exchid, instid, curTm, offset);
			}

			//这里写成交记录
			log_trade(stdCode, dInfo._long, false, curTm, trdPx, maxQty, fee, userTag);
			//这里写平仓记录
			log_close(stdCode, dInfo._long, dInfo._opentime, dInfo._price, curTm, trdPx, maxQty, profit, maxProf, maxLoss, pInfo._closeprofit, dInfo._usertag, userTag);

			if (left == 0)
				break;
		}

		//需要清理掉已经平仓完的明细
		while (count > 0)
		{
			auto it = pInfo._details.begin();
			pInfo._details.erase(it);
			count--;
		}

		//最后,如果还有剩余的,则需要反手了
		if (left > 0)
		{
			left = left * qty / abs(qty);

			DetailInfo dInfo;
			dInfo._long = decimal::gt(qty, 0);
			dInfo._price = trdPx;
			dInfo._volume = abs(left);
			dInfo._opentime = curTm;
			dInfo._opentdate = curTDate;
			strcpy(dInfo._usertag, userTag);
			pInfo._details.emplace_back(dInfo);

			//这里还需要写一笔成交记录
			double fee = _replayer->calc_fee(stdCode, trdPx, abs(left), 0);
			_fund_info._total_fees += fee;
			//添加减去费率
			_total_money -= fee;

			//_engine->mutate_fund(fee, FFT_Fee);

			int offset = 1;
			if (_name != "")
			{
				insert_his_position(dInfo, pInfo, fee, exchid, instid, curTm);
				insert_his_trades(dInfo, pInfo, fee, exchid, instid, curTm, offset);
				insert_his_trade(dInfo, pInfo, fee, exchid, instid, curTm, offset);
			}

			log_trade(stdCode, dInfo._long, true, curTm, trdPx, abs(left), fee, userTag);
		}
	}
}

void  HftMocker::set_initacc(double money)
{
	init_money = money;
	_total_money = init_money;
	_static_balance = init_money;
}
