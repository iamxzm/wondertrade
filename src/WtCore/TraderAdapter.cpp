/*!
 * \file TraderAdapter.cpp
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#include "EventNotifier.h"
#include "WtLocalExecuter.h"
#include "TraderAdapter.h"
#include "ActionPolicyMgr.h"
#include "WtHelper.h"
#include "ITrdNotifySink.h"
#include "../Includes/RiskMonDefs.h"

#include <atomic>

#include "../WTSTools/WTSLogger.h"

#include "../Share/CodeHelper.hpp"
#include "../Includes/WTSError.hpp"
#include "../Includes/WTSVariant.hpp"
#include "../Includes/WTSTradeDef.hpp"
#include "../Includes/WTSRiskDef.hpp"
#include "../Share/StrUtil.hpp"
#include "../Share/TimeUtils.hpp"
#include "../Includes/WTSSessionInfo.hpp"

#include "../Includes/WTSContractInfo.hpp"
#include "../Includes/IBaseDataMgr.h"
#include "../Share/DLLHelper.hpp"
#include "../Share/StdUtils.hpp"
#include "../Share/decimal.h"

#include <exception>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

namespace rj = rapidjson;

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

inline const char* formatAction(WTSDirectionType dType, WTSOffsetType oType)
{
	if(dType == WDT_LONG)
	{
		if (oType == WOT_OPEN)
			return "OL";
		else if (oType == WOT_CLOSE)
			return "CL";
		else
			return "CNL";
	}
	else
	{
		if (oType == WOT_OPEN)
			return "OS";
		else if (oType == WOT_CLOSE)
			return "CS";
		else
			return "CNS";
	}
}

TraderAdapter::TraderAdapter(EventNotifier* caster /* = NULL */)
	: _id("")
	, _cfg(NULL)
	, _state(AS_NOTLOGIN)
	, _trader_api(NULL)
	, _orders(NULL)
	, _undone_qty(0)
	, _stat_map(NULL)
	, _risk_mon_enabled(false)
	, _save_data(false)
	, _notifier(caster)
{
}


TraderAdapter::~TraderAdapter()
{
	if (_stat_map)
		_stat_map->release();
}

bool TraderAdapter::init(const char* id, WTSVariant* params, IBaseDataMgr* bdMgr, ActionPolicyMgr* policyMgr)
{
	if (params == NULL)
		return false;

	_policy_mgr = policyMgr;
	_bd_mgr = bdMgr;
	_id = id;

	_order_pattern = StrUtil::printf("otp.%s", id);

	if (_cfg != NULL)
		return false;

	_cfg = params;
	_cfg->retain();

	_save_data = _cfg->getBoolean("savedata");
	if (_save_data)
		initSaveData();

	//�������������ز���
	WTSVariant* cfgRisk = params->get("riskmon");
	if (cfgRisk)
	{
		if (cfgRisk->getBoolean("active"))
		{
			_risk_mon_enabled = true;

			WTSVariant* cfgPolicy = cfgRisk->get("policy");
			auto keys = cfgPolicy->memberNames();
			for (auto it = keys.begin(); it != keys.end(); it++)
			{
				const char* product = (*it).c_str();
				WTSVariant*	vProdItem = cfgPolicy->get(product);
				RiskParams& rParam = _risk_params_map[product];
				rParam._cancel_total_limits = vProdItem->getUInt32("cancel_total_limits");
				rParam._cancel_times_boundary = vProdItem->getUInt32("cancel_times_boundary");
				rParam._cancel_stat_timespan = vProdItem->getUInt32("cancel_stat_timespan");

				rParam._order_total_limits = vProdItem->getUInt32("order_total_limits");
				rParam._order_times_boundary = vProdItem->getUInt32("order_times_boundary");
				rParam._order_stat_timespan = vProdItem->getUInt32("order_stat_timespan");

				WTSLogger::log_dyn("trader", _id.c_str(), LL_INFO, "[%s] Risk control rule %s of trading channel loaded", _id.c_str(), product);
			}

			auto it = _risk_params_map.find("default");
			if (it == _risk_params_map.end())
			{
				//WTSLogger::log_dyn("trader", _id.c_str(), LL_WARN, "[%s] ����ͨ��û��Ĭ�Ϸ�ع���, ���ܻ���һЩƷ�ֲ�����н���ͨ�����", _id.c_str());
				WTSLogger::log_dyn("trader", _id.c_str(), LL_WARN, "[%s] Some instruments may not be monitored due to no default risk control rule of trading channel", _id.c_str());
			}
		}
		else
		{
			WTSLogger::log_dyn("trader", _id.c_str(), LL_WARN, "[%s] Risk control rule of trading channel not activated", _id.c_str());
		}
	}
	else
	{
		WTSLogger::log_dyn("trader", _id.c_str(), LL_WARN, "[%s] No risk control rule setup of trading channel", _id.c_str());
	}

	if (params->getString("module").empty())
		return false;

	std::string module = DLLHelper::wrap_module(params->getCString("module"), "lib");;

	//�ȿ�����Ŀ¼���Ƿ��н���ģ��
	std::string dllpath = WtHelper::getModulePath(module.c_str(), "traders", true);
	//���û��,���ٿ�ģ��Ŀ¼,��dllͬĿ¼��
	if (!StdFile::exists(dllpath.c_str()))
		dllpath = WtHelper::getModulePath(module.c_str(), "traders", false);
	DllHandle hInst = DLLHelper::load_library(dllpath.c_str());
	if (hInst == NULL)
	{
		WTSLogger::log_dyn("trader", _id.c_str(), LL_ERROR, "[%s] Loading trading module %s failed", _id.c_str(), dllpath.c_str());
		return false;
	}
	else
	{
		WTSLogger::log_dyn("trader", _id.c_str(), LL_INFO, "[%s] Trader module %s loaded", _id.c_str(), dllpath.c_str());
	}

	FuncCreateTrader pFunCreateTrader = (FuncCreateTrader)DLLHelper::get_symbol(hInst, "createTrader");
	if (NULL == pFunCreateTrader)
	{
		WTSLogger::log_dyn("trader", _id.c_str(), LL_FATAL, "[%s] Entrance function createTrader not found", _id.c_str());
		return false;
	}

	_trader_api = pFunCreateTrader();
	if (NULL == _trader_api)
	{
		WTSLogger::log_dyn("trader", _id.c_str(), LL_FATAL, "[%s] Creating trading api failed", _id.c_str());
		return false;
	}

	_remover = (FuncDeleteTrader)DLLHelper::get_symbol(hInst, "deleteTrader");

	WTSParams* cfg = params->toParams();
	if (!_trader_api->init(cfg))
	{
		WTSLogger::log_dyn("trader", _id.c_str(), LL_ERROR, "[%s] Entrance function deleteTrader not found", id);
		cfg->release();
		return false;
	}

	cfg->release();

	return true;
}

void TraderAdapter::initSaveData()
{
	/*std::string folder = WtHelper::getOutputDir();
	folder += _name;
	folder += "//";*/
	std::stringstream ss;
	ss << WtHelper::getBaseDir() << "traders/" << _id << "//";
	std::string folder = ss.str();
	BoostFile::create_directories(folder.c_str());

	std::string filename = folder + "trades.csv";
	_trades_log.reset(new BoostFile());
	{
		bool isNewFile = !BoostFile::exists(filename.c_str());
		_trades_log->create_or_open_file(filename.c_str());
		if (isNewFile)
		{
			_trades_log->write_file("localid,date,time,code,action,volume,price,tradeid,orderid\n");
		}
		else
		{
			_trades_log->seek_to_end();
		}
	}

	filename = folder + "orders.csv";
	_orders_log.reset(new BoostFile());
	{
		bool isNewFile = !BoostFile::exists(filename.c_str());
		_orders_log->create_or_open_file(filename.c_str());
		if (isNewFile)
		{
			_orders_log->write_file("localid,date,inserttime,code,action,volume,traded,price,orderid,canceled,remark\n");
		}
		else
		{
			_orders_log->seek_to_end();
		}
	}

	_rt_data_file = folder + "rtdata.json";
}

void TraderAdapter::logTrade(uint32_t localid, const char* stdCode, WTSTradeInfo* trdInfo)
{
	if (_trades_log == NULL || trdInfo == NULL)
		return;

	_trades_log->write_file(fmt::format("{},{},{},{},{},{},{},{},{}\n",
		localid, trdInfo->getTradeDate(), trdInfo->getTradeTime(), stdCode,
		formatAction(trdInfo->getDirection(), trdInfo->getOffsetType()),
		trdInfo->getVolume(), trdInfo->getPrice(), trdInfo->getTradeID(), trdInfo->getRefOrder()));
}

void TraderAdapter::logOrder(uint32_t localid, const char* stdCode, WTSOrderInfo* ordInfo)
{
	if (_orders_log == NULL || ordInfo == NULL)
		return;

	_orders_log->write_file(fmt::format("{},{},{},{},{},{},{},{},{},{},{}\n",
		localid, ordInfo->getOrderDate(), ordInfo->getOrderTime(), stdCode,
		formatAction(ordInfo->getDirection(), ordInfo->getOffsetType()),
		ordInfo->getVolume(), ordInfo->getVolTraded(), ordInfo->getPrice(), 
		ordInfo->getOrderID(), ordInfo->getOrderState()==WOS_Canceled?"TRUE":"FALSE", ordInfo->getStateMsg()));
}

void TraderAdapter::saveData(WTSArray* ayFunds /* = NULL */)
{
	rj::Document root(rj::kObjectType);
	rj::Document::AllocatorType &allocator = root.GetAllocator();

	{//�ֲ����ݱ���
		rj::Value jPos(rj::kArrayType);
		/*
		//�������
		double	l_newvol;
		double	l_newavail;
		double	l_prevol;
		double	l_preavail;

		//�ղ�����
		double	s_newvol;
		double	s_newavail;
		double	s_prevol;
		double	s_preavail;
		 */

		for (auto it = _positions.begin(); it != _positions.end(); it++)
		{
			const char* stdCode = it->first.c_str();
			const PosItem& pInfo = it->second;

			rj::Value pItem(rj::kObjectType);
			pItem.AddMember("code", rj::Value(stdCode, allocator), allocator);

			rj::Value longItem(rj::kObjectType);
			longItem.AddMember("newvol", pInfo.l_newvol, allocator);
			longItem.AddMember("newavail", pInfo.l_newavail, allocator);
			longItem.AddMember("prevol", pInfo.l_prevol, allocator);
			longItem.AddMember("preavail", pInfo.l_preavail, allocator);
			pItem.AddMember("long", longItem, allocator);

			rj::Value shortItem(rj::kObjectType);
			shortItem.AddMember("newvol", pInfo.s_newvol, allocator);
			shortItem.AddMember("newavail", pInfo.s_newavail, allocator);
			shortItem.AddMember("prevol", pInfo.s_prevol, allocator);
			shortItem.AddMember("preavail", pInfo.s_preavail, allocator);
			pItem.AddMember("short", shortItem, allocator);

			jPos.PushBack(pItem, allocator);
		}

		root.AddMember("positions", jPos, allocator);
	}

	{//�ʽ𱣴�
		rj::Value jFunds(rj::kObjectType);

		if(ayFunds && ayFunds->size() > 0)
		{
			for(auto it = ayFunds->begin(); it != ayFunds->end(); it++)
			{
				WTSAccountInfo* fundInfo = (WTSAccountInfo*)(*it);
				rj::Value fItem(rj::kObjectType);
				fItem.AddMember("prebalance", fundInfo->getPreBalance(), allocator);
				fItem.AddMember("balance", fundInfo->getBalance(), allocator);
				fItem.AddMember("closeprofit", fundInfo->getCloseProfit(), allocator);
				fItem.AddMember("dynprofit", fundInfo->getDynProfit(), allocator);
				fItem.AddMember("margin", fundInfo->getMargin(), allocator);
				fItem.AddMember("fee", fundInfo->getCommission(), allocator);
				fItem.AddMember("available", fundInfo->getAvailable(), allocator);

				fItem.AddMember("deposit", fundInfo->getDeposit(), allocator);
				fItem.AddMember("withdraw", fundInfo->getWithdraw(), allocator);

				jFunds.AddMember(rj::Value(fundInfo->getCurrency(), allocator), fItem, allocator);
			}
		}

		root.AddMember("funds", jFunds, allocator);
	}

	{
		BoostFile bf;
		if (bf.create_new_file(_rt_data_file.c_str()))
		{
			rj::StringBuffer sb;
			rj::PrettyWriter<rj::StringBuffer> writer(sb);
			root.Accept(writer);
			bf.write_file(sb.GetString());
			bf.close_file();
		}
	}
}

const TraderAdapter::RiskParams* TraderAdapter::getRiskParams(const char* stdCode)
{
	std::string pid = CodeHelper::stdCodeToStdCommID(stdCode);
	auto it = _risk_params_map.find(pid);
	if (it != _risk_params_map.end())
		return &it->second;

	it = _risk_params_map.find("default");
	return &it->second;
}

bool TraderAdapter::run()
{
	if (_trader_api == NULL)
		return false;

	if (_stat_map == NULL)
		_stat_map = TradeStatMap::create();

	_trader_api->registerSpi(this);

	_trader_api->connect();
	_state = AS_LOGINING;
	return true;
}

void TraderAdapter::release()
{
	if (_trader_api)
	{
		_trader_api->registerSpi(NULL);
		_trader_api->release();
	}
}

double TraderAdapter::getPosition(const char* stdCode, int32_t flag /* = 3 */)
{
	auto it = _positions.find(stdCode);
	if (it == _positions.end())
		return 0;

	double ret = 0;
	const PosItem& pItem = it->second;
	if(flag & 1)
	{
		ret += (pItem.l_newvol + pItem.l_prevol);
	}

	if (flag & 2)
	{
		ret -= pItem.s_newvol + pItem.s_prevol;
	}
	return ret;
}

OrderMap* TraderAdapter::getOrders(const char* stdCode)
{
	if (_orders == NULL)
		return NULL;

	bool isAll = strlen(stdCode) == 0;

	StdUniqueLock lock(_mtx_orders);
	OrderMap* ret = OrderMap::create();
	for (auto it = _orders->begin(); it != _orders->end(); it++)
	{
		uint32_t localid = it->first;
		WTSOrderInfo* ordInfo = (WTSOrderInfo*)it->second;

		if (isAll || strcmp(ordInfo->getCode(), stdCode) == 0)
			ret->add(localid, ordInfo);
	}
	return ret;
}

uint32_t TraderAdapter::doEntrust(WTSEntrust* entrust)
{
	char entrustid[64] = { 0 };
	if (_trader_api->makeEntrustID(entrustid, 64))
	{
		entrust->setEntrustID(entrustid);
	}

	CodeHelper::CodeInfo cInfo;
	CodeHelper::extractStdCode(entrust->getCode(), cInfo);
	entrust->setCode(cInfo._code);
	entrust->setExchange(cInfo._exchg);

	uint32_t localid = makeLocalOrderID();
	entrust->setUserTag(StrUtil::printf("%s.%u", _order_pattern.c_str(), localid).c_str());
	
	int32_t ret = _trader_api->orderInsert(entrust);
	entrust->setSent();
	if(ret < 0)
	{
		WTSLogger::log_dyn("trader", _id.c_str(), LL_ERROR, "[%s] Order placing failed: %d", _id.c_str(), ret);
		return UINT_MAX;
	}
	else
	{
		_order_time_cache[entrust->getCode()].emplace_back(TimeUtils::getLocalTimeNow());
	}
	return localid;
}

WTSContractInfo* TraderAdapter::getContract(const char* stdCode)
{
	CodeHelper::CodeInfo cInfo;
	CodeHelper::extractStdCode(stdCode, cInfo);
	return _bd_mgr->getContract(cInfo._code, cInfo._exchg);
}

WTSCommodityInfo* TraderAdapter::getCommodify(const char* stdCode)
{
	std::string commID = CodeHelper::stdCodeToStdCommID(stdCode);
	return _bd_mgr->getCommodity(commID.c_str());
}

bool TraderAdapter::checkCancelLimits(const char* stdCode)
{
	if (!_risk_mon_enabled)
		return true;

	if (_exclude_codes.find(stdCode) != _exclude_codes.end())
		return false;

	const RiskParams* riskPara = getRiskParams(stdCode);
	if (riskPara == NULL)
		return true;

	WTSTradeStateInfo* statInfo = (WTSTradeStateInfo*)_stat_map->get(stdCode);
	if (statInfo && riskPara->_cancel_total_limits != 0 && statInfo->total_cancels() >= riskPara->_cancel_total_limits )
	{
		WTSLogger::log_dyn("trader", _id.c_str(), LL_ERROR, "[%s] %s cancel %u times totaly, beyond boundary %u times, adding to excluding list",
			_id.c_str(), stdCode, statInfo->total_cancels(), riskPara->_cancel_total_limits);
		_exclude_codes.insert(stdCode);
		return false;
	}

	//����Ƶ�ʼ��
	auto it = _cancel_time_cache.find(stdCode);
	if (it != _cancel_time_cache.end())
	{
		TimeCacheList& cache = (TimeCacheList&)it->second;
		uint32_t cnt = cache.size();
		if (cnt >= riskPara->_cancel_times_boundary)
		{
			uint64_t eTime = cache[cnt - 1];
			uint64_t sTime = eTime - riskPara->_cancel_stat_timespan * 1000;
			auto tit = std::lower_bound(cache.begin(), cache.end(), sTime);
			uint32_t sIdx = tit - cache.begin();
			uint32_t times = cnt - sIdx - 1;
			if (times > riskPara->_cancel_times_boundary)
			{
				WTSLogger::log_dyn("trader", _id.c_str(), LL_ERROR, "[%s] %s cancel %u times within %u seconds, beyond boundary %u times, adding to excluding list",
					_id.c_str(), stdCode, times, riskPara->_cancel_stat_timespan, riskPara->_cancel_times_boundary);
				_exclude_codes.insert(stdCode);
				return false;
			}

			//�������Ҫ����һ��, û���ر�õİ취
			//��Ȼ����ʱ������, vector���Ȼ�Խ��Խ��
			if(tit != cache.begin())
			{
				cache.erase(cache.begin(), tit);
			}
		}
	}

	return true;
}

bool TraderAdapter::isTradeEnabled(const char* stdCode) const
{
	if (!_risk_mon_enabled)
		return true;

	if (_exclude_codes.find(stdCode) != _exclude_codes.end())
		return false;

	return true;
}

bool TraderAdapter::checkOrderLimits(const char* stdCode)
{
	if (!_risk_mon_enabled)
		return true;

	if (_exclude_codes.find(stdCode) != _exclude_codes.end())
		return false;

	const RiskParams* riskPara = getRiskParams(stdCode);
	if (riskPara == NULL)
		return true;

	WTSTradeStateInfo* statInfo = (WTSTradeStateInfo*)_stat_map->get(stdCode);
	if (statInfo && riskPara->_order_total_limits != 0 && statInfo->total_orders() >= riskPara->_order_total_limits)
	{
		WTSLogger::log_dyn("trader", _id.c_str(), LL_ERROR, "[%s] %s entrust %u times totally, beyond boundary %u times, adding to excluding list",
			_id.c_str(), stdCode, statInfo->total_orders(), riskPara->_order_total_limits);
		_exclude_codes.insert(stdCode);
		return false;
	}

	//����Ƶ�ʼ��
	auto it = _order_time_cache.find(stdCode);
	if (it != _order_time_cache.end())
	{
		TimeCacheList& cache = (TimeCacheList&)it->second;
		uint32_t cnt = cache.size();
		if (cnt >= riskPara->_order_times_boundary)
		{
			uint64_t eTime = cache[cnt - 1];
			uint64_t sTime = eTime - riskPara->_order_stat_timespan * 1000;
			auto tit = std::lower_bound(cache.begin(), cache.end(), sTime);
			uint32_t sIdx = tit - cache.begin();
			uint32_t times = cnt - sIdx - 1;
			if (times > riskPara->_order_times_boundary)
			{
				WTSLogger::log_dyn("trader", _id.c_str(), LL_ERROR, "[%s] %s entrust %u times within %u seconds, beyond boundary %u times, adding to excluding list",
					_id.c_str(), stdCode, times, riskPara->_order_stat_timespan, riskPara->_order_times_boundary);
				_exclude_codes.insert(stdCode);
				return false;
			}

			//�������Ҫ����һ��, û���ر�õİ취
			//��Ȼ����ʱ������, vector���Ȼ�Խ��Խ��
			if (tit != cache.begin())
			{
				cache.erase(cache.begin(), tit);
			}
		}
	}

	return true;
}

OrderIDs TraderAdapter::buy(const char* stdCode, double price, double qty, bool bForceClose/* = false*/)
{
	OrderIDs ret;
	if (qty == 0)
		return ret;

	WTSContractInfo* cInfo = getContract(stdCode);
	WTSCommodityInfo* commInfo = getCommodify(stdCode);
	WTSSessionInfo* sInfo = _bd_mgr->getSession(commInfo->getSession());

	if (!sInfo->isInTradingTime(WtHelper::getTime(), true))
	{
		WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_ERROR, fmt::format("[{}] Buying {} of quantity {}, {:04d} not in trading time", _id.c_str(), stdCode, qty, WtHelper::getTime()).c_str());
		return ret;
	}


	WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, fmt::format("[{}] Buying {} of quantity {}", _id.c_str(), stdCode, qty).c_str());

	double oldQty = _undone_qty[stdCode];
	double newQty = oldQty + qty;
	_undone_qty[stdCode] = newQty;
	WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, fmt::format("[{}]{} undone orders updated, {} -> {}", _id.c_str(), stdCode, oldQty, newQty).c_str());

	const PosItem& pItem = _positions[stdCode];
	WTSTradeStateInfo* statInfo = (WTSTradeStateInfo*)_stat_map->get(stdCode);
	if (statInfo == NULL)
	{
		statInfo = WTSTradeStateInfo::create(stdCode);
		_stat_map->add(stdCode, statInfo, false);
	}
	TradeStatInfo& statItem = statInfo->statInfo();

	std::string stdCommID = CodeHelper::stdCodeToStdCommID(stdCode);
	const ActionRuleGroup& ruleGP = _policy_mgr->getActionRules(stdCommID.c_str());

	double left = qty;

	double unitQty = (price == 0.0) ? cInfo->getMaxMktVol() : cInfo->getMaxLmtVol();
	if (decimal::eq(unitQty, 0))
		unitQty = DBL_MAX;

	for (auto it = ruleGP.begin(); it != ruleGP.end(); it++)
	{
		const ActionRule& curRule = (*it);
		if(curRule._atype == AT_Open && !bForceClose)
		{
			//�ȼ���Ƿ��Ѿ������޶�
			//���뿪��, �������
			double maxQty = left;

			if (curRule._limit_l != 0)
			{
				if (statItem.l_openvol >= curRule._limit_l)
				{
					WTSLogger::log_dyn("trader", _id.c_str(), LL_WARN, "[%s] %s long position opened today is up to limit %u", _id.c_str(), stdCode, curRule._limit_l);
					continue;
				}
				else
				{
					maxQty = min(maxQty, curRule._limit_l - statItem.l_openvol);
				}
			}

			if (curRule._limit != 0)
			{
				if (statItem.l_openvol + statItem.s_openvol >= curRule._limit)
				{
					WTSLogger::log_dyn("trader", _id.c_str(), LL_WARN, "[%s] %s position opened today is up to limit %u", _id.c_str(), stdCode, curRule._limit);
					continue;
				}
				else
				{
					maxQty = min(maxQty, curRule._limit - statItem.l_openvol - statItem.s_openvol);
				}
			}

			//���ﻹҪ���ǵ������ί������
			double leftQty = maxQty;
			for (;;)
			{
				double curQty = min(leftQty, unitQty);
				uint32_t localid = openLong(stdCode, price, curQty);
				ret.emplace_back(localid);

				leftQty -= curQty;

				if (decimal::eq(leftQty, 0))
					break;
			}			

			left -= maxQty;

			WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, 
				fmt::format("[{}] Signal of buying {} of quantity {} triggered: Opening long of quantity {}", _id.c_str(), stdCode, qty, maxQty).c_str());
		}
		else if(curRule._atype == AT_CloseToday)
		{
			double maxQty = 0;
			//���Ҫ����ƽ��ƽ���Ʒ��, ��ֻ��ȡ��ƽ��ּ���
			//���������ƽ��ƽ���Ʒ��, ���ȡȫ����ƽ, ��Ϊ��ȡ��ƴ���Ҳû����
			if (commInfo->getCoverMode() == CM_CoverToday)
				maxQty = min(left, pItem.s_newavail);	//�ȿ�����ƽ���
			else
				maxQty = min(left, pItem.avail_pos(false));
			
			
			//���Ҫ��龻��֣�������ֲ�Ϊ0����������������
			if (!bForceClose && curRule._pure && !decimal::eq(pItem.s_prevol, 0.0))
			{
				WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_WARN,
					fmt::format("[{}] Closing new short position of {} skipped because of non-zero pre short position", _id.c_str(), stdCode).c_str());
				continue;
			}

			//���ﻹҪ���ǵ������ί������
			//if (maxQty > 0)
			if (decimal::gt(maxQty, 0))
			{
				double leftQty = maxQty;
				for (;;)
				{
					double curQty = min(leftQty, unitQty);
					uint32_t localid = closeShort(stdCode, price, curQty, (commInfo->getCoverMode() == CM_CoverToday));//�����֧��ƽ��, ��ֱ����ƽ�ֱ�Ǽ���
					ret.emplace_back(localid);

					leftQty -= curQty;

					//if (leftQty == 0)
					if (decimal::eq(leftQty, 0))
						break;
				}
				left -= maxQty;

				if (commInfo->getCoverMode() == CM_CoverToday)
				{
					//WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, fmt::format("[{}]{} ��������{}�ź�ִ��: ƽ�������{}", _id.c_str(), stdCode, qty, maxQty).c_str());
					WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, 
						fmt::format("[{}] Signal of buying {} of quantity {} triggered: Closing new short of quantity {}", _id.c_str(), stdCode, qty, maxQty).c_str());
				}
				else
				{
					WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, 
						fmt::format("[{}] Signal of buying {} of quantity {} triggered: Closing short of quantity {}", _id.c_str(), stdCode, qty, maxQty).c_str());
				}
			}
			
		}
		else if (curRule._atype == AT_CloseYestoday)
		{
			//ƽ��Ƚϼ�, ��Ϊ����Ҫ���ֱ��
			double maxQty = min(left, pItem.s_preavail);

			//���Ҫ��龻��֣����ǽ�ֲ�Ϊ0����������������
			if (!bForceClose && curRule._pure && !decimal::eq(pItem.s_newvol, 0.0))
			{
				WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_WARN,
					fmt::format("[{}] Closing pre short position of {} skipped because of non-zero new short position", _id.c_str(), stdCode).c_str());
				continue;
			}

			//���ﻹҪ���ǵ������ί������
			//if (maxQty > 0)
			if (decimal::gt(maxQty, 0))
			{
				double leftQty = maxQty;
				for (;;)
				{
					double curQty = min(leftQty, unitQty);
					uint32_t localid = closeShort(stdCode, price, curQty, false);//�����֧��ƽ��, ��ֱ����ƽ�ֱ�Ǽ���
					ret.emplace_back(localid);

					leftQty -= curQty;

					//if (leftQty == 0)
					if (decimal::eq(leftQty, 0))
						break;
				}

				left -= maxQty;

				if (commInfo->getCoverMode() == CM_CoverToday)
				{
					//WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, fmt::format("[{}]{} ��������{}�ź�ִ��: ƽ�������{}", _id.c_str(), stdCode, qty, maxQty).c_str());
					WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, 
						fmt::format("[{}] Signal of buying {} of quantity {} triggered: Closing old short of quantity {}", _id.c_str(), stdCode, qty, maxQty).c_str());
				}
				else
				{
					//WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, fmt::format("[{}]{} ��������{}�ź�ִ��: ƽ������{}", _id.c_str(), stdCode, qty, maxQty).c_str());
					WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO,
						fmt::format("[{}] Signal of buying {} of quantity {} triggered: Closing short of quantity {}", _id.c_str(), stdCode, qty, maxQty).c_str());
				}
			}
		}
		else if (curRule._atype == AT_Close)
		{
			//���ֻ��ƽ��, ����������
			//�������ƽ��ƽ��, ��Ҫ��ƽ����ƽ��
			//���������ƽ��ƽ��, ��ͳһƽ��
			if (commInfo->getCoverMode() != CM_CoverToday)
			{
				double maxQty = min(pItem.avail_pos(false), left);
				
				//if (maxQty > 0)
				if (decimal::gt(maxQty, 0))
				{
					double leftQty = maxQty;
					for (;;)
					{
						double curQty = min(leftQty, unitQty);
						uint32_t localid = closeShort(stdCode, price, curQty, false);
						ret.emplace_back(localid);

						leftQty -= curQty;

						//if (leftQty == 0)
						if (decimal::eq(leftQty, 0))
							break;
					}
					left -= maxQty;

					//WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, fmt::format("[{}]{} ��������{}�ź�ִ��: ƽ������{}", _id.c_str(), stdCode, qty, maxQty).c_str());
					WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO,
						fmt::format("[{}] Signal of buying {} of quantity {} triggered: Closing short of quantity {}", _id.c_str(), stdCode, qty, maxQty).c_str());
				}
			}
			else
			{
				//if (pItem.s_preavail > 0)
				if (decimal::gt(pItem.s_preavail, 0))
				{
					//�Ƚ���ƽ���ƽ��
					double maxQty = min(pItem.s_preavail, qty);
					double leftQty = maxQty;
					for (;;)
					{
						double curQty = min(leftQty, unitQty);
						uint32_t localid = closeShort(stdCode, price, curQty, false);
						ret.emplace_back(localid);

						leftQty -= curQty;

						//if (leftQty == 0)
						if (decimal::eq(leftQty, 0))
							break;
					}
					left -= maxQty;

					//WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, fmt::format("[{}]{} ��������{}�ź�ִ��: ƽ�������{}", _id.c_str(), stdCode, qty, maxQty).c_str());
					WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO,
						fmt::format("[{}] Signal of buying {} of quantity {} triggered: Closing old short of quantity {}", _id.c_str(), stdCode, qty, maxQty).c_str());
				}

				//if (left > 0 && pItem.s_newavail > 0)
				if (decimal::gt(left, 0) && decimal::gt(pItem.s_newavail, 0))
				{
					//�ٽ���ƽ���ƽ��
					//TODO: ���ﻹ��һ������, ����ǿ������ֵĻ�, ����߼�������ȥ��
					double maxQty = min(pItem.s_newavail, left);
					double leftQty = maxQty;
					for (;;)
					{
						double curQty = min(leftQty, unitQty);
						uint32_t localid = closeShort(stdCode, price, curQty, true);
						ret.emplace_back(localid);

						leftQty -= curQty;

						//if (leftQty == 0)
						if (decimal::eq(leftQty, 0))
							break;
					}
					left -= maxQty;

					//WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, fmt::format("[{}]{} ��������{}�ź�ִ��: ƽ�������{}", _id.c_str(), stdCode, qty, maxQty).c_str());
					WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO,
						fmt::format("[{}] Signal of buying {} of quantity {} triggered: Closing new short of quantity {}", _id.c_str(), stdCode, qty, maxQty).c_str());
				}
			}
		}

		if(left == 0)
			break;
	}

	if(left > 0)
	{
		//WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_ERROR,fmt::format("[{}]{} ��������{}�����ź�δִ�����", _id.c_str(), stdCode, left).c_str());
		WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_ERROR, fmt::format("[{}] Signal of buying {} of quantity {} left quantity of {} not triggered", _id.c_str(), stdCode, qty, left).c_str());
	}

	return ret;
}

OrderIDs TraderAdapter::sell(const char* stdCode, double price, double qty, bool bForceClose/* = false*/)
{
	OrderIDs ret;
	if (qty == 0)
		return ret;

	WTSContractInfo* cInfo = getContract(stdCode);
	WTSCommodityInfo* commInfo = getCommodify(stdCode);
	WTSSessionInfo* sInfo = _bd_mgr->getSession(commInfo->getSession());

	if(!sInfo->isInTradingTime(WtHelper::getTime(), true))
	{
		WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_ERROR, fmt::format("[{}] Selling {} of quantity {}, {:04d} not in trading time", _id.c_str(), stdCode, qty, WtHelper::getTime()).c_str());
		return ret;
	}

	WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, fmt::format("[{}] Selling {} of quantity {}", _id.c_str(), stdCode, qty).c_str());

	double oldQty = _undone_qty[stdCode];
	double newQty = oldQty - qty;
	_undone_qty[stdCode] = newQty;
	WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, fmt::format("[{}]{} undone orders updated, {} -> {}", _id.c_str(), stdCode, oldQty, newQty).c_str());

	const PosItem& pItem = _positions[stdCode];	
	WTSTradeStateInfo* statInfo = (WTSTradeStateInfo*)_stat_map->get(stdCode);
	if (statInfo == NULL)
	{
		statInfo = WTSTradeStateInfo::create(stdCode);
		_stat_map->add(stdCode, statInfo, false);
	}
	TradeStatInfo& statItem = statInfo->statInfo();

	std::string stdCommID = CodeHelper::stdCodeToStdCommID(stdCode);
	const ActionRuleGroup& ruleGP = _policy_mgr->getActionRules(stdCommID.c_str());

	double left = qty;

	double unitQty = (price == 0.0) ? cInfo->getMaxMktVol() : cInfo->getMaxLmtVol();
	if (decimal::eq(unitQty, 0))
		unitQty = DBL_MAX;

	for (auto it = ruleGP.begin(); it != ruleGP.end(); it++)
	{
		const ActionRule& curRule = (*it);
		if (curRule._atype == AT_Open && !bForceClose)
		{
			//�ȼ���Ƿ��Ѿ������޶�
			//���뿪��, �������
			double maxQty = left;

			if (curRule._limit_s != 0)
			{
				if (statItem.s_openvol >= curRule._limit_s)
				{
					WTSLogger::log_dyn("trader", _id.c_str(), LL_WARN, "[%s] %s short position opened today is up to limit %u", _id.c_str(), stdCode, curRule._limit_l);
					continue;
				}
				else
				{
					maxQty = min(maxQty, curRule._limit_s - statItem.s_openvol);
				}
			}

			if (curRule._limit != 0)
			{
				if (statItem.l_openvol + statItem.s_openvol >= curRule._limit)
				{
					WTSLogger::log_dyn("trader", _id.c_str(), LL_WARN, "[%s] %s position opened today is up to limit %u", _id.c_str(), stdCode, curRule._limit);
					continue;
				}
				else
				{
					maxQty = min(maxQty, curRule._limit - statItem.l_openvol - statItem.s_openvol);
				}
			}

			//���ﻹҪ���ǵ������ί������
			double leftQty = maxQty;
			for (;;)
			{
				double curQty = min(leftQty, unitQty);
				uint32_t localid = openShort(stdCode, price, curQty);
				ret.emplace_back(localid);

				leftQty -= curQty;

				if (decimal::eq(leftQty, 0))
					break;
			}

			left -= maxQty;

			WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO,
				fmt::format("[{}] Signal of selling {} of quantity {} triggered: Opening short of quantity {}", _id.c_str(), stdCode, qty, maxQty).c_str());
		}
		else if (curRule._atype == AT_CloseToday)
		{
			double maxQty = 0;
			//���Ҫ����ƽ��ƽ���Ʒ��, ��ֻ��ȡ��ƽ��ּ���
			//���������ƽ��ƽ���Ʒ��, ���ȡȫ����ƽ, ��Ϊ��ȡ��ƽ���Ҳû����
			if (commInfo->getCoverMode() == CM_CoverToday)
				maxQty = min(left, pItem.l_newavail);	//�ȿ�����ƽ���
			else
				maxQty = min(left, pItem.avail_pos(true));

			//���Ҫ��龻��֣�������ֲ�Ϊ0����������������
			if (!bForceClose && curRule._pure && !decimal::eq(pItem.l_prevol, 0.0))
			{
				WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_WARN,
					fmt::format("[{}] Closing new long position of {} skipped because of non-zero pre long position", _id.c_str(), stdCode).c_str());
				continue;
			}

			//���ﻹҪ���ǵ������ί������
			if(decimal::gt(maxQty, 0))
			{
				double leftQty = maxQty;
				for (;;)
				{
					double curQty = min(leftQty, unitQty);
					uint32_t localid = closeLong(stdCode, price, curQty, (commInfo->getCoverMode() == CM_CoverToday));//�����֧��ƽ��, ��ֱ����ƽ�ֱ�Ǽ���
					ret.emplace_back(localid);

					leftQty -= curQty;

					if (decimal::eq(leftQty, 0))
						break;
				}
				left -= maxQty;

				if (commInfo->getCoverMode() == CM_CoverToday)
				{
					WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO,
						fmt::format("[{}] Signal of selling {} of quantity {} triggered: Closing new long of quantity {}", _id.c_str(), stdCode, qty, maxQty).c_str());
				}
				else
				{
					WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO,
						fmt::format("[{}] Signal of selling {} of quantity {} triggered: Closing long of quantity {}", _id.c_str(), stdCode, qty, maxQty).c_str());
				}
			}

		}
		else if (curRule._atype == AT_CloseYestoday)
		{
			//ƽ��Ƚϼ�, ��Ϊ����Ҫ���ֱ��
			double maxQty = min(left, pItem.l_preavail);
			
			//���Ҫ��龻��֣����ǽ�ֲ�Ϊ0����������������
			if (!bForceClose && curRule._pure && !decimal::eq(pItem.l_newvol, 0.0))
			{
				WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_WARN,
					fmt::format("[{}] Closing pre long position of {} skipped because of non-zero new long position", _id.c_str(), stdCode).c_str());
				continue;
			}

			//���ﻹҪ���ǵ������ί������
			if(decimal::gt(maxQty, 0))
			{
				double leftQty = maxQty;
				for (;;)
				{
					double curQty = min(leftQty, unitQty);
					uint32_t localid = closeLong(stdCode, price, curQty, false);//�����֧��ƽ��, ��ֱ����ƽ�ֱ�Ǽ���
					ret.emplace_back(localid);

					leftQty -= curQty;

					if (decimal::eq(leftQty, 0))
						break;
				}
				left -= maxQty;

				if (commInfo->getCoverMode() == CM_CoverToday)
				{
					WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO,
						fmt::format("[{}] Signal of selling {} of quantity {} triggered: Closing old long of quantity {}", _id.c_str(), stdCode, qty, maxQty).c_str());
				}
				else
				{
					WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO,
						fmt::format("[{}] Signal of selling {} of quantity {} triggered: Closing long of quantity {}", _id.c_str(), stdCode, qty, maxQty).c_str());
				}
			}
		}
		else if (curRule._atype == AT_Close)
		{
			//���ֻ��ƽ��, ����������
			//�������ƽ��ƽ��, ��Ҫ��ƽ����ƽ��
			//���������ƽ��ƽ��, ��ͳһƽ��
			if (commInfo->getCoverMode() != CM_CoverToday)
			{
				double maxQty = min(pItem.avail_pos(true), left);	//������ƽ��ƽ��, ���ȡȫ����ƽ��
				if(decimal::gt(maxQty, 0))
				{
					double leftQty = maxQty;
					for (;;)
					{
						double curQty = min(leftQty, unitQty);
						uint32_t localid = closeLong(stdCode, price, curQty, false);
						ret.emplace_back(localid);

						leftQty -= curQty;

						if (decimal::eq(leftQty, 0))
							break;
					}
					left -= maxQty;

					WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO,
						fmt::format("[{}] Signal of selling {} of quantity {} triggered: Closing long of quantity {}", _id.c_str(), stdCode, qty, maxQty).c_str());
				}
				
			}
			else
			{
				if (decimal::gt(left, 0) && decimal::gt(pItem.l_preavail, 0))
				{
					//�Ƚ���ƽ���ƽ��
					double maxQty = min(pItem.l_preavail, qty);
					if(decimal::gt(maxQty, 0))
					{
						double leftQty = maxQty;
						for (;;)
						{
							double curQty = min(leftQty, unitQty);
							uint32_t localid = closeLong(stdCode, price, curQty, false);
							ret.emplace_back(localid);

							leftQty -= curQty;

							if (decimal::eq(leftQty, 0))
								break;
						}
						left -= maxQty;

						WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO,
							fmt::format("[{}] Signal of selling {} of quantity {} triggered: Closing old long of quantity {}", _id.c_str(), stdCode, qty, maxQty).c_str());
					}
					
				}

				if (decimal::gt(left, 0) && decimal::gt(pItem.l_newavail, 0))
				{
					//�ٽ���ƽ���ƽ��
					//TODO: ���ﻹ��һ������, ����ǿ������ֵĻ�, ����߼�������ȥ��
					double maxQty = min(pItem.l_newavail, left);
					if(decimal::gt(maxQty, 0))
					{
						double leftQty = maxQty;
						for (;;)
						{
							double curQty = min(leftQty, unitQty);
							uint32_t localid = closeLong(stdCode, price, curQty, true);
							ret.emplace_back(localid);

							leftQty -= curQty;

							if (decimal::eq(leftQty, 0))
								break;
						}
						left -= maxQty;

						WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO,
							fmt::format("[{}] Signal of selling {} of quantity {} triggered: Closing new long of quantity {}", _id.c_str(), stdCode, qty, maxQty).c_str());
					}
					
				}
			}
		}

		if (left == 0)
			break;
	}

	if (left > 0)
	{
		WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_ERROR, fmt::format("[{}] Signal of buying {} of quantity {} left quantity of {} not triggered", _id.c_str(), stdCode, qty, left).c_str());
	}

	return ret;
}

bool TraderAdapter::doCancel(WTSOrderInfo* ordInfo)
{
	if (ordInfo == NULL || !ordInfo->isAlive())
		return false;

	WTSContractInfo* cInfo = _bd_mgr->getContract(ordInfo->getCode(), ordInfo->getExchg());
	WTSCommodityInfo* commInfo = _bd_mgr->getCommodity(cInfo);
	std::string stdCode;
	if (commInfo->getCategoty() == CC_Future)
		stdCode = CodeHelper::bscFutCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
	else if (commInfo->getCategoty() == CC_FutOption)
		stdCode = CodeHelper::bscFutOptCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
	else if(commInfo->getCategoty() == CC_Stock)
		stdCode = CodeHelper::bscStkCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
	else
		stdCode = CodeHelper::bscStkCodeToStdCode(cInfo->getCode(), cInfo->getExchg(), commInfo->getProduct());
	//����Ƶ�ʼ��
	if (!checkCancelLimits(stdCode.c_str()))
		return false;

	WTSEntrustAction* action = WTSEntrustAction::create(ordInfo->getCode(), cInfo->getExchg());
	action->setEntrustID(ordInfo->getEntrustID());
	action->setOrderID(ordInfo->getOrderID());
	int ret = _trader_api->orderAction(action);
	bool isSent = (ret >= 0);
	action->release();
	return isSent;
}

bool TraderAdapter::cancel(uint32_t localid)
{
	if (_orders == NULL || _orders->size() == 0)
		return false;

	WTSOrderInfo* ordInfo = NULL;
	{
		StdUniqueLock lock(_mtx_orders);
		ordInfo = (WTSOrderInfo*)_orders->grab(localid);
		if (ordInfo == NULL)
			return false;
	}
	
	bool bRet = doCancel(ordInfo);

	_cancel_time_cache[ordInfo->getCode()].emplace_back(TimeUtils::getLocalTimeNow());

	ordInfo->release();

	return bRet;
}

OrderIDs TraderAdapter::cancel(const char* stdCode, bool isBuy, double qty /* = 0 */)
{
	CodeHelper::CodeInfo cInfo;
	CodeHelper::extractStdCode(stdCode, cInfo);

	OrderIDs ret;

	double actQty = 0;
	bool isAll = strlen(stdCode) == 0;
	if (_orders != NULL && _orders->size() > 0)
	{
		for (auto it = _orders->begin(); it != _orders->end(); it++)
		{
			WTSOrderInfo* orderInfo = (WTSOrderInfo*)it->second;
			if(!orderInfo->isAlive())
				continue;

			bool bBuy = (orderInfo->getDirection() == WDT_LONG && orderInfo->getOffsetType() == WOT_OPEN) || (orderInfo->getDirection() == WDT_SHORT && orderInfo->getOffsetType() != WOT_OPEN);
			if(bBuy != isBuy)
				continue;

			if (isAll || strcmp(orderInfo->getCode(), cInfo._code) == 0)
			{
				if(doCancel(orderInfo))
				{
					actQty += orderInfo->getVolLeft();
					ret.emplace_back(it->first);
					//_cancel_time_cache[orderInfo->getCode()].emplace_back(TimeUtils::getLocalTimeNow());
				}
			}

			if (!decimal::eq(qty, 0) && decimal::ge(actQty, qty))
				break;
		}
	}

	return ret;
}

uint32_t TraderAdapter::openLong(const char* stdCode, double price, double qty)
{
	WTSEntrust* entrust = WTSEntrust::create(stdCode, qty, price);
	if(price == 0.0)
	{
		entrust->setPriceType(WPT_ANYPRICE);
		entrust->setTimeCondition(WTC_IOC);
	}
	else
	{
		entrust->setPriceType(WPT_LIMITPRICE);
		entrust->setTimeCondition(WTC_GFD);
	}
	entrust->setDirection(WDT_LONG);
	entrust->setOffsetType(WOT_OPEN);

	uint32_t ret = doEntrust(entrust);
	entrust->release();
	return ret;
}

uint32_t TraderAdapter::openShort(const char* stdCode, double price, double qty)
{
	WTSEntrust* entrust = WTSEntrust::create(stdCode, qty, price);
	if (price == 0.0)
	{
		entrust->setPriceType(WPT_ANYPRICE);
		entrust->setTimeCondition(WTC_IOC);
	}
	else
	{
		entrust->setPriceType(WPT_LIMITPRICE);
		entrust->setTimeCondition(WTC_GFD);
	}
	entrust->setDirection(WDT_SHORT);
	entrust->setOffsetType(WOT_OPEN);

	uint32_t ret = doEntrust(entrust);
	entrust->release();
	return ret;
}

uint32_t TraderAdapter::closeLong(const char* stdCode, double price, double qty, bool isToday /* = false */)
{
	WTSEntrust* entrust = WTSEntrust::create(stdCode, qty, price);
	if (price == 0.0)
	{
		entrust->setPriceType(WPT_ANYPRICE);
		entrust->setTimeCondition(WTC_IOC);
	}
	else
	{
		entrust->setPriceType(WPT_LIMITPRICE);
		entrust->setTimeCondition(WTC_GFD);
	}
	entrust->setDirection(WDT_LONG);
	entrust->setOffsetType(isToday ? WOT_CLOSETODAY : WOT_CLOSE);

	uint32_t ret = doEntrust(entrust);
	entrust->release();
	return ret;
}

uint32_t TraderAdapter::closeShort(const char* stdCode, double price, double qty, bool isToday /* = false */)
{
	WTSEntrust* entrust = WTSEntrust::create(stdCode, qty, price);
	if (price == 0.0)
	{
		entrust->setPriceType(WPT_ANYPRICE);
		entrust->setTimeCondition(WTC_IOC);
	}
	else
	{
		entrust->setPriceType(WPT_LIMITPRICE);
		entrust->setTimeCondition(WTC_GFD);
	}
	entrust->setDirection(WDT_SHORT);
	entrust->setOffsetType(isToday ? WOT_CLOSETODAY : WOT_CLOSE);

	uint32_t ret = doEntrust(entrust);
	entrust->release();
	return ret;
}


#pragma region "ITraderSpi�ӿ�"
void TraderAdapter::handleEvent(WTSTraderEvent e, int32_t ec)
{
	if(e == WTE_Connect)
	{
		if(ec == 0)
		{
			_trader_api->login(_cfg->getCString("user"), _cfg->getCString("pass"), _cfg->getCString("product"));
		}
		else
		{
			WTSLogger::log_dyn("trader", _id.c_str(), LL_ERROR,"[%s] Trading channel connecting failed: %d", _id.c_str(), ec);
		}
	}
	else if(e == WTE_Close)
	{
		WTSLogger::log_dyn("trader", _id.c_str(), LL_ERROR,"[%s] Trading channel disconnected: %d", _id.c_str(), ec);
		for (auto sink : _sinks)
			sink->on_channel_lost();
	}
}

void TraderAdapter::onLoginResult(bool bSucc, const char* msg, uint32_t tradingdate)
{
	if(!bSucc)
	{
		_state = AS_LOGINFAILED;
		WTSLogger::log_dyn("trader", _id.c_str(), LL_ERROR,"[%s] Trader login failed: %s", _id.c_str(), msg);

		if (_notifier)
			_notifier->notify(id(), fmt::format("login failed: {}", msg).c_str());
	}
	else
	{
		_state = AS_LOGINED;
		WTSLogger::log_dyn("trader", _id.c_str(), LL_INFO,"[%s] Trader login succeed, trading date: %u", _id.c_str(), tradingdate);
		_trading_day = tradingdate;
		_trader_api->queryPositions();	//��ֲ�
	}
}

void TraderAdapter::onLogout()
{
	
}

void TraderAdapter::onRspEntrust(WTSEntrust* entrust, WTSError *err)
{
	if (err && err->getErrorCode() != WEC_NONE)
	{
		WTSLogger::log_dyn("trader", _id.c_str(), LL_ERROR,err->getMessage());
		WTSContractInfo* cInfo = _bd_mgr->getContract(entrust->getCode(), entrust->getExchg());
		WTSCommodityInfo* commInfo = _bd_mgr->getCommodity(cInfo);
		std::string stdCode;
		if (commInfo->getCategoty() == CC_Future)
			stdCode = CodeHelper::bscFutCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
		else if (commInfo->getCategoty() == CC_FutOption)
			stdCode = CodeHelper::bscFutOptCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
		else if (commInfo->getCategoty() == CC_Stock)
			stdCode = CodeHelper::bscStkCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
		else
			stdCode = CodeHelper::bscStkCodeToStdCode(cInfo->getCode(), cInfo->getExchg(), commInfo->getProduct());

		bool isLong = (entrust->getDirection() == WDT_LONG);
		bool isToday = (entrust->getOffsetType() == WOT_CLOSETODAY);
		bool isOpen = (entrust->getOffsetType() == WOT_OPEN);
		double qty = entrust->getVolume();

		std::string action;
		if (isOpen)
			action = "open ";
		else if (isToday)
			action = "closetoday ";
		else
			action = "close ";
		action += isLong ? "long" : "short";

		WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_ERROR, fmt::format("[{}] Order placing failed: {}, instrument: {}, action: {}, qty: {}", _id.c_str(), err->getMessage(), entrust->getCode(), action.c_str(), qty).c_str());

		//����µ�ʧ��, Ҫ����δ�������
		//ʵ���з��ִ�����ʱ�����������
		//���������һ�����δ��ɵ����߼�
		//����д������������δ��ɵ�һ����Ϊ0
		//���δ��ɶ���Ϊ0����˵����һ�����ظ�֪ͨ�����ٴ�����
		double oldQty = _undone_qty[stdCode];
		if (decimal::eq(oldQty, 0))
			return;

		bool isBuy = (isLong&&isOpen) || (!isLong && !isOpen);
		double newQty = oldQty - qty*(isBuy ? 1 : -1);
		_undone_qty[stdCode] = newQty;

		WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, fmt::format("[{}] {} undone order updated, {} -> {}", _id.c_str(), stdCode.c_str(), oldQty, newQty).c_str());


		if (strlen(entrust->getUserTag()) > 0)
		{
			char* userTag = (char*)entrust->getUserTag();
			userTag += _order_pattern.size() + 1;
			uint32_t localid = strtoul(userTag, NULL, 10);

			for(auto sink : _sinks)
				sink->on_entrust(localid, stdCode.c_str(), false, err->getMessage());

			if (_notifier)
				_notifier->notify(id(), fmt::format(" Order placing failed: {}", err->getMessage()).c_str());
		}
		
	}
}

void TraderAdapter::onRspAccount(WTSArray* ayAccounts)
{
	if (_save_data)
	{
		saveData(ayAccounts);
	}

	if(_state == AS_TRADES_QRYED)
	{
		_state = AS_ALLREADY;

		WTSLogger::log_dyn("trader", _id.c_str(), LL_INFO, "[%s] Trading channel ready", _id.c_str());
		for (auto sink : _sinks)
			sink->on_channel_ready();
	}
}

void TraderAdapter::onRspPosition(const WTSArray* ayPositions)
{
	if (ayPositions && ayPositions->size() > 0)
	{
		for (auto it = ayPositions->begin(); it != ayPositions->end(); it++)
		{
			WTSPositionItem* pItem = (WTSPositionItem*)(*it);
			WTSContractInfo* cInfo = _bd_mgr->getContract(pItem->getCode(), pItem->getExchg());
			if (cInfo == NULL)
				continue;

			WTSCommodityInfo* commInfo = _bd_mgr->getCommodity(cInfo);
			std::string stdCode;
			if (commInfo->getCategoty() == CC_Future)
				stdCode = CodeHelper::bscFutCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
			else if (commInfo->getCategoty() == CC_FutOption)
				stdCode = CodeHelper::bscFutOptCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
			else if (commInfo->getCategoty() == CC_Stock)
				stdCode = CodeHelper::bscStkCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
			else
				stdCode = CodeHelper::bscStkCodeToStdCode(cInfo->getCode(), cInfo->getExchg(), commInfo->getProduct());
			PosItem& pos = _positions[stdCode];
			if (pItem->getDirection() == WDT_LONG)
			{
				pos.l_newavail = pItem->getAvailNewPos();
				pos.l_newvol = pItem->getNewPosition();
				pos.l_preavail = pItem->getAvailPrePos();
				pos.l_prevol = pItem->getPrePosition();
			}
			else
			{
				pos.s_newavail = pItem->getAvailNewPos();
				pos.s_newvol = pItem->getNewPosition();
				pos.s_preavail = pItem->getAvailPrePos();
				pos.s_prevol = pItem->getPrePosition();
			}
		}

		for (auto it = _positions.begin(); it != _positions.end(); it++)
		{
			const char* stdCode = it->first.c_str();
			const PosItem& pItem = it->second;
			printPosition(stdCode, pItem);
			for (auto sink : _sinks)
			{
				sink->on_position(stdCode, true, pItem.l_prevol, pItem.l_preavail, pItem.l_newvol, pItem.l_newavail, _trading_day);
				sink->on_position(stdCode, false, pItem.s_prevol, pItem.s_preavail, pItem.s_newvol, pItem.s_newavail, _trading_day);
			}
		}
	}

	WTSLogger::log_dyn("trader", _id.c_str(), LL_INFO,"[%s] Position data updated", _id.c_str());

	if (_state == AS_LOGINED)
	{
		_state = AS_POSITION_QRYED;

		_trader_api->queryOrders();
	}
}

void TraderAdapter::onRspOrders(const WTSArray* ayOrders)
{
	if (ayOrders)
	{
		if (_orders == NULL)
			_orders = OrderMap::create();

		_undone_qty.clear();

		for (auto it = ayOrders->begin(); it != ayOrders->end(); it++)
		{
			WTSOrderInfo* orderInfo = (WTSOrderInfo*)(*it);
			if (orderInfo == NULL)
				continue;

			WTSContractInfo* cInfo = _bd_mgr->getContract(orderInfo->getCode(), orderInfo->getExchg());
			if (cInfo == NULL)
				continue;

			bool isBuy = (orderInfo->getDirection() == WDT_LONG && orderInfo->getOffsetType() == WOT_OPEN) || (orderInfo->getDirection() == WDT_SHORT && orderInfo->getOffsetType() != WOT_OPEN);

			WTSCommodityInfo* commInfo = _bd_mgr->getCommodity(cInfo);
			std::string stdCode;
			if (commInfo->getCategoty() == CC_Future)
				stdCode = CodeHelper::bscFutCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
			else if (commInfo->getCategoty() == CC_FutOption)
				stdCode = CodeHelper::bscFutOptCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
			else if (commInfo->getCategoty() == CC_Stock)
				stdCode = CodeHelper::bscStkCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
			else
				stdCode = CodeHelper::bscStkCodeToStdCode(cInfo->getCode(), cInfo->getExchg(), commInfo->getProduct());

			_orderids.insert(orderInfo->getOrderID());

			WTSTradeStateInfo* statInfo = (WTSTradeStateInfo*)_stat_map->get(stdCode.c_str());
			if (statInfo == NULL)
			{
				statInfo = WTSTradeStateInfo::create(stdCode.c_str());
				_stat_map->add(stdCode, statInfo, false);
			}
			TradeStatInfo& statItem = statInfo->statInfo();
			if (isBuy)
			{
				statItem.b_orders++;
				statItem.b_ordqty += orderInfo->getVolume();

				if (orderInfo->isError())
				{
					statItem.b_wrongs++;
					statItem.b_wrongqty += orderInfo->getVolume() - orderInfo->getVolTraded();
				}
				else if (orderInfo->getOrderState() == WOS_Canceled)
				{			
					statItem.b_cancels++;
					statItem.b_canclqty += orderInfo->getVolume() - orderInfo->getVolTraded();
				}
				
			}
			else
			{
				statItem.s_orders++;
				statItem.s_ordqty += orderInfo->getVolume();

				if (orderInfo->isError())
				{
					statItem.s_wrongs++;
					statItem.s_wrongqty += orderInfo->getVolume() - orderInfo->getVolTraded();
				}
				else if (orderInfo->getOrderState() == WOS_Canceled)
				{
					statItem.s_cancels++;
					statItem.s_canclqty += orderInfo->getVolume() - orderInfo->getVolTraded();
				}
			}			

			if (!orderInfo->isAlive())
				continue;

			if (!StrUtil::startsWith(orderInfo->getUserTag(), _order_pattern))
				continue;;

			char* userTag = (char*)orderInfo->getUserTag();
			userTag += _order_pattern.size() + 1;
			uint32_t localid = strtoul(userTag, NULL, 10);

			{
				StdUniqueLock lock(_mtx_orders);
				_orders->add(localid, orderInfo);
			}

			double& curQty = _undone_qty[stdCode];
			curQty += orderInfo->getVolLeft()*(isBuy ? 1 : -1);
		}

		for (auto it = _undone_qty.begin(); it != _undone_qty.end(); it++)
		{
			const char* stdCode = it->first.c_str();
			const double& curQty = _undone_qty[stdCode];

			WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, fmt::format("[{}]{} undone quantity {}", _id.c_str(), stdCode, curQty).c_str());
		}
	}

	if (_state == AS_POSITION_QRYED)
	{
		_state = AS_ORDERS_QRYED;

		_trader_api->queryTrades();
	}
}

void TraderAdapter::printPosition(const char* code, const PosItem& pItem)
{
	WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, fmt::format("[{}] {} position updated, long:{}[{}]|{}[{}], short:{}[{}]|{}[{}]",
		_id.c_str(), code, pItem.l_prevol, pItem.l_preavail, pItem.l_newvol, pItem.l_newavail, 
		pItem.s_prevol, pItem.s_preavail, pItem.s_newvol, pItem.s_newavail).c_str());
}

void TraderAdapter::onRspTrades(const WTSArray* ayTrades)
{
	if (ayTrades)
	{
		for (auto it = ayTrades->begin(); it != ayTrades->end(); it++)
		{
			WTSTradeInfo* tInfo = (WTSTradeInfo*)(*it);

			WTSContractInfo* cInfo = _bd_mgr->getContract(tInfo->getCode(), tInfo->getExchg());
			if (cInfo == NULL)
				continue;

			WTSCommodityInfo* commInfo = _bd_mgr->getCommodity(cInfo);
			std::string stdCode;
			if (commInfo->getCategoty() == CC_Future)
				stdCode = CodeHelper::bscFutCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
			else if (commInfo->getCategoty() == CC_FutOption)
				stdCode = CodeHelper::bscFutOptCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
			else if (commInfo->getCategoty() == CC_Stock)
				stdCode = CodeHelper::bscStkCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
			else
				stdCode = CodeHelper::bscStkCodeToStdCode(cInfo->getCode(), cInfo->getExchg(), commInfo->getProduct());

			WTSTradeStateInfo* statInfo = (WTSTradeStateInfo*)_stat_map->get(stdCode.c_str());
			if(statInfo == NULL)
			{
				statInfo = WTSTradeStateInfo::create(stdCode.c_str());
				_stat_map->add(stdCode, statInfo, false);
			}
			TradeStatInfo& statItem = statInfo->statInfo();

			bool isLong = (tInfo->getDirection() == WDT_LONG);
			bool isOpen = (tInfo->getOffsetType() == WOT_OPEN);
			bool isCloseT = (tInfo->getOffsetType() == WOT_CLOSETODAY);
			double qty = tInfo->getVolume();

			if (isLong)
			{
				if (isOpen)
					statItem.l_openvol += qty;
				else if (isCloseT)
					statItem.l_closetvol += qty;
				else
					statItem.l_closevol += qty;
			}
			else
			{
				if (isOpen)
					statItem.s_openvol += qty;
				else if (isCloseT)
					statItem.s_closetvol += qty;
				else
					statItem.s_closevol += qty;
			}
		}

		for (auto it = _stat_map->begin(); it != _stat_map->end(); it++)
		{
			const char* stdCode = it->first.c_str();
			WTSTradeStateInfo* pItem = (WTSTradeStateInfo*)it->second;
			WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, 
				fmt::format("[{}] {} action stats updated, long opened: {}, long closed: {}, new long closed: {}, short opened: {}, short closed: {}, new short closed: {}",
				_id.c_str(), stdCode, pItem->open_volume_long(), pItem->close_volume_long(), pItem->closet_volume_long(),
				pItem->open_volume_short(), pItem->close_volume_short(), pItem->closet_volume_short()).c_str());
		}
	}

	if (_state == AS_ORDERS_QRYED)
	{
		_state = AS_TRADES_QRYED;

		_trader_api->queryAccount();
	}
}

inline const char* stateToName(WTSOrderState woState)
{
	if (woState == WOS_AllTraded)
		return "AllTrd";
	else if (woState == WOS_PartTraded_NotQueuing || woState == WOS_PartTraded_Queuing)
		return "PrtTrd";
	else if (woState == WOS_NotTraded_NotQueuing || woState == WOS_NotTraded_Queuing)
		return "UnTrd";
	else if (woState == WOS_Canceled)
		return "Cncld";
	else if (woState == WOS_Submitting)
		return "Smtting";
	else if (woState == WOS_Nottouched)
		return "UnSmt";
	else
		return "Error";
}

void TraderAdapter::onPushOrder(WTSOrderInfo* orderInfo)
{
	if (orderInfo == NULL)
		return;


	WTSContractInfo* cInfo = _bd_mgr->getContract(orderInfo->getCode(), orderInfo->getExchg());
	if (cInfo == NULL)
		return;

	WTSCommodityInfo* commInfo = _bd_mgr->getCommodity(cInfo);
	std::string stdCode;
	if (commInfo->getCategoty() == CC_Future)
		stdCode = CodeHelper::bscFutCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
	else if (commInfo->getCategoty() == CC_FutOption)
		stdCode = CodeHelper::bscFutOptCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
	else if (commInfo->getCategoty() == CC_Stock)
		stdCode = CodeHelper::bscStkCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
	else
		stdCode = CodeHelper::bscStkCodeToStdCode(cInfo->getCode(), cInfo->getExchg(), commInfo->getProduct());

	bool isBuy = (orderInfo->getDirection() == WDT_LONG && orderInfo->getOffsetType() == WOT_OPEN) || (orderInfo->getDirection() == WDT_SHORT && orderInfo->getOffsetType() != WOT_OPEN);
	
	//�����Ļ�, Ҫ����ͳ������
	if (orderInfo->getOrderState() == WOS_Canceled)
	{
		WTSTradeStateInfo* statInfo = (WTSTradeStateInfo*)_stat_map->get(stdCode.c_str());
		if (statInfo == NULL)
		{
			statInfo = WTSTradeStateInfo::create(stdCode.c_str());
			_stat_map->add(stdCode, statInfo, false);
		}
		TradeStatInfo& statItem = statInfo->statInfo();
		if(isBuy)
		{
			if (orderInfo->isError())//��Ҫ�ͳ������ֿ�
			{
				statItem.b_wrongs++;
				statItem.b_wrongqty += orderInfo->getVolume() - orderInfo->getVolTraded();
			}
			else
			{
				statItem.b_cancels++;
				statItem.b_canclqty += orderInfo->getVolume() - orderInfo->getVolTraded();
			}
		}
		else
		{
			if (orderInfo->isError())//��Ҫ�ͳ������ֿ�
			{
				statItem.s_wrongs++;
				statItem.s_wrongqty += orderInfo->getVolume() - orderInfo->getVolTraded();
			}
			else
			{
				statItem.s_cancels++;
				statItem.s_canclqty += orderInfo->getVolume() - orderInfo->getVolTraded();
			}
		}
	}

	WTSLogger::log_dyn("trader", _id.c_str(), LL_INFO,"[%s] Order notified, instrument: %s, usertag: %s, state: %s", _id.c_str(), stdCode.c_str(), orderInfo->getUserTag(), stateToName(orderInfo->getOrderState()));

	//�����������, ������wt�Ķ���, ��Ҫ�ȸ���δ�������
	if (orderInfo->getOrderState() == WOS_Canceled && StrUtil::startsWith(orderInfo->getUserTag(), _order_pattern, false))
	{
		//������ʱ��, Ҫ����δ���
		bool isLong = (orderInfo->getDirection() == WDT_LONG);
		bool isOpen = (orderInfo->getOffsetType() == WOT_OPEN);
		bool isToday = (orderInfo->getOffsetType() == WOT_CLOSETODAY);
		double qty = orderInfo->getVolume() - orderInfo->getVolTraded();

		bool isBuy = (isLong&&isOpen) || (!isLong&&!isOpen);
		double oldQty = _undone_qty[stdCode];
		double newQty = oldQty - qty*(isBuy ? 1 : -1);
		_undone_qty[stdCode] = newQty;
		WTSLogger::log_dyn("trader", _id.c_str(), LL_INFO, fmt::format("[{}] {} qty of undone order updated, {} -> {}", _id.c_str(), stdCode.c_str(), oldQty, newQty).c_str());

		WTSLogger::log_dyn("trader", _id.c_str(), LL_INFO, fmt::format("[{}] Order {} of {} canceled:{}, action: {}, leftqty: {}",
			_id.c_str(), orderInfo->getUserTag(), stdCode.c_str(), orderInfo->getStateMsg(),
			formatAction(orderInfo->getDirection(), orderInfo->getOffsetType()), qty).c_str());
	}

	//�ȼ��ö����ǲ��ǵ�һ�����͹���
	//����ǵ�һ�����͹���, ��Ҫ���ݿ�ƽ���¿�ƽ
	if (strlen(orderInfo->getOrderID()) > 0)
	{
		auto it = _orderids.find(orderInfo->getOrderID());
		if (it == _orderids.end())
		{
			//�ȰѶ����Ż�������, ��ֹ�ظ�����
			_orderids.insert(orderInfo->getOrderID());

			WTSTradeStateInfo* statInfo = (WTSTradeStateInfo*)_stat_map->get(stdCode.c_str());
			if (statInfo == NULL)
			{
				statInfo = WTSTradeStateInfo::create(stdCode.c_str());
				_stat_map->add(stdCode, statInfo, false);
			}
			TradeStatInfo& statItem = statInfo->statInfo();
			if (isBuy)
			{
				statItem.b_orders++;
				statItem.b_ordqty += orderInfo->getVolume();
			}
			else
			{
				statItem.s_orders++;
				statItem.s_ordqty += orderInfo->getVolume();
			}

			//ֻ��ƽ����Ҫ���¿�ƽ
			if (orderInfo->getOffsetType() != WOT_OPEN)
			{
				//const char* code = stdCode.c_str();
				bool isLong = (orderInfo->getDirection() == WDT_LONG);
				bool isToday = (orderInfo->getOffsetType() == WOT_CLOSETODAY);
				double qty = orderInfo->getVolume();

				PosItem& pItem = _positions[stdCode];
				if (isLong)	//ƽ��
				{
					if (isToday)
					{
						pItem.l_newavail -= min(pItem.l_newavail, qty);	//�����ƽ��, ��ֻ��Ҫ���¿�ƽ���
					}
					else
					{
						double left = qty;

						//�����ƽ��, ���ȸ��¿�ƽ���, ����ʣ��, �ٸ��¿�ƽ���
						//���Ʒ������ƽ��ƽ��, Ҳ�����������, ��Ϊƽ��������������ܳ������
						double maxQty = min(pItem.l_preavail, qty);
						pItem.l_preavail -= maxQty;
						left -= maxQty;

						if (left > 0)
							pItem.l_newavail -= min(pItem.l_newavail, left);
					}
				}
				else //ƽ��
				{
					if (isToday)
					{
						pItem.s_newavail -= min(pItem.s_newavail, qty);
					}
					else
					{
						double left = qty;

						double maxQty = min(pItem.s_preavail, qty);
						pItem.s_preavail -= maxQty;
						left -= maxQty;

						if (left > 0)
							pItem.s_newavail -= min(pItem.s_newavail, left);
					}
				}
				printPosition(stdCode.c_str(), pItem);
			}
		}
		else if (orderInfo->getOrderState() == WOS_Canceled && orderInfo->getOffsetType() != WOT_OPEN)
		{
			//����������ǵ�һ������, �ҳ�����, ��Ҫ���¿�ƽ��
			//const char* code = orderInfo->getCode();
			bool isLong = (orderInfo->getDirection() == WDT_LONG);
			bool isToday = (orderInfo->getOffsetType() == WOT_CLOSETODAY);
			double qty = orderInfo->getVolume() - orderInfo->getVolTraded();

			PosItem& pItem = _positions[stdCode];
			if (isLong)	//ƽ��
			{
				if (isToday)
				{
					pItem.l_newavail += qty;	//�����ƽ��, ��ֻ��Ҫ���¿�ƽ���
				}
				else
				{
					pItem.l_preavail += qty;
					if (pItem.l_preavail > pItem.l_prevol)
					{
						pItem.l_newavail += (pItem.l_preavail - pItem.l_prevol);
						pItem.l_preavail = pItem.l_prevol;
					}
				}
			}
			else //ƽ��
			{
				if (isToday)
				{
					pItem.s_newavail += qty;
				}
				else
				{
					pItem.s_preavail += qty;
					if (pItem.s_preavail > pItem.s_prevol)
					{
						pItem.s_newavail += (pItem.s_preavail - pItem.s_prevol);
						pItem.s_preavail = pItem.s_prevol;
					}
				}
			}
			printPosition(stdCode.c_str(), pItem);
		}
	}

	uint32_t localid = 0;

	//�ȿ����ǲ���wt����ȥ�ĵ���
	if (StrUtil::startsWith(orderInfo->getUserTag(), _order_pattern))
	{
		char* userTag = (char*)orderInfo->getUserTag();
		userTag += _order_pattern.size() + 1;
		localid = strtoul(userTag, NULL, 10);
	}

	//�����wt����ȥ�ĵ�������Ҫ�����ڲ�����
	if(localid != 0)
	{
		StdUniqueLock lock(_mtx_orders);
		if (!orderInfo->isAlive() && _orders)
		{
			_orders->remove(localid);
		}
		else
		{
			if (_orders == NULL)
				_orders = OrderMap::create();

			_orders->add(localid, orderInfo);
		}

		//֪ͨ���м����ӿ�
		for (auto sink : _sinks)
			sink->on_order(localid, stdCode.c_str(), isBuy, orderInfo->getVolume(), orderInfo->getVolLeft(), orderInfo->getPrice(), orderInfo->getOrderState() == WOS_Canceled);
	}

	//�����ǲ����ڲ�����,����������,��Ҫд����־��
	if (_save_data && !orderInfo->isAlive())
	{
		logOrder(localid, stdCode.c_str(), orderInfo);
	}

	if (_notifier)
		_notifier->notify(id(), localid, stdCode.c_str(), orderInfo);
}

void TraderAdapter::onPushTrade(WTSTradeInfo* tradeRecord)
{
	WTSContractInfo* cInfo = _bd_mgr->getContract(tradeRecord->getCode(), tradeRecord->getExchg());
	if (cInfo == NULL)
		return;

	bool isLong = (tradeRecord->getDirection() == WDT_LONG);
	bool isOpen = (tradeRecord->getOffsetType() == WOT_OPEN);
	bool isBuy = (tradeRecord->getDirection() == WDT_LONG && tradeRecord->getOffsetType() == WOT_OPEN) || (tradeRecord->getDirection() == WDT_SHORT && tradeRecord->getOffsetType() != WOT_OPEN);

	WTSCommodityInfo* commInfo = _bd_mgr->getCommodity(cInfo);
	std::string stdCode;
	if (commInfo->getCategoty() == CC_Future)
		stdCode = CodeHelper::bscFutCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
	else if (commInfo->getCategoty() == CC_FutOption)
		stdCode = CodeHelper::bscFutOptCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
	else if (commInfo->getCategoty() == CC_Stock)
		stdCode = CodeHelper::bscStkCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
	else
		stdCode = CodeHelper::bscStkCodeToStdCode(cInfo->getCode(), cInfo->getExchg(), commInfo->getProduct());

	WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, 
		fmt::format("[{}] Trade notified, instrument: {}, usertag: {}, trdqty: {}, trdprice: {}", 
			_id.c_str(), stdCode.c_str(), tradeRecord->getUserTag(), tradeRecord->getVolume(), tradeRecord->getPrice()).c_str());

	uint32_t localid = 0;
	if (StrUtil::startsWith(tradeRecord->getUserTag(), _order_pattern, false))
	{
		char* userTag = (char*)tradeRecord->getUserTag();
		userTag += _order_pattern.size() + 1;
		localid = strtoul(userTag, NULL, 10);

		double oldQty = _undone_qty[stdCode];
		double newQty = oldQty - tradeRecord->getVolume()*(isBuy ? 1 : -1);
		_undone_qty[stdCode] = newQty;
		WTSLogger::log_dyn_raw("trader", _id.c_str(), LL_INFO, fmt::format("[{}] {} qty of undone orders updated, {} -> {}", _id.c_str(), stdCode.c_str(), oldQty, newQty).c_str());
	}

	PosItem& pItem = _positions[stdCode];
	WTSTradeStateInfo* statInfo = (WTSTradeStateInfo*)_stat_map->get(stdCode.c_str());
	if (statInfo == NULL)
	{
		statInfo = WTSTradeStateInfo::create(stdCode.c_str());
		_stat_map->add(stdCode, statInfo, false);
	}

	TradeStatInfo& statItem = statInfo->statInfo();
	double vol = tradeRecord->getVolume();
	if(isLong)
	{
		if (isOpen)
		{
			pItem.l_newvol += vol;

			if(commInfo->getCategoty() != CC_Stock)
				pItem.l_newavail += vol;

			statItem.l_openvol += vol;
		}
		else if (tradeRecord->getOffsetType() == WOT_CLOSETODAY)
		{
			pItem.l_newvol -= vol;

			statItem.l_closevol += vol;
		}
		else
		{
			double left = vol;
			double maxVol = min(left, pItem.l_prevol);
			pItem.l_prevol -= maxVol;
			left -= maxVol;
			pItem.l_newvol -= left;

			statItem.l_closevol += vol;
		}
	}
	else
	{
		if (isOpen)
		{
			pItem.s_newvol += vol;
			if (commInfo->getCategoty() != CC_Stock)
				pItem.s_newavail += vol;

			statItem.s_openvol += vol;
		}
		else if (tradeRecord->getOffsetType() == WOT_CLOSETODAY)
		{
			pItem.s_newvol -= vol;

			statItem.s_closevol += vol;
		}
		else
		{
			double left = vol;
			double maxVol = min(left, pItem.s_prevol);
			pItem.s_prevol -= maxVol;
			left -= maxVol;
			pItem.s_newvol -= left;

			statItem.s_closevol += vol;
		}
	}

	printPosition(stdCode.c_str(), pItem);

	for (auto sink : _sinks)
		sink->on_trade(localid, stdCode.c_str(), isBuy, vol, tradeRecord->getPrice());

	if (_save_data)
	{
		logTrade(localid, stdCode.c_str(), tradeRecord);
	}

	if (_notifier)
		_notifier->notify(id(), localid, stdCode.c_str(), tradeRecord);

	_trader_api->queryAccount();
	_trader_api->queryPositions();	//��ֲ�
}

void TraderAdapter::onTraderError(WTSError* err)
{
	if(err)
		WTSLogger::log_dyn("trader", _id.c_str(), LL_ERROR,"[%s] Error of trading channel occured: %s", _id.c_str(), err->getMessage());

	if (_notifier)
		_notifier->notify(id(), fmt::format("Trading channel error: {}", err->getMessage()).c_str());
}

IBaseDataMgr* TraderAdapter::getBaseDataMgr()
{
	return _bd_mgr;
}

void TraderAdapter::handleTraderLog(WTSLogLevel ll, const char* format, ...)
{
	char szBuf[2048] = { 0 };
	uint32_t length = sprintf(szBuf, "[%s]", _id.c_str());
	strcat(szBuf, format);
	va_list args;
	va_start(args, format);
	//WTSLogger::log2_direct("trader", ll, szBuf, args);
	WTSLogger::vlog_dyn("trader", _id.c_str(), ll, szBuf, args);
	va_end(args);
}

#pragma endregion "ITraderSpi�ӿ�"


//////////////////////////////////////////////////////////////////////////
//CTPWrapperMgr
bool TraderAdapterMgr::addAdapter(const char* tname, TraderAdapterPtr& adapter)
{
	if (adapter == NULL || strlen(tname) == 0)
		return false;

	auto it = _adapters.find(tname);
	if(it != _adapters.end())
	{
		WTSLogger::error("Same name of trading channels: %s", tname);
		return false;
	}

	_adapters[tname] = adapter;

	return true;
}

TraderAdapterPtr TraderAdapterMgr::getAdapter(const char* tname)
{
	auto it = _adapters.find(tname);
	if (it != _adapters.end())
	{
		return it->second;
	}

	return TraderAdapterPtr();
}

void TraderAdapterMgr::run()
{
	for (auto it = _adapters.begin(); it != _adapters.end(); it++)
	{
		it->second->run();
	}

	WTSLogger::info("%u trading channels started", _adapters.size());
}

void TraderAdapterMgr::release()
{
	for (auto it = _adapters.begin(); it != _adapters.end(); it++)
	{
		it->second->release();
	}

	_adapters.clear();
}