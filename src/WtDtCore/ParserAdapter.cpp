/*!
 * \file ParserAdapter.cpp
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#include "ParserAdapter.h"
#include "DataManager.h"
#include "StateMonitor.h"
#include "WtHelper.h"

#include "../Share/TimeUtils.hpp"
#include "../Share/StrUtil.hpp"
#include "../Share/DLLHelper.hpp"

#include "../Includes/WTSParams.hpp"
#include "../Includes/WTSContractInfo.hpp"
#include "../Includes/WTSDataDef.hpp"
#include "../Includes/WTSVariant.hpp"

#include "../WTSTools/WTSBaseDataMgr.h"
#include "../WTSTools/WTSLogger.h"


//////////////////////////////////////////////////////////////////////////
//ParserAdapter
ParserAdapter::ParserAdapter(WTSBaseDataMgr * bgMgr, DataManager* dtMgr)
	: _parser_api(NULL)
	, _remover(NULL)
	, _stopped(false)
	, _bd_mgr(bgMgr)
	, _dt_mgr(dtMgr)
	, _cfg(NULL)
{
}


ParserAdapter::~ParserAdapter()
{
}

bool ParserAdapter::initExt(const char* id, IParserApi* api)
{
	if (api == NULL)
		return false;

	_parser_api = api;
	_id = id;

	if (_parser_api)
	{
		_parser_api->registerSpi(this);

		if (_parser_api->init(NULL))
		{
			ContractSet contractSet;
			WTSArray* ayContract = _bd_mgr->getContracts();
			WTSArray::Iterator it = ayContract->begin();
			for (; it != ayContract->end(); it++)
			{
				WTSContractInfo* contract = STATIC_CONVERT(*it, WTSContractInfo*);
				WTSCommodityInfo* pCommInfo = _bd_mgr->getCommodity(contract);
				contractSet.insert(contract->getFullCode());
			}

			ayContract->release();

			_parser_api->subscribe(contractSet);
			contractSet.clear();
		}
		else
		{
			WTSLogger::log_dyn("parser", _id.c_str(), LL_ERROR, "[%s] Parser initializing failed: api initializing failed...", _id.c_str());
		}
	}

	return true;
}


bool ParserAdapter::init(const char* id, WTSVariant* cfg)
{
	if (cfg == NULL)
		return false;

	_id = id;

	if (_cfg != NULL)
		return false;

	_cfg = cfg;
	_cfg->retain();

	{
		//����ģ��
		if (cfg->getString("module").empty())
			return false;

		std::string module = DLLHelper::wrap_module(cfg->getCString("module"), "lib");;

		if (!StdFile::exists(module.c_str()))
		{
			module = WtHelper::get_module_dir();
			module += "parsers/";
			module += DLLHelper::wrap_module(cfg->getCString("module"), "lib");
		}

		DllHandle hInst = DLLHelper::load_library(module.c_str());
		if (hInst == NULL)
		{
			WTSLogger::log_dyn("parser", _id.c_str(), LL_ERROR, "[%s] Parser module %s loading failed", _id.c_str(), module.c_str());
			return false;
		}
		else
		{
			WTSLogger::log_dyn("parser", _id.c_str(), LL_INFO, "[%s] Parser module %s loaded", _id.c_str(), module.c_str());
		}

		FuncCreateParser pFuncCreateParser = (FuncCreateParser)DLLHelper::get_symbol(hInst, "createParser");
		if (NULL == pFuncCreateParser)
		{
			WTSLogger::log_dyn("parser", _id.c_str(), LL_FATAL, "[%s] Entrance function createParser not found", _id.c_str());
			return false;
		}

		_parser_api = pFuncCreateParser();
		if (NULL == _parser_api)
		{
			WTSLogger::log_dyn("parser", _id.c_str(), LL_FATAL, "[%s] Creating parser api failed", _id.c_str());
			return false;
		}

		_remover = (FuncDeleteParser)DLLHelper::get_symbol(hInst, "deleteParser");
	}


	const std::string& strFilter = cfg->getString("filter");
	if (!strFilter.empty())
	{
		const StringVector &ayFilter = StrUtil::split(strFilter, ",");
		auto it = ayFilter.begin();
		for (; it != ayFilter.end(); it++)
		{
			_exchg_filter.insert(*it);
		}
	}

	std::string strCodes = cfg->getString("code");
	if (!strCodes.empty())
	{
		const StringVector &ayCodes = StrUtil::split(strCodes, ",");
		auto it = ayCodes.begin();
		for (; it != ayCodes.end(); it++)
		{
			_code_filter.insert(*it);
		}
	}

	if (_parser_api)
	{
		_parser_api->registerSpi(this);

		WTSParams* params = cfg->toParams();
		if (_parser_api->init(params))
		{
			ContractSet contractSet;
			if (!_code_filter.empty())//�����жϺ�Լ������
			{
				ExchgFilter::iterator it = _code_filter.begin();
				for (; it != _code_filter.end(); it++)
				{
					//ȫ����,��ʽ��SSE.600000,�ڻ�����ΪCFFEX.IF2005
					std::string code, exchg;
					auto ay = StrUtil::split((*it).c_str(), ".");
					if (ay.size() == 1)
						code = ay[0];
					else
					{
						exchg = ay[0];
						code = ay[1];
					}
					WTSContractInfo* contract = _bd_mgr->getContract(code.c_str(), exchg.c_str());
					WTSCommodityInfo* pCommInfo = _bd_mgr->getCommodity(contract);
					contractSet.insert(contract->getFullCode());
				}
			}
			else if (!_exchg_filter.empty())
			{
				ExchgFilter::iterator it = _exchg_filter.begin();
				for (; it != _exchg_filter.end(); it++)
				{
					WTSArray* ayContract = _bd_mgr->getContracts((*it).c_str());
					WTSArray::Iterator it = ayContract->begin();
					for (; it != ayContract->end(); it++)
					{
						WTSContractInfo* contract = STATIC_CONVERT(*it, WTSContractInfo*);
						WTSCommodityInfo* pCommInfo = _bd_mgr->getCommodity(contract);
						contractSet.insert(contract->getFullCode());
					}

					ayContract->release();
				}
			}
			else
			{
				WTSArray* ayContract = _bd_mgr->getContracts();
				WTSArray::Iterator it = ayContract->begin();
				for (; it != ayContract->end(); it++)
				{
					WTSContractInfo* contract = STATIC_CONVERT(*it, WTSContractInfo*);
					WTSCommodityInfo* pCommInfo = _bd_mgr->getCommodity(contract);
					contractSet.insert(contract->getFullCode());
				}

				ayContract->release();
			}

			_parser_api->subscribe(contractSet);
			contractSet.clear();
		}
		else
		{
			WTSLogger::log_dyn("parser", _id.c_str(), LL_ERROR, "[%s] Parser initializing failed: api initializing failed...", _id.c_str());
		}

		params->release();
	}
	else
	{
		WTSLogger::log_dyn("parser", _id.c_str(), LL_ERROR, "[%s] Parser initializing failed: creating api failed...", _id.c_str());
	}

	return true;
}

void ParserAdapter::release()
{
	_stopped = true;
	if (_parser_api)
	{
		_parser_api->release();
	}

	if (_remover)
		_remover(_parser_api);
	else
		delete _parser_api;
}

bool ParserAdapter::run()
{
	if (_parser_api == NULL)
		return false;

	_parser_api->connect();
	return true;
}

void ParserAdapter::handleSymbolList( const WTSArray* aySymbols )
{
	
}

void ParserAdapter::handleTransaction(WTSTransData* transData)
{
	if (_stopped)
		return;


	if (transData->actiondate() == 0 || transData->tradingdate() == 0)
		return;

	WTSContractInfo* contract = _bd_mgr->getContract(transData->code(), transData->exchg());
	if (contract == NULL)
		return;

	_dt_mgr->writeTransaction(transData);
}

void ParserAdapter::handleOrderDetail(WTSOrdDtlData* ordDetailData)
{
	if (_stopped)
		return;

	if (ordDetailData->actiondate() == 0 || ordDetailData->tradingdate() == 0)
		return;

	WTSContractInfo* contract = _bd_mgr->getContract(ordDetailData->code(), ordDetailData->exchg());
	if (contract == NULL)
		return;

	_dt_mgr->writeOrderDetail(ordDetailData);
}

void ParserAdapter::handleOrderQueue(WTSOrdQueData* ordQueData)
{
	if (_stopped)
		return;

	if (ordQueData->actiondate() == 0 || ordQueData->tradingdate() == 0)
		return;

	WTSContractInfo* contract = _bd_mgr->getContract(ordQueData->code(), ordQueData->exchg());
	if (contract == NULL)
		return;
		
	_dt_mgr->writeOrderQueue(ordQueData);
}

void ParserAdapter::handleQuote( WTSTickData *quote, bool bNeedSlice )
{
	if (_stopped)
		return;

	if (quote->actiondate() == 0 || quote->tradingdate() == 0)
		return;

	WTSContractInfo* contract = _bd_mgr->getContract(quote->code(), quote->exchg());
	if (contract == NULL)
		return;

	if (!_dt_mgr->writeTick(quote, bNeedSlice))
		return;
}

void ParserAdapter::handleParserLog( WTSLogLevel ll, const char* format, ... )
{
	if (_stopped)
		return;

	char szBuf[2048] = {0};
	va_list args;
	va_start(args, format);        
	vsprintf(szBuf, format, args);
	va_end(args);

	WTSLogger::log2("parser", ll, szBuf);
}

IBaseDataMgr* ParserAdapter::getBaseDataMgr()
{
	return _bd_mgr;
}


//////////////////////////////////////////////////////////////////////////
//ParserAdapterMgr
void ParserAdapterMgr::release()
{
	for (auto it = _adapters.begin(); it != _adapters.end(); it++)
	{
		it->second->release();
	}

	_adapters.clear();
}

bool ParserAdapterMgr::addAdapter(const char* id, ParserAdapterPtr& adapter)
{
	if (adapter == NULL || strlen(id) == 0)
		return false;

	auto it = _adapters.find(id);
	if (it != _adapters.end())
	{
		WTSLogger::error(" Same name of parsers: %s", id);
		return false;
	}

	_adapters[id] = adapter;

	return true;
}

ParserAdapterPtr ParserAdapterMgr::getAdapter(const char* id)
{
	auto it = _adapters.find(id);
	if (it != _adapters.end())
	{
		return it->second;
	}

	return ParserAdapterPtr();
}

void ParserAdapterMgr::run()
{
	for (auto it = _adapters.begin(); it != _adapters.end(); it++)
	{
		it->second->run();
	}

	WTSLogger::info("%u parsers started", _adapters.size());
}