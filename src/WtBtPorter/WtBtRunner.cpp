/*!
 * \file WtBtRunner.cpp
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#include "WtBtRunner.h"
#include "ExpCtaMocker.h"
#include "ExpSelMocker.h"
#include "ExpHftMocker.h"

#include <iomanip>

#include "../WtBtCore/ExecMocker.h"
#include "../WtBtCore/WtHelper.h"

#include "../Includes/WTSDataDef.hpp"

#include "../Share/TimeUtils.hpp"
#include "../Share/JsonToVariant.hpp"
#include "../Share/StrUtil.hpp"
#include "../Share/DLLHelper.hpp"

#include "../WTSTools/WTSLogger.h"
#include "../WTSTools/WTSCmpHelper.hpp"

#include "../WTSUtils/SignalHook.hpp"
#include <iostream>

#ifdef _WIN32
#define my_stricmp _stricmp
#else
#define my_stricmp strcasecmp
#endif

extern std::string getBinDir();


WtBtRunner::WtBtRunner()
	: _cta_mocker(NULL)
	, _sel_mocker(NULL)

	, _cb_cta_init(NULL)
	, _cb_cta_tick(NULL)
	, _cb_cta_calc(NULL)
	, _cb_cta_calc_done(NULL)
	, _cb_cta_bar(NULL)
	, _cb_cta_sessevt(NULL)

	, _cb_sel_init(NULL)
	, _cb_sel_tick(NULL)
	, _cb_sel_calc(NULL)
	, _cb_sel_calc_done(NULL)
	, _cb_sel_bar(NULL)
	, _cb_sel_sessevt(NULL)

	, _cb_hft_init(NULL)
	, _cb_hft_tick(NULL)
	, _cb_hft_bar(NULL)
	, _cb_hft_ord(NULL)
	, _cb_hft_trd(NULL)
	, _cb_hft_entrust(NULL)
	, _cb_hft_chnl(NULL)

	, _cb_hft_orddtl(NULL)
	, _cb_hft_ordque(NULL)
	, _cb_hft_trans(NULL)

	, _cb_hft_sessevt(NULL)
	, _inited(false)
	, _running(false)
	, _async(false)
{
	install_signal_hooks([](const char* message) {
		WTSLogger::error(message);
	});
}


WtBtRunner::~WtBtRunner()
{
}

void WtBtRunner::registerCtaCallbacks(FuncStraInitCallback cbInit, FuncStraTickCallback cbTick, FuncStraCalcCallback cbCalc, 
	FuncStraBarCallback cbBar, FuncSessionEvtCallback cbSessEvt, FuncStraCalcCallback cbCalcDone/* = NULL*/)
{
	_cb_cta_init = cbInit;
	_cb_cta_tick = cbTick;
	_cb_cta_calc = cbCalc;
	_cb_cta_bar = cbBar;
	_cb_cta_sessevt = cbSessEvt;

	_cb_cta_calc_done = cbCalcDone;

	WTSLogger::info("Callbacks of CTA engine registration done");
}

void WtBtRunner::registerSelCallbacks(FuncStraInitCallback cbInit, FuncStraTickCallback cbTick, FuncStraCalcCallback cbCalc, 
	FuncStraBarCallback cbBar, FuncSessionEvtCallback cbSessEvt, FuncStraCalcCallback cbCalcDone/* = NULL*/)
{
	_cb_sel_init = cbInit;
	_cb_sel_tick = cbTick;
	_cb_sel_calc = cbCalc;
	_cb_sel_bar = cbBar;
	_cb_sel_sessevt = cbSessEvt;

	_cb_sel_calc_done = cbCalcDone;

	WTSLogger::info("Callbacks of SEL engine registration done");
}

void WtBtRunner::registerHftCallbacks(FuncStraInitCallback cbInit, FuncStraTickCallback cbTick, FuncStraBarCallback cbBar,
	FuncHftChannelCallback cbChnl, FuncHftOrdCallback cbOrd, FuncHftTrdCallback cbTrd, FuncHftEntrustCallback cbEntrust, 
	FuncStraOrdDtlCallback cbOrdDtl, FuncStraOrdQueCallback cbOrdQue, FuncStraTransCallback cbTrans, FuncSessionEvtCallback cbSessEvt)
{
	_cb_hft_init = cbInit;
	_cb_hft_tick = cbTick;
	_cb_hft_bar = cbBar;

	_cb_hft_chnl = cbChnl;
	_cb_hft_ord = cbOrd;
	_cb_hft_trd = cbTrd;
	_cb_hft_entrust = cbEntrust;

	_cb_hft_orddtl = cbOrdDtl;
	_cb_hft_ordque = cbOrdQue;
	_cb_hft_trans = cbTrans;

	_cb_hft_sessevt = cbSessEvt;

	WTSLogger::info("Callbacks of HFT engine registration done");
}

uint32_t WtBtRunner::initCtaMocker(const char* name, int32_t slippage /* = 0 */, bool hook /* = false */, bool persistData /* = true */)
{
	if(_cta_mocker)
	{
		delete _cta_mocker;
		_cta_mocker = NULL;
	}

	_cta_mocker = new ExpCtaMocker(&_replayer, name, slippage, persistData, &_notifier);
	if(hook) _cta_mocker->install_hook();

	_cta_mocker->set_initacc(_init_money);
	_replayer.register_sink(_cta_mocker, name);
	return _cta_mocker->id();
}

uint32_t WtBtRunner::initHftMocker(const char* name, bool hook/* = false*/)
{
	if (_hft_mocker)
	{
		delete _hft_mocker;
		_hft_mocker = NULL;
	}

	_hft_mocker = new ExpHftMocker(&_replayer, name);
	if (hook) _hft_mocker->install_hook();
	_hft_mocker->set_initacc(_init_money);
	_replayer.register_sink(_hft_mocker, name);
	return _hft_mocker->id();
}

uint32_t WtBtRunner::initSelMocker(const char* name, uint32_t date, uint32_t time, const char* period, const char* trdtpl /* = "CHINA" */, const char* session /* = "TRADING" */, int32_t slippage/* = 0*/)
{
	if (_sel_mocker)
	{
		delete _sel_mocker;
		_sel_mocker = NULL;
	}

	_sel_mocker = new ExpSelMocker(&_replayer, name, slippage);
	_replayer.register_sink(_sel_mocker, name);

	_replayer.register_task(_sel_mocker->id(), date, time, period, trdtpl, session);
	return _sel_mocker->id();
}

void WtBtRunner::ctx_on_bar(uint32_t id, const char* stdCode, const char* period, WTSBarStruct* newBar, EngineType eType/*= ET_CTA*/)
{
	switch (eType)
	{
	case ET_CTA: if (_cb_cta_bar) _cb_cta_bar(id, stdCode, period, newBar); break;
	case ET_HFT: if (_cb_hft_bar) _cb_hft_bar(id, stdCode, period, newBar); break;
	case ET_SEL: if (_cb_sel_bar) _cb_sel_bar(id, stdCode, period, newBar); break;
	default:
		break;
	}
}

void WtBtRunner::ctx_on_calc(uint32_t id, uint32_t curDate, uint32_t curTime, EngineType eType /* = ET_CTA */)
{
	switch (eType)
	{
	case ET_CTA: if (_cb_cta_calc) _cb_cta_calc(id, curDate, curTime); break;
	case ET_SEL: if (_cb_sel_calc) _cb_sel_calc(id, curDate, curTime); break;
	default:
		break;
	}
}

void WtBtRunner::ctx_on_calc_done(uint32_t id, uint32_t curDate, uint32_t curTime, EngineType eType /* = ET_CTA */)
{
	switch (eType)
	{
	case ET_CTA: if (_cb_cta_calc_done) _cb_cta_calc_done(id, curDate, curTime); break;
	case ET_SEL: if (_cb_sel_calc_done) _cb_sel_calc_done(id, curDate, curTime); break;
	default:
		break;
	}
}

void WtBtRunner::ctx_on_init(uint32_t id, EngineType eType/*= ET_CTA*/)
{
	switch (eType)
	{
	case ET_CTA: if (_cb_cta_init) _cb_cta_init(id); break;
	case ET_HFT: if (_cb_hft_init) _cb_hft_init(id); break;
	case ET_SEL: if (_cb_sel_init) _cb_sel_init(id); break;
	default:
		break;
	}
}

void WtBtRunner::ctx_on_session_event(uint32_t id, uint32_t curTDate, bool isBegin /* = true */, EngineType eType /* = ET_CTA */)
{
	switch (eType)
	{
	case ET_CTA: if (_cb_cta_sessevt) _cb_cta_sessevt(id, curTDate, isBegin); break;
	case ET_HFT: if (_cb_hft_sessevt) _cb_hft_sessevt(id, curTDate, isBegin); break;
	case ET_SEL: if (_cb_sel_sessevt) _cb_sel_sessevt(id, curTDate, isBegin); break;
	default:
		break;
	}
}

void WtBtRunner::ctx_on_tick(uint32_t id, const char* stdCode, WTSTickData* newTick, EngineType eType/*= ET_CTA*/)
{
	switch (eType)
	{
	case ET_CTA: if (_cb_cta_tick) _cb_cta_tick(id, stdCode, &newTick->getTickStruct()); break;
	case ET_HFT: if (_cb_hft_tick) _cb_hft_tick(id, stdCode, &newTick->getTickStruct()); break;
	case ET_SEL: if (_cb_sel_tick) _cb_sel_tick(id, stdCode, &newTick->getTickStruct()); break;
	default:
		break;
	}
}

void WtBtRunner::hft_on_order_queue(uint32_t id, const char* stdCode, WTSOrdQueData* newOrdQue)
{
	if (_cb_hft_ordque)
		_cb_hft_ordque(id, stdCode, &newOrdQue->getOrdQueStruct());
}

void WtBtRunner::hft_on_order_detail(uint32_t id, const char* stdCode, WTSOrdDtlData* newOrdDtl)
{
	if (_cb_hft_orddtl)
		_cb_hft_orddtl(id, stdCode, &newOrdDtl->getOrdDtlStruct());
}

void WtBtRunner::hft_on_transaction(uint32_t id, const char* stdCode, WTSTransData* newTrans)
{
	if (_cb_hft_trans)
		_cb_hft_trans(id, stdCode, &newTrans->getTransStruct());
}

void WtBtRunner::hft_on_channel_ready(uint32_t cHandle, const char* trader)
{
	if (_cb_hft_chnl)
		_cb_hft_chnl(cHandle, trader, 1000/*CHNL_EVENT_READY*/);
}

void WtBtRunner::hft_on_entrust(uint32_t cHandle, WtUInt32 localid, const char* stdCode, bool bSuccess, const char* message, const char* userTag)
{
	if (_cb_hft_entrust)
		_cb_hft_entrust(cHandle, localid, stdCode, bSuccess, message, userTag);
}

void WtBtRunner::hft_on_order(uint32_t cHandle, WtUInt32 localid, const char* stdCode, bool isBuy, double totalQty, double leftQty, double price, bool isCanceled, const char* userTag)
{
	if (_cb_hft_ord)
		_cb_hft_ord(cHandle, localid, stdCode, isBuy, totalQty, leftQty, price, isCanceled, userTag);
}

void WtBtRunner::hft_on_trade(uint32_t cHandle, WtUInt32 localid, const char* stdCode, bool isBuy, double vol, double price, const char* userTag)
{
	if (_cb_hft_trd)
		_cb_hft_trd(cHandle, localid, stdCode, isBuy, vol, price, userTag);
}

extern std::string getBinDir();

void WtBtRunner::init(const char* logProfile /* = "" */, bool isFile /* = true */)
{
	WTSLogger::init(logProfile, isFile);

	WtHelper::setInstDir(getBinDir().c_str());
}

unsigned long long timetrans(std::string time,bool isStart)
{
	uint64_t ulltime = 0;
	if (!time.empty())
	{
		std::string year, month, day;
		year = time.substr(0, 4);
		month = time.substr(5, 2);
		day = time.substr(8, 2);

		std::string s_time = "";
		s_time += year;
		s_time += month;
		s_time += day;
		stringstream strValue;
		if (isStart)
		{
			s_time += "2100";
			strValue << s_time;
			strValue >> ulltime;
			return ulltime;
		}
		else
		{
			s_time += "1515";
			strValue << s_time;
			strValue >> ulltime;
			return ulltime;
		}
	}
	return 0;
}

void WtBtRunner::config(const char* cfgFile, bool isFile /* = true */)
{
	if(_inited)
	{
		WTSLogger::error("WtBtEngine has already been inited");
		return;
	}
	std::string content;
	if (isFile)
		StdFile::read_file_content(cfgFile, content);
	else
		content = cfgFile;

	rj::Document root;
	if (root.Parse(content.c_str()).HasParseError())
	{
		WTSLogger::info("Parsing configuration file failed");
		return;
	}

#ifdef _WIN32
	std::string module = getBinDir() + "libmysql.dll";
	DLLHelper::load_library(module.c_str());
#endif

	WTSVariant* cfg = WTSVariant::createObject();
	jsonToVariant(root, cfg);

	//????????????????
	initEvtNotifier(cfg->get("notifier"));

	_replayer.init(cfg->get("replayer"), &_notifier);

	WTSVariant* cfgEnv = cfg->get("env");
	_init_money = cfgEnv->getDouble("init_money");
	const char* mode = cfgEnv->getCString("mocker");
	WTSVariant* cfgMode = cfg->get(mode);

	if (strcmp(mode, "cta") == 0 && cfgMode)
	{
		const char* name = cfgMode->getCString("name");
		int32_t slippage = cfgMode->getInt32("slippage");
		_cta_mocker = new ExpCtaMocker(&_replayer, name, slippage, &_notifier);
		_cta_mocker->init_cta_factory(cfgMode);
		_cta_mocker->set_initacc(_init_money);
		_replayer.register_sink(_cta_mocker, name);
	}
	else if (strcmp(mode, "hft") == 0 && cfgMode)
	{
		const char* name = cfgMode->getCString("name");
		_hft_mocker = new ExpHftMocker(&_replayer, name);
		_hft_mocker->init_hft_factory(cfgMode);
		_hft_mocker->set_initacc(_init_money);
		_replayer.register_sink(_hft_mocker, name);
	}
	else if (strcmp(mode, "sel") == 0 && cfgMode)
	{
		const char* name = cfgMode->getCString("name");
		int32_t slippage = cfgMode->getInt32("slippage");
		_sel_mocker = new ExpSelMocker(&_replayer, name, slippage);
		_sel_mocker->init_sel_factory(cfgMode);
		_replayer.register_sink(_sel_mocker, name);

		WTSVariant* cfgTask = cfgMode->get("task");
		if(cfgTask)
			_replayer.register_task(_sel_mocker->id(), cfgTask->getUInt32("date"), cfgTask->getUInt32("time"), 
				cfgTask->getCString("period"), cfgTask->getCString("trdtpl"), cfgTask->getCString("session"));
	}
	else if (strcmp(mode, "exec") == 0 && cfgMode)
	{
		const char* name = cfgMode->getCString("name");
		_exec_mocker = new ExecMocker(&_replayer);
		_exec_mocker->init(cfgMode);
		_replayer.register_sink(_exec_mocker, name);
	}
}

void WtBtRunner::config_setting(const char* cfgFile, bool isFile /* = true */)
{
	if (_inited)
	{
		WTSLogger::error("WtBtEngine has already been inited");
		return;
	}
	std::string content;
	if (isFile)
		StdFile::read_file_content(cfgFile, content);
	else
		content = cfgFile;

	rj::Document root;
	if (root.Parse(content.c_str()).HasParseError())
	{
		WTSLogger::info("Parsing configuration file failed");
		return;
	}

	WTSVariant* cfg = WTSVariant::createObject();
	jsonToVariant(root, cfg);

	const char* markerDataType = cfg->getCString("marketDataType");
	const char* traderWsUrl = cfg->getCString("traderWsUrl");
	const char* strategyRecordId = cfg->getCString("strategyRecordId");
	const char* type = cfg->getCString("type");
	const char* quotationSubUrl = cfg->getCString("quotationSubUrl");
	const char* initMoney = cfg->getCString("initMoney");
	const char* backtestStartDate = cfg->getCString("backtestStartDate");
	const char* backtestEndDate = cfg->getCString("backtestEndDate");
	const char* logstashHost = cfg->getCString("logstashHost");
	int logstashPort = cfg->getInt32("logstashPort");
	/*const char* RealStartDate = cfg->getCString("RealStartDate");
	const char* RealEndDate = cfg->getCString("RealEndDate");*/
	WTSLogger::info("after read setting.json");

	std::string content_1;
	StdFile::read_file_content("config.json", content_1);

	rj::Document root_1;
	if (root_1.Parse(content_1.c_str()).HasParseError())
	{
		WTSLogger::info("Parsing configuration file failed");
		return;
	}

	//assert(root_1.IsObject());
	WTSVariant* cfg_1 = WTSVariant::createObject();
	jsonToVariant(root_1, cfg_1);

	unsigned long long stime = timetrans(backtestStartDate, true);
	unsigned long long etime = timetrans(backtestEndDate, false);
	WTSVariant* cfgEnv = cfg_1->get("env");
	const char* mode = cfgEnv->getCString("mocker");
	WTSVariant* cfgMode = cfg_1->get(mode);
	rj::Value& var = root_1;
	root_1["replayer"]["stime"].SetUint64(stime);
	root_1["replayer"]["etime"].SetUint64(etime);
	double dinitMoney = atof(initMoney);
	root_1["env"]["init_money"] = dinitMoney;
	if (strcmp(mode, "hft") == 0 && cfgMode)
	{
		std::string stra_id = strategyRecordId;
		root_1["hft"]["name"].SetString(stra_id.c_str(), root_1.GetAllocator());
	}

	rj::StringBuffer strBuf;
	rj::Writer<rj::StringBuffer> write(strBuf);
	root_1.Accept(write);

	std::string data = strBuf.GetString();
	std::cout << data << std::endl;
	//rj::Value& var = root_1;
	//if (root_1.HasMember("env"))
	//{
	//	var = root_1["env"];
	//	if (var.HasMember("init_money"))
	//	{
	//		var["init_money"] = initMoney;
	//		// std::cout << var.getstring() << std::endl;
	//	}
	//}
	/*std::string data = strBuf.GetString();
	std::cout << data << std::endl;
	std::ofstream out("config.json", std::ios_base::out);
	out << data << std::endl;*/
	/*write.StartObject();

	write.Key("env");
	write.StartObject();
	write.Key("init_money");
	write.Double(initMoney);
	write.EndObject();
	write.EndObject();*/

	/*std::cout << strBuf.GetString() << std::endl;
	std::string data = strBuf.GetString();

	root_1.Accept(write);
	std::cout << strBuf.GetString() << std::endl;*/
	std::ofstream out("config.json", std::ios_base::out);
	out << data << std::endl;
	/*root_1.Accept(write);
	std::string data = strBuf.GetString();
	std::cout << data << std::endl;*/
	
	int a;

}

void WtBtRunner::run(bool bNeedDump /* = false */, bool bAsync /* = false */)
{
	if (_running)
		return;

	_async = bAsync;

	WTSLogger::info("Backtesting will run in %s mode", _async ? "async" : "sync");

	if (_cta_mocker)
		_cta_mocker->enable_hook(_async);
	else if (_hft_mocker)
		_hft_mocker->enable_hook(_async);

	_replayer.prepare();

	_worker.reset(new StdThread([this, bNeedDump]() {
		_running = true;
		try
		{
			_replayer.run(bNeedDump);
		}
		catch (...)
		{
			WTSLogger::error("Exception raised while worker running");
			print_stack_trace([](const char* message) {
				WTSLogger::error(message);
			});
		}
		WTSLogger::debug("Worker thread of backtest finished");
		_running = false;

	}));

	if (!bAsync)
		_worker->join();
}

void WtBtRunner::stop()
{
	if (!_running)
	{
		if (_worker)
		{
			_worker->join();
			_worker.reset();
		}
		return;
	}

	_replayer.stop();

	WTSLogger::debug("Notify to finish last round");

	if (_cta_mocker)
		_cta_mocker->step_calc();

	if (_hft_mocker)
		_hft_mocker->step_tick();

	WTSLogger::debug("Last round ended");

	if (_worker)
	{
		_worker->join();
		_worker.reset();
	}

	WTSLogger::freeAllDynLoggers();

	WTSLogger::debug("Backtest stopped");
}

void WtBtRunner::release()
{
	WTSLogger::stop();
}

void WtBtRunner::set_time_range(WtUInt64 stime, WtUInt64 etime)
{
	_replayer.set_time_range(stime, etime);
}

void WtBtRunner::enable_tick(bool bEnabled /* = true */)
{
	_replayer.enable_tick(bEnabled);
}

void WtBtRunner::clear_cache()
{
	_replayer.clear_cache();
}

const char* LOG_TAGS[] = {
	"all",
	"debug",
	"info",
	"warn",
	"error",
	"fatal",
	"none",
};

bool WtBtRunner::initEvtNotifier(WTSVariant* cfg)
{
	if (cfg == NULL || cfg->type() != WTSVariant::VT_Object)
		return false;

	_notifier.init(cfg);

	return true;
}