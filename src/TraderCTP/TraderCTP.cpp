/*!
 * \file TraderCTP.cpp
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#include "TraderCTP.h"
#include "../Includes/WTSError.hpp"
#include "../Includes/WTSContractInfo.hpp"
#include "../Includes/WTSSessionInfo.hpp"
#include "../Includes/WTSTradeDef.hpp"
#include "../Includes/WTSDataDef.hpp"
#include "../Includes/WTSParams.hpp"
#include "../Share/TimeUtils.hpp"
#include "../Includes/IBaseDataMgr.h"
#include "../Share/DLLHelper.hpp"
#include "../Share/decimal.h"
#include "../Share/StrUtil.hpp"

#include <boost/filesystem.hpp>
#include <iostream>

std::mutex c1_mtx{};

void inst_hlp(){}

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

const std::string& getInstPath()
{
	static std::string moduleName;
	if (moduleName.empty())
	{
		Dl_info dl_info;
		dladdr((void *)inst_hlp, &dl_info);
		moduleName = dl_info.dli_fname;
	}

	return moduleName;
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
		_bin_dir = getInstPath();
#endif
		boost::filesystem::path p(_bin_dir);
		_bin_dir = p.branch_path().string() + "/";
	}

	return _bin_dir;
}

const char* ENTRUST_SECTION = "entrusts";
const char* ORDER_SECTION = "orders";


uint32_t strToTime(const char* strTime)
{
	std::string str;
	const char *pos = strTime;
	while (strlen(pos) > 0)
	{
		if (pos[0] != ':')
		{
			str.append(pos, 1);
		}
		pos++;
	}

	return strtoul(str.c_str(), NULL, 10);
}

extern "C"
{
	EXPORT_FLAG ITraderApi* createTrader()
	{
		TraderCTP *instance = new TraderCTP();
		return instance;
	}

	EXPORT_FLAG void deleteTrader(ITraderApi* &trader)
	{
		if (NULL != trader)
		{
			delete trader;
			trader = NULL;
		}
	}
}

TraderCTP::TraderCTP()
	: m_pUserAPI(NULL)
	, m_mapPosition(NULL)
	, m_ayOrders(NULL)
	, m_ayTrades(NULL)
	, m_ayPosDetail(NULL)
	, m_wrapperState(WS_NOTLOGIN)
	, m_uLastQryTime(0)
	, m_iRequestID(0)
	, m_bQuickStart(false)
	, m_bInQuery(false)
	, m_bStopped(false)
	, m_lastQryTime(0)
	,_uri("mongodb://192.168.214.199:27017")
	,_client(_uri)
{
}


TraderCTP::~TraderCTP()
{
}

bool TraderCTP::init(WTSParams* params)
{
	m_stra_name = params->get("name")->asCString();
	m_strFront = params->get("front")->asCString();
	m_strBroker = params->get("broker")->asCString();
	m_strUser = params->get("user")->asCString();
	m_strPass = params->get("pass")->asCString();

	m_strAppID = params->getCString("appid");
	m_strAuthCode = params->getCString("authcode");
	m_strFlowDir = params->getCString("flowdir");

	if (m_strFlowDir.empty())
		m_strFlowDir = "CTPTDFlow";

	m_strFlowDir = StrUtil::standardisePath(m_strFlowDir);

	WTSParams* param = params->get("ctpmodule");
	if (param != NULL)
		m_strModule = getBinDir() + DLLHelper::wrap_module(param->asCString());
	else
		m_strModule = getBinDir() + DLLHelper::wrap_module("thosttraderapi_se", "");

	m_hInstCTP = DLLHelper::load_library(m_strModule.c_str());
#ifdef _WIN32
#	ifdef _WIN64
	const char* creatorName = "?CreateFtdcTraderApi@CThostFtdcTraderApi@@SAPEAV1@PEBD@Z";
#	else
	const char* creatorName = "?CreateFtdcTraderApi@CThostFtdcTraderApi@@SAPAV1@PBD@Z";
#	endif
#else
	const char* creatorName = "_ZN19CThostFtdcTraderApi19CreateFtdcTraderApiEPKc";
#endif
	m_funcCreator = (CTPCreator)DLLHelper::get_symbol(m_hInstCTP, creatorName);

	m_bQuickStart = params->getBoolean("quick");

	return true;
}

void TraderCTP::release()
{
	m_bStopped = true;

	if (m_pUserAPI)
	{
		m_pUserAPI->RegisterSpi(NULL);
		m_pUserAPI->Release();
		m_pUserAPI = NULL;
	}

	if (m_ayOrders)
		m_ayOrders->clear();

	if (m_ayPosDetail)
		m_ayPosDetail->clear();

	if (m_mapPosition)
		m_mapPosition->clear();

	if (m_ayTrades)
		m_ayTrades->clear();
}

void TraderCTP::connect()
{
	std::stringstream ss;
	ss << m_strFlowDir << "flows/" << m_strBroker << "/" << m_strUser << "/";
	boost::filesystem::create_directories(ss.str().c_str());
	m_pUserAPI = m_funcCreator(ss.str().c_str());
	m_pUserAPI->RegisterSpi(this);
	if (m_bQuickStart)
	{
		m_pUserAPI->SubscribePublicTopic(THOST_TERT_QUICK);			// ע�ṫ����
		m_pUserAPI->SubscribePrivateTopic(THOST_TERT_QUICK);		// ע��˽����
	}
	else
	{
		m_pUserAPI->SubscribePublicTopic(THOST_TERT_RESUME);		// ע�ṫ����
		m_pUserAPI->SubscribePrivateTopic(THOST_TERT_RESUME);		// ע��˽����
	}

	m_pUserAPI->RegisterFront((char*)m_strFront.c_str());

	if (m_pUserAPI)
	{
		m_pUserAPI->Init();
	}

	if (m_thrdWorker == NULL)
	{
		m_thrdWorker.reset(new StdThread([this](){
			while (!m_bStopped)
			{
				if(m_queQuery.empty() || m_bInQuery)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					continue;
				}

				uint64_t curTime = TimeUtils::getLocalTimeNow();
				if (curTime - m_lastQryTime < 1000)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
					continue;
				}


				m_bInQuery = true;
				CommonExecuter& handler = m_queQuery.front();
				handler();

				{
					StdUniqueLock lock(m_mtxQuery);
					m_queQuery.pop();
				}

				m_lastQryTime = TimeUtils::getLocalTimeNow();
			}
		}));
	}
}

void TraderCTP::disconnect()
{
	m_queQuery.push([this]() {
		release();
	});

	if (m_thrdWorker)
	{
		m_thrdWorker->join();
		m_thrdWorker = NULL;
	}
}

bool TraderCTP::makeEntrustID(char* buffer, int length)
{
	if (buffer == NULL || length == 0)
		return false;

	try
	{
		memset(buffer, 0, length);
		uint32_t orderref = m_orderRef.fetch_add(1) + 1;
		sprintf(buffer, "%06u#%010u#%06u", m_frontID, m_sessionID, orderref);
		return true;
	}
	catch (...)
	{

	}

	return false;
}

void TraderCTP::registerSpi(ITraderSpi *listener)
{
	m_sink = listener;
	if (m_sink)
	{
		m_bdMgr = listener->getBaseDataMgr();
	}
}

uint32_t TraderCTP::genRequestID()
{
	return m_iRequestID.fetch_add(1) + 1;
}

int TraderCTP::login(const char* user, const char* pass, const char* productInfo)
{
	m_strUser = user;
	m_strPass = pass;

	if (m_pUserAPI == NULL)
	{
		return -1;
	}

	m_wrapperState = WS_LOGINING;
	authenticate();

	return 0;
}

int TraderCTP::doLogin()
{
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, m_strBroker.c_str());
	strcpy(req.UserID, m_strUser.c_str());
	strcpy(req.Password, m_strPass.c_str());
	strcpy(req.UserProductInfo, m_strProdInfo.c_str());
	int iResult = m_pUserAPI->ReqUserLogin(&req, genRequestID());
	if (iResult != 0)
	{
		m_sink->handleTraderLog(LL_ERROR, "[TraderCTP] Sending login request failed: %d", iResult);
	}

	return 0;
}

int TraderCTP::logout()
{
	if (m_pUserAPI == NULL)
	{
		return -1;
	}

	CThostFtdcUserLogoutField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, m_strBroker.c_str());
	strcpy(req.UserID, m_strUser.c_str());
	int iResult = m_pUserAPI->ReqUserLogout(&req, genRequestID());
	if (iResult != 0)
	{
		m_sink->handleTraderLog(LL_ERROR, "[TraderCTP] Sending logout request failed: %d", iResult);
	}

	return 0;
}

int TraderCTP::orderInsert(WTSEntrust* entrust)
{
	if (m_pUserAPI == NULL || m_wrapperState != WS_ALLREADY)
	{
		return -1;
	}

	CThostFtdcInputOrderField req;
	memset(&req, 0, sizeof(req));
	///���͹�˾����
	strcpy(req.BrokerID, m_strBroker.c_str());
	///Ͷ���ߴ���
	strcpy(req.InvestorID, m_strUser.c_str());
	///��Լ����
	strcpy(req.InstrumentID, entrust->getCode());

	strcpy(req.ExchangeID, entrust->getExchg());

	if (strlen(entrust->getUserTag()) == 0)
	{
		///��������
		sprintf(req.OrderRef, "%u", m_orderRef.fetch_add(0));

		//���ɱ���ί�е���
		//entrust->setEntrustID(generateEntrustID(m_frontID, m_sessionID, m_orderRef++).c_str());	
	}
	else
	{
		uint32_t fid, sid, orderref;
		extractEntrustID(entrust->getEntrustID(), fid, sid, orderref);
		//entrust->setEntrustID(entrust->getUserTag());
		///��������
		sprintf(req.OrderRef, "%d", orderref);
	}

	if (strlen(entrust->getUserTag()) > 0)
	{
		//m_mapEntrustTag[entrust->getEntrustID()] = entrust->getUserTag();
		m_iniHelper.writeString(ENTRUST_SECTION, entrust->getEntrustID(), entrust->getUserTag());
		m_iniHelper.save();
	}

	WTSContractInfo* ct = m_bdMgr->getContract(entrust->getCode(), entrust->getExchg());

	///�û�����
	//	TThostFtdcUserIDType	UserID;
	///�����۸�����: �޼�
	req.OrderPriceType = wrapPriceType(entrust->getPriceType(), strcmp(entrust->getExchg(), "CFFEX") == 0);
	///��������: 
	req.Direction = wrapDirectionType(entrust->getDirection(), entrust->getOffsetType());
	///��Ͽ�ƽ��־: ����
	req.CombOffsetFlag[0] = wrapOffsetType(entrust->getOffsetType());
	///���Ͷ���ױ���־
	req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	///�۸�
	req.LimitPrice = entrust->getPrice();
	///����: 1
	req.VolumeTotalOriginal = (int)entrust->getVolume();
	///��Ч������: ������Ч
	req.TimeCondition = wrapTimeCondition(entrust->getTimeCondition());
	///GTD����
	//	TThostFtdcDateType	GTDDate;
	///�ɽ�������: �κ�����
	req.VolumeCondition = THOST_FTDC_VC_AV;
	///��С�ɽ���: 1
	req.MinVolume = 1;
	///��������: ����
	req.ContingentCondition = THOST_FTDC_CC_Immediately;
	///ֹ���
	//	TThostFtdcPriceType	StopPrice;
	///ǿƽԭ��: ��ǿƽ
	req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///�Զ������־: ��
	req.IsAutoSuspend = 0;
	///ҵ��Ԫ
	//	TThostFtdcBusinessUnitType	BusinessUnit;
	///������
	//	TThostFtdcRequestIDType	RequestID;
	///�û�ǿ����־: ��
	req.UserForceClose = 0;

	int iResult = m_pUserAPI->ReqOrderInsert(&req, genRequestID());
	if (iResult != 0)
	{
		m_sink->handleTraderLog(LL_ERROR, "[TraderCTP] Order inserting failed: %d", iResult);
	}

	return 0;
}

int TraderCTP::orderAction(WTSEntrustAction* action)
{
	if (m_wrapperState != WS_ALLREADY)
		return -1;

	uint32_t frontid, sessionid, orderref;
	if (!extractEntrustID(action->getEntrustID(), frontid, sessionid, orderref))
		return -1;

	CThostFtdcInputOrderActionField req;
	memset(&req, 0, sizeof(req));
	///���͹�˾����
	strcpy(req.BrokerID, m_strBroker.c_str());
	///Ͷ���ߴ���
	strcpy(req.InvestorID, m_strUser.c_str());
	///��������
	sprintf(req.OrderRef, "%u", orderref);
	///������
	///ǰ�ñ��
	req.FrontID = frontid;
	///�Ự���
	req.SessionID = sessionid;
	///������־
	req.ActionFlag = wrapActionFlag(action->getActionFlag());
	///��Լ����
	strcpy(req.InstrumentID, action->getCode());

	req.LimitPrice = action->getPrice();

	req.VolumeChange = (int32_t)action->getVolume();

	strcpy(req.OrderSysID, action->getOrderID());
	strcpy(req.ExchangeID, action->getExchg());

	int iResult = m_pUserAPI->ReqOrderAction(&req, genRequestID());
	if (iResult != 0)
	{
		m_sink->handleTraderLog(LL_ERROR, "[TraderCTP] Sending cancel request failed: %d", iResult);
	}

	return 0;
}

int TraderCTP::queryAccount()
{
	if (m_pUserAPI == NULL || m_wrapperState != WS_ALLREADY)
	{
		return -1;
	}

	{
		StdUniqueLock lock(m_mtxQuery);
		m_queQuery.push([this]() {
			CThostFtdcQryTradingAccountField req;
			memset(&req, 0, sizeof(req));
			strcpy(req.BrokerID, m_strBroker.c_str());
			strcpy(req.InvestorID, m_strUser.c_str());
			m_pUserAPI->ReqQryTradingAccount(&req, genRequestID());
		});
	}

	//triggerQuery();

	return 0;
}

int TraderCTP::queryPositions()
{
	if (m_pUserAPI == NULL || m_wrapperState != WS_ALLREADY)
	{
		return -1;
	}

	{
		StdUniqueLock lock(m_mtxQuery);
		m_queQuery.push([this]() {
			CThostFtdcQryInvestorPositionField req;
			memset(&req, 0, sizeof(req));
			strcpy(req.BrokerID, m_strBroker.c_str());
			strcpy(req.InvestorID, m_strUser.c_str());
			m_pUserAPI->ReqQryInvestorPosition(&req, genRequestID());
		});
	}

	//triggerQuery();

	return 0;
}

int TraderCTP::queryOrders()
{
	if (m_pUserAPI == NULL || m_wrapperState != WS_ALLREADY)
	{
		return -1;
	}

	{
		StdUniqueLock lock(m_mtxQuery);
		m_queQuery.push([this]() {
			CThostFtdcQryOrderField req;
			memset(&req, 0, sizeof(req));
			strcpy(req.BrokerID, m_strBroker.c_str());
			strcpy(req.InvestorID, m_strUser.c_str());

			m_pUserAPI->ReqQryOrder(&req, genRequestID());
		});

		//triggerQuery();
	}

	return 0;
}

int TraderCTP::queryTrades()
{
	if (m_pUserAPI == NULL || m_wrapperState != WS_ALLREADY)
	{
		return -1;
	}

	{
		StdUniqueLock lock(m_mtxQuery);
		m_queQuery.push([this]() {
			CThostFtdcQryTradeField req;
			memset(&req, 0, sizeof(req));
			strcpy(req.BrokerID, m_strBroker.c_str());
			strcpy(req.InvestorID, m_strUser.c_str());

			m_pUserAPI->ReqQryTrade(&req, genRequestID());
		});
	}

	//triggerQuery();

	return 0;
}

int TraderCTP::querySettlement(uint32_t uDate)
{
	if (m_pUserAPI == NULL || m_wrapperState != WS_ALLREADY)
	{
		return -1;
	}

	m_strSettleInfo.clear();
	StdUniqueLock lock(m_mtxQuery);
	m_queQuery.push([this, uDate]() {
		CThostFtdcQrySettlementInfoField req;
		memset(&req, 0, sizeof(req));
		strcpy(req.BrokerID, m_strBroker.c_str());
		strcpy(req.InvestorID, m_strUser.c_str());
		sprintf(req.TradingDay, "%u", uDate);

		m_pUserAPI->ReqQrySettlementInfo(&req, genRequestID());
	});

	//triggerQuery();

	return 0;
}

void TraderCTP::OnFrontConnected()
{
	if (m_sink)
		m_sink->handleEvent(WTE_Connect, 0);
}

void TraderCTP::OnFrontDisconnected(int nReason)
{
	m_wrapperState = WS_NOTLOGIN;
	if (m_sink)
		m_sink->handleEvent(WTE_Close, nReason);
}

void TraderCTP::OnHeartBeatWarning(int nTimeLapse)
{

}

void TraderCTP::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo))
	{
		doLogin();
	}
	else
	{
		m_sink->handleTraderLog(LL_INFO, "[TraderCTP][%s-%s] Authentiation failed: %s", m_strBroker.c_str(), m_strUser.c_str(), pRspInfo->ErrorMsg);
		m_wrapperState = WS_LOGINFAILED;

		if (m_sink)
			m_sink->onLoginResult(false, pRspInfo->ErrorMsg, 0);
	}

}

void TraderCTP::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo))
	{
		m_wrapperState = WS_LOGINED;

		// ����Ự����
		m_frontID = pRspUserLogin->FrontID;
		m_sessionID = pRspUserLogin->SessionID;
		m_orderRef = atoi(pRspUserLogin->MaxOrderRef);
		///��ȡ��ǰ������
		m_lDate = atoi(m_pUserAPI->GetTradingDay());

		m_sink->handleTraderLog(LL_INFO, "[TraderCTP][%s-%s] Login succeed, AppID: %s, Sessionid: %u, login time: %s...",
			m_strBroker.c_str(), m_strUser.c_str(), m_strAppID.c_str(), m_sessionID, pRspUserLogin->LoginTime);

		std::stringstream ss;
		ss << m_strFlowDir << "local/" << m_strBroker << "/";
		std::string path = StrUtil::standardisePath(ss.str());
		if (!StdFile::exists(path.c_str()))
			boost::filesystem::create_directories(path.c_str());
		ss << m_strUser << ".dat";

		m_iniHelper.load(ss.str().c_str());
		uint32_t lastDate = m_iniHelper.readUInt("marker", "date", 0);
		if(lastDate != m_lDate)
		{
			//�����ղ�ͬ,�����ԭ��������
			m_iniHelper.removeSection(ENTRUST_SECTION);
			m_iniHelper.removeSection(ORDER_SECTION);
			m_iniHelper.writeUInt("marker", "date", m_lDate);
			m_iniHelper.save();

			m_sink->handleTraderLog(LL_INFO, "[TraderCTP][%s-%s] Trading date changed[%u -> %u], local cache cleared...", m_strBroker.c_str(), m_strUser.c_str(), lastDate, m_lDate);
		}

		m_sink->handleTraderLog(LL_INFO, "[TraderCTP][%s-%s] Login succeed, trading date: %u...", m_strBroker.c_str(), m_strUser.c_str(), m_lDate);

		m_sink->handleTraderLog(LL_INFO, "[TraderCTP][%s-%s] Querying confirming state of settlement data...", m_strBroker.c_str(), m_strUser.c_str());
		queryConfirm();
	}
	else
	{
		m_sink->handleTraderLog(LL_INFO, "[TraderCTP][%s-%s] Login failed: %s", m_strBroker.c_str(), m_strUser.c_str(), pRspInfo->ErrorMsg);
		m_wrapperState = WS_LOGINFAILED;

		if (m_sink)
			m_sink->onLoginResult(false, pRspInfo->ErrorMsg, 0);
	}
}

void TraderCTP::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	m_wrapperState = WS_NOTLOGIN;
	if (m_sink)
		m_sink->handleEvent(WTE_Logout, 0);
}

void TraderCTP::OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (bIsLast)
	{
		m_bInQuery = false;
		//triggerQuery();
	}

	if (!IsErrorRspInfo(pRspInfo))
	{
		if (pSettlementInfoConfirm != NULL)
		{
			uint32_t uConfirmDate = strtoul(pSettlementInfoConfirm->ConfirmDate, NULL, 10);
			if (uConfirmDate >= m_lDate)
			{
				m_wrapperState = WS_CONFIRMED;

				m_sink->handleTraderLog(LL_INFO, "[TraderCTP][%s-%s] Trading channel initialized...", m_strBroker.c_str(), m_strUser.c_str());
				m_wrapperState = WS_ALLREADY;
				if (m_sink)
					m_sink->onLoginResult(true, "", m_lDate);
			}
			else
			{
				m_wrapperState = WS_CONFIRM_QRYED;

				m_sink->handleTraderLog(LL_INFO, "[TraderCTP][%s-%s] Confirming settlement data...", m_strBroker.c_str(), m_strUser.c_str());
				confirm();
			}
		}
		else
		{
			m_wrapperState = WS_CONFIRM_QRYED;
			confirm();
		}
	}

}

void TraderCTP::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo) && pSettlementInfoConfirm != NULL)
	{
		if (m_wrapperState == WS_CONFIRM_QRYED)
		{
			m_wrapperState = WS_CONFIRMED;

			m_sink->handleTraderLog(LL_INFO, "[TraderCTP][%s-%s] Trading channel initialized...", m_strBroker.c_str(), m_strUser.c_str());
			m_wrapperState = WS_ALLREADY;
			if (m_sink)
				m_sink->onLoginResult(true, "", m_lDate);
		}
	}
}

void TraderCTP::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	WTSEntrust* entrust = makeEntrust(pInputOrder);
	if (entrust)
	{
		WTSError *err = makeError(pRspInfo, WEC_ORDERINSERT);
		//g_orderMgr.onRspEntrust(entrust, err);
		if (m_sink)
			m_sink->onRspEntrust(entrust, err);
		entrust->release();
		err->release();
	}
	else if(IsErrorRspInfo(pRspInfo))
	{
		WTSError *err = makeError(pRspInfo, WEC_ORDERINSERT);
		if (m_sink)
			m_sink->onTraderError(err);
		err->release();
	}
}

void TraderCTP::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (IsErrorRspInfo(pRspInfo))
	{

	}
	else
	{
		WTSError* error = WTSError::create(WEC_ORDERCANCEL, pRspInfo->ErrorMsg);
		if (m_sink)
			m_sink->onTraderError(error);
	}
}

void TraderCTP::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (bIsLast)
	{
		m_bInQuery = false;
		//triggerQuery();
	}

	if (bIsLast && !IsErrorRspInfo(pRspInfo))
	{
		WTSAccountInfo* accountInfo = WTSAccountInfo::create();
		accountInfo->setDescription(StrUtil::printf("%s-%s", m_strBroker.c_str(), m_strUser.c_str()).c_str());
		//accountInfo->setUsername(m_strUserName.c_str());
		accountInfo->setPreBalance(pTradingAccount->PreBalance);
		accountInfo->setCloseProfit(pTradingAccount->CloseProfit);
		accountInfo->setDynProfit(pTradingAccount->PositionProfit);
		accountInfo->setMargin(pTradingAccount->CurrMargin);
		accountInfo->setAvailable(pTradingAccount->Available);
		accountInfo->setCommission(pTradingAccount->Commission);
		accountInfo->setFrozenMargin(pTradingAccount->FrozenMargin);
		accountInfo->setFrozenCommission(pTradingAccount->FrozenCommission);
		accountInfo->setDeposit(pTradingAccount->Deposit);
		accountInfo->setWithdraw(pTradingAccount->Withdraw);
		accountInfo->setBalance(accountInfo->getPreBalance() + accountInfo->getCloseProfit() - accountInfo->getCommission() + accountInfo->getDeposit() - accountInfo->getWithdraw());
		accountInfo->setCurrency("CNY");

		WTSArray * ay = WTSArray::create();
		ay->append(accountInfo, false);
		if (m_sink)
			m_sink->onRspAccount(ay);

		ay->release();
	}
}

void TraderCTP::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (bIsLast)
	{
		m_bInQuery = false;
		//triggerQuery();
	}

	if (!IsErrorRspInfo(pRspInfo) && pInvestorPosition)
	{
		if (NULL == m_mapPosition)
			m_mapPosition = PositionMap::create();

		WTSContractInfo* contract = m_bdMgr->getContract(pInvestorPosition->InstrumentID, pInvestorPosition->ExchangeID);
		WTSCommodityInfo* commInfo = m_bdMgr->getCommodity(contract);
		if (contract)
		{
			std::string key = StrUtil::printf("%s-%d", pInvestorPosition->InstrumentID, pInvestorPosition->PosiDirection);
			WTSPositionItem* pos = (WTSPositionItem*)m_mapPosition->get(key);
			if(pos == NULL)
			{
				pos = WTSPositionItem::create(pInvestorPosition->InstrumentID, commInfo->getCurrency(), commInfo->getExchg());
				m_mapPosition->add(key, pos, false);
			}
			pos->setDirection(wrapPosDirection(pInvestorPosition->PosiDirection));
			if(commInfo->getCoverMode() == CM_CoverToday)
			{
				if (pInvestorPosition->PositionDate == THOST_FTDC_PSD_Today)
					pos->setNewPosition(pInvestorPosition->Position);
				else
					pos->setPrePosition(pInvestorPosition->Position);
			}
			else
			{
				pos->setNewPosition(pInvestorPosition->TodayPosition);
				pos->setPrePosition(pInvestorPosition->Position - pInvestorPosition->TodayPosition);
			}

			pos->setMargin(pos->getMargin() + pInvestorPosition->UseMargin);
			pos->setDynProfit(pos->getDynProfit() + pInvestorPosition->PositionProfit);
			pos->setPositionCost(pos->getPositionCost() + pInvestorPosition->PositionCost);

			if (pos->getTotalPosition() != 0)
			{
				pos->setAvgPrice(pos->getPositionCost() / pos->getTotalPosition() / commInfo->getVolScale());
			}
			else
			{
				pos->setAvgPrice(0);
			}

			if (commInfo->getCategoty() != CC_Combination)
			{
				if (commInfo->getCoverMode() == CM_CoverToday)
				{
					if (pInvestorPosition->PositionDate == THOST_FTDC_PSD_Today)
					{
						int availNew = pInvestorPosition->Position;
						if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long)
						{
							availNew -= pInvestorPosition->LongFrozen;
						}
						else
						{
							availNew -= pInvestorPosition->ShortFrozen;
						}
						if (availNew < 0)
							availNew = 0;
						pos->setAvailNewPos(availNew);
					}
					else
					{
						int availPre = pInvestorPosition->Position;
						if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long)
						{
							availPre -= pInvestorPosition->LongFrozen;
						}
						else
						{
							availPre -= pInvestorPosition->ShortFrozen;
						}
						if (availPre < 0)
							availPre = 0;
						pos->setAvailPrePos(availPre);
					}
				}
				else
				{
					int availNew = pInvestorPosition->TodayPosition;
					if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long)
					{
						availNew -= pInvestorPosition->LongFrozen;
					}
					else
					{
						availNew -= pInvestorPosition->ShortFrozen;
					}
					if (availNew < 0)
						availNew = 0;
					pos->setAvailNewPos(availNew);

					double availPre = pos->getNewPosition() + pos->getPrePosition()
						- pInvestorPosition->LongFrozen - pInvestorPosition->ShortFrozen
						- pos->getAvailNewPos();
					pos->setAvailPrePos(availPre);
				}
			}
			else
			{

			}

			if (decimal::lt(pos->getTotalPosition(), 0.0) && decimal::eq(pos->getMargin(), 0.0))
			{
				//�в�λ,���Ǳ�֤��Ϊ0,��˵����������Լ,������Լ�Ŀ��óֲ�ȫ����Ϊ0
				pos->setAvailNewPos(0);
				pos->setAvailPrePos(0);
			}
		}
	}

	if (bIsLast)
	{

		WTSArray* ayPos = WTSArray::create();

		if(m_mapPosition && m_mapPosition->size() > 0)
		{
			for (auto it = m_mapPosition->begin(); it != m_mapPosition->end(); it++)
			{
				ayPos->append(it->second, true);
			}
		}

		if (m_sink)
			m_sink->onRspPosition(ayPos);

		if (m_mapPosition)
		{
			m_mapPosition->release();
			m_mapPosition = NULL;
		}

		ayPos->release();
	}
}

void TraderCTP::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (bIsLast)
	{
		m_bInQuery = false;
		//triggerQuery();
	}

	if (!IsErrorRspInfo(pRspInfo) && pSettlementInfo)
	{
		m_strSettleInfo += pSettlementInfo->Content;
	}

	if (bIsLast && !m_strSettleInfo.empty())
	{
		m_sink->onRspSettlementInfo(atoi(pSettlementInfo->TradingDay), m_strSettleInfo.c_str());
	}
}

void TraderCTP::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (bIsLast)
	{
		m_bInQuery = false;
		//triggerQuery();
	}

	if (!IsErrorRspInfo(pRspInfo) && pTrade)
	{
		if (NULL == m_ayTrades)
			m_ayTrades = WTSArray::create();

		WTSTradeInfo* trade = makeTradeRecord(pTrade);
		if (trade)
		{
			m_ayTrades->append(trade, false);
		}
	}

	if (bIsLast)
	{
		if (m_sink)
			m_sink->onRspTrades(m_ayTrades);

		if (NULL != m_ayTrades)
			m_ayTrades->clear();
	}
}

void TraderCTP::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (bIsLast)
	{
		m_bInQuery = false;
		//triggerQuery();
	}

	if (!IsErrorRspInfo(pRspInfo) && pOrder)
	{
		if (NULL == m_ayOrders)
			m_ayOrders = WTSArray::create();

		WTSOrderInfo* orderInfo = makeOrderInfo(pOrder);
		if (orderInfo)
		{
			m_ayOrders->append(orderInfo, false);
		}
	}

	if (bIsLast)
	{
		if (m_sink)
			m_sink->onRspOrders(m_ayOrders);

		if (m_ayOrders)
			m_ayOrders->clear();
	}
}

void TraderCTP::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int x = 0;
}

time_t StringToDatetime(std::string str)
{
	tm tm_;												// ����tm�ṹ�塣
	int year, month, day, hour, minute, second;			// ����ʱ��ĸ���int��ʱ������
	year = atoi((str.substr(0, 4)).c_str());
	month = atoi((str.substr(4, 2)).c_str());
	day = atoi((str.substr(6, 2)).c_str());
	hour = atoi((str.substr(8, 2)).c_str());
	minute = atoi((str.substr(10, 2)).c_str());
	second = atoi((str.substr(12, 2)).c_str());
	//milsecond = atoi((str.substr(14, 3)).c_str());	//��ȡ����

	tm_.tm_year = year - 1900;                 // �꣬����tm�ṹ��洢���Ǵ�1900�꿪ʼ��ʱ�䣬����tm_yearΪint��ʱ������ȥ1900��      
	tm_.tm_mon = month - 1;                    // �£�����tm�ṹ����·ݴ洢��ΧΪ0-11������tm_monΪint��ʱ������ȥ1��
	tm_.tm_mday = day;                         // �ա�
	tm_.tm_hour = hour;                        // ʱ��
	tm_.tm_min = minute;                       // �֡�
	tm_.tm_sec = second;                       // �롣
	tm_.tm_isdst = 0;                          // ������ʱ��
	time_t t_ = mktime(&tm_);                  // ��tm�ṹ��ת����time_t��ʽ��
	return t_;						           // ����ֵ��
}

time_t timetrans(std::string pdate,std::string ptime)	//20220126,09:00:00
{
	std::string stime, hour, minu, second;
	long long itime;
	if (!pdate.empty() && !ptime.empty())
	{
		hour = ptime.substr(0, 2);
		minu = ptime.substr(3, 2);
		second = ptime.substr(6, 2);
		stime = (pdate + hour + minu + second);	//20220126090000
		//itime = atoi(stime.c_str()) - 8 * 10000;
		//stime = to_string(itime);
		//std::cout << "stime=" << stime << std::endl;
		return StringToDatetime(stime);
	}
}

void TraderCTP::insert_his_positions(CThostFtdcInvestorPositionField* pInvestorPosition)
{
	auto db = _client["lsqt_db"];
	auto _poscoll_1 = db["his_positions"];

	std::string exch_inst = pInvestorPosition->ExchangeID;
	exch_inst += "::";
	exch_inst += pInvestorPosition->InstrumentID;
	if (pInvestorPosition->OpenVolume)
	{
		return;
	}
	bsoncxx::document::value position_doc = document{} << finalize;
	if (pInvestorPosition->PosiDirection== THOST_FTDC_PD_Long)
	{
		position_doc = document{} <<
			"trade_day" << pInvestorPosition->PositionDate <<
			"strategy_id" << m_stra_name <<//
			"position" << open_document <<
			exch_inst << open_document <<
			"position_profit" << pInvestorPosition->PositionProfit <<
			//"float_profit_short" <<  <<
			//"open_price_short" <<  <<
			"volume_long_frozen_today" << pInvestorPosition->LongFrozen <<
			"open_cost_long" << pInvestorPosition->OpenCost <<
			//"position_price_short" <<  <<
			//"float_profit_long" <<  <<
			//"open_price_long" <<  <<
			"exchange_id" << pInvestorPosition->ExchangeID <<
			"volume_short_frozen_today" << 0 <<
			//"position_price_long" <<  <<
			"position_profit_long" << pInvestorPosition->PositionProfit <<
			"volume_short_today" << 0 <<
			"position_profit_short" << 0.0 <<
			"volume_long" << pInvestorPosition->Position <<
			//"margin_short" << pInvestorPosition->UseMargin <<
			//"volume_long_frozen_his" <<  <<
			//"float_profit" <<  <<
			"open_cost_short" << 0.0 <<
			"margin" << pInvestorPosition->UseMargin <<
			"position_cost_short" << 0.0 <<
			//"volume_short_frozen_his" <<  <<
			"instrument_id" << pInvestorPosition->InstrumentID <<
			"volume_short" << 0 <<
			"account_id" << pInvestorPosition->InvestorID <<
			"volume_long_today" << pInvestorPosition->TodayPosition <<
			"position_cost_long" << pInvestorPosition->PositionCost <<
			//"volume_long_his" <<  <<
			"hedge_flag" << pInvestorPosition->HedgeFlag <<
			//"margin_long" << pInvestorPosition->UseMargin <<
			//"volume_short_his" <<  <<
			"last_price" << pInvestorPosition->PreSettlementPrice << //
			close_document <<
			close_document <<
			"timestamp" << 0 <<
			finalize;
	}
	
	else if (pInvestorPosition->PosiDirection==THOST_FTDC_PD_Short)
	{
		position_doc = document{} <<
			"trade_day" << pInvestorPosition->PositionDate <<
			"strategy_id" << m_stra_name <<//
			"position" << open_document <<
			exch_inst << open_document <<
			"position_profit" << pInvestorPosition->PositionProfit <<
			//"float_profit_short" <<  <<
			//"open_price_short" <<  <<
			"volume_long_frozen_today" << 0 <<
			"open_cost_long" << 0.0 <<
			//"position_price_short" <<  <<
			//"float_profit_long" <<  <<
			//"open_price_long" <<  <<
			"exchange_id" << pInvestorPosition->ExchangeID <<
			"volume_short_frozen_today" << pInvestorPosition->ShortFrozen <<
			//"position_price_long" <<  <<
			"position_profit_long" << 0.0 <<
			"volume_short_today" << pInvestorPosition->TodayPosition <<
			"position_profit_short" << pInvestorPosition->PositionProfit <<
			"volume_long" << 0 <<
			//"margin_short" << pInvestorPosition->UseMargin <<
			//"volume_long_frozen_his" <<  <<
			//"float_profit" <<  <<
			"open_cost_short" << pInvestorPosition->OpenCost <<
			"margin" << pInvestorPosition->UseMargin <<
			"position_cost_short" << pInvestorPosition->PositionCost <<
			//"volume_short_frozen_his" <<  <<
			"instrument_id" << pInvestorPosition->InstrumentID <<
			"volume_short" << pInvestorPosition->Position <<
			"account_id" << pInvestorPosition->InvestorID <<
			"volume_long_today" << 0 <<
			"position_cost_long" << 0.0 <<
			//"volume_long_his" <<  <<
			"hedge_flag" << pInvestorPosition->HedgeFlag <<
			//"margin_long" << pInvestorPosition->UseMargin <<
			//"volume_short_his" <<  <<
			"last_price" << pInvestorPosition->PreSettlementPrice << //
			close_document <<
			close_document <<
			"timestamp" << 0 <<
			finalize;
	}
	c1_mtx.lock();
	auto result = _poscoll_1.insert_one(move(position_doc));
	bsoncxx::oid oid = result->inserted_id().get_oid().value;
	//std::cout << "insert one:" << oid.to_string() << std::endl;
	c1_mtx.unlock();

}

void TraderCTP::insert_his_order(CThostFtdcOrderField* pOrder)
{
	auto db = _client["lsqt_db"];
	auto _poscoll_1 = db["his_order"];

	std::string direction = "";
	if (pOrder->Direction == THOST_FTDC_D_Buy)
	{
		direction = "P032_1";
	}
	else if (pOrder->Direction = THOST_FTDC_D_Sell)
	{
		direction = "P032_2";
	}
	bsoncxx::document::value order_doc = document{} << 
		"offset" << pOrder->CombOffsetFlag <<
		"seqno" << pOrder->SequenceNo <<
		"trading_day" << pOrder->TradingDay <<
		"time_condition" << pOrder->TimeCondition <<
		"volume_condition" << pOrder->VolumeCondition <<
		"instrument_id" << pOrder->InstrumentID <<
		"limit_price" << pOrder->LimitPrice <<
		"exchange_order_id" << pOrder->OrderSysID <<
		"insert_date_time" << timetrans(pOrder->TradingDay, pOrder->InsertTime) * 1000 <<
		"last_msg" << 0 <<
		"exchange_id" << pOrder->ExchangeID <<
		"account_id" << pOrder->AccountID <<
		"volume_left" << pOrder->VolumeTotal <<
		"min_volume" << pOrder->MinVolume <<
		"hedge_flag" << pOrder->CombHedgeFlag <<
		"strategy_id" << m_stra_name <<
		"price_type" << pOrder->OrderPriceType <<
		"volume_orign" << pOrder->VolumeTotalOriginal <<
		"frozen_margin" << 0 <<
		"order_id" << pOrder->OrderLocalID <<
		"order_type" << pOrder->OrderType <<
		"direction" << direction <<
		"status" << pOrder->OrderStatus << finalize;

	c1_mtx.lock();
	auto result = _poscoll_1.insert_one(move(order_doc));
	bsoncxx::oid oid = result->inserted_id().get_oid().value;
	//std::cout << "insert one:" << oid.to_string() << std::endl;
	c1_mtx.unlock();
}

std::string getproduct(std::string instrument_id)
{
	std::string stdcode = "";
	if (!instrument_id.empty())
	{
		for (int i=0;i<instrument_id.length();i++)
		{
			if ((instrument_id[i]>='a'&&instrument_id[i]<='z')
				||(instrument_id[i]>='A'&&instrument_id[i]<='Z'))
			{
				stdcode += instrument_id[i];
			}
		}
	}

	return stdcode;
}

double TraderCTP::calc_fee(const char* stdCode, double price, double qty, uint32_t offset)
{
	double ret = 0.0;
	switch (offset)
	{
	case 0: ret = m_feeitem._open * qty; break;
	case 1: ret = m_feeitem._close * qty; break;
	case 2: ret = m_feeitem._close_today * qty; break;
	default: ret = 0.0; break;
	}

	return (int32_t)(ret * 100 + 0.5) / 100.0;
}

void TraderCTP::insert_his_trades(CThostFtdcTradeField* pTrade)
{
	auto db = _client["lsqt_db"];
	auto _poscoll_1 = db["test_trades"];

	//std::string productid = getproduct(pTrade->InstrumentID);
	//double fee = m_sink->getBaseDataMgr()->calc_fee(productid.c_str(), pTrade->Price, pTrade->Volume, 0);

	std::string offset = "";
	if (pTrade->OffsetFlag== THOST_FTDC_OF_Open){
		offset = "P033_1";
	}
	else if (pTrade->OffsetFlag== THOST_FTDC_OF_Close){
		offset = "P033_2";
	}
	else if (pTrade->OffsetFlag== THOST_FTDC_OF_CloseToday){
		offset = "P033_3";
	}
	bsoncxx::document::value trade_doc = document{} << 
		"exchange_trade_id" << pTrade->TradeID <<
		"account_id" << "" <<
		"commission" << calc_fee(pTrade->InstrumentID,pTrade->Price,pTrade->Volume,0) <<
		"direction" << pTrade->Direction <<
		"exchange_id" << pTrade->ExchangeID <<
		"instrument_id" << pTrade->InstrumentID <<
		"offset" << offset <<
		"order_id" << pTrade->OrderLocalID <<
		"price" << pTrade->Price <<
		"seqno" << pTrade->SequenceNo <<
		"strategy_id" << "0" <<
		//"trade_date_time" << timetrans(pTrade->TradeDate,pTrade->TradeTime) <<
		"volume" << pTrade->Volume << finalize;

	c1_mtx.lock();
	auto result = _poscoll_1.insert_one(move(trade_doc));
	bsoncxx::oid oid = result->inserted_id().get_oid().value;
	//std::cout << "insert one:" << oid.to_string() << std::endl;
	c1_mtx.unlock();
}

void TraderCTP::insert_his_trade(CThostFtdcTradeField* pTrade)
{
	auto db = _client["lsqt_db"];
	auto _poscoll_1 = db["his_trade"];

	//std::string productid = getproduct(pTrade->InstrumentID);
	//double fee = m_sink->getBaseDataMgr()->calc_fee(productid.c_str(), pTrade->Price, pTrade->Volume, 0);
	auto it = _sys_front_map[pTrade->OrderSysID];
	
	time_t insert_date_time = _front_time_map[_f_s_r_struct];

	std::string offset = "";
	if (pTrade->OffsetFlag == THOST_FTDC_OF_Open) {
		offset = "P033_1";
	}
	else if (pTrade->OffsetFlag == THOST_FTDC_OF_Close) {
		offset = "P033_2";
	}
	else if (pTrade->OffsetFlag == THOST_FTDC_OF_CloseToday) {
		offset = "P033_3";
	}
	std::string direction = "";
	if (pTrade->Direction== THOST_FTDC_D_Buy)
	{
		direction = "P032_1";
	}
	else if (pTrade->Direction= THOST_FTDC_D_Sell)
	{
		direction = "P032_2";
	}
	bsoncxx::document::value trade_doc = document{} <<
		"trade_date_time" << timetrans(pTrade->TradingDay, pTrade->TradeTime) * 1000 <<
		"offset" << offset <<
		"seqno" << pTrade->SequenceNo <<
		"exchange_trade_id" << pTrade->TradeID <<
		"trading_day" << pTrade->TradingDay <<
		"type" << pTrade->TradeType <<
		"instrument_id" << pTrade->InstrumentID <<
		"exchange_order_id" << pTrade->OrderSysID <<
		"order_type" << direction <<
		"exchange_type" << "M008_1" <<
		"close_profit" << 0.0 <<
		"volume" << pTrade->Volume <<
		"exchange_id" << pTrade->ExchangeID <<
		"account_id" << "" <<
		"price" << pTrade->Price <<
		"strategy_id" << m_stra_name <<
		"commission" << calc_fee(pTrade->InstrumentID, pTrade->Price, pTrade->Volume, 0) <<
		"order_id" << pTrade->OrderLocalID <<
		"insert_date_time" << insert_date_time <<
		"direction" << direction << finalize;

	c1_mtx.lock();
	auto result = _poscoll_1.insert_one(move(trade_doc));
	bsoncxx::oid oid = result->inserted_id().get_oid().value;
	//std::cout << "insert one:" << oid.to_string() << std::endl;
	c1_mtx.unlock();
}

void TraderCTP::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	/*std::cout << __FUNCTION__ << endl << "pOrder->date:" << pOrder->TradingDay << endl <<
		"pOrder->time:" << pOrder->InsertTime << endl;*/
	WTSOrderInfo *orderInfo = makeOrderInfo(pOrder);
	insert_his_order(pOrder);
	auto sys_fro_map = _sys_front_map[pOrder->OrderSysID];
	sys_fro_map.front_id = pOrder->FrontID;
	sys_fro_map.session_id = pOrder->SessionID;
	sys_fro_map.oref = pOrder->OrderRef;

	_front_time_map[sys_fro_map] = timetrans(pOrder->TradingDay, pOrder->InsertTime) * 1000;
	if (orderInfo)
	{
		if (m_sink)
			m_sink->onPushOrder(orderInfo);

		orderInfo->release();
	}

	//ReqQryTradingAccount();
}

void TraderCTP::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	/*std::cout << __FUNCTION__ << endl << "pTrade->date:" << pTrade->TradingDay << endl <<
		"pTrade->time:" << pTrade->TradeTime << endl;*/
	WTSTradeInfo *tRecord = makeTradeRecord(pTrade);
	//insert_his_trades(pTrade);
	insert_his_trade(pTrade);
	if (tRecord)
	{
		if (m_sink)
			m_sink->onPushTrade(tRecord);

		tRecord->release();
	}
}

int TraderCTP::wrapDirectionType(WTSDirectionType dirType, WTSOffsetType offsetType)
{
	if (WDT_LONG == dirType)
		if (offsetType == WOT_OPEN)
			return THOST_FTDC_D_Buy;
		else
			return THOST_FTDC_D_Sell;
	else
		if (offsetType == WOT_OPEN)
			return THOST_FTDC_D_Sell;
		else
			return THOST_FTDC_D_Buy;
}

WTSDirectionType TraderCTP::wrapDirectionType(TThostFtdcDirectionType dirType, TThostFtdcOffsetFlagType offsetType)
{
	if (THOST_FTDC_D_Buy == dirType)
		if (offsetType == THOST_FTDC_OF_Open)
			return WDT_LONG;
		else
			return WDT_SHORT;
	else
		if (offsetType == THOST_FTDC_OF_Open)
			return WDT_SHORT;
		else
			return WDT_LONG;
}

WTSDirectionType TraderCTP::wrapPosDirection(TThostFtdcPosiDirectionType dirType)
{
	if (THOST_FTDC_PD_Long == dirType)
		return WDT_LONG;
	else
		return WDT_SHORT;
}

int TraderCTP::wrapOffsetType(WTSOffsetType offType)
{
	if (WOT_OPEN == offType)
		return THOST_FTDC_OF_Open;
	else if (WOT_CLOSE == offType)
		return THOST_FTDC_OF_Close;
	else if (WOT_CLOSETODAY == offType)
		return THOST_FTDC_OF_CloseToday;
	else if (WOT_CLOSEYESTERDAY == offType)
		return THOST_FTDC_OF_Close;
	else
		return THOST_FTDC_OF_ForceClose;
}

WTSOffsetType TraderCTP::wrapOffsetType(TThostFtdcOffsetFlagType offType)
{
	if (THOST_FTDC_OF_Open == offType)
		return WOT_OPEN;
	else if (THOST_FTDC_OF_Close == offType)
		return WOT_CLOSE;
	else if (THOST_FTDC_OF_CloseToday == offType)
		return WOT_CLOSETODAY;
	else
		return WOT_FORCECLOSE;
}

int TraderCTP::wrapPriceType(WTSPriceType priceType, bool isCFFEX /* = false */)
{
	if (WPT_ANYPRICE == priceType)
		return isCFFEX ? THOST_FTDC_OPT_FiveLevelPrice : THOST_FTDC_OPT_AnyPrice;
	else if (WPT_LIMITPRICE == priceType)
		return THOST_FTDC_OPT_LimitPrice;
	else if (WPT_BESTPRICE == priceType)
		return THOST_FTDC_OPT_BestPrice;
	else
		return THOST_FTDC_OPT_LastPrice;
}

WTSPriceType TraderCTP::wrapPriceType(TThostFtdcOrderPriceTypeType priceType)
{
	if (THOST_FTDC_OPT_AnyPrice == priceType || THOST_FTDC_OPT_FiveLevelPrice == priceType)
		return WPT_ANYPRICE;
	else if (THOST_FTDC_OPT_LimitPrice == priceType)
		return WPT_LIMITPRICE;
	else if (THOST_FTDC_OPT_BestPrice == priceType)
		return WPT_BESTPRICE;
	else
		return WPT_LASTPRICE;
}

int TraderCTP::wrapTimeCondition(WTSTimeCondition timeCond)
{
	if (WTC_IOC == timeCond)
		return THOST_FTDC_TC_IOC;
	else if (WTC_GFD == timeCond)
		return THOST_FTDC_TC_GFD;
	else
		return THOST_FTDC_TC_GFS;
}

WTSTimeCondition TraderCTP::wrapTimeCondition(TThostFtdcTimeConditionType timeCond)
{
	if (THOST_FTDC_TC_IOC == timeCond)
		return WTC_IOC;
	else if (THOST_FTDC_TC_GFD == timeCond)
		return WTC_GFD;
	else
		return WTC_GFS;
}

WTSOrderState TraderCTP::wrapOrderState(TThostFtdcOrderStatusType orderState)
{
	if (orderState != THOST_FTDC_OST_Unknown)
		return (WTSOrderState)orderState;
	else
		return WOS_Submitting;
}

int TraderCTP::wrapActionFlag(WTSActionFlag actionFlag)
{
	if (WAF_CANCEL == actionFlag)
		return THOST_FTDC_AF_Delete;
	else
		return THOST_FTDC_AF_Modify;
}


WTSOrderInfo* TraderCTP::makeOrderInfo(CThostFtdcOrderField* orderField)
{
	WTSContractInfo* contract = m_bdMgr->getContract(orderField->InstrumentID, orderField->ExchangeID);
	if (contract == NULL)
		return NULL;

	WTSOrderInfo* pRet = WTSOrderInfo::create();
	pRet->setPrice(orderField->LimitPrice);
	pRet->setVolume(orderField->VolumeTotalOriginal);
	pRet->setDirection(wrapDirectionType(orderField->Direction, orderField->CombOffsetFlag[0]));
	pRet->setPriceType(wrapPriceType(orderField->OrderPriceType));
	pRet->setTimeCondition(wrapTimeCondition(orderField->TimeCondition));
	pRet->setOffsetType(wrapOffsetType(orderField->CombOffsetFlag[0]));

	pRet->setVolTraded(orderField->VolumeTraded);
	pRet->setVolLeft(orderField->VolumeTotal);

	pRet->setCode(orderField->InstrumentID);
	pRet->setExchange(contract->getExchg());

	uint32_t uDate = strtoul(orderField->InsertDate, NULL, 10);
	std::string strTime = orderField->InsertTime;
	StrUtil::replace(strTime, ":", "");
	uint32_t uTime = strtoul(strTime.c_str(), NULL, 10);
	if (uTime >= 210000 && uDate == m_lDate)
	{
		uDate = TimeUtils::getNextDate(uDate, -1);
	}

	pRet->setOrderDate(uDate);
	pRet->setOrderTime(TimeUtils::makeTime(uDate, uTime * 1000));

	pRet->setOrderState(wrapOrderState(orderField->OrderStatus));
	if (orderField->OrderSubmitStatus >= THOST_FTDC_OSS_InsertRejected)
		pRet->setError(true);		

	pRet->setEntrustID(generateEntrustID(orderField->FrontID, orderField->SessionID, atoi(orderField->OrderRef)).c_str());
	pRet->setOrderID(orderField->OrderSysID);

	pRet->setStateMsg(orderField->StatusMsg);


	std::string usertag = m_iniHelper.readString(ENTRUST_SECTION, pRet->getEntrustID(), "");
	if(usertag.empty())
	{
		pRet->setUserTag(pRet->getEntrustID());
	}
	else
	{
		pRet->setUserTag(usertag.c_str());

		if (strlen(pRet->getOrderID()) > 0)
		{
			m_iniHelper.writeString(ORDER_SECTION, StrUtil::trim(pRet->getOrderID()).c_str(), usertag.c_str());
			m_iniHelper.save();
		}
	}

	return pRet;
}

WTSEntrust* TraderCTP::makeEntrust(CThostFtdcInputOrderField *entrustField)
{
	WTSContractInfo* ct = m_bdMgr->getContract(entrustField->InstrumentID, entrustField->ExchangeID);
	if (ct == NULL)
		return NULL;

	WTSEntrust* pRet = WTSEntrust::create(
		entrustField->InstrumentID,
		entrustField->VolumeTotalOriginal,
		entrustField->LimitPrice,
		ct->getExchg());

	pRet->setDirection(wrapDirectionType(entrustField->Direction, entrustField->CombOffsetFlag[0]));
	pRet->setPriceType(wrapPriceType(entrustField->OrderPriceType));
	pRet->setOffsetType(wrapOffsetType(entrustField->CombOffsetFlag[0]));
	pRet->setTimeCondition(wrapTimeCondition(entrustField->TimeCondition));

	pRet->setEntrustID(generateEntrustID(m_frontID, m_sessionID, atoi(entrustField->OrderRef)).c_str());

	//StringMap::iterator it = m_mapEntrustTag.find(pRet->getEntrustID());
	//if (it != m_mapEntrustTag.end())
	//{
	//	pRet->setUserTag(it->second.c_str());
	//}
	std::string usertag = m_iniHelper.readString(ENTRUST_SECTION, pRet->getEntrustID());
	if (!usertag.empty())
		pRet->setUserTag(usertag.c_str());

	return pRet;
}

WTSError* TraderCTP::makeError(CThostFtdcRspInfoField* rspInfo, WTSErroCode ec /* = WEC_NONE */)
{
	WTSError* pRet = WTSError::create(ec, rspInfo->ErrorMsg);
	return pRet;
}

WTSTradeInfo* TraderCTP::makeTradeRecord(CThostFtdcTradeField *tradeField)
{
	WTSContractInfo* contract = m_bdMgr->getContract(tradeField->InstrumentID, tradeField->ExchangeID);
	if (contract == NULL)
		return NULL;

	WTSCommodityInfo* commInfo = m_bdMgr->getCommodity(contract);
	WTSSessionInfo* sInfo = m_bdMgr->getSession(commInfo->getSession());

	WTSTradeInfo *pRet = WTSTradeInfo::create(tradeField->InstrumentID, commInfo->getExchg());
	pRet->setVolume(tradeField->Volume);
	pRet->setPrice(tradeField->Price);
	pRet->setTradeID(tradeField->TradeID);

	std::string strTime = tradeField->TradeTime;
	StrUtil::replace(strTime, ":", "");
	uint32_t uTime = strtoul(strTime.c_str(), NULL, 10);
	uint32_t uDate = strtoul(tradeField->TradeDate, NULL, 10);
	
	//�����ҹ��ʱ�䣬���ҳɽ����ڵ��ڽ����գ�˵���ɽ���������Ҫ����
	//��Ϊҹ������ǰ�ģ��ɽ����ڱ�ȻС�ڽ�����
	//��������ֻ����һ����������
	if(uTime >= 210000 && uDate == m_lDate)
	{
		uDate = TimeUtils::getNextDate(uDate, -1);
	}

	pRet->setTradeDate(uDate);
	pRet->setTradeTime(TimeUtils::makeTime(uDate, uTime * 1000));

	WTSDirectionType dType = wrapDirectionType(tradeField->Direction, tradeField->OffsetFlag);

	pRet->setDirection(dType);
	pRet->setOffsetType(wrapOffsetType(tradeField->OffsetFlag));
	pRet->setRefOrder(tradeField->OrderSysID);
	pRet->setTradeType((WTSTradeType)tradeField->TradeType);

	double amount = commInfo->getVolScale()*tradeField->Volume*pRet->getPrice();
	pRet->setAmount(amount);

	//StringMap::iterator it = m_mapOrderTag.find(pRet->getRefOrder());
	//if (it != m_mapOrderTag.end())
	//{
	//	pRet->setUserTag(it->second.c_str());
	//}
	std::string usertag = m_iniHelper.readString(ORDER_SECTION, StrUtil::trim(pRet->getRefOrder()).c_str());
	if (!usertag.empty())
		pRet->setUserTag(usertag.c_str());

	return pRet;
}

std::string TraderCTP::generateEntrustID(uint32_t frontid, uint32_t sessionid, uint32_t orderRef)
{
	return StrUtil::printf("%06u#%010u#%06u", frontid, sessionid, orderRef);
}

bool TraderCTP::extractEntrustID(const char* entrustid, uint32_t &frontid, uint32_t &sessionid, uint32_t &orderRef)
{
	//Market.FrontID.SessionID.OrderRef
	const StringVector &vecString = StrUtil::split(entrustid, "#");
	if (vecString.size() != 3)
		return false;

	frontid = strtoul(vecString[0].c_str(), NULL, 10);
	sessionid = strtoul(vecString[1].c_str(), NULL, 10);
	orderRef = strtoul(vecString[2].c_str(), NULL, 10);

	return true;
}

bool TraderCTP::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
	if (pRspInfo && pRspInfo->ErrorID != 0)
		return true;

	return false;
}

void TraderCTP::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{
	WTSEntrust* entrust = makeEntrust(pInputOrder);
	if (entrust)
	{
		WTSError *err = makeError(pRspInfo, WEC_ORDERINSERT);
		//g_orderMgr.onRspEntrust(entrust, err);
		if (m_sink)
			m_sink->onRspEntrust(entrust, err);
		entrust->release();
		err->release();
	}
}

bool TraderCTP::isConnected()
{
	return (m_wrapperState == WS_ALLREADY);
}

int TraderCTP::queryConfirm()
{
	if (m_pUserAPI == NULL || m_wrapperState != WS_LOGINED)
	{
		return -1;
	}

	{
		StdUniqueLock lock(m_mtxQuery);
		m_queQuery.push([this]() {
			CThostFtdcQrySettlementInfoConfirmField req;
			memset(&req, 0, sizeof(req));
			strcpy(req.BrokerID, m_strBroker.c_str());
			strcpy(req.InvestorID, m_strUser.c_str());

			int iResult = m_pUserAPI->ReqQrySettlementInfoConfirm(&req, genRequestID());
			if (iResult != 0)
			{
				m_sink->handleTraderLog(LL_ERROR, "[TraderCTP][%s-%s] Sending query of settlement data confirming state failed: %d", m_strBroker.c_str(), m_strUser.c_str(), iResult);
			}
		});
	}

	//triggerQuery();

	return 0;
}

int TraderCTP::confirm()
{
	if (m_pUserAPI == NULL || m_wrapperState != WS_CONFIRM_QRYED)
	{
		return -1;
	}

	//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	CThostFtdcSettlementInfoConfirmField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, m_strBroker.c_str());
	strcpy(req.InvestorID, m_strUser.c_str());

	sprintf(req.ConfirmDate, "%u", TimeUtils::getCurDate());
	strncpy(req.ConfirmTime, TimeUtils::getLocalTime().c_str(), 8);

	int iResult = m_pUserAPI->ReqSettlementInfoConfirm(&req, genRequestID());
	if (iResult != 0)
	{
		m_sink->handleTraderLog(LL_ERROR, "[TraderCTP][%s-%s] Sending confirming of settlement data failed: %d", m_strBroker.c_str(), m_strUser.c_str(), iResult);
		return -1;
	}

	return 0;
}

int TraderCTP::authenticate()
{
	CThostFtdcReqAuthenticateField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, m_strBroker.c_str());
	strcpy(req.UserID, m_strUser.c_str());
	//strcpy(req.UserProductInfo, m_strProdInfo.c_str());
	strcpy(req.AuthCode, m_strAuthCode.c_str());
	strcpy(req.AppID, m_strAppID.c_str());
	m_pUserAPI->ReqAuthenticate(&req, genRequestID());

	return 0;
}

/*
void TraderCTP::triggerQuery()
{
	m_strandIO->post([this](){
		if (m_queQuery.empty() || m_bInQuery)
			return;

		uint64_t curTime = TimeUtils::getLocalTimeNow();
		if (curTime - m_lastQryTime < 1000)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			m_strandIO->post([this](){
				triggerQuery();
			});
			return;
		}


		m_bInQuery = true;
		CommonExecuter& handler = m_queQuery.front();
		handler();

		{
			StdUniqueLock lock(m_mtxQuery);
			m_queQuery.pop();
		}

		m_lastQryTime = TimeUtils::getLocalTimeNow();
	});
}
*/