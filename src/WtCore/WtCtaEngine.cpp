/*!
 * \file WtCtaEngine.cpp
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#define WIN32_LEAN_AND_MEAN

#include "WtCtaEngine.h"
#include "WtDataManager.h"
#include "WtCtaTicker.h"
#include "WtHelper.h"
#include "TraderAdapter.h"

#include "../Share/CodeHelper.hpp"
#include "../Share/StrUtil.hpp"
#include "../Includes/WTSVariant.hpp"
#include "../Share/TimeUtils.hpp"
#include "../Includes/IBaseDataMgr.h"
#include "../Includes/IHotMgr.h"
#include "../Includes/WTSContractInfo.hpp"
#include "../Includes/WTSRiskDef.hpp"
#include "../Share/decimal.h"
#include "../Share/StdUtils.hpp"

#include "../WTSTools/WTSLogger.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
namespace rj = rapidjson;

#include <boost/asio.hpp>

boost::asio::io_service g_asyncIO;


WtCtaEngine::WtCtaEngine()
	: _tm_ticker(NULL)
{
	
}


WtCtaEngine::~WtCtaEngine()
{
	if (_tm_ticker)
	{
		delete _tm_ticker;
		_tm_ticker = NULL;
	}

	if (_cfg)
		_cfg->release();
}

void WtCtaEngine::run(bool bAsync /* = false */)
{
	_tm_ticker = new WtCtaRtTicker(this);
	WTSVariant* cfgProd = _cfg->get("product");
	_tm_ticker->init(_data_mgr->reader(), cfgProd->getCString("session"));

	//����֮ǰ,�Ȱ������еĲ������
	{
		rj::Document root(rj::kObjectType);
		rj::Document::AllocatorType &allocator = root.GetAllocator();

		rj::Value jStraList(rj::kArrayType);
		for (auto& m : _ctx_map)
		{
			const CtaContextPtr& ctx = m.second;
			jStraList.PushBack(rj::Value(ctx->name(), allocator), allocator);
		}

		root.AddMember("marks", jStraList, allocator);

		rj::Value jChnlList(rj::kArrayType);
		for (auto& m : _adapter_mgr->getAdapters())
		{
			const TraderAdapterPtr& adapter = m.second;
			jChnlList.PushBack(rj::Value(adapter->id(), allocator), allocator);
		}

		root.AddMember("channels", jChnlList, allocator);

		rj::Value jExecList(rj::kArrayType);
		_exec_mgr.enum_executer([&jExecList, &allocator](ExecCmdPtr executer) {
			if(executer)
				jExecList.PushBack(rj::Value(executer->name(), allocator), allocator);
		});

		root.AddMember("executers", jExecList, allocator);

		root.AddMember("engine", rj::Value("CTA", allocator), allocator);

		std::string filename = WtHelper::getBaseDir();
		filename += "marker.json";

		rj::StringBuffer sb;
		rj::PrettyWriter<rj::StringBuffer> writer(sb);
		root.Accept(writer);
		StdFile::write_file_content(filename.c_str(), sb.GetString());
	}

	_tm_ticker->run();

	if (_risk_mon)
		_risk_mon->self()->run();

	if (!bAsync)
	{
		boost::asio::io_service::work work(g_asyncIO);
		g_asyncIO.run();
	}
}

void WtCtaEngine::init(WTSVariant* cfg, IBaseDataMgr* bdMgr, WtDataManager* dataMgr, IHotMgr* hotMgr, EventNotifier* notifier /* = NULL */)
{
	WtEngine::init(cfg, bdMgr, dataMgr, hotMgr, notifier);

	_cfg = cfg;
	_cfg->retain();

	_exec_mgr.set_filter_mgr(&_filter_mgr);
}

void WtCtaEngine::addContext(CtaContextPtr ctx)
{
	uint32_t sid = ctx->id();
	_ctx_map[sid] = ctx;
}

CtaContextPtr WtCtaEngine::getContext(uint32_t id)
{
	auto it = _ctx_map.find(id);
	if (it == _ctx_map.end())
		return CtaContextPtr();

	return it->second;
}

void WtCtaEngine::on_init()
{
	faster_hashmap<std::string, double> target_pos;
	for (auto it = _ctx_map.begin(); it != _ctx_map.end(); it++)
	{
		CtaContextPtr& ctx = (CtaContextPtr&)it->second;
		ctx->on_init();

		ctx->enum_position([this, &target_pos](const char* stdCode, double qty){
			std::string realCode = stdCode;
			if (CodeHelper::isStdFutHotCode(stdCode))
			{
				CodeHelper::CodeInfo cInfo;
				CodeHelper::extractStdCode(stdCode, cInfo);
				std::string code = _hot_mgr->getRawCode(cInfo._exchg, cInfo._product, _cur_tdate);
				realCode = CodeHelper::bscFutCodeToStdCode(code.c_str(), cInfo._exchg);
			}
			else if (CodeHelper::isStdFut2ndCode(stdCode))
			{
				CodeHelper::CodeInfo cInfo;
				CodeHelper::extractStdCode(stdCode, cInfo);
				std::string code = _hot_mgr->getSecondRawCode(cInfo._exchg, cInfo._product, _cur_tdate);
				realCode = CodeHelper::bscFutCodeToStdCode(code.c_str(), cInfo._exchg);
			}

			double& vol = target_pos[realCode];
			vol += qty;
		});
	}

	_exec_mgr.set_positions(target_pos);

	if (_evt_listener)
		_evt_listener->on_initialize_event();
}

void WtCtaEngine::on_session_begin()
{
	WTSLogger::info("Trading day %u begun", _cur_tdate);
	for (auto it = _ctx_map.begin(); it != _ctx_map.end(); it++)
	{
		CtaContextPtr& ctx = (CtaContextPtr&)it->second;
		ctx->on_session_begin(_cur_tdate);
	}

	if (_evt_listener)
		_evt_listener->on_session_event(_cur_tdate, true);
}

void WtCtaEngine::on_session_end()
{
	WtEngine::on_session_end();

	for (auto it = _ctx_map.begin(); it != _ctx_map.end(); it++)
	{
		CtaContextPtr& ctx = (CtaContextPtr&)it->second;
		ctx->on_session_end(_cur_tdate);
	}

	WTSLogger::info("Trading day %u ended", _cur_tdate);
	if (_evt_listener)
		_evt_listener->on_session_event(_cur_tdate, false);
}

void WtCtaEngine::on_schedule(uint32_t curDate, uint32_t curTime)
{
	//ȥ���һ�¹�����
	_filter_mgr.load_filters();

	faster_hashmap<std::string, double> target_pos;

	for (auto it = _ctx_map.begin(); it != _ctx_map.end(); it++)
	{
		CtaContextPtr& ctx = (CtaContextPtr&)it->second;
		ctx->on_schedule(curDate, curTime);
		ctx->enum_position([this, &target_pos, ctx](const char* stdCode, double qty){

			double oldQty = qty;
			bool bFilterd = _filter_mgr.is_filtered_by_strategy(ctx->name(), qty);
			if(!bFilterd)
			{
				if(!decimal::eq(qty, oldQty))
				{
					//�����־
					//WTSLogger::info(fmt::format("[Filters] ����{}��{}��Ŀ���λ�����Թ���������: {} -> {}", ctx->name(), stdCode, oldQty, qty).c_str());
					WTSLogger::info(fmt::format("[Filters] Target position of {} of strategy {} reset by strategy filter: {} -> {}", stdCode, ctx->name(), oldQty, qty).c_str());
				}

				std::string realCode = stdCode;
				if (CodeHelper::isStdFutHotCode(stdCode))
				{
					CodeHelper::CodeInfo cInfo;
					CodeHelper::extractStdCode(stdCode, cInfo);
					std::string code = _hot_mgr->getRawCode(cInfo._exchg, cInfo._product, _cur_tdate);
					realCode = CodeHelper::bscFutCodeToStdCode(code.c_str(), cInfo._exchg);
				}
				else if (CodeHelper::isStdFut2ndCode(stdCode))
				{
					CodeHelper::CodeInfo cInfo;
					CodeHelper::extractStdCode(stdCode, cInfo);
					std::string code = _hot_mgr->getSecondRawCode(cInfo._exchg, cInfo._product, _cur_tdate);
					realCode = CodeHelper::bscFutCodeToStdCode(code.c_str(), cInfo._exchg);
				}

				double& vol = target_pos[realCode];
				vol += qty;
			}
			else
			{
				//�����־
				//WTSLogger::info("[������] ����%s��%s��Ŀ���λ�����Թ���������", ctx->name(), stdCode);
				WTSLogger::info("[Filters] Target position of %s of strategy %s ignored by strategy filter", stdCode, ctx->name());
			}
		});
	}

	bool bRiskEnabled = false;
	if(!decimal::eq(_risk_volscale, 1.0) && _risk_date == _cur_tdate)
	{
		WTSLogger::info2("risk", "Risk scale of strategy group is %.2f", _risk_volscale);
		bRiskEnabled = true;
	}

	//����������۲�λ
	for (auto it = target_pos.begin(); it != target_pos.end(); it++)
	{
		std::string stdCode = it->first;
		double& pos = (double&)it->second;

		if (bRiskEnabled && !decimal::eq(pos, 0))
		{
			double symbol = pos / abs(pos);
			pos = decimal::rnd(abs(pos)*_risk_volscale)*symbol;
		}

		append_signal(stdCode.c_str(), pos);
	}

	for(auto& m : _pos_map)
	{
		std::string stdCode = m.first;
		if (target_pos.find(stdCode) == target_pos.end())
		{
			if(!decimal::eq(m.second._volume, 0))
			{
				//������֪ͨWtEngineȥ������ϳֲ�����
				append_signal(stdCode.c_str(), 0);

				WTSLogger::error("Instrument %s not in target positions, setup to 0 automatically", stdCode.c_str());
			}

			//��Ϊ��ϳֲ�����й��ڵĺ�Լ������ڣ����������ڶ���ִ����ǰҪ��һ�����
			auto cInfo = get_contract_info(stdCode.c_str());
			if(cInfo != NULL)
				target_pos[stdCode] = 0;
		}
	}

	push_task([this](){
		update_fund_dynprofit();
	});

	_exec_mgr.set_positions(target_pos);

	save_datas();

	if (_evt_listener)
		_evt_listener->on_schedule_event(curDate, curTime);
}


void WtCtaEngine::handle_push_quote(WTSTickData* newTick, uint32_t hotFlag)
{
	if (_tm_ticker)
		_tm_ticker->on_tick(newTick, hotFlag);
}

void WtCtaEngine::handle_pos_change(const char* straName, const char* stdCode, double diffQty)
{
	//�����ǳֲ�����,���Բ��ô���δ���˵����,��Ϊ���������,����ı�Ŀ��diffQty
	if(_filter_mgr.is_filtered_by_strategy(straName, diffQty, true))
	{
		//�����־
		WTSLogger::info("[Filters] Target position of %s of strategy %s ignored by strategy filter", stdCode, straName);
		return;
	}

	std::string realCode = stdCode;
	if (CodeHelper::isStdFutHotCode(stdCode))
	{
		CodeHelper::CodeInfo cInfo;
		CodeHelper::extractStdCode(stdCode, cInfo);
		std::string code = _hot_mgr->getRawCode(cInfo._exchg, cInfo._product, _cur_tdate);
		realCode = CodeHelper::bscFutCodeToStdCode(code.c_str(), cInfo._exchg);
	}
	else if (CodeHelper::isStdFut2ndCode(stdCode))
	{
		CodeHelper::CodeInfo cInfo;
		CodeHelper::extractStdCode(stdCode, cInfo);
		std::string code = _hot_mgr->getSecondRawCode(cInfo._exchg, cInfo._product, _cur_tdate);
		realCode = CodeHelper::bscFutCodeToStdCode(code.c_str(), cInfo._exchg);
	}

	PosInfo& pItem = _pos_map[realCode];
	double targetPos = pItem._volume + diffQty;

	bool bRiskEnabled = false;
	if (!decimal::eq(_risk_volscale, 1.0) && _risk_date == _cur_tdate)
	{
		WTSLogger::info2("risk", "Risk scale of strategy group is %.2f", _risk_volscale);
		bRiskEnabled = true;
	}
	if (bRiskEnabled && !decimal::eq(targetPos, 0))
	{
		double symbol = targetPos / abs(targetPos);
		targetPos = decimal::rnd(abs(targetPos)*_risk_volscale)*symbol;
	}

	append_signal(realCode.c_str(), targetPos);
	save_datas();

	_exec_mgr.handle_pos_change(realCode.c_str(), targetPos);
}

void WtCtaEngine::on_tick(const char* stdCode, WTSTickData* curTick)
{
	WtEngine::on_tick(stdCode, curTick);

	_data_mgr->handle_push_quote(stdCode, curTick);

	//�������ʵ����, ��Ҫ���ݸ�ִ����
	/*
	 *	���ﲻ�����жϣ�ֱ��ȫ�����ݸ�ִ��������������Ϊִ�������ܻᴦ��δ���ĵĺ�Լ
	 *	��Ҫ����Ϊ������Լ�����ڼ�
	 *	By Wesley @ 2021.08.19
	 */
	//auto it = _ticksubed_raw_codes.find(stdCode);
	//if (it != _ticksubed_raw_codes.end())
	{
		//�Ƿ�������Լ����ı��, ��Ҫ���ڸ�ִ���������ݵ�
		_exec_mgr.handle_tick(stdCode, curTick);
	}

	auto sit = _tick_sub_map.find(stdCode);
	if (sit != _tick_sub_map.end())
	{
		const SIDSet& sids = sit->second;
		for (auto it = sids.begin(); it != sids.end(); it++)
		{
			uint32_t sid = *it;
			auto cit = _ctx_map.find(sid);
			if (cit != _ctx_map.end() && curTick->volume())
			{
				CtaContextPtr& ctx = (CtaContextPtr&)cit->second;
				ctx->on_tick(stdCode, curTick);
			}
		}
	}

	//��ʱ�Ȳ����ǳɱ�, �ȼ������
	/*
	double dynprofit = 0.0;
	for(auto v : _ctx_map)
	{
		const CtaContextPtr& ctx = v.second;
		dynprofit += ctx->get_dyn_profit();
	}

	WTSFundStruct& fundInfo = _port_fund->fundInfo();
	fundInfo._dynprofit = dynprofit;
	double dynbal = fundInfo._balance + dynprofit;
	if (fundInfo._max_dyn_bal != DBL_MAX)
		fundInfo._max_dyn_bal = max(fundInfo._max_dyn_bal, dynbal);
	else
		fundInfo._max_dyn_bal = dynbal;

	if (fundInfo._min_dyn_bal != DBL_MAX)
		fundInfo._min_dyn_bal = min(fundInfo._min_dyn_bal, dynbal);
	else
		fundInfo._min_dyn_bal = dynbal;
	*/
}

void WtCtaEngine::on_bar(const char* stdCode, const char* period, uint32_t times, WTSBarStruct* newBar)
{
	std::string key = StrUtil::printf("%s-%s-%u", stdCode, period, times);
	const SIDSet& sids = _bar_sub_map[key];
	for (auto it = sids.begin(); it != sids.end(); it++)
	{
		uint32_t sid = *it;
		auto cit = _ctx_map.find(sid);
		if(cit != _ctx_map.end())
		{
			CtaContextPtr& ctx = (CtaContextPtr&)cit->second;
			ctx->on_bar(stdCode, period, times, newBar);
		}
	}

	WTSLogger::info("KBar [%s#%s%d] @ %u closed", stdCode, period, times, period[0] == 'd' ? newBar->date : newBar->time);
}

bool WtCtaEngine::isInTrading()
{
	return _tm_ticker->is_in_trading();
}

uint32_t WtCtaEngine::transTimeToMin(uint32_t uTime)
{
	return _tm_ticker->time_to_mins(uTime);
}

WTSCommodityInfo* WtCtaEngine::get_comm_info(const char* stdCode)
{
	return _base_data_mgr->getCommodity(CodeHelper::stdCodeToStdCommID(stdCode).c_str());
}

WTSSessionInfo* WtCtaEngine::get_sess_info(const char* stdCode)
{
	WTSCommodityInfo* cInfo = _base_data_mgr->getCommodity(CodeHelper::stdCodeToStdCommID(stdCode).c_str());
	if (cInfo == NULL)
		return NULL;

	return _base_data_mgr->getSession(cInfo->getSession());
}

uint64_t WtCtaEngine::get_real_time()
{
	return TimeUtils::makeTime(_cur_date, _cur_raw_time * 100000 + _cur_secs);
}