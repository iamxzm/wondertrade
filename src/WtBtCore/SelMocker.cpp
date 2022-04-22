/*!
* \file MfMocker.cpp
* \project	WonderTrader
*
* \author Wesley
* \date 2020/03/30
*
* \brief
*/
#include "SelMocker.h"
#include "WtHelper.h"
#include "HisDataReplayer.h"

#include <exception>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <boost/filesystem.hpp>

#include "../Share/StdUtils.hpp"
#include "../Share/StrUtil.hpp"
#include "../Includes/WTSContractInfo.hpp"
#include "../Includes/WTSSessionInfo.hpp"
#include "../Includes/WTSTradeDef.hpp"
#include "../Share/decimal.h"
#include "../Includes/WTSVariant.hpp"

#include "../WTSTools/WTSLogger.h"

namespace rj = rapidjson;

inline uint32_t makeSelCtxId()
{
	static std::atomic<uint32_t> _auto_context_id{ 3000 };
	return _auto_context_id.fetch_add(1);
}


SelMocker::SelMocker(HisDataReplayer* replayer, const char* name, int32_t slippage /* = 0 */)
	: ISelStraCtx(name)
	, _replayer(replayer)
	, _total_calc_time(0)
	, _emit_times(0)
	, _is_in_schedule(false)
	, _ud_modified(false)
	, _strategy(NULL)
	, _slippage(slippage)
{
	_context_id = makeSelCtxId();
}


SelMocker::~SelMocker()
{
}

void SelMocker::dump_outputs()
{
	std::string folder = WtHelper::getOutputDir();
	folder += _name;
	folder += "/";
	boost::filesystem::create_directories(folder.c_str());

	std::string filename = folder + "trades.csv";
	std::string content = "code,time,direct,action,price,qty,tag,fee\n";
	content += _trade_logs.str();
	StdFile::write_file_content(filename.c_str(), (void*)content.c_str(), content.size());

	filename = folder + "closes.csv";
	content = "code,direct,opentime,openprice,closetime,closeprice,qty,profit,totalprofit,entertag,exittag\n";
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

void SelMocker::log_signal(const char* stdCode, double target, double price, uint64_t gentime, const char* usertag /* = "" */)
{
	_sig_logs << stdCode << "," << target << "," << price << "," << gentime << "," << usertag << "\n";
}

void SelMocker::log_trade(const char* stdCode, bool isLong, bool isOpen, uint64_t curTime, double price, double qty, const char* userTag, double fee)
{
	_trade_logs << stdCode << "," << curTime << "," << (isLong ? "LONG" : "SHORT") << "," << (isOpen ? "OPEN" : "CLOSE") << "," << price << "," << qty << "," << userTag << "," << fee << "\n";
}

void SelMocker::log_close(const char* stdCode, bool isLong, uint64_t openTime, double openpx, uint64_t closeTime, double closepx, double qty,
	double profit, double totalprofit /* = 0 */, const char* enterTag /* = "" */, const char* exitTag /* = "" */)
{
	_close_logs << stdCode << "," << (isLong ? "LONG" : "SHORT") << "," << openTime << "," << openpx
		<< "," << closeTime << "," << closepx << "," << qty << "," << profit << ","
		<< totalprofit << "," << enterTag << "," << exitTag << "\n";
}

bool SelMocker::init_sel_factory(WTSVariant* cfg)
{
	if (cfg == NULL)
		return false;

	const char* module = cfg->getCString("module");

	DllHandle hInst = DLLHelper::load_library(module);
	if (hInst == NULL)
		return false;

	FuncCreateSelStraFact creator = (FuncCreateSelStraFact)DLLHelper::get_symbol(hInst, "createSelStrategyFact");
	if (creator == NULL)
	{
		DLLHelper::free_library(hInst);
		return false;
	}

	_factory._module_inst = hInst;
	_factory._module_path = module;
	_factory._creator = creator;
	_factory._remover = (FuncDeleteSelStraFact)DLLHelper::get_symbol(hInst, "deleteSelStrategyFact");
	_factory._fact = _factory._creator();

	WTSVariant* cfgStra = cfg->get("strategy");
	if (cfgStra)
	{
		_strategy = _factory._fact->createStrategy(cfgStra->getCString("name"), cfgStra->getCString("id"));
		if (_strategy)
		{
			WTSLogger::info("Strategy %s.%s created,strategy ID: %s", _factory._fact->getName(), _strategy->getName(), _strategy->id());
		}
		_strategy->init(cfgStra->get("params"));
		_name = _strategy->id();
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
//IDataSink
void SelMocker::handle_init()
{
	this->on_init();
}

void SelMocker::handle_bar_close(const char* stdCode, const char* period, uint32_t times, WTSBarStruct* newBar)
{
	this->on_bar(stdCode, period, times, newBar);
}

void SelMocker::handle_schedule(uint32_t uDate, uint32_t uTime)
{
	uint32_t nextTime = TimeUtils::getNextMinute(uTime, 1);
	if (nextTime < uTime)
		uDate = TimeUtils::getNextDate(uDate);
	this->on_schedule(uDate, uTime, nextTime);
}

void SelMocker::handle_session_begin(uint32_t uCurDate)
{
	this->on_session_begin(uCurDate);
}

void SelMocker::handle_session_end(uint32_t uCurDate)
{
	this->on_session_end(uCurDate);
}

void SelMocker::handle_replay_done()
{
	WTSLogger::log_dyn_raw("strategy", _name.c_str(), LL_INFO, fmt::format("Strategy has been scheduled for {} times,totally taking {} microsecs,average of {} microsecs",
		_emit_times, _total_calc_time, _total_calc_time / _emit_times).c_str());

	dump_outputs();

	this->on_bactest_end();
}

void SelMocker::handle_tick(const char* stdCode, WTSTickData* curTick)
{
	this->on_tick(stdCode, curTick, true);
}


//////////////////////////////////////////////////////////////////////////
//�ص�����
void SelMocker::on_bar(const char* stdCode, const char* period, uint32_t times, WTSBarStruct* newBar)
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

void SelMocker::on_init()
{
	if (_strategy)
		_strategy->on_init(this);

	WTSLogger::info("SEL Strategy initialized, with slippage: %d", _slippage);
}

void SelMocker::update_dyn_profit(const char* stdCode, double price)
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
}

void SelMocker::on_tick(const char* stdCode, WTSTickData* newTick, bool bEmitStrategy /* = true */)
{
	_price_map[stdCode].first = newTick->price();
	_price_map[stdCode].second = (uint64_t)newTick->actiondate() * 1000000000 + newTick->actiontime();

	//�ȼ���Ƿ�Ҫ�ź�Ҫ����
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
				do_set_position(stdCode, sInfo._volume, price, sInfo._usertag.c_str(), sInfo._triggered);
				_sig_map.erase(it);
			}

		}
	}

	update_dyn_profit(stdCode, newTick->price());

	if (bEmitStrategy)
		on_tick_updated(stdCode, newTick);
}

void SelMocker::on_bar_close(const char* code, const char* period, WTSBarStruct* newBar)
{
	if (_strategy)
		_strategy->on_bar(this, code, period, newBar);
}

void SelMocker::on_tick_updated(const char* code, WTSTickData* newTick)
{
	if (_strategy)
		_strategy->on_tick(this, code, newTick);
}

void SelMocker::on_strategy_schedule(uint32_t curDate, uint32_t curTime)
{
	if (_strategy)
		_strategy->on_schedule(this, curDate, curTime);
}


bool SelMocker::on_schedule(uint32_t curDate, uint32_t curTime, uint32_t fireTime)
{
	_is_in_schedule = true;//��ʼ����,�޸ı��

	TimeUtils::Ticker ticker;
	on_strategy_schedule(curDate, curTime);

	faster_hashset<std::string> to_clear;
	for(auto& v : _pos_map)
	{
		const PosInfo& pInfo = v.second;
		const char* code = v.first.c_str();
		if(_sig_map.find(code) == _sig_map.end() && !decimal::eq(pInfo._volume, 0.0))
		{
			//�µ��ź���û�иóֲ�,��Ҫ���
			to_clear.insert(code);
		}
	}

	for(const std::string& code : to_clear)
	{
		append_signal(code.c_str(), 0, "autoexit");
	}

	_emit_times++;
	_total_calc_time += ticker.micro_seconds();

	_is_in_schedule = false;//���Ƚ���,�޸ı��
	return true;
}

void SelMocker::on_session_begin(uint32_t curTDate)
{
}

void SelMocker::enum_position(FuncEnumSelPositionCallBack cb)
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

void SelMocker::on_session_end(uint32_t curTDate)
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

	//save_data();
}

//////////////////////////////////////////////////////////////////////////
//���Խӿ�
double SelMocker::stra_get_price(const char* stdCode)
{
	if (_replayer)
		return _replayer->get_cur_price(stdCode);

	return 0.0;
}

void SelMocker::stra_set_position(const char* stdCode, double qty, const char* userTag /* = "" */)
{
	_replayer->sub_tick(id(), stdCode);
	append_signal(stdCode, qty, userTag);
}

void SelMocker::append_signal(const char* stdCode, double qty, const char* userTag /* = "" */, double price/* = 0.0*/)
{
	double curPx = _price_map[stdCode].first;

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

void SelMocker::do_set_position(const char* stdCode, double qty, double price /* = 0.0 */, const char* userTag /* = "" */, bool bTriggered /* = false */)
{
	PosInfo& pInfo = _pos_map[stdCode];
	double curPx = price;
	if (decimal::eq(price, 0.0))
		curPx = _price_map[stdCode].first;
	uint64_t curTm = (uint64_t)_replayer->get_date() * 10000 + _replayer->get_min_time();
	uint32_t curTDate = _replayer->get_trading_date();

	if (decimal::eq(pInfo._volume, qty))
		return;

	WTSCommodityInfo* commInfo = _replayer->get_commodity_info(stdCode);

	//�ɽ���
	double trdPx = curPx;

	if (decimal::gt(pInfo._volume*qty, 0))//��ǰ�ֲֺ�Ŀ���λ����һ��,����һ����ϸ,������������
	{
		//Ŀ���λ����ֵ���ڵ�ǰ��λ����ֵ,���Ǽ�������,����һ����¼����
		if (decimal::gt(abs(qty), abs(pInfo._volume)))
		{
			double diff = abs(qty - pInfo._volume);
			pInfo._volume = qty;

			if (_slippage != 0)
			{
				bool isBuy = decimal::gt(qty, 0.0);
				trdPx += _slippage * commInfo->getPriceTick()*(isBuy ? 1 : -1);
			}

			DetailInfo dInfo;
			dInfo._long = decimal::gt(qty, 0);
			dInfo._price = trdPx;
			dInfo._volume = diff;
			dInfo._opentime = curTm;
			dInfo._opentdate = curTDate;
			strcpy(dInfo._opentag, userTag);
			pInfo._details.emplace_back(dInfo);

			double fee = _replayer->calc_fee(stdCode, trdPx, abs(diff), 0);
			_fund_info._total_fees += fee;
			
			log_trade(stdCode, dInfo._long, true, curTm, trdPx, abs(diff), userTag, fee);
		}
		else
		{
			//Ŀ���λ����ֵС�ڵ�ǰ��λ����ֵ,��Ҫƽ��
			double left = abs(qty - pInfo._volume);

			if (_slippage != 0)
			{
				//ƽ�ֵĻ�,�����Ƿ���
				bool isBuy = !decimal::gt(qty, 0.0);
				trdPx += _slippage * commInfo->getPriceTick()*(isBuy ? 1 : -1);
			}

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

				dInfo._volume -= maxQty;
				left -= maxQty;

				if (decimal::eq(dInfo._volume, 0))
					count++;

				double profit = (trdPx - dInfo._price) * maxQty * commInfo->getVolScale();
				if (!dInfo._long)
					profit *= -1;
				pInfo._closeprofit += profit;
				pInfo._dynprofit = pInfo._dynprofit*dInfo._volume / (dInfo._volume + maxQty);//��ӯҲҪ���ȱ�����
				_fund_info._total_profit += profit;

				double fee = _replayer->calc_fee(stdCode, trdPx, maxQty, dInfo._opentdate == curTDate ? 2 : 1);
				_fund_info._total_fees += fee;
				
				//����д�ɽ���¼
				log_trade(stdCode, dInfo._long, false, curTm, trdPx, maxQty, userTag, fee);
				//����дƽ�ּ�¼
				log_close(stdCode, dInfo._long, dInfo._opentime, dInfo._price, curTm, trdPx, maxQty, profit, pInfo._closeprofit, dInfo._opentag, userTag);

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
		}
	}
	else
	{//�ֲַ����Ŀ���λ����һ��,��Ҫƽ��
		double left = abs(pInfo._volume) + abs(qty);

		if (_slippage != 0)
		{
			bool isBuy = decimal::gt(qty, 0.0);
			trdPx += _slippage * commInfo->getPriceTick()*(isBuy ? 1 : -1);
		}

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

			dInfo._volume -= maxQty;
			left -= maxQty;

			if (decimal::eq(dInfo._volume, 0))
				count++;

			double profit = (trdPx - dInfo._price) * maxQty * commInfo->getVolScale();
			if (!dInfo._long)
				profit *= -1;
			pInfo._closeprofit += profit;
			pInfo._dynprofit = pInfo._dynprofit*dInfo._volume / (dInfo._volume + maxQty);//��ӯҲҪ���ȱ�����
			_fund_info._total_profit += profit;

			double fee = _replayer->calc_fee(stdCode, trdPx, maxQty, dInfo._opentdate == curTDate ? 2 : 1);
			_fund_info._total_fees += fee;
			//����д�ɽ���¼
			log_trade(stdCode, dInfo._long, false, curTm, trdPx, maxQty, userTag, fee);
			//����дƽ�ּ�¼
			log_close(stdCode, dInfo._long, dInfo._opentime, dInfo._price, curTm, trdPx, maxQty, profit, pInfo._closeprofit, dInfo._opentag, userTag);

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
			strcpy(dInfo._opentag, userTag);
			pInfo._details.emplace_back(dInfo);

			//TODO: 
			//���ﻹ��Ҫдһ�ʳɽ���¼
			double fee = _replayer->calc_fee(stdCode, trdPx, abs(qty), 0);
			_fund_info._total_fees += fee;

			log_trade(stdCode, dInfo._long, true, curTm, trdPx, abs(left), userTag, fee);
		}
	}
}

WTSKlineSlice* SelMocker::stra_get_bars(const char* stdCode, const char* period, uint32_t count)
{
	std::string key = StrUtil::printf("%s#%s", stdCode, period);

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

	WTSKlineSlice* kline = _replayer->get_kline_slice(stdCode, basePeriod.c_str(), count, times, false);

	KlineTag& tag = _kline_tags[key];
	tag._closed = false;

	if (kline)
	{
		double lastClose = kline->close(-1);
		uint64_t lastTime = 0;
		if(basePeriod[0] == 'd')
		{
			lastTime = kline->date(-1);
			WTSSessionInfo* sInfo = _replayer->get_session_info(stdCode, true);
			lastTime *= 1000000000;
			lastTime += (uint64_t)sInfo->getCloseTime() * 100000;
		}
		else
		{
			lastTime = kline->time(-1);
			lastTime += 199000000000;
			lastTime *= 100000;
		}

		if(lastTime > _price_map[stdCode].second)
		{
			_price_map[stdCode].second = lastTime;
			_price_map[stdCode].first = lastClose;
		}
	}

	return kline;
}

WTSTickSlice* SelMocker::stra_get_ticks(const char* stdCode, uint32_t count)
{
	return _replayer->get_tick_slice(stdCode, count);
}

WTSTickData* SelMocker::stra_get_last_tick(const char* stdCode)
{
	return _replayer->get_last_tick(stdCode);
}

void SelMocker::stra_sub_ticks(const char* code)
{
	_replayer->sub_tick(_context_id, code);
}

WTSCommodityInfo* SelMocker::stra_get_comminfo(const char* stdCode)
{
	return _replayer->get_commodity_info(stdCode);
}

WTSSessionInfo* SelMocker::stra_get_sessinfo(const char* stdCode)
{
	return _replayer->get_session_info(stdCode, true);
}


uint32_t SelMocker::stra_get_date()
{
	return _replayer->get_date();
}

uint32_t SelMocker::stra_get_time()
{
	return _replayer->get_min_time();
}

void SelMocker::stra_log_info(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	WTSLogger::vlog_dyn("strategy", _name.c_str(), LL_INFO, fmt, args);
	va_end(args);
}

void SelMocker::stra_log_debug(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	WTSLogger::vlog_dyn("strategy", _name.c_str(), LL_DEBUG, fmt, args);
	va_end(args);
}

void SelMocker::stra_log_error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	WTSLogger::vlog_dyn("strategy", _name.c_str(), LL_ERROR, fmt, args);
	va_end(args);
}

const char* SelMocker::stra_load_user_data(const char* key, const char* defVal /*= ""*/)
{
	auto it = _user_datas.find(key);
	if (it != _user_datas.end())
		return it->second.c_str();

	return defVal;
}

void SelMocker::stra_save_user_data(const char* key, const char* val)
{
	_user_datas[key] = val;
	_ud_modified = true;
}

double SelMocker::stra_get_position(const char* stdCode, const char* userTag /* = "" */)
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
