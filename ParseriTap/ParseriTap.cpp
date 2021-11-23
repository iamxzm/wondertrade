#include "ParseriTap.h"
#include "./TapQuote/TapAPIError.h"

#include "../Share/StrUtil.hpp"
#include "../Share/TimeUtils.hpp"
#include "../Includes/WTSDataDef.hpp"
#include "../Share/StdUtils.hpp"
#include "../Includes/IBaseDataMgr.h"
#include "../Includes/WTSParams.hpp"
#include "../Includes/WTSContractInfo.hpp"
#include "../Share/DLLHelper.hpp"


#ifdef _WIN32
#include <wtypes.h>
HMODULE	g_dllModule = NULL;

BOOL APIENTRY DllMain(
	HANDLE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		g_dllModule = (HMODULE)hModule;
		break;
	}
	return TRUE;
}
#else
#include <dlfcn.h>

char PLATFORM_NAME[] = "UNIX";

std::string	g_moduleName;

__attribute__((constructor))
void on_load(void) {
	Dl_info dl_info;
	dladdr((void *)on_load, &dl_info);
	g_moduleName = dl_info.dli_fname;
}
#endif


std::string getBinDir()
{
	static std::string _bin_dir;
	if (_bin_dir.empty())
	{


#ifdef _WIN32
		char strPath[MAX_PATH];
		GetModuleFileName(g_dllModule, strPath, MAX_PATH);

		_bin_dir = StrUtil::standardisePath(strPath, false);
#else
		_bin_dir = g_moduleName;
#endif

		uint32_t nPos = _bin_dir.find_last_of('/');
		_bin_dir = _bin_dir.substr(0, nPos + 1);
	}

	return _bin_dir;
}


extern "C"
{
	EXPORT_FLAG IParserApi* createParser()
	{
		ParseriTap* parser = new ParseriTap();
		return parser;
	}

	EXPORT_FLAG void deleteParser(IParserApi* &parser)
	{
		if (NULL != parser)
		{
			delete parser;
			parser = NULL;
		}
	}
};


uint32_t strToTime(const char* strTime)
{
	std::string str = strTime;
	StrUtil::replace(str, ":", "");
	StrUtil::replace(str, ".", "");

	return strtoul(str.c_str(), NULL, 10);
}

const int RECONNECT_SECONDS = 2;

ParseriTap::ParseriTap()
	:m_pUserAPI(NULL)
	,m_uTradingDate(0)
	,m_loginState(LS_NOTLOGIN)
	,m_uSessionID(0)
	,m_bStopped(false)
	,m_bReconnect(false)
{
}


ParseriTap::~ParseriTap()
{
	m_pUserAPI = NULL;
}

bool ParseriTap::init(WTSParams* config)
{
	m_strHost = config->getCString("host");
	m_nPort = config->getInt32("port");

	m_strUser= config->getCString("user");
	m_strPass= config->getCString("pass");
	m_strAuthCode = config->getCString("authcode");

	const StringVector& ay = StrUtil::split(config->getString("commodity"), ",");
	for(auto it = ay.begin(); it != ay.end(); it++)
	{
		if((*it).empty())
			continue;

		m_setComms.insert(*it);
	}

	WTSParams* param = config->get("tapmodule");
	std::string m_strModule;
	if (param != NULL)
		m_strModule = getBinDir() + DLLHelper::wrap_module(config->asCString(), "lib");
	else
		m_strModule = getBinDir() + DLLHelper::wrap_module("TapQuoteAPI", "lib");

	m_hInst = DLLHelper::load_library(m_strModule.c_str());
	const char* creatorName = "CreateTapQuoteAPI";
	const char* removerName = "FreeTapQuoteAPI";
	m_funcCreator = (iTapCreator)DLLHelper::get_symbol(m_hInst, creatorName);
	m_funcRemover = (iTapRemover)DLLHelper::get_symbol(m_hInst, removerName);

	return true;
}

void ParseriTap::release()
{
	disconnect();
}

bool ParseriTap::connect()
{
	return login();

	return true;
}

void ParseriTap::reconnect()
{
	login(true);
}

bool ParseriTap::disconnect()
{
	m_bStopped = true;
	if(m_pUserAPI)
	{
		m_pUserAPI->Disconnect();
	}

	return true;
}

void ParseriTap::OnRspLogin(TAPIINT32 errorCode, const TapAPIQuotLoginRspInfo *info)
{
	if(errorCode == TAPIERROR_SUCCEED)
	{
		if(m_sink)
		{
			m_sink->handleParserLog(LL_INFO, "[ParseriTap] %s login succeed", m_strUser.c_str());
			m_sink->handleEvent(WPE_Login, 0);
		}

		m_loginState = LS_LOGINED;

		//subscribe();
	}
	else
	{
		if(m_sink)
		{
			m_sink->handleParserLog(LL_INFO, "[ParseriTap] %s login failed: %d", m_strUser.c_str(), errorCode);
			m_sink->handleEvent(WPE_Login, errorCode);
		}
	}

	m_bReconnect = false;
}

void ParseriTap::OnAPIReady()
{
	subscribe();
}

void ParseriTap::OnDisconnect(TAPIINT32 reasonCode)
{
	if(m_sink && !m_bStopped)
	{
		m_sink->handleParserLog(LL_ERROR, StrUtil::printf("[ParseriTap] Market data server disconnected: %d...", reasonCode).c_str());
		m_sink->handleEvent(WPE_Close, 0);
	}


	if (!m_bStopped)
	{
		if (!m_bReconnect)
		{
			m_bReconnect = true;
			//���ﶪ���߳���ȥ����,��OnClose�������Ϸ���
			StdThreadPtr thrd(new StdThread([this](){
				std::this_thread::sleep_for(std::chrono::seconds(RECONNECT_SECONDS));
				m_sink->handleParserLog(LL_WARN, "[ParseriTap] Reconnecting server...", m_strUser.c_str());
				reconnect();
			}));
		}
	}
}

void ParseriTap::OnRtnQuote(const TapAPIQuoteWhole *info)
{	
	if(m_bdMgr == NULL)
		return;

	if (info->QLimitUpPrice != 0 && (info->QLastPrice < info->QLimitDownPrice || info->QLastPrice > info->QLimitUpPrice))
		return;

	if (info->QLastPrice == 0)
		return;

	std::string code = StrUtil::printf("%s%s", productO2I(info->Contract.Commodity.CommodityNo), info->Contract.ContractNo1);
	const char* exchg = exchgO2I(info->Contract.Commodity.ExchangeNo);

	WTSContractInfo* contract = m_bdMgr->getContract(code.c_str(), exchg);
	if(contract == NULL)
		return;

	WTSTickData* tick = WTSTickData::create(code.c_str());
	WTSTickStruct& quote = tick->getTickStruct();
	strcpy(quote.exchg, exchg);
	
	uint32_t curDate,curTime;
	TimeUtils::getDateTime(curDate, curTime);
	StringVector ay = StrUtil::split(info->DateTimeStamp, " ");
	StrUtil::replace(ay[0], "-", "");
	quote.action_date = strtoul(ay[0].c_str(), NULL, 10);
	quote.action_time = strToTime(ay[1].c_str());

	uint32_t hour = quote.action_time/10000000;	
	uint32_t curHour = curTime/10000000;
	if(hour == 23 && curHour == 0)
	{
		//����ʱ������ϵͳʱ��
		quote.action_date = TimeUtils::getNextDate(curDate, -1);
	}
	else if(hour == 0 && curHour == 23)
	{
		//ϵͳʱ����������ʱ��
		quote.action_date = TimeUtils::getNextDate(curDate, 1);
	}
	/*
	if(quote.action_time < 005000000 && quote.action_date != m_uTradingDate)
	{//���ʱ��С��00:05��,���ҵ�ǰ���ڲ����ڽ�����,˵����ҹ��ʱ��,������ǿ������Ϊ������
		quote.action_date = m_uTradingDate;
	}
	*/
	//quote.pre_close = PRICE_DOUBLE_TO_INT(info->QPreSettlePrice);
	quote.price = info->QLastPrice;
	quote.open = info->QOpeningPrice;
	quote.high = info->QHighPrice;
	quote.low = info->QLowPrice;
	quote.total_volume = (uint32_t)(info->QTotalQty+0.5);
	quote.trading_date = m_uTradingDate;
	if(info->QSettlePrice != 0.0)
		quote.settle_price = info->QSettlePrice;
	if(info->QTotalTurnover != 0.0)
		quote.total_turnover = info->QTotalTurnover;

	quote.open_interest = (uint32_t)(info->QPositionQty+0.5);

	quote.upper_limit = (info->QLimitUpPrice);
	quote.lower_limit = (info->QLimitDownPrice);

	quote.pre_close = (info->QPreClosingPrice);
	quote.pre_settle = (info->QPreSettlePrice);
	quote.pre_interest = (uint32_t)(info->QPrePositionQty + 0.5);

	//ί���۸�
	for(uint32_t i = 0; i < 10; i++)
	{
		quote.ask_prices[i] = (info->QAskPrice[i]);
		quote.bid_prices[i] = (info->QBidPrice[i]);

		quote.ask_qty[i] = (uint32_t)(info->QAskQty[i] + 0.5);
		quote.bid_qty[i] = (uint32_t)(info->QBidQty[i] + 0.5);
	}

	if(m_sink)
		m_sink->handleQuote(tick, true);

	tick->release();
}

void ParseriTap::OnRspSubscribeQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteWhole *info)
{
	if(errorCode != 0)
	{
		printf("%d", errorCode);
	}
	else
	{
		std::string code = StrUtil::printf("%s%s", productO2I(info->Contract.Commodity.CommodityNo), info->Contract.ContractNo1);
		const char* exchg = exchgO2I(info->Contract.Commodity.ExchangeNo);

		WTSContractInfo* contract = m_bdMgr->getContract(code.c_str(), exchg);
		if (contract == NULL)
			return;

		WTSTickData* tick = WTSTickData::create(code.c_str());
		WTSTickStruct& quote = tick->getTickStruct();
		strcpy(quote.exchg, exchg);

		uint32_t curDate, curTime;
		TimeUtils::getDateTime(curDate, curTime);
		StringVector ay = StrUtil::split(info->DateTimeStamp, " ");
		StrUtil::replace(ay[0], "-", "");
		quote.action_date = strtoul(ay[0].c_str(), NULL, 10);
		quote.action_time = strToTime(ay[1].c_str());
		uint32_t hour = quote.action_time / 10000000;
		uint32_t curHour = curTime / 10000000;
		if (hour == 23 && curHour == 0)
		{
			//����ʱ������ϵͳʱ��
			quote.action_date = TimeUtils::getNextDate(curDate, -1);
		}
		else if (hour == 0 && curHour == 23)
		{
			//ϵͳʱ����������ʱ��
			quote.action_date = TimeUtils::getNextDate(curDate, 1);
		}

		quote.price = (info->QLastPrice);
		quote.open = (info->QOpeningPrice);
		quote.high = (info->QHighPrice);
		quote.low = (info->QLowPrice);
		quote.total_volume = (uint32_t)(info->QTotalQty + 0.5);
		quote.trading_date = m_uTradingDate;
		if (info->QSettlePrice != 0.0)
			quote.settle_price = (info->QSettlePrice);
		if (info->QTotalTurnover != 0.0)
			quote.total_turnover = (info->QTotalTurnover);

		quote.open_interest = (uint32_t)(info->QPositionQty + 0.5);
		
		quote.upper_limit = (info->QLimitUpPrice);
		quote.lower_limit = (info->QLimitDownPrice);

		quote.pre_close = (info->QPreClosingPrice);
		quote.pre_settle = (info->QPreSettlePrice);
		quote.pre_interest = (uint32_t)(info->QPrePositionQty + 0.5);

		//ί���۸�
		for (uint32_t i = 0; i < 10; i++)
		{
			quote.ask_prices[i] = (info->QAskPrice[i]);
			quote.bid_prices[i] = (info->QBidPrice[i]);

			quote.ask_qty[i] = (uint32_t)(info->QAskQty[i] + 0.5);
			quote.bid_qty[i] = (uint32_t)(info->QBidQty[i] + 0.5);
		}

		if (m_sink)
			m_sink->handleQuote(tick, true);

		tick->release();
	}
}

void ParseriTap::OnRspUnSubscribeQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIContract *info)
{

}

bool ParseriTap::login(bool bNeedReconn /* = false */)
{
	if(m_pUserAPI != NULL)
	{
		m_pUserAPI->SetAPINotify(NULL);
		m_funcRemover(m_pUserAPI);
		m_pUserAPI = NULL;
	}

	TAPIINT32 iResult = TAPIERROR_SUCCEED;
	TapAPIApplicationInfo stAppInfo;
	strcpy(stAppInfo.AuthCode, m_strAuthCode.c_str());
	strcpy(stAppInfo.KeyOperationLogPath, "");

	m_pUserAPI = m_funcCreator(&stAppInfo, iResult);
	if(m_pUserAPI == NULL)
		return false;

	m_pUserAPI->SetAPINotify(this);

	m_pUserAPI->SetHostAddress(m_strHost.c_str(), m_nPort);

	TapAPIQuoteLoginAuth stLoginAuth;
	memset(&stLoginAuth, 0, sizeof(stLoginAuth));
	strcpy(stLoginAuth.UserNo, m_strUser.c_str());
	strcpy(stLoginAuth.Password, m_strPass.c_str());
	stLoginAuth.ISModifyPassword = APIYNFLAG_NO;
	stLoginAuth.ISDDA = APIYNFLAG_NO;

	iResult = m_pUserAPI->Login(&stLoginAuth);
	if(iResult != TAPIERROR_SUCCEED)
	{
		if(m_sink)
			m_sink->handleParserLog(LL_ERROR, StrUtil::printf("[ParseriTap] Sending login request failed: %d", iResult).c_str());

		//�������ʧ��,����Ҫ����,��������
		if(iResult == TAPIERROR_ConnectFail && bNeedReconn && !m_bStopped)
		{
			std::this_thread::sleep_for(std::chrono::seconds(RECONNECT_SECONDS));
			m_sink->handleParserLog(LL_INFO, StrUtil::printf("[ParseriTap] Reconnecting server...").c_str());
			reconnect();
		}
	}

	return true;
}

void ParseriTap::subscribe()
{
	CodeSet codeFilter = m_filterSubs;
	if(codeFilter.empty())
	{//����������ֻ�յ�,��ȡ��ȫ����Լ�б�
		return;
	}

	if(m_pUserAPI)
	{
		for(const std::string& fullcode : codeFilter)
		{
			std::size_t pos = fullcode.find(".");
			std::string code, exchg;
			if (pos != std::string::npos)
			{
				exchg = fullcode.substr(0, pos);
				code = fullcode.substr(pos + 1);
			}
			else
			{
				code = fullcode;
			}

			WTSContractInfo* contractInfo = m_bdMgr->getContract(code.c_str(), exchg.c_str());
			//char* code = (char*)(*it).c_str();
			//code += strlen(contractInfo->getProduct());

			TapAPIContract stContract;
			memset(&stContract, 0, sizeof(stContract));
			strcpy(stContract.Commodity.ExchangeNo, exchgI2O(contractInfo->getExchg()));
			stContract.Commodity.CommodityType = TAPI_COMMODITY_TYPE_FUTURES;
			strcpy(stContract.Commodity.CommodityNo, productI2O(contractInfo->getProduct()));
			strcpy(stContract.ContractNo1, code.substr(strlen(contractInfo->getProduct())).c_str());
			stContract.CallOrPutFlag1 = TAPI_CALLPUT_FLAG_NONE;
			stContract.CallOrPutFlag2 = TAPI_CALLPUT_FLAG_NONE;

			m_pUserAPI->SubscribeQuote(&m_uSessionID, &stContract);
		}
	}
}

void ParseriTap::unsubscribe(const CodeSet& vecSymbols)
{
	if (!m_setComms.empty())
	{
		auto it = vecSymbols.begin();
		for (; it != vecSymbols.end(); it++)
		{
			auto temp = m_setComms.find((*it));
			if (temp != m_setComms.end())
				m_setComms.erase(temp);
		}
	}

	if(m_loginState == LS_LOGINED)
	{
		for (const std::string& fullcode : vecSymbols)
		{
			std::size_t pos = fullcode.find(".");
			std::string code, exchg;
			if (pos != std::string::npos)
			{
				exchg = fullcode.substr(0, pos);
				code = fullcode.substr(pos + 1);
			}
			else
			{
				code = fullcode;
			}

			WTSContractInfo* contractInfo = m_bdMgr->getContract(code.c_str(), exchg.c_str());

			TapAPIContract stContract;
			memset(&stContract, 0, sizeof(stContract));
			strcpy(stContract.Commodity.ExchangeNo, exchgI2O(contractInfo->getExchg()));
			stContract.Commodity.CommodityType = TAPI_COMMODITY_TYPE_FUTURES;
			strcpy(stContract.Commodity.CommodityNo, productI2O(contractInfo->getProduct()));
			strcpy(stContract.ContractNo1, code.substr(strlen(contractInfo->getProduct())).c_str());
			stContract.CallOrPutFlag1 = TAPI_CALLPUT_FLAG_NONE;
			stContract.CallOrPutFlag2 = TAPI_CALLPUT_FLAG_NONE;

			m_pUserAPI->UnSubscribeQuote(&m_uSessionID, &stContract);
		}
	}
}

void ParseriTap::subscribe(const CodeSet &vecSymbols)
{
	if(m_loginState != LS_LOGINED)
	{
		if(m_setComms.empty())
		{
			m_filterSubs = vecSymbols;
		}
		else
		{
			auto it = vecSymbols.begin();
			for(; it != vecSymbols.end(); it++)
			{
				if(m_setComms.find(*it) != m_setComms.end())
					m_filterSubs.insert(*it);
			}
		}
	}
	else
	{
		m_filterSubs = vecSymbols;

		if(m_pUserAPI && vecSymbols.size() > 0)
		{
			for (const std::string& fullcode : vecSymbols)
			{
				std::size_t pos = fullcode.find(".");
				std::string code, exchg;
				if (pos != std::string::npos)
				{
					exchg = fullcode.substr(0, pos);
					code = fullcode.substr(pos + 1);
				}
				else
				{
					code = fullcode;
				}

				WTSContractInfo* contractInfo = m_bdMgr->getContract(code.c_str(), exchg.c_str());

				TapAPIContract stContract;
				memset(&stContract, 0, sizeof(stContract));
				strcpy(stContract.Commodity.ExchangeNo, exchgI2O(contractInfo->getExchg()));
				stContract.Commodity.CommodityType = TAPI_COMMODITY_TYPE_FUTURES;
				strcpy(stContract.Commodity.CommodityNo, productI2O(contractInfo->getProduct()));
				strcpy(stContract.ContractNo1, code.substr(strlen(contractInfo->getProduct())).c_str());
				stContract.CallOrPutFlag1 = TAPI_CALLPUT_FLAG_NONE;
				stContract.CallOrPutFlag2 = TAPI_CALLPUT_FLAG_NONE;

				m_pUserAPI->SubscribeQuote(&m_uSessionID, &stContract);
			}

			if(m_sink)
				m_sink->handleParserLog(LL_INFO, StrUtil::printf("[ParseriTap] Market data of %u contracts subscribed in total", vecSymbols.size()).c_str());
		}
	}
}

bool ParseriTap::isConnected()
{
	return m_pUserAPI!=NULL;
}

void ParseriTap::registerSpi(IParserSpi* listener)
{
	m_sink = listener;

	if(m_sink)
	{
		m_bdMgr = m_sink->getBaseDataMgr();
	}
}

const char* ParseriTap::exchgO2I(const char* exchg)
{
	auto it = m_mapExchgOToI.find(exchg);
	if(it != m_mapExchgOToI.end())
		return it->second.c_str();

	return exchg;
}

const char* ParseriTap::exchgI2O(const char* exchg)
{
	auto it = m_mapExchgIToO.find(exchg);
	if(it != m_mapExchgIToO.end())
		return it->second.c_str();

	return exchg;
}

const char* ParseriTap::productO2I(const char* pid)
{
	auto it = m_mapProductOToI.find(pid);
	if(it != m_mapProductOToI.end())
		return it->second.c_str();

	return pid;
}

const char* ParseriTap::productI2O(const char* pid)
{
	auto it = m_mapProductIToO.find(pid);
	if(it != m_mapProductIToO.end())
		return it->second.c_str();

	return pid;
}