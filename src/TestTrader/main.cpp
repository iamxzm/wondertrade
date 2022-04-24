#include <iostream>
#include <boost/filesystem.hpp>

#include "../Includes/ITraderApi.h"
#include "../Includes/WTSVariant.hpp"
#include "../Includes/WTSTradeDef.hpp"
#include "../Includes/WTSError.hpp"
#include "../Includes/WTSCollection.hpp"

#include "../Share/TimeUtils.hpp"
#include "../Share/StdUtils.hpp"
#include "../Share/DLLHelper.hpp"
#include "../Share/StrUtil.hpp"

#include "../WTSTools/WTSBaseDataMgr.h"
#include "../WTSTools/WTSLogger.h"
#include "../WTSUtils/WTSCfgLoader.h"


WTSBaseDataMgr	g_bdMgr;
StdUniqueMutex	g_mtxOpt;
StdCondVariable	g_condOpt;
bool		g_exitNow = false;
bool		g_riskAct = false;
std::set<std::string>	g_blkList;

USING_NS_WTP;

class TraderSpi : public ITraderSpi
{
public:
	TraderSpi() :m_bLogined(false), m_mapOrds(NULL){}

	bool init(WTSVariant* params, const char* ttype)
	{
		m_pParams = params;
		if (m_pParams)
			m_pParams->retain();

		m_strModule = ttype;
		return true;
	}

	void release()
	{
		if (m_pTraderApi)
		{
			m_pTraderApi->release();
		}
	}

	void run(bool bRestart = false)
	{
		if (!createTrader(m_strModule.c_str()))
		{
			return;
		}

		m_pTraderApi->registerSpi(this);

		if (!m_pTraderApi->init(m_pParams))
		{
			return;
		}

		m_pTraderApi->connect();
	}

	bool createTrader(const char* moduleName)
	{
		DllHandle hInst = DLLHelper::load_library(moduleName);
		if (hInst == NULL)
		{
			WTSLogger::info_f("Loading module {} failed", moduleName);
			return false;
		}

		FuncCreateTrader pFunCreateTrader = (FuncCreateTrader)DLLHelper::get_symbol(hInst, "createTrader");
		if (NULL == pFunCreateTrader)
		{
			WTSLogger::info_f("Entry function createTrader not exists");
			return false;
		}

		m_pTraderApi = pFunCreateTrader();
		if (NULL == m_pTraderApi)
		{
			WTSLogger::info_f("Creating trader api failed");
			return false;
		}

		m_funcDelTrader = (FuncDeleteTrader)DLLHelper::get_symbol(hInst, "deleteTrader");
		return true;
	}

	bool qryFund()
	{
		WTSLogger::info_f("Querying fund info...");
		m_pTraderApi->queryAccount();

		return true;
	}

	bool qryOrders()
	{
		WTSLogger::info_f("Querying orders...");
		m_pTraderApi->queryOrders();

		return true;
	}

	bool qryTrades()
	{
		WTSLogger::info_f("Querying trades...");
		m_pTraderApi->queryTrades();

		return true;
	}

	bool qryPosition()
	{
		WTSLogger::info_f("Querying positions...");
		m_pTraderApi->queryPositions();

		return true;
	}

	bool qrySettle()
	{
		uint32_t uDate = TimeUtils::getNextDate(TimeUtils::getCurDate(), -1);
		WTSLogger::info_f("Querying settlement info on {}...", uDate);
		m_pTraderApi->querySettlement(uDate);

		return true;
	}

	bool entrustLmt(bool isNet)
	{
		char code[32] = { 0 };
		char exchg[32] = { 0 };
		double price = 0.0;
		double qty = 0;
		uint32_t bs = 0;
		uint32_t offset = 0;

		for (;;)
		{
			printf("������Ʒ�ִ���: ");
			std::cin >> code;

			printf("�����뽻��������: ");
			std::cin >> exchg;

			printf("������ί�м۸�: ");
			std::cin >> price;

			printf("����������: ");
			std::cin >> qty;

			if(isNet)
			{
				printf("�����뷽��,0-����,1-����: ");
				std::cin >> bs;
				if (bs != 0 && bs != 1)
					continue;

				printf("Ʒ��: %s.%s,�۸�: %f,����: %f,����: %s,ȷ��y/n? ", exchg, code, price, qty, bs == 0 ? "����" : "����");
			}
			else
			{
				printf("�����뷽��,0-����,1-����: ");
				std::cin >> bs;
				if (bs != 0 && bs != 1)
					continue;

				printf("�����뿪ƽ,0-����,1-ƽ��: ");
				std::cin >> offset;
				if (offset != 0 && offset != 1)
					continue;

				printf("Ʒ��: %s.%s,�۸�: %f,����: %f,����: %s,��ƽ: %s,ȷ��y/n? ", exchg, code, price, qty, bs == 0 ? "����" : "����", offset == 0 ? "����" : "ƽ��");
			}
			
			char c;
			std::cin >> c;
			if(c == 'y')
				break;
		}

		if(g_riskAct)
		{
			auto it = g_blkList.find(code);
			if (it != g_blkList.end())
			{
				WTSLogger::info_f("{}�ѱ���ֹ����", code);
				return false;
			}
		}

		bool bNeedToday = (strcmp(exchg, "SHFE") == 0 || strcmp(exchg, "INE") == 0);

		WTSEntrust* entrust = WTSEntrust::create(code, qty, price, exchg);
		if(!isNet)
		{
			entrust->setDirection(bs == 0 ? WDT_LONG : WDT_SHORT);
			entrust->setOffsetType(offset == 0 ? WOT_OPEN : (bNeedToday ? WOT_CLOSETODAY : WOT_CLOSE));
		}
		else
		{
			entrust->setNetDirection(bs == 0);
		}
		
		entrust->setPriceType(WPT_LIMITPRICE);
		entrust->setOrderFlag(WOF_NOR);

		char entrustid[64] = { 0 };
		m_pTraderApi->makeEntrustID(entrustid, 64);
		entrust->setEntrustID(entrustid);

		if(!isNet)
			WTSLogger::info_f("[{}]��ʼ�µ�,Ʒ��: {}.{},�۸�: {},����: {},����: {}{}", m_pParams->getCString("user"), exchg, code, price, qty, offset == 0 ? "��" : "ƽ", bs == 0 ? "��" : "��");
		else
			WTSLogger::info_f("[{}]��ʼ�µ�,Ʒ��: {}.{},�۸�: {},����: {},����: {}", m_pParams->getCString("user"), exchg, code, price, qty, bs == 0 ? "����" : "����");

		entrust->setContractInfo(g_bdMgr.getContract(code, exchg));
		m_pTraderApi->orderInsert(entrust);
		entrust->release();

		return true;
	}

	bool entrustMkt()
	{
		char code[32] = { 0 };
		char exchg[32] = { 0 };
		uint32_t qty = 0;
		uint32_t bs = 0;
		uint32_t offset = 0;

		for (;;)
		{
			printf("������Ʒ�ִ���: ");
			std::cin >> code;

			printf("�����뽻��������: ");
			std::cin >> exchg;

			printf("����������: ");
			std::cin >> qty;

			printf("�����뷽��,0-����,1-����: ");
			std::cin >> bs;

			printf("�����뿪ƽ,0-����,1-ƽ��: ");
			std::cin >> offset;

			printf("Ʒ��: %s.%s,����: %u,����: %s,��ƽ: %s,ȷ��y/n? ", exchg, code, qty, bs == 0 ? "����" : "����", offset == 0 ? "����" : "ƽ��");
			char c;
			std::cin >> c;
			if (c == 'y')
				break;
		}

		if (g_riskAct)
		{
			auto it = g_blkList.find(code);
			if (it != g_blkList.end())
			{
				WTSLogger::info_f("{}�ѱ���ֹ����", code);
				return false;
			}
		}

		bool bNeedToday = (strcmp(exchg, "SHFE") == 0 || strcmp(exchg, "INE") == 0);
		WTSEntrust* entrust = WTSEntrust::create(code, qty, 0, exchg);
		entrust->setDirection(bs == 0 ? WDT_LONG : WDT_SHORT);
		entrust->setOffsetType(offset == 0 ? WOT_OPEN : (bNeedToday ? WOT_CLOSETODAY : WOT_CLOSE));
		entrust->setPriceType(WPT_ANYPRICE);
		entrust->setOrderFlag(WOF_NOR);

		char entrustid[64] = { 0 };
		m_pTraderApi->makeEntrustID(entrustid, 64);
		entrust->setEntrustID(entrustid);

		WTSLogger::info_f("[{}]��ʼ�µ�,Ʒ��: {}.{},�۸�: �м�,����: {},����: {}{}", m_pParams->getCString("user"), exchg, code, qty, offset == 0 ? "��" : "ƽ", bs == 0 ? "��" : "��");

		m_pTraderApi->orderInsert(entrust);
		entrust->release();

		return true;
	}

	bool cancel()
	{
		char orderid[128] = { 0 };

		for (;;)
		{
			printf("�����붩��ID: ");
			std::cin >> orderid;

			printf("����ID: %s,ȷ��y\n? ", orderid);
			char c;
			std::cin >> c;
			if (c == 'y')
				break;
		}

		if (m_mapOrds == NULL)
			m_mapOrds = WTSObjectMap::create();

		WTSOrderInfo* ordInfo = (WTSOrderInfo*)m_mapOrds->get(orderid);
		if (ordInfo == NULL)
		{
			WTSLogger::info_f("����������,���鶩�����Ƿ�����,�����Ȳ�ѯ����");
			return false;
		}


		WTSLogger::info_f("[{}]��ʼ����[{}]...", m_pParams->getCString("user"), orderid);
		WTSEntrustAction* action = WTSEntrustAction::create(ordInfo->getCode());
		action->setEntrustID(ordInfo->getEntrustID());
		action->setOrderID(orderid);
		action->setActionFlag(WAF_CANCEL);

		m_pTraderApi->orderAction(action);
		action->release();

		return true;
	}

public:
	virtual void handleEvent(WTSTraderEvent e, int32_t ec)
	{
		if(e == WTE_Connect)
		{
			if (ec == 0)
			{
				WTSLogger::info_f("[{}]���ӳɹ�", m_pParams->getCString("user"));
				m_pTraderApi->login(m_pParams->getCString("user"), m_pParams->getCString("pass"), "");
			}
			else
			{
				g_exitNow = true;
				StdUniqueLock lock(g_mtxOpt);
				g_condOpt.notify_all();
			}
		}
	}

	virtual void handleTraderLog(WTSLogLevel ll, const char* message) override
	{
		WTSLogger::log_raw(ll, message);
	}

	virtual void onLoginResult(bool bSucc, const char* msg, uint32_t tradingdate)
	{
		if(bSucc)
		{
			WTSLogger::info_f("[{}]��¼�ɹ�" , m_pParams->getCString("user"));
			m_bLogined = true;
		}
		else
		{
			WTSLogger::info_f("[{}]��¼ʧ��: {}", m_pParams->getCString("user"), msg);
			g_exitNow = true;
		}

		StdUniqueLock lock(g_mtxOpt);
		g_condOpt.notify_all();
	}

	virtual void onRspEntrust(WTSEntrust* entrust, WTSError *err)
	{
		if(err)
		{
			WTSLogger::info_f("[{}]�µ�ʧ��: {}", m_pParams->getCString("user"), err->getMessage());
			StdUniqueLock lock(g_mtxOpt);
			g_condOpt.notify_all();
		}
		
	}

	virtual void onRspAccount(WTSArray* ayAccounts)
	{
		if(ayAccounts != NULL)
		{
			WTSAccountInfo* accInfo = (WTSAccountInfo*)ayAccounts->at(0);
			if(accInfo)
			{
				WTSLogger::info_f("[{}]�ʽ����ݸ���, ��ǰ��̬Ȩ��: {:.2f}", m_pParams->getCString("user"), accInfo->getBalance());
			}
		}

		StdUniqueLock lock(g_mtxOpt);
		g_condOpt.notify_all();
	}

	virtual void onRspPosition(const WTSArray* ayPositions)
	{
		uint32_t cnt = 0;
		if (ayPositions != NULL)
			cnt = ayPositions->size();

		WTSLogger::info_f("[{}]�ֲ������Ѹ���, ���չ���{}�ʳֲ�", m_pParams->getCString("user"), cnt);
		for(uint32_t i = 0; i < cnt; i++)
		{
			WTSPositionItem* posItem = (WTSPositionItem*)((WTSArray*)ayPositions)->at(i);
			if(posItem && posItem->getTotalPosition() > 0 && g_riskAct)
			{
				g_blkList.insert(posItem->getCode());
				WTSLogger::info_f("{}�ֲ�������,���ƿ���", posItem->getCode());
			}
		}
		StdUniqueLock lock(g_mtxOpt);
		g_condOpt.notify_all();
	}

	virtual void onRspOrders(const WTSArray* ayOrders)
	{
		uint32_t cnt = 0;
		if (ayOrders != NULL)
			cnt = ayOrders->size();

		if (m_mapOrds == NULL)
			m_mapOrds = WTSObjectMap::create();

		m_mapOrds->clear();
		for (uint32_t i = 0; i < cnt; i++)
		{
			WTSOrderInfo* ordInfo = (WTSOrderInfo*)((WTSArray*)ayOrders)->at(i);
			if (ordInfo->isAlive())
				m_mapOrds->add(ordInfo->getOrderID(), ordInfo, true);
		}

		WTSLogger::info_f("[{}]ί���б��Ѹ���, ���չ���{}��ί��, δ���{}��", m_pParams->getCString("user"), cnt, m_mapOrds->size());

		StdUniqueLock lock(g_mtxOpt);
		g_condOpt.notify_all();
	}

	virtual void onRspTrades(const WTSArray* ayTrades)
	{
		uint32_t cnt = 0;
		if (ayTrades != NULL)
			cnt = ayTrades->size();

		WTSLogger::info_f("[{}]�ɽ���ϸ�Ѹ���, ���չ���{}�ʳɽ�", m_pParams->getCString("user"), cnt);
		StdUniqueLock lock(g_mtxOpt);
		g_condOpt.notify_all();
	}

	virtual void onRspSettlementInfo(uint32_t uDate, const char* content)
	{
		WTSLogger::info_f("[{}]{}���㵥�ѽ���", m_pParams->getCString("user"), uDate);
		WTSLogger::info_f(content);
		StdUniqueLock lock(g_mtxOpt);
		g_condOpt.notify_all();
	}

	virtual void onPushOrder(WTSOrderInfo* orderInfo)
	{
		if(orderInfo->getOrderState() != WOS_Canceled)
		{
			if(strlen(orderInfo->getOrderID()) > 0)
			{
				if (m_mapOrds == NULL)
					m_mapOrds = WTSObjectMap::create();

				if (m_mapOrds->find(orderInfo->getOrderID()) == m_mapOrds->end())
				{
					WTSLogger::info_f("[{}]�µ��ɹ�,����ID: {}",  m_pParams->getCString("user"), orderInfo->getOrderID());
					m_mapOrds->add(orderInfo->getOrderID(), orderInfo, true);
				}

				StdUniqueLock lock(g_mtxOpt);
				g_condOpt.notify_all();
			}
		}
		else 
		{
			if (m_mapOrds)
				m_mapOrds->remove(orderInfo->getOrderID());

			if (strlen(orderInfo->getOrderID()) == 0)
			{
				WTSLogger::info_f("[{}����{}�ύʧ�ܱ�����:{}", m_pParams->getCString("user"), orderInfo->getEntrustID(), orderInfo->getStateMsg());
				StdUniqueLock lock(g_mtxOpt);
				g_condOpt.notify_all();
			}
			else
			{
				WTSLogger::info_f("[{}]����{}�ѳ���:{}", m_pParams->getCString("user"), orderInfo->getOrderID(), orderInfo->getStateMsg());
				StdUniqueLock lock(g_mtxOpt);
				g_condOpt.notify_all();
			}			
		}
	}

	virtual void onPushTrade(WTSTradeInfo* tradeRecord)
	{
		WTSLogger::info_f("[{}]�յ��ɽ��ر�,��Լ{},�ɽ���: {},�ɽ�����: {}", m_pParams->getCString("user"), tradeRecord->getCode(), tradeRecord->getPrice(), tradeRecord->getVolume());

		if(g_riskAct)
		{
			WTSLogger::info_f("[{}]{}�������ֲ�����,��ֹ����", m_pParams->getCString("user"), tradeRecord->getCode());

			g_blkList.insert(tradeRecord->getCode());
		}
	}

	virtual void onTraderError(WTSError*	err)
	{
		if(err && err->getErrorCode() == WEC_ORDERCANCEL)
		{
			WTSLogger::info_f("[{}]����ʧ��: {}", m_pParams->getCString("user"), err->getMessage());
			StdUniqueLock lock(g_mtxOpt);
			g_condOpt.notify_all();
		}
		else if (err && err->getErrorCode() == WEC_ORDERINSERT)
		{
			WTSLogger::info_f("[{}]�µ�ʧ��: {}", m_pParams->getCString("user"), err->getMessage());
			StdUniqueLock lock(g_mtxOpt);
			g_condOpt.notify_all();
		}
	}

public:
	virtual IBaseDataMgr*	getBaseDataMgr()
	{
		return &g_bdMgr;
	}
	

private:
	ITraderApi*			m_pTraderApi;
	FuncDeleteTrader	m_funcDelTrader;
	std::string			m_strModule;
	WTSVariant*			m_pParams;

	typedef WTSHashMap<std::string>	WTSObjectMap;
	WTSObjectMap*		m_mapOrds;

	bool				m_bLogined;
};

std::string getBaseFolder()
{
	static std::string basePath;
	if (basePath.empty())
	{
		basePath = boost::filesystem::initial_path<boost::filesystem::path>().string();

		basePath = StrUtil::standardisePath(basePath);
	}

	return basePath.c_str();
}

int main()
{
	WTSLogger::init("logcfg.yaml");

	WTSLogger::info_f("�����ɹ�,��ǰϵͳ�汾��: v1.0");

	WTSVariant* root = WTSCfgLoader::load_from_file("config.yaml", true);
	if(root == NULL)
	{
		WTSLogger::log_raw(LL_ERROR, "�����ļ�config.yaml����ʧ��");
		return 0;
	}

	WTSVariant* cfg = root->get("config");
	bool isUTF8 = cfg->getBoolean("utf8");
	if(cfg->has("session"))
		g_bdMgr.loadSessions(cfg->getCString("session"), isUTF8);

	if (cfg->has("commodity"))
		g_bdMgr.loadCommodities(cfg->getCString("commodity"), isUTF8);

	if (cfg->has("contract"))
		g_bdMgr.loadContracts(cfg->getCString("contract"), isUTF8);

	if (cfg->has("holiday"))
		g_bdMgr.loadHolidays(cfg->getCString("holiday"));

	g_riskAct = cfg->getBoolean("risk");
	WTSLogger::info_f("��ؿ���: {}", g_riskAct ? "��" : "��");

	std::string module = cfg->getCString("trader");
	std::string profile = cfg->getCString("profile");
	WTSVariant* params = root->get(profile.c_str());
	if(params == NULL)
	{
		WTSLogger::error_f("������{}������", profile);
		return 0;
	}

	TraderSpi* trader = new TraderSpi;
	trader->init(params, module.c_str());
	trader->run();

	root->release();

	{
		StdUniqueLock lock(g_mtxOpt);
		g_condOpt.wait(lock);
	}
	
	while(true)
	{
		printf("��ѡ�����\r\n");
		printf("1����ѯ�ʽ�\r\n");
		printf("2����ѯ����\r\n");
		printf("3����ѯ�ɽ�\r\n");
		printf("4����ѯ�ֲ�\r\n");
		printf("5����ѯ���㵥\r\n");
		printf("6���޼��µ�\r\n");
		printf("7���м��µ�\r\n");
		printf("8������\r\n");
		printf("9�����ֲֽ���\r\n");
		printf("0���˳�\r\n");

		char cmd;
		for (;;)
		{
			scanf("%c", &cmd);

			if(cmd >= '0' && cmd <= '9')
				break;
		}

		bool bSucc = false;
		switch (cmd)
		{
		case '1':
			bSucc = trader->qryFund();
			break;
		case '2': 
			bSucc = trader->qryOrders();
			break;
		case '3': 
			bSucc = trader->qryTrades();
			break;
		case '4': 
			bSucc = trader->qryPosition();
			break;
		case '5': 
			bSucc = trader->qrySettle();
			break;
		case '6': 
			bSucc = trader->entrustLmt(false);
			break;
		case '7': 
			bSucc = trader->entrustMkt();
			break;
		case '8': 
			bSucc = trader->cancel();
			break;
		case '9':
			bSucc = trader->entrustLmt(true);
			break;
		case '0': break;
		default:
			cmd = 'X';
			break;
		}

		if (cmd != '0' && cmd != 'X' && bSucc)
		{
			StdUniqueLock lock(g_mtxOpt);
			g_condOpt.wait(lock);
		}
		else if(cmd == '0')
			break;
	}
	
	//exit(9);
	trader->release();
	delete trader;
	return 0;
}