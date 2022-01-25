/*!
 * \file CtaMocker.cpp
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#include "CtaMocker.h"
#include "WtHelper.h"
#include "EventNotifier.h"

#include <exception>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <boost/filesystem.hpp>

#include "../Share/StdUtils.hpp"
#include "../Includes/WTSContractInfo.hpp"
#include "../Includes/WTSSessionInfo.hpp"
#include "../Includes/WTSTradeDef.hpp"
#include "../Share/decimal.h"
#include "../Includes/WTSVariant.hpp"
#include "../Share/CodeHelper.hpp"

#include "../WTSTools/WTSLogger.h"

std::mutex c1_mtx{};

namespace rj = rapidjson;
using namespace std;

const char* CMP_ALG_NAMES[] =
{
	"��",
	"��",
	"��",
	"��",
	"��"
};

const char* ACTION_NAMES[] =
{
	"OL",
	"CL",
	"OS",
	"CS",
	"SYN"
};


inline uint32_t makeCtxId()
{
	static std::atomic<uint32_t> _auto_context_id{ 1 };
	return _auto_context_id.fetch_add(1);
}


CtaMocker::CtaMocker(HisDataReplayer* replayer, const char* name, int32_t slippage /* = 0 */, bool persistData /* = true */, EventNotifier* notifier /* = NULL */)
	: ICtaStraCtx(name)
	, _replayer(replayer)
	, _total_calc_time(0)
	, _emit_times(0)
	, _is_in_schedule(false)
	, _ud_modified(false)
	, _strategy(NULL)
	, _slippage(slippage)
	, _schedule_times(0)
	, _total_closeprofit(0)
	, _notifier(notifier)
	, _has_hook(false)
	, _hook_valid(true)
	, _cur_step(0)
	, _wait_calc(false)
	, _in_backtest(false)
	, _persist_data(persistData)
{
	_context_id = makeCtxId();	
}


CtaMocker::~CtaMocker()
{
}

void CtaMocker::dump_outputs()
{
	if (!_persist_data)
		return;

	std::string folder = WtHelper::getOutputDir();
	folder += _name;
	folder += "/";
	boost::filesystem::create_directories(folder.c_str());

	std::string filename = folder + "trades.csv";
	std::string content = "code,time,direct,action,price,qty,tag,fee,barno\n";
	content += _trade_logs.str();
	StdFile::write_file_content(filename.c_str(), (void*)content.c_str(), content.size());

	filename = folder + "closes.csv";
	content = "code,direct,opentime,openprice,closetime,closeprice,qty,profit,maxprofit,maxloss,totalprofit,entertag,exittag,openbarno,closebarno\n";
	content += _close_logs.str();
	StdFile::write_file_content(filename.c_str(), (void*)content.c_str(), content.size());


	filename = folder + "funds.csv";
	content = "date,closeprofit,positionprofit,dynbalance,fee\n";
	content += _fund_logs.str();
	StdFile::write_file_content(filename.c_str(), (void*)content.c_str(), content.size());


	filename = folder + "signals.csv";
	content = "code,target,sigprice,gentime,usertag\n";
	content += _sig_logs.str();
	StdFile::write_file_content(filename.c_str(), (void*)content.c_str(), content.size());
}

void CtaMocker::log_signal(const char* stdCode, double target, double price, uint64_t gentime, const char* usertag /* = "" */)
{
	_sig_logs << stdCode << "," << target << "," << price << "," << gentime << "," << usertag << "\n";
}

void CtaMocker::log_trade(const char* stdCode, bool isLong, bool isOpen, uint64_t curTime, double price, double qty, const char* userTag, double fee, uint32_t barNo)
{
	_trade_logs << stdCode << "," << curTime << "," << (isLong ? "LONG" : "SHORT") << "," << (isOpen ? "OPEN" : "CLOSE") 
		<< "," << price << "," << qty << "," << userTag << "," << fee << "," << barNo << "\n";
}

void CtaMocker::log_close(const char* stdCode, bool isLong, uint64_t openTime, double openpx, uint64_t closeTime, double closepx, double qty, double profit, double maxprofit, double maxloss, 
	double totalprofit /* = 0 */, const char* enterTag /* = "" */, const char* exitTag /* = "" */, uint32_t openBarNo /* = 0 */, uint32_t closeBarNo /* = 0 */)
{
	_close_logs << stdCode << "," << (isLong ? "LONG" : "SHORT") << "," << openTime << "," << openpx
		<< "," << closeTime << "," << closepx << "," << qty << "," << profit << "," << maxprofit << "," << maxloss << ","
		<< totalprofit << "," << enterTag << "," << exitTag << "," << openBarNo << "," << closeBarNo << "\n";
}

bool CtaMocker::init_cta_factory(WTSVariant* cfg)
{
	if (cfg == NULL)
		return false;

	const char* module = cfg->getCString("module");

	DllHandle hInst = DLLHelper::load_library(module);
	if (hInst == NULL)
		return false;

	FuncCreateStraFact creator = (FuncCreateStraFact)DLLHelper::get_symbol(hInst, "createStrategyFact");
	if (creator == NULL)
	{
		DLLHelper::free_library(hInst);
		return false;
	}

	_factory._module_inst = hInst;
	_factory._module_path = module;
	_factory._creator = creator;
	_factory._remover = (FuncDeleteStraFact)DLLHelper::get_symbol(hInst, "deleteStrategyFact");
	_factory._fact = _factory._creator();

	WTSVariant* cfgStra = cfg->get("strategy");
	if (cfgStra)
	{
		_strategy = _factory._fact->createStrategy(cfgStra->getCString("name"), cfgStra->getCString("id"));
		if(_strategy)
		{
			WTSLogger::info("Strategy %s.%s is created,strategy ID: %s", _factory._fact->getName(), _strategy->getName(), _strategy->id());
		}
		_strategy->init(cfgStra->get("params"));
		_name = _strategy->id();
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
//IDataSink
void CtaMocker::handle_init()
{
	this->on_init();
}

void CtaMocker::handle_bar_close(const char* stdCode, const char* period, uint32_t times, WTSBarStruct* newBar)
{
	this->on_bar(stdCode, period, times, newBar);
}

void CtaMocker::handle_schedule(uint32_t uDate, uint32_t uTime)
{
	this->on_schedule(uDate, uTime);
}

void CtaMocker::handle_session_begin(uint32_t curTDate)
{
	this->on_session_begin(curTDate);
}

void CtaMocker::handle_session_end(uint32_t curTDate)
{
	this->on_session_end(curTDate);
}

void CtaMocker::handle_replay_done()
{
	_in_backtest = false;

	if(_emit_times > 0)
	{
		WTSLogger::log_dyn_raw("strategy", _name.c_str(), LL_INFO, fmt::format("Strategy has been scheduled for {} times,totally taking {} microsecs,average of {} microsecs",
			_emit_times, _total_calc_time, _total_calc_time / _emit_times).c_str());
	}
	else
	{
		WTSLogger::log_dyn_raw("strategy", _name.c_str(), LL_INFO, fmt::format("Strategy has been scheduled for {} times", _emit_times).c_str());
	}

	dump_outputs();

	if (_has_hook && _hook_valid)
	{
		WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Replay done, notify control thread");
		while(_wait_calc)
			_cond_calc.notify_all();
		WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Notify control thread the end done");
	}

	WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Notify strategy the end of backtest");
	this->on_bactest_end();
}

void CtaMocker::handle_tick(const char* stdCode, WTSTickData* curTick)
{
	this->on_tick(stdCode, curTick, true);
}


//////////////////////////////////////////////////////////////////////////
//�ص�����
void CtaMocker::on_bar(const char* stdCode, const char* period, uint32_t times, WTSBarStruct* newBar)
{
	if (newBar == NULL)
		return;

	std::string realPeriod;
	if (period[0] == 'd')
		realPeriod = StrUtil::printf("%s%u", period, times);
	else
		realPeriod = StrUtil::printf("m%u", times);

	std::string key = StrUtil::printf("%s#%s", stdCode, realPeriod.c_str());
	KlineTag& tag = _kline_tags[key];
	tag._closed = true;

	on_bar_close(stdCode, realPeriod.c_str(), newBar);
}

void CtaMocker::on_init()
{
	_in_backtest = true;
	if (_strategy)
		_strategy->on_init(this);
	

	//mongocxx::instance instance{};
	

	WTSLogger::info("CTA Strategy initialized, with slippage: %d", _slippage);
}

void CtaMocker::update_dyn_profit(const char* stdCode, double price)
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

	double total_dynprofit = 0;
	for (auto v : _pos_map)
	{
		const PosInfo& pInfo = v.second;
		total_dynprofit += pInfo._dynprofit;
	}

	_fund_info._total_dynprofit = total_dynprofit;

	//�����ڳ�Ȩ��=�����ʽ�+�ֲ�ռ�ñ�֤��+������-�ֲ�ӯ��-ƽ��ӯ��
	if (_new_trade_day)
	{
		_static_balance = _total_money + _used_margin + _fund_info._total_fees - _fund_info._total_dynprofit - _fund_info._total_profit;
		_new_trade_day = false;
	}
	//�����ʲ� = �ڳ�Ȩ�� + �ֲ�ӯ�� + ƽ��ӯ�� - ������
	_balance = _static_balance + _fund_info._total_dynprofit + _total_closeprofit - _fund_info._total_fees;
	//����ӯ��
	_day_profit = _balance - _static_balance;
	//��������
	_total_profit = _balance - init_money;

	//�����ʹ�ʽ = (��ǰ��ֵ/�����ֵ) -1
	_daily_rate_of_return = (_day_profit / _static_balance) - 1;
	if (isnan(_daily_rate_of_return) || !isfinite(_daily_rate_of_return))
	{
		_daily_rate_of_return = 0;
	}

	//��׼������
	double benchmarkPrePrice = _close_price;		//cacheHandler.getClosePriceByDate(BENCHMARK_CODE, preTradeDay).doubleValue();  //���ռ�
	double benchmarkEndPrice = _settlepx;			//cacheHandler.getClosePriceByDate(BENCHMARK_CODE, tradeDay).doubleValue(); //���ռ�

	_benchmark_rate_of_return = (benchmarkPrePrice / benchmarkEndPrice) - 1;
	if (isnan(_benchmark_rate_of_return) || !isfinite(_benchmark_rate_of_return))
	{
		_benchmark_rate_of_return = 0;
	}

	//�ճ���������
	_abnormal_rate_of_return = (_daily_rate_of_return + 1) / (_benchmark_rate_of_return + 1) - 1;
	if (isnan(_abnormal_rate_of_return) || !isfinite(_abnormal_rate_of_return))
	{
		_abnormal_rate_of_return = 0;
	}

	_win_or_lose_flag = _daily_rate_of_return > _benchmark_rate_of_return ? 1 : 0;

}

void CtaMocker::set_dayaccount(const char* stdCode, WTSTickData* newTick, bool bEmitStrategy /* = true */)
{
	mongocxx::database mongodb = _replayer->_client["lsqt_db"];
	mongocxx::collection acccoll = mongodb["his_account"];
	mongocxx::collection daycoll = mongodb["day_account"];
	mongocxx::collection allcoll = mongodb["account"];

	bsoncxx::document::value position_doc = document{} << "test" << "INIT DOC" << finalize;

	int64_t curTime = _replayer->get_date() * 1000000 + _replayer->get_min_time() * 100 + _replayer->get_secs();

	position_doc = document{} <<
			"position_profit" << 0.0 <<
			"available" << _total_money <<
			"frozen_premium" << 0.0<<
			"close_profit" << _total_closeprofit <<
			"day_profit" << _day_profit <<
			"premium" << 0.0<<
			"balance" << _balance <<
			"static_balance" << _static_balance <<
			"currency" << "CNY"<<
			"commission" << 0.0 <<
			"frozen_margin" << 0.0<<
			"pre_balance" << _static_balance <<
			"benchmark_rate_of_return" << _benchmark_rate_of_return <<
			"float_profit" << 0.0<<
			"timestamp" << curTime <<
			"margin" << _used_margin <<
			"risk_ratio" << 0.0 <<
			"trade_day" << to_string(_traderday) <<
			"frozen_commission" << 0.0<<
			"abnormal_rate_of_return" << _abnormal_rate_of_return <<
			"daily_rate_of_return" << _daily_rate_of_return <<
			"win_or_lose_flag" << _win_or_lose_flag <<
			"strategy_id" << _name <<
			"deposit" << 0.0<<
			"accounts" << open_document <<
			"314159" << open_document <<
					"position_profit" << 0.0 <<
					"margin" << _used_margin <<
					"risk_ratio" << 0.0<<
					"frozen_commission" << 0.0<<
					"frozen_premium" << 0.0<<
					"available" << 0.0 <<
					"close_profit" << _total_closeprofit <<
					"account_id" << "314159"<<
					"premium" << 0.0<<
					"static_balance" << _static_balance <<
					"balance" << _balance <<
					"deposit" << 0.0<<
					"currency" << "rmb"<<
					"pre_balance" << 0.0 <<
					"commission" << 0.0 <<
					"frozen_margin" << 0.0<<
					"float_profit" << 0.0<<
					"withdraw" << 0.0 <<
					close_document <<
			close_document <<
			"withdraw" << 0.0 <<
		finalize;

	
	acccoll.insert_one(std::move(position_doc));
	WTSLogger::info("Callbacks of insert_one start %lld", newTick->actiontime());

	//accounts���ݿ�
	bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = allcoll.find_one(make_document(kvp("accounts.314159.account_id", "314159")));
	if (maybe_result)
	{
		allcoll.update_one(
			make_document(kvp("accounts", "314159")),
			make_document(kvp("$set", make_document(kvp("available", _total_money),
				kvp("day_profit", _day_profit),
				kvp("close_profit" ,_total_closeprofit),
				kvp("balance", _balance),
				kvp("static_balance", _static_balance),
				kvp("benchmark_rate_of_return", _benchmark_rate_of_return),
				kvp("timestamp", curTime),
				kvp("margin", _used_margin),
				kvp("abnormal_rate_of_return", _abnormal_rate_of_return),
				kvp("daily_rate_of_return", _daily_rate_of_return),
				kvp("win_or_lose_flag", _win_or_lose_flag),
				kvp("strategy_id", _name),
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


	//����day acc
	if (_dayacc_insert_flag)
	{
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
	else //����day acc
	{
		daycoll.update_one(
			make_document(kvp("trade_day", to_string(_traderday)), kvp("accounts.314159.account_id", "314159")),
			make_document(kvp("$set", make_document(kvp("available", _total_money),
													kvp("close_profit", _total_closeprofit),
													kvp("day_profit", _day_profit),
													kvp("balance", _balance),
													kvp("static_balance", _static_balance),
													kvp("benchmark_rate_of_return", _benchmark_rate_of_return),
													kvp("timestamp", curTime),
													kvp("margin", _used_margin),
													kvp("abnormal_rate_of_return", _abnormal_rate_of_return),
													kvp("daily_rate_of_return", _daily_rate_of_return),
													kvp("win_or_lose_flag", _win_or_lose_flag),
													kvp("total_profit" , _total_profit),
													kvp("strategy_id", _name),
														kvp("accounts.314159.margin", _used_margin),
														kvp("accounts.314159.static_balance", _static_balance),
														kvp("accounts.314159.balance", _balance),
														kvp("accounts.314159.close_profit", _total_closeprofit)
				))));
	}
}

void CtaMocker::on_tick(const char* stdCode, WTSTickData* newTick, bool bEmitStrategy /* = true */)
{
	double last_px = _price_map[stdCode];
	double cur_px = newTick->price();
	_price_map[stdCode] = cur_px;

	//cout << _name << endl;
	//������
	if (_traderday < _replayer->get_trading_date())
	{
		_new_trade_day = true;
		_dayacc_insert_flag = true;
		_traderday = _replayer->get_trading_date();
	}

	//���
	_close_price = newTick->presettle();
	//�����
	_settlepx = newTick->price();

	//�ȼ���Ƿ�Ҫ�ź�Ҫ����

	std::string exch_inst = "";
	auto barinstdate = _replayer->get_barinstdate();
	for (auto it = barinstdate->begin(); it != barinstdate->end(); it++)
	{
		bool quit = false;
		for (auto iit=it->second.begin();iit!=it->second.end();iit++)
		{
			if (iit->second._s_date <= newTick->actiondate() && iit->second._e_date >= newTick->actiondate())
			{
				exch_inst = iit->first;
				quit = true;
				break;
			}
		}
		if (quit) break;
	}

	//do_set_position("SHFE.ag.HOT", 100, 30, exch_inst, "", false); //����ʹ��
	{
		auto it = _sig_map.find(stdCode);
		if (it != _sig_map.end())
		{
			//if (sInfo->isInTradingTime(_replayer->get_raw_time(), true))
			{
				const SigInfo& sInfo = it->second;
				double price;
				if (decimal::eq(sInfo._desprice, 0.0))
					price = newTick->price();
				else
					price = sInfo._desprice;

				do_set_position(stdCode, sInfo._volume, price, exch_inst, sInfo._usertag.c_str(), sInfo._triggered);
				_changepos = true;
				_sig_map.erase(it);
			}

		}
	}

	update_dyn_profit(stdCode, newTick->price());

	//�ֱֲ仯��������
	if (_changepos)
	{
		_changepos = false;
		set_dayaccount(stdCode, newTick, bEmitStrategy);
	}

	

	//////////////////////////////////////////////////////////////////////////
	//���������
	if (!_condtions.empty())
	{
		auto it = _condtions.find(stdCode);
		if (it == _condtions.end())
			return;

		const CondList& condList = it->second;
		double curPrice = cur_px;
		for (const CondEntrust& entrust : condList)
		{
			/*
			 * ���������tickģʽ���������Ƚ�
			 * �������û�п���tickģʽ���߼��ͷǳ�����
			 * ��Ϊ�����ز��ʱ��tick���ÿ��ߵ���ģ������ģ����ֱ�Ӱ���Ŀ��۸񴥷����������������
			 * ����Ҫ�õ���һ�ʼ۸񣬺͵�ǰ���¼۸���һ���ȼۣ��õ���߽���ұ߽�
			 * ����ֻ�ܼ���ǰ�����ʼ۸�֮���������ģ�������Ҫ�����ʼ۸񶼼����ж�
			 * �������ǵ���ʱ�����Ŀ��۸������ұ߽�֮�䣬˵��Ŀ��۸������ڼ��ǳ��ֹ��ģ�����Ϊ�۸�ƥ��
			 * �������Ǵ��ڵ�ʱ��������Ҫ�ж��ұ߽磬���Դ��ֵ�Ƿ�������������ȡ��߽���Ŀ������Դ����Ϊ��ǰ��
			 * ��������С�ڵ�ʱ��������Ҫ�ж���߽磬����С��ֵ�Ƿ�������������ȡ�ұ߽���Ŀ�������С����Ϊ��ǰ��
			 */

			double left_px = min(last_px, cur_px);
			double right_px = max(last_px, cur_px);

			bool isMatched = false;	
			if(!_replayer->is_tick_simulated())
			{
				//���tick���ݲ���ģ��ģ���ʹ�����¼۸�
				switch (entrust._alg)
				{
				case WCT_Equal:
					isMatched = decimal::eq(curPrice, entrust._target);
					break;
				case WCT_Larger:
					isMatched = decimal::gt(curPrice, entrust._target);
					break;
				case WCT_LargerOrEqual:
					isMatched = decimal::ge(curPrice, entrust._target);
					break;
				case WCT_Smaller:
					isMatched = decimal::lt(curPrice, entrust._target);
					break;
				case WCT_SmallerOrEqual:
					isMatched = decimal::le(curPrice, entrust._target);
					break;
				default:
					break;
				}
			}
			else
			{
				//���tick������ģ��ģ���Ҫ����һ��
				switch (entrust._alg)
				{
				case WCT_Equal:
					isMatched = decimal::le(left_px, entrust._target) && decimal::ge(right_px, entrust._target);
					curPrice = entrust._target;
					break;
				case WCT_Larger:
					isMatched = decimal::gt(right_px, entrust._target);
					curPrice = max(left_px, entrust._target);
					break;
				case WCT_LargerOrEqual:
					isMatched = decimal::ge(right_px, entrust._target);
					curPrice = max(left_px, entrust._target);
					break;
				case WCT_Smaller:
					isMatched = decimal::lt(left_px, entrust._target);
					curPrice = min(right_px, entrust._target);
					break;
				case WCT_SmallerOrEqual:
					isMatched = decimal::le(left_px, entrust._target);
					curPrice = min(right_px, entrust._target);
					break;
				default:
					break;
				}
			}
			

			if (isMatched)
			{
				double price = curPrice;
				double curQty = stra_get_position(stdCode);
				//_replayer->is_tick_enabled() ? newTick->price() : entrust._target;	//���������tick�ز�,����tick���ݵļ۸�,���û�п���,��ֻ�����������۸�
				WTSLogger::log_dyn_raw("strategy", _name.c_str(), LL_INFO, fmt::format("Condition order triggered[newprice: {}{}{}], instrument: {}, {} {}", cur_px, CMP_ALG_NAMES[entrust._alg], entrust._target, stdCode, ACTION_NAMES[entrust._action], entrust._qty).c_str());
				switch (entrust._action)
				{
				case COND_ACTION_OL:
				{
					if(decimal::lt(curQty, 0))
						append_signal(stdCode, entrust._qty, entrust._usertag, price);
					else
						append_signal(stdCode, curQty + entrust._qty, entrust._usertag, price);
				}
				break;
				case COND_ACTION_CL:
				{
					double maxQty = min(curQty, entrust._qty);
					append_signal(stdCode, curQty - maxQty, entrust._usertag, price);
				}
				break;
				case COND_ACTION_OS:
				{
					if(decimal::gt(curQty, 0))
						append_signal(stdCode, -entrust._qty, entrust._usertag, price);
					else
						append_signal(stdCode, curQty - entrust._qty, entrust._usertag, price);
				}
				break;
				case COND_ACTION_CS:
				{
					double maxQty = min(abs(curQty), entrust._qty);
					append_signal(stdCode, curQty + maxQty, entrust._usertag, price);
				}
				break;
				case COND_ACTION_SP:
				{
					append_signal(stdCode, entrust._qty, entrust._usertag, price);
				}
				default: break;
				}

				//ͬһ��bar�������ͬһ����Լ��������,ֻ���ܴ���һ��
				//��������ֱ�����������
				_condtions.erase(it);
				break;
			}
		}
	}

	if (bEmitStrategy)
		on_tick_updated(stdCode, newTick);
}

void CtaMocker::on_bar_close(const char* code, const char* period, WTSBarStruct* newBar)
{
	if (_strategy)
		_strategy->on_bar(this, code, period, newBar);
}

void CtaMocker::on_tick_updated(const char* code, WTSTickData* newTick)
{
	if (_strategy)
		_strategy->on_tick(this, code, newTick);
}

void CtaMocker::on_calculate(uint32_t curDate, uint32_t curTime)
{
	if (_strategy)
		_strategy->on_schedule(this, curDate, curTime);
}

void CtaMocker::enable_hook(bool bEnabled /* = true */)
{
	_hook_valid = bEnabled;

	WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Calculating hook %s", bEnabled?"enabled":"disabled");
}

void CtaMocker::install_hook()
{
	_has_hook = true;

	WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "CTA hook installed");
}

bool CtaMocker::step_calc()
{
	if (!_has_hook)
	{
		return false;
	}

	//�ܹ���Ϊ4��״̬
	//0-��ʼ״̬��1-oncalc��2-oncalc������3-oncalcdone
	//���ԣ��������0/2����˵��û����ִ���У���Ҫnotify
	bool bNotify = false;
	while (_in_backtest && (_cur_step == 0 || _cur_step == 2))
	{
		_cond_calc.notify_all();
		bNotify = true;
	}

	if(bNotify)
		WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Notify calc thread, wait for calc done");

	if(_in_backtest)
	{
		_wait_calc = true;
		StdUniqueLock lock(_mtx_calc);
		_cond_calc.wait(_mtx_calc);
		_wait_calc = false;
		WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Calc done notified");
		_cur_step = (_cur_step + 1) % 4;

		return true;
	}
	else
	{
		_hook_valid = false;
		WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Backtest exit automatically");
		return false;
	}
}

bool CtaMocker::on_schedule(uint32_t curDate, uint32_t curTime)
{
	_is_in_schedule = true;//��ʼ����,�޸ı��

	_schedule_times++;

	bool isMainUdt = false;
	bool emmited = false;

	for (auto it = _kline_tags.begin(); it != _kline_tags.end(); it++)
	{
		const std::string& key = it->first;
		KlineTag& marker = (KlineTag&)it->second;

		StringVector ay = StrUtil::split(key, "#");
		const char* stdCode = ay[0].c_str();

		if (key == _main_key)
		{
			if (marker._closed)
			{
				isMainUdt = true;
				marker._closed = false;
			}
			else
			{
				isMainUdt = false;
				break;
			}
		}

		WTSSessionInfo* sInfo = _replayer->get_session_info(stdCode, true);

		if (isMainUdt || _kline_tags.empty())
		{
			TimeUtils::Ticker ticker;

			uint32_t offTime = sInfo->offsetTime(curTime);
			if (offTime <= sInfo->getCloseTime(true))
			{
				_condtions.clear();
				if(_has_hook && _hook_valid)
				{
					WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Waiting for resume notify");
					StdUniqueLock lock(_mtx_calc);
					_cond_calc.wait(_mtx_calc);
					WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Calc resumed");
					_cur_step = 1;
				}

				on_calculate(curDate, curTime);

				if (_has_hook && _hook_valid)
				{
					WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Calc done, notify control thread");
					while (_cur_step==1)
						_cond_calc.notify_all();

					WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Waiting for resume notify");
					StdUniqueLock lock(_mtx_calc);
					_cond_calc.wait(_mtx_calc);
					WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Calc resumed");
					_cur_step = 3;
				}

				on_calculate_done(curDate, curTime);
				emmited = true;

				_emit_times++;
				_total_calc_time += ticker.micro_seconds();

				if (_has_hook && _hook_valid)
				{
					WTSLogger::log_dyn("strategy", _name.c_str(), LL_DEBUG, "Calc done, notify control thread");
					while(_cur_step == 3)
						_cond_calc.notify_all();
				}
			}
			else
			{
				WTSLogger::log_dyn("strategy", _name.c_str(), LL_INFO, "%u is not trading time,strategy will not be scheduled", curTime);
			}
			break;
		}
	}

	_is_in_schedule = false;//���Ƚ���,�޸ı��
	return emmited;
}


void CtaMocker::on_session_begin(uint32_t curTDate)
{
}

void CtaMocker::enum_position(FuncEnumCtaPosCallBack cb)
{
	faster_hashmap<std::string, double> desPos;
	for (auto it : _pos_map)
	{
		const char* stdCode = it.first.c_str();
		const PosInfo& pInfo = it.second;
		desPos[stdCode] = pInfo._volume;
	}

	for (auto sit : _sig_map)
	{
		const char* stdCode = sit.first.c_str();
		const SigInfo& sInfo = sit.second;
		desPos[stdCode] = sInfo._volume;
	}

	for (auto v : desPos)
	{
		cb(v.first.c_str(), v.second);
	}
}

void CtaMocker::on_session_end(uint32_t curTDate)
{
	uint32_t curDate = curTDate;//_replayer->get_trading_date();

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

	
	if (_notifier)
		_notifier->notifyFund("BT_FUND", curDate, _fund_info._total_profit, _fund_info._total_dynprofit,
			_fund_info._total_profit + _fund_info._total_dynprofit - _fund_info._total_fees, _fund_info._total_fees);

	//save_data();
}

CondList& CtaMocker::get_cond_entrusts(const char* stdCode)
{
	CondList& ce = _condtions[stdCode];
	return ce;
}

//////////////////////////////////////////////////////////////////////////
//���Խӿ�
void CtaMocker::stra_enter_long(const char* stdCode, double qty, const char* userTag /* = "" */, double limitprice, double stopprice)
{
	_replayer->sub_tick(_context_id, stdCode);
	if (decimal::eq(limitprice, 0.0) && decimal::eq(stopprice, 0.0))	//������Ƕ�̬�µ�ģʽ,��ֱ�Ӵ���
	{
		double curQty = stra_get_position(stdCode);
		//if (curQty < 0)
		if(decimal::lt(curQty, 0))
			//do_set_position(stdCode, qty, userTag, !_is_in_schedule);
			append_signal(stdCode, qty, userTag);
		else
			//do_set_position(stdCode, curQty + qty, userTag, !_is_in_schedule);
			append_signal(stdCode, curQty + qty, userTag);
	}
	else
	{
		CondList& condList = get_cond_entrusts(stdCode);

		CondEntrust entrust;
		strcpy(entrust._code, stdCode);
		strcpy(entrust._usertag, userTag);

		entrust._qty = qty;
		entrust._field = WCF_NEWPRICE;
		if (!decimal::eq(limitprice))
		{
			entrust._target = limitprice;
			entrust._alg = WCT_SmallerOrEqual;
		}
		else if (!decimal::eq(stopprice))
		{
			entrust._target = stopprice;
			entrust._alg = WCT_LargerOrEqual;
		}

		entrust._action = COND_ACTION_OL;

		condList.emplace_back(entrust);
	}
}

void CtaMocker::stra_enter_short(const char* stdCode, double qty, const char* userTag /* = "" */, double limitprice, double stopprice)
{
	_replayer->sub_tick(_context_id, stdCode);
	if (decimal::eq(limitprice, 0.0) && decimal::eq(stopprice, 0.0))	//������Ƕ�̬�µ�ģʽ,��ֱ�Ӵ���
	{
		double curQty = stra_get_position(stdCode);
		//if (curQty > 0)
		if(decimal::gt(curQty, 0))
			//do_set_position(stdCode, -qty, userTag, !_is_in_schedule);
			append_signal(stdCode, -qty, userTag);
		else
			//do_set_position(stdCode, curQty - qty, userTag, !_is_in_schedule);
			append_signal(stdCode, curQty - qty, userTag);

	}
	else
	{
		CondList& condList = get_cond_entrusts(stdCode);

		CondEntrust entrust;
		strcpy(entrust._code, stdCode);
		strcpy(entrust._usertag, userTag);

		entrust._qty = qty;
		entrust._field = WCF_NEWPRICE;
		if (!decimal::eq(limitprice))
		{
			entrust._target = limitprice;
			entrust._alg = WCT_LargerOrEqual;
		}
		else if (!decimal::eq(stopprice))
		{
			entrust._target = stopprice;
			entrust._alg = WCT_SmallerOrEqual;
		}

		entrust._action = COND_ACTION_OS;

		condList.emplace_back(entrust);
	}
}

void CtaMocker::stra_exit_long(const char* stdCode, double qty, const char* userTag /* = "" */, double limitprice, double stopprice)
{
	if (decimal::eq(limitprice, 0.0) && decimal::eq(stopprice, 0.0))	//������Ƕ�̬�µ�ģʽ,��ֱ�Ӵ���
	{
		double curQty = stra_get_position(stdCode);
		//if (curQty <= 0)
		if(decimal::le(curQty, 0))
			return;

		double maxQty = min(curQty, qty);
		//do_set_position(stdCode, curQty - maxQty, userTag, !_is_in_schedule);
		append_signal(stdCode, curQty - qty, userTag);
	}
	else
	{
		CondList& condList = get_cond_entrusts(stdCode);

		CondEntrust entrust;
		strcpy(entrust._code, stdCode);
		strcpy(entrust._usertag, userTag);

		entrust._qty = qty;
		entrust._field = WCF_NEWPRICE;
		if (!decimal::eq(limitprice))
		{
			entrust._target = limitprice;
			entrust._alg = WCT_LargerOrEqual;
		}
		else if (!decimal::eq(stopprice))
		{
			entrust._target = stopprice;
			entrust._alg = WCT_SmallerOrEqual;
		}

		entrust._action = COND_ACTION_CL;

		condList.emplace_back(entrust);
	}
}

void CtaMocker::stra_exit_short(const char* stdCode, double qty, const char* userTag /* = "" */, double limitprice, double stopprice)
{
	if (decimal::eq(limitprice, 0.0) && decimal::eq(stopprice, 0.0))	//������Ƕ�̬�µ�ģʽ,��ֱ�Ӵ���
	{
		double curQty = stra_get_position(stdCode);
		//if (curQty >= 0)
		if(decimal::ge(curQty, 0))
			return;

		double maxQty = min(abs(curQty), qty);
		//do_set_position(stdCode, curQty + maxQty, userTag, !_is_in_schedule);
		append_signal(stdCode, curQty + maxQty, userTag);
	}
	else
	{
		CondList& condList = get_cond_entrusts(stdCode);

		CondEntrust entrust;
		strcpy(entrust._code, stdCode);
		strcpy(entrust._usertag, userTag);

		entrust._qty = qty;
		entrust._field = WCF_NEWPRICE;
		if (!decimal::eq(limitprice))
		{
			entrust._target = limitprice;
			entrust._alg = WCT_SmallerOrEqual;
		}
		else if (!decimal::eq(stopprice))
		{
			entrust._target = stopprice;
			entrust._alg = WCT_LargerOrEqual;
		}

		entrust._action = COND_ACTION_CS;

		condList.emplace_back(entrust);
	}
}

double CtaMocker::stra_get_price(const char* stdCode)
{
	if (_replayer)
		return _replayer->get_cur_price(stdCode);

	return 0.0;
}

void CtaMocker::stra_set_position(const char* stdCode, double qty, const char* userTag /* = "" */, double limitprice /* = 0.0 */, double stopprice /* = 0.0 */)
{
	_replayer->sub_tick(_context_id, stdCode);
	if (decimal::eq(limitprice, 0.0) && decimal::eq(stopprice, 0.0))	//������Ƕ�̬�µ�ģʽ,��ֱ�Ӵ���
	{
		//do_set_position(stdCode, qty, userTag, !_is_in_schedule);
		append_signal(stdCode, qty, userTag);
	}
	else
	{
		CondList& condList = get_cond_entrusts(stdCode);

		double curQty = stra_get_position(stdCode);
		//���Ŀ���λ�͵�ǰ��λ��һ�µģ���������������
		if (decimal::eq(curQty, qty))
			return;

		bool isBuy = decimal::gt(qty, curQty);


		CondEntrust entrust;
		strcpy(entrust._code, stdCode);
		strcpy(entrust._usertag, userTag);
		entrust._qty = qty;
		entrust._field = WCF_NEWPRICE;
		if (!decimal::eq(limitprice))
		{
			entrust._target = limitprice;
			entrust._alg = isBuy ? WCT_SmallerOrEqual : WCT_LargerOrEqual;
		}
		else if (!decimal::eq(stopprice))
		{
			entrust._target = stopprice;
			entrust._alg = isBuy ? WCT_LargerOrEqual : WCT_SmallerOrEqual;
		}

		entrust._action = COND_ACTION_SP;

		condList.emplace_back(entrust);
	}
}

void CtaMocker::append_signal(const char* stdCode, double qty, const char* userTag /* = "" */, double price/* = 0.0*/)
{
	double curPx = _price_map[stdCode];

	SigInfo& sInfo = _sig_map[stdCode];
	sInfo._volume = qty;
	sInfo._sigprice = curPx;
	sInfo._desprice = price;
	sInfo._usertag = userTag;
	sInfo._gentime = (uint64_t)_replayer->get_date() * 1000000000 + (uint64_t)_replayer->get_raw_time() * 100000 + _replayer->get_secs();
	sInfo._triggered = !_is_in_schedule;

	log_signal(stdCode, qty, curPx, sInfo._gentime, userTag);

	//save_data();
}

void CtaMocker::insert_his_position(DetailInfo dInfo, PosInfo pInfo, double fee, std::string exch_id, std::string inst_id, uint64_t curTime)
{
	auto db = _replayer->_client["lsqt_db"];
	auto _poscoll_1 = db["his_positions"];
	bsoncxx::document::value position_doc = document{} << finalize;
	std::string exch_inst = exch_id;
	exch_inst += "::";
	exch_inst += inst_id;
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
	c1_mtx.lock();
	auto result = _poscoll_1.insert_one(move(position_doc));
	bsoncxx::oid oid = result->inserted_id().get_oid().value;
	//std::cout << "insert one:" << oid.to_string() << std::endl;
	c1_mtx.unlock();

}

void CtaMocker::insert_his_trades(DetailInfo dInfo, PosInfo pInfo, double fee, std::string exch_id, std::string inst_id, uint64_t curTime)
{
	auto db = _replayer->_client["lsqt_db"];
	auto _poscoll_1 = db["his_trades"];
	bsoncxx::document::value position_doc = document{} << finalize;
	std::string exch_inst = exch_id;
	exch_inst += "::";
	exch_inst += inst_id;
	if (dInfo._long)
	{
		position_doc = document{} << "exchange_trade_id" << "111111" <<
			"account_id" << "111111" <<
			"commission" << 0.0 <<
			"direction" << 1 <<
			"exchange_id" << exch_id <<
			"exchange_order_id" << "123456" <<
			"instrument_id" << inst_id <<
			"offset" << "" <<
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
			"offset" << "" <<
			"order_id" << "123456" <<
			"price" << dInfo._price <<
			"seqno" << 0 <<
			"strategy_id" << _name <<
			"trade_date_time" << _replayer->StringToDatetime(to_string(curTime)) * 1000 <<
			"volume" << dInfo._volume <<
			finalize;
	}
	c1_mtx.lock();
	auto result = _poscoll_1.insert_one(move(position_doc));
	bsoncxx::oid oid = result->inserted_id().get_oid().value;
	//std::cout << "insert one:" << oid.to_string() << std::endl;
	c1_mtx.unlock();
}

void CtaMocker::do_set_position(const char* stdCode, double qty, double price /* = 0.0 */, std::string instid /*=""*/,const char* userTag /* = "" */, bool bTriggered /* = false */)
{
	//mongocxx::instance instance{};
	/*mongocxx::uri _uri("mongodb://192.168.214.199:27017");
	mongocxx::client _client(_uri);*/
	/*auto db = _replayer->_client["lsqt_db"];
	auto _poscoll_1 = db["test_positions"];*/
	//_poscoll_2 = db["his_trades"];
	//_poscoll_3 = db["his_orders"];//
	PosInfo& pInfo = _pos_map[stdCode];
	double curPx = price;
	if (decimal::eq(price, 0.0))
		curPx = _price_map[stdCode];
	uint64_t curTm = (uint64_t)_replayer->get_date() * 10000 + _replayer->get_min_time();
	uint32_t curTDate = _replayer->get_trading_date();
	uint64_t curTime = (uint64_t)_replayer->get_date() * 1000000 + _replayer->get_min_time() * 100 + _replayer->get_secs();

	//����������ò�����
	if (decimal::eq(pInfo._volume, qty))
		return;


	WTSCommodityInfo* commInfo = _replayer->get_commodity_info(stdCode);

	//�ɽ���
	std::string exchid = commInfo->getExchg();
	std::string exch_inst = exchid + "::";
	exch_inst += instid;

	double trdPx = curPx;

	double diff = qty - pInfo._volume;
	bool isBuy = decimal::gt(diff, 0.0);

	//��֤�����Ƿ�ɽ�
	double tempfee = _replayer->calc_fee(stdCode, trdPx, abs(diff), 0);
	double tempmargin = _margin_rate * _cur_multiplier * _close_price * abs(diff);
	if (!decimal::gt(_total_money - (tempmargin + tempfee), 0))
	{
		WTSLogger::log_dyn("strategy", _name.c_str(), LL_WARN, "error:�ʽ��˻�����");
		return;
	}


	if (decimal::gt(pInfo._volume*diff, 0))//��ǰ�ֲֺͲ�λ�仯����һ��, ����һ����ϸ, ������������
	{
		pInfo._volume = qty;

		if (_slippage != 0)
		{
			trdPx += _slippage * commInfo->getPriceTick() * (isBuy ? 1 : -1);
		}

		DetailInfo dInfo;
		dInfo._long = decimal::gt(qty, 0);
		dInfo._price = trdPx;
		dInfo._volume = abs(diff);
		dInfo._opentime = curTm;
		dInfo._opentdate = curTDate;
		dInfo._margin = _margin_rate * _cur_multiplier * _close_price * abs(diff);
		strcpy(dInfo._opentag, userTag);
		dInfo._open_barno = _schedule_times;
		pInfo._details.emplace_back(dInfo);
		pInfo._last_entertime = curTm;

		double fee = _replayer->calc_fee(stdCode, trdPx, abs(diff), 0);
		_fund_info._total_fees += fee;

		//��֤�����
		if (decimal::gt(_total_money - (dInfo._margin + fee), 0))
		{
			_total_money -= dInfo._margin;
			_total_money -= fee;
		}
		else
		{
			WTSLogger::log_dyn("strategy", _name.c_str(), LL_WARN, "error:�ʽ��˻�����");
		}

		_used_margin += dInfo._margin;

		if (_name != "")
		{
			insert_his_position(dInfo, pInfo, fee, exchid, instid, curTime);
			insert_his_trades(dInfo, pInfo, fee, exchid, instid, curTime);
		}

		log_trade(stdCode, dInfo._long, true, curTm, trdPx, abs(diff), userTag, fee, _schedule_times);
	}
	else if(decimal::lt(pInfo._volume * diff, 0))
	{//�ֲַ���Ͳ�λ�仯����һ��,��Ҫƽ��
		double left = abs(diff);
		bool isBuy = decimal::gt(diff, 0.0);
		if (_slippage != 0)
			trdPx += _slippage * commInfo->getPriceTick() * (isBuy ? 1 : -1);

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
			pInfo._dynprofit = pInfo._dynprofit * dInfo._volume / (dInfo._volume + maxQty);//��ӯҲҪ���ȱ�����

			pInfo._last_exittime = curTm;
			_fund_info._total_profit += profit;

			double fee = _replayer->calc_fee(stdCode, trdPx, maxQty, dInfo._opentdate == curTDate ? 2 : 1);
			_fund_info._total_fees += fee;
			//����д�ɽ���¼


			//�ͷű�֤��
			double cur_margin = _margin_rate * _cur_multiplier * _close_price * maxQty;

			_total_money += profit;
			_total_money -= fee;
			_total_money += cur_margin;

			_used_margin -= cur_margin;

			dInfo._margin -= cur_margin;

			if (_name != "")
			{
				insert_his_position(dInfo, pInfo, fee, exchid, instid, curTime);
				insert_his_trades(dInfo, pInfo, fee, exchid, instid, curTime);
			}

			//����д�ɽ���¼
			log_trade(stdCode, dInfo._long, false, curTm, trdPx, maxQty, userTag, fee, _schedule_times);

			//����дƽ�ּ�¼
			log_close(stdCode, dInfo._long, dInfo._opentime, dInfo._price, curTm, trdPx, maxQty, profit, maxProf, maxLoss,
				_total_closeprofit - _fund_info._total_fees, dInfo._opentag, userTag, dInfo._open_barno, _schedule_times);

			if (left == 0)
				break;
		}

		//��Ҫ������Ѿ�ƽ�������ϸ
		while (count > 0)
		{
			auto it = pInfo._details.begin();
			pInfo._details.erase(it);
			count--;
		}

		//���,�������ʣ���,����Ҫ������
		if (left > 0)
		{
			left = left * qty / abs(qty);

			DetailInfo dInfo;
			dInfo._long = decimal::gt(qty, 0);
			dInfo._price = trdPx;
			dInfo._volume = abs(left);
			dInfo._opentime = curTm;
			dInfo._opentdate = curTDate;
			dInfo._open_barno = _schedule_times;
			strcpy(dInfo._opentag, userTag);
			pInfo._details.emplace_back(dInfo);

			double fee = _replayer->calc_fee(stdCode, trdPx, abs(left), 0);
			_fund_info._total_fees += fee;
			//_engine->mutate_fund(fee, FFT_Fee);
			
			//��Ӽ�ȥ����
			_total_money -= fee;

			if (_name != "")
			{
				insert_his_position(dInfo, pInfo, fee, exchid, instid, curTime);
				insert_his_trades(dInfo, pInfo, fee, exchid, instid, curTime);
			}

			log_trade(stdCode, dInfo._long, true, curTm, trdPx, abs(left), userTag, fee, _schedule_times);

			pInfo._last_entertime = curTm;
		}
	}
}


WTSKlineSlice* CtaMocker::stra_get_bars(const char* stdCode, const char* period, uint32_t count, bool isMain /* = false */)
{
	std::string key = StrUtil::printf("%s#%s", stdCode, period);
	if (isMain)
	{
		if (_main_key.empty())
			_main_key = key;
		else if (_main_key != key)
			throw std::runtime_error("Main k bars can only be setup once");
	}

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

	WTSKlineSlice* kline = _replayer->get_kline_slice(stdCode, basePeriod.c_str(), count, times, isMain);

	bool bFirst = (_kline_tags.find(key) == _kline_tags.end());
	KlineTag& tag = _kline_tags[key];
	tag._closed = false;

	if (kline)
	{
		//double lastClose = kline->close(-1);
		CodeHelper::CodeInfo cInfo;
		CodeHelper::extractStdCode(stdCode, cInfo);

		std::string realCode = stdCode;
		if(cInfo._category == CC_Stock && cInfo.isExright())
		{
			realCode = cInfo._exchg;
			realCode += ".";
			realCode += cInfo._code;
		}
		_replayer->sub_tick(id(), realCode.c_str());
	}

	return kline;
}

WTSTickSlice* CtaMocker::stra_get_ticks(const char* stdCode, uint32_t count)
{
	return _replayer->get_tick_slice(stdCode, count);
}

WTSTickData* CtaMocker::stra_get_last_tick(const char* stdCode)
{
	return _replayer->get_last_tick(stdCode);
}

void CtaMocker::stra_sub_ticks(const char* code)
{
	_replayer->sub_tick(_context_id, code);
}

WTSCommodityInfo* CtaMocker::stra_get_comminfo(const char* stdCode)
{
	return _replayer->get_commodity_info(stdCode);
}

uint32_t CtaMocker::stra_get_tdate()
{
	return _replayer->get_trading_date();
}

uint32_t CtaMocker::stra_get_date()
{
	return _replayer->get_date();
}

uint32_t CtaMocker::stra_get_time()
{
	return _replayer->get_min_time();
}

double CtaMocker::stra_get_fund_data(int flag)
{
	switch (flag)
	{
	case 0:
		return _fund_info._total_profit - _fund_info._total_fees + _fund_info._total_dynprofit;
	case 1:
		return _fund_info._total_profit;
	case 2:
		return _fund_info._total_dynprofit;
	case 3:
		return _fund_info._total_fees;
	default:
		return 0.0;
	}
}

void CtaMocker::stra_log_info(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	WTSLogger::vlog_dyn("strategy", _name.c_str(), LL_INFO, fmt, args);
	va_end(args);
}

void CtaMocker::stra_log_debug(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	WTSLogger::vlog_dyn("strategy", _name.c_str(), LL_DEBUG, fmt, args);
	va_end(args);
}

void CtaMocker::stra_log_error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	WTSLogger::vlog_dyn("strategy", _name.c_str(), LL_ERROR, fmt, args);
	va_end(args);
}

const char* CtaMocker::stra_load_user_data(const char* key, const char* defVal /*= ""*/)
{
	auto it = _user_datas.find(key);
	if (it != _user_datas.end())
		return it->second.c_str();

	return defVal;
}

void CtaMocker::stra_save_user_data(const char* key, const char* val)
{
	_user_datas[key] = val;
	_ud_modified = true;
}

uint64_t CtaMocker::stra_get_first_entertime(const char* stdCode)
{
	auto it = _pos_map.find(stdCode);
	if (it == _pos_map.end())
		return INVALID_UINT64;

	const PosInfo& pInfo = it->second;
	if (pInfo._details.empty())
		return INVALID_UINT64;

	return pInfo._details[0]._opentime;
}

uint64_t CtaMocker::stra_get_last_entertime(const char* stdCode)
{
	auto it = _pos_map.find(stdCode);
	if (it == _pos_map.end())
		return INVALID_UINT64;

	const PosInfo& pInfo = it->second;
	if (pInfo._details.empty())
		return INVALID_UINT64;

	return pInfo._details[pInfo._details.size() - 1]._opentime;
}

uint64_t CtaMocker::stra_get_last_exittime(const char* stdCode)
{
	auto it = _pos_map.find(stdCode);
	if (it == _pos_map.end())
		return INVALID_UINT64;

	const PosInfo& pInfo = it->second;
	return pInfo._last_exittime;
}

double CtaMocker::stra_get_last_enterprice(const char* stdCode)
{
	auto it = _pos_map.find(stdCode);
	if (it == _pos_map.end())
		return INVALID_DOUBLE;

	const PosInfo& pInfo = it->second;
	if (pInfo._details.empty())
		return INVALID_DOUBLE;

	return pInfo._details[pInfo._details.size() - 1]._price;
}

double CtaMocker::stra_get_position(const char* stdCode, const char* userTag /* = "" */)
{
	auto it = _pos_map.find(stdCode);
	if (it == _pos_map.end())
		return 0;

	const PosInfo& pInfo = it->second;
	if (strlen(userTag) == 0)
		return pInfo._volume;

	for (auto it = pInfo._details.begin(); it != pInfo._details.end(); it++)
	{
		const DetailInfo& dInfo = (*it);
		if (strcmp(dInfo._opentag, userTag) != 0)
			continue;

		return dInfo._volume;
	}

	return 0;
}

double CtaMocker::stra_get_position_avgpx(const char* stdCode)
{
	auto it = _pos_map.find(stdCode);
	if (it == _pos_map.end())
		return 0;

	const PosInfo& pInfo = it->second;
	if (pInfo._volume == 0)
		return 0.0;

	double amount = 0.0;
	for (auto dit = pInfo._details.begin(); dit != pInfo._details.end(); dit++)
	{
		const DetailInfo& dInfo = *dit;
		amount += dInfo._price*dInfo._volume;
	}

	return amount / pInfo._volume;
}

double CtaMocker::stra_get_position_profit(const char* stdCode)
{
	auto it = _pos_map.find(stdCode);
	if (it == _pos_map.end())
		return 0;

	const PosInfo& pInfo = it->second;
	return pInfo._dynprofit;
}

uint64_t CtaMocker::stra_get_detail_entertime(const char* stdCode, const char* userTag)
{
	auto it = _pos_map.find(stdCode);
	if (it == _pos_map.end())
		return 0;

	const PosInfo& pInfo = it->second;
	for (auto it = pInfo._details.begin(); it != pInfo._details.end(); it++)
	{
		const DetailInfo& dInfo = (*it);
		if (strcmp(dInfo._opentag, userTag) != 0)
			continue;

		return dInfo._opentime;
	}

	return 0;
}

double CtaMocker::stra_get_detail_cost(const char* stdCode, const char* userTag)
{
	auto it = _pos_map.find(stdCode);
	if (it == _pos_map.end())
		return 0;

	const PosInfo& pInfo = it->second;
	for (auto it = pInfo._details.begin(); it != pInfo._details.end(); it++)
	{
		const DetailInfo& dInfo = (*it);
		if (strcmp(dInfo._opentag, userTag) != 0)
			continue;

		return dInfo._price;
	}

	return 0.0;
}

double CtaMocker::stra_get_detail_profit(const char* stdCode, const char* userTag, int flag /* = 0 */)
{
	auto it = _pos_map.find(stdCode);
	if (it == _pos_map.end())
		return 0;

	const PosInfo& pInfo = it->second;
	for (auto it = pInfo._details.begin(); it != pInfo._details.end(); it++)
	{
		const DetailInfo& dInfo = (*it);
		if (strcmp(dInfo._opentag, userTag) != 0)
			continue;

		if (flag == 0)
			return dInfo._profit;
		else if (flag > 0)
			return dInfo._max_profit;
		else
			return dInfo._max_loss;
	}

	return 0.0;
}

void  CtaMocker::set_initacc(double money)
{
	init_money = money;
	_total_money = init_money;	
	_static_balance = init_money;
}


