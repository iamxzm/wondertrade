#include "WtExecRunner.h"

#include "../WtCore/WtHelper.h"
#include "../WTSTools/WTSLogger.h"
#include "../WTSUtils/WTSCfgLoader.h"

#include "../Includes/WTSContractInfo.hpp"
#include "../Includes/WTSVariant.hpp"
#include "../Share/CodeHelper.hpp"
#include "../Share/ModuleHelper.hpp"
#include "../Share/TimeUtils.hpp"
#include "../WTSUtils/SignalHook.hpp"

#ifdef _MSC_VER
#include "../Common/mdump.h"
#include <boost/filesystem.hpp>
const char* getModuleName()
{
	static char MODULE_NAME[250] = { 0 };
	if (strlen(MODULE_NAME) == 0)
	{

		GetModuleFileName(g_dllModule, MODULE_NAME, 250);
		boost::filesystem::path p(MODULE_NAME);
		strcpy(MODULE_NAME, p.filename().string().c_str());
	}

	return MODULE_NAME;
}
#endif

WtExecRunner::WtExecRunner()
{
#if _WIN32
#pragma message("Signal hooks disabled in WIN32")
#else
#pragma message("Signal hooks enabled in UNIX")
	install_signal_hooks([](const char* message) {
		WTSLogger::error_f(message);
	});
#endif
}

bool WtExecRunner::init(const char* logCfg /* = "logcfgexec.json" */, bool isFile /* = true */)
{
#ifdef _MSC_VER
	CMiniDumper::Enable(getModuleName(), true, WtHelper::getCWD().c_str());
#endif

	if(isFile)
	{
		std::string path = WtHelper::getCWD() + logCfg;
		WTSLogger::init(path.c_str(), true);
	}
	else
	{
		WTSLogger::init(logCfg, false);
	}
	

	WtHelper::setInstDir(getBinDir());
	return true;
}

bool WtExecRunner::config(const char* cfgFile, bool isFile /* = true */)
{
	_config = isFile ? WTSCfgLoader::load_from_file(cfgFile, true) : WTSCfgLoader::load_from_content(cfgFile, false, true);
	if(_config == NULL)
	{
		WTSLogger::log_raw(LL_ERROR, "Loading config file failed");
		return false;
	}

	//���������ļ�
	WTSVariant* cfgBF = _config->get("basefiles");
	bool isUTF8 = cfgBF->getBoolean("utf-8");
	if (cfgBF->get("session"))
	{
		_bd_mgr.loadSessions(cfgBF->getCString("session"), isUTF8);
		WTSLogger::info_f("Trading sessions loaded");
	}

	WTSVariant* cfgItem = cfgBF->get("commodity");
	if (cfgItem)
	{
		if (cfgItem->type() == WTSVariant::VT_String)
		{
			_bd_mgr.loadCommodities(cfgItem->asCString(), isUTF8);
		}
		else if (cfgItem->type() == WTSVariant::VT_Array)
		{
			for (uint32_t i = 0; i < cfgItem->size(); i++)
			{
				_bd_mgr.loadCommodities(cfgItem->get(i)->asCString(), isUTF8);
			}
		}
	}

	cfgItem = cfgBF->get("contract");
	if (cfgItem)
	{
		if (cfgItem->type() == WTSVariant::VT_String)
		{
			_bd_mgr.loadContracts(cfgItem->asCString(), isUTF8);
		}
		else if (cfgItem->type() == WTSVariant::VT_Array)
		{
			for (uint32_t i = 0; i < cfgItem->size(); i++)
			{
				_bd_mgr.loadContracts(cfgItem->get(i)->asCString(), isUTF8);
			}
		}
	}

	if (cfgBF->get("holiday"))
	{
		_bd_mgr.loadHolidays(cfgBF->getCString("holiday"));
		WTSLogger::info_f("Holidays loaded");
	}


	//��ʼ�����ݹ���
	initDataMgr();

	//��ʼ����ƽ����
	if (!initActionPolicy())
		return false;

	//��ʼ������ͨ��
	const char* cfgParser = _config->getCString("parsers");
	if (StdFile::exists(cfgParser))
	{
		WTSLogger::info_f("Reading parser config from {}...", cfgParser);
		WTSVariant* var = WTSCfgLoader::load_from_file(cfgParser, true);
		if (var)
		{
			if (!initParsers(var))
				WTSLogger::error_f("Loading parsers failed");
			var->release();
		}
		else
		{
			WTSLogger::error_f("Loading parser config {} failed", cfgParser);
		}
	}

	//��ʼ������ͨ��
	const char* cfgTraders = _config->getCString("traders");
	if (StdFile::exists(cfgTraders))
	{
		WTSLogger::info_f("Reading trader config from {}...", cfgTraders);
		WTSVariant* var = WTSCfgLoader::load_from_file(cfgTraders, true);
		if (var)
		{
			if (!initTraders(var))
				WTSLogger::error_f("Loading traders failed");
			var->release();
		}
		else
		{
			WTSLogger::error_f("Loading trader config {} failed", cfgTraders);
		}
	}

	const char* cfgExecuters = _config->getCString("executers");
	if (StdFile::exists(cfgExecuters))
	{
		WTSLogger::info_f("Reading executer config from {}...", cfgExecuters);
		WTSVariant* var = WTSCfgLoader::load_from_file(cfgExecuters, true);
		if (var)
		{
			if (!initExecuters(var))
				WTSLogger::error_f("Loading executers failed");
			var->release();
		}
		else
		{
			WTSLogger::error_f("Loading executer config {} failed", cfgExecuters);
		}
	}

	return true;
}


void WtExecRunner::run()
{
	try
	{
		_parsers.run();
		_traders.run();
	}
	catch (...)
	{
		print_stack_trace([](const char* message) {
			WTSLogger::error_f(message);
		});
	}
}

bool WtExecRunner::initParsers(WTSVariant* cfgParser)
{
	WTSVariant* cfg = cfgParser->get("parsers");
	if (cfg == NULL)
		return false;

	uint32_t count = 0;
	for (uint32_t idx = 0; idx < cfg->size(); idx++)
	{
		WTSVariant* cfgItem = cfg->get(idx);
		if (!cfgItem->getBoolean("active"))
			continue;

		const char* id = cfgItem->getCString("id");

		// By Wesley @ 2021.12.14
		// ���idΪ�գ��������Զ�id
		std::string realid = id;
		if (realid.empty())
		{
			static uint32_t auto_parserid = 1000;
			realid = StrUtil::printf("auto_parser_%u", auto_parserid++);
		}

		ParserAdapterPtr adapter(new ParserAdapter);
		adapter->init(realid.c_str(), cfgItem, this, &_bd_mgr, &_hot_mgr);
		_parsers.addAdapter(realid.c_str(), adapter);

		count++;
	}

	WTSLogger::info_f("{} parsers loaded", count);

	return true;
}

bool WtExecRunner::initExecuters(WTSVariant* cfgExecuter)
{
	WTSVariant* cfg = cfgExecuter->get("executers");
	if (cfg == NULL || cfg->type() != WTSVariant::VT_Array)
		return false;

	//�ȼ����Դ���ִ��������
	std::string path = WtHelper::getInstDir() + "executer//";
	_exe_factory.loadFactories(path.c_str());

	uint32_t count = 0;
	for (uint32_t idx = 0; idx < cfg->size(); idx++)
	{
		WTSVariant* cfgItem = cfg->get(idx);
		if (!cfgItem->getBoolean("active"))
			continue;

		const char* id = cfgItem->getCString("id");

		WtExecuterPtr executer(new WtLocalExecuter(&_exe_factory, id, &_data_mgr));
		executer->setStub(this);
		if (!executer->init(cfgItem))
			return false;

		const char* tid = cfgItem->getCString("trader");
		if (strlen(tid) == 0)
		{
			WTSLogger::error_f("No Trader configured for Executer {}", id);
		}
		else
		{
			TraderAdapterPtr trader = _traders.getAdapter(tid);
			if (trader)
			{
				executer->setTrader(trader.get());
				trader->addSink(executer.get());
			}
			else
			{
				WTSLogger::error_f("Trader {} not exists, cannot configured for executer {}", tid, id);
			}
		}

		_exe_mgr.add_executer(executer);

		count++;
	}

	WTSLogger::info_f("{} executers loaded", count);

	return true;
}

bool WtExecRunner::initTraders(WTSVariant* cfgTrader)
{
	WTSVariant* cfg = cfgTrader->get("traders");
	if (cfg == NULL || cfg->type() != WTSVariant::VT_Array)
		return false;

	uint32_t count = 0;
	for (uint32_t idx = 0; idx < cfg->size(); idx++)
	{
		WTSVariant* cfgItem = cfg->get(idx);
		if (!cfgItem->getBoolean("active"))
			continue;

		const char* id = cfgItem->getCString("id");

		TraderAdapterPtr adapter(new TraderAdapter);
		adapter->init(id, cfgItem, &_bd_mgr, &_act_policy);

		_traders.addAdapter(id, adapter);
		count++;
	}

	WTSLogger::info_f("{} traders loaded", count);

	return true;
}

bool WtExecRunner::initDataMgr()
{
	WTSVariant* cfg = _config->get("data");
	if (cfg == NULL)
		return false;

	_data_mgr.init(cfg, this);

	WTSLogger::info_f("Data Manager initialized");
	return true;
}

bool WtExecRunner::addExeFactories(const char* folder)
{
	return _exe_factory.loadFactories(folder);
}

WTSSessionInfo* WtExecRunner::get_session_info(const char* sid, bool isCode /* = true */)
{
	if (!isCode)
		return _bd_mgr.getSession(sid);

	WTSCommodityInfo* cInfo = _bd_mgr.getCommodity(CodeHelper::stdCodeToStdCommID(sid).c_str());
	if (cInfo == NULL)
		return NULL;

	return _bd_mgr.getSession(cInfo->getSession());
}

void WtExecRunner::handle_push_quote(WTSTickData* quote, uint32_t hotFlag /* = 0 */)
{
	WTSContractInfo* cInfo = quote->getContractInfo();
	if (cInfo == NULL)
	{
		cInfo = _bd_mgr.getContract(quote->code(), quote->exchg());
		quote->setContractInfo(cInfo);
	}

	if (cInfo == NULL)
		return;

	uint32_t uDate = quote->actiondate();
	uint32_t uTime = quote->actiontime();
	uint32_t curMin = uTime / 100000;
	uint32_t curSec = uTime % 100000;
	WtHelper::setTime(uDate, curMin, curSec);
	WtHelper::setTDate(quote->tradingdate());

	WTSCommodityInfo* commInfo = cInfo->getCommInfo();

	std::string stdCode;
	if (commInfo->getCategoty() == CC_FutOption)
	{
		stdCode = CodeHelper::rawFutOptCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
	}
	else if (CodeHelper::isMonthlyCode(quote->code()))
	{
		stdCode = CodeHelper::rawMonthCodeToStdCode(cInfo->getCode(), cInfo->getExchg());
	}
	else
	{
		stdCode = CodeHelper::rawFlatCodeToStdCode(cInfo->getCode(), cInfo->getExchg(), cInfo->getProduct());
	}
	quote->setCode(stdCode.c_str());
	_data_mgr.handle_push_quote(stdCode.c_str(), quote);

	_exe_mgr.handle_tick(stdCode.c_str(), quote);
}

void WtExecRunner::release()
{
	WTSLogger::stop();
}


void WtExecRunner::setPosition(const char* stdCode, double targetPos)
{
	_exe_mgr.handle_pos_change(stdCode, targetPos);
}

bool WtExecRunner::initActionPolicy()
{
	const char* action_file = _config->getCString("bspolicy");
	if (strlen(action_file) <= 0)
		return false;

	bool ret = _act_policy.init(action_file);
	WTSLogger::info_f("Action policies initialized");
	return ret;
}

uint64_t WtExecRunner::get_real_time()
{
	return TimeUtils::makeTime(_data_mgr.get_date(), _data_mgr.get_raw_time() * 100000 + _data_mgr.get_secs());
}

WTSCommodityInfo* WtExecRunner::get_comm_info(const char* stdCode)
{
	return _bd_mgr.getCommodity(CodeHelper::stdCodeToStdCommID(stdCode).c_str());
}

WTSSessionInfo* WtExecRunner::get_sess_info(const char* stdCode)
{
	WTSCommodityInfo* cInfo = _bd_mgr.getCommodity(CodeHelper::stdCodeToStdCommID(stdCode).c_str());
	if (cInfo == NULL)
		return NULL;

	return _bd_mgr.getSession(cInfo->getSession());
}

uint32_t WtExecRunner::get_trading_day()
{
	return _data_mgr.get_trading_day();
}