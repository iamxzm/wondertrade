/*!
 * \file ITraderApi.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief ����ͨ���Խӽӿڶ����ļ�
 */
#pragma once
#include <set>
#include <map>
#include <stdint.h>
#include <functional>
#include "WTSTypes.h"

NS_WTP_BEGIN
class WTSVariant;
class WTSEntrust;
class WTSOrderInfo;
class WTSTradeInfo;
class WTSEntrustAction;
class WTSAccountInfo;
class WTSPositionItem;
class WTSContractInfo;
class WTSError;
class WTSTickData;
class WTSNotify;
class WTSArray;
class IBaseDataMgr;

typedef std::function<void()>	CommonExecuter;

#pragma region "Stock Trading API definations"
/*
 *	��Ʊ���׽ӿڻص�
 *	Added By Wesley @ 2020/05/06
 */
class IStkTraderSpi
{

};

/*
 *	��Ʊ���׽ӿ�
 *	Added By Wesley @ 2020/05/06
 *	Ԥ�������Ժ�ʹ��,�Ȱѽӿڵ��໥��ܴ����
 *	��Ҫ�ṩ������ȯ�ȹ�Ʊ���нӿ�
 */
class IStkTraderApi
{
};
#pragma endregion

#pragma region "Option Trading API definations"
/*
 *	��Ȩ���׽ӿڻص�
 *	Added By Wesley @ 2020/05/06
 */
class IOptTraderSpi
{
public:
	virtual void onRspEntrustOpt(WTSEntrust* entrust, WTSError *err) {}
	virtual void onRspOrdersOpt(const WTSArray* ayOrders) {}
	virtual void onPushOrderOpt(WTSOrderInfo* orderInfo) {}
};

/*
 *	��Ȩ���׽ӿ�
 *	Added By Wesley @ 2020/05/06
 *	Ԥ�������Ժ�ʹ��,�Ȱѽӿڵ��໥��ܴ����
 *	��Ҫ�ṩ���ۡ���Ȩ����Ȩ���нӿ�
 */
class IOptTraderApi
{
public:
	/*
	 *	�µ��ӿ�
	 *	entrust �µ��ľ������ݽṹ
	 */
	virtual int orderInsertOpt(WTSEntrust* eutrust) { return -1; }

	/*
	 *	���������ӿ�
	 *	action	�����ľ������ݽṹ
	 */
	virtual int orderActionOpt(WTSEntrustAction* action) { return -1; }

	/*
	 *	��ѯ��Ȩ����
	 */
	virtual int	queryOrdersOpt(WTSBusinessType bType) { return -1; }
};
#pragma endregion


//ί�лص��ӿ�
class ITraderSpi
{
public:
	virtual IBaseDataMgr*	getBaseDataMgr() = 0;
	virtual void handleTraderLog(WTSLogLevel ll, const char* message){}

	virtual IStkTraderSpi* getStkSpi(){ return NULL; }
	virtual IOptTraderSpi* getOptSpi(){ return NULL; }

public:
	virtual void handleEvent(WTSTraderEvent e, int32_t ec) = 0;
	virtual void onLoginResult(bool bSucc, const char* msg, uint32_t tradingdate) = 0;
	virtual void onLogout(){}
	virtual void onRspEntrust(WTSEntrust* entrust, WTSError *err){}
	virtual void onRspAccount(WTSArray* ayAccounts) {}
	virtual void onRspPosition(const WTSArray* ayPositions){}
	virtual void onRspOrders(const WTSArray* ayOrders){}
	virtual void onRspTrades(const WTSArray* ayTrades){}
	virtual void onRspSettlementInfo(uint32_t uDate, const char* content){}

	virtual void onPushOrder(WTSOrderInfo* orderInfo){}
	virtual void onPushTrade(WTSTradeInfo* tradeRecord){}

	virtual void onTraderError(WTSError* err){}
};

//�µ��ӿڹ���ӿ�
class ITraderApi
{
public:
	virtual ~ITraderApi(){}

	virtual IStkTraderApi* getStkTrader() { return NULL; }
	virtual IOptTraderApi* getOptTrader() { return NULL; }

public:
	/*
	 *	��ʼ������������
	 */
	virtual bool init(WTSVariant *params) { return false; }

	/*
	 *	�ͷŽ���������
	 */
	virtual void release(){}

	/*
	 *	ע��ص��ӿ�
	 */
	virtual void registerSpi(ITraderSpi *listener) {}


	//////////////////////////////////////////////////////////////////////////
	//ҵ���߼��ӿ�

	/*
	 *	���ӷ�����
	 */
	virtual void connect() {}

	/*
	 *	�Ͽ�����
	 */
	virtual void disconnect() {}

	virtual bool isConnected() { return false; }

	/*
	 *	����ί�е���
	 */
	virtual bool makeEntrustID(char* buffer, int length){ return false; }

	/*
	 *	��¼�ӿ�
	 */
	virtual int login(const char* user, const char* pass, const char* productInfo) { return -1; }

	/*
	 *	ע���ӿ�
	 */
	virtual int logout() { return -1; }

	/*
	 *	�µ��ӿ�
	 *	entrust �µ��ľ������ݽṹ
	 */
	virtual int orderInsert(WTSEntrust* eutrust) { return -1; }

	/*
	 *	���������ӿ�
	 *	action	�����ľ������ݽṹ
	 */
	virtual int orderAction(WTSEntrustAction* action) { return -1; }

	/*
	 *	��ѯ�˻���Ϣ
	 */
	virtual int queryAccount() { return -1; }

	/*
	 *	��ѯ�ֲ���Ϣ
	 */
	virtual int queryPositions() { return -1; }

	/*
	 *	��ѯ���ж���
	 */
	virtual int queryOrders() { return -1; }

	/*
	 *	��ѯ�ɽ���ϸ
	 */
	virtual int	queryTrades() { return -1; }

	/*
	 *	��ѯ���㵥
	 */
	virtual int querySettlement(uint32_t uDate){ return 0; }

};

NS_WTP_END

//��ȡIDataMgr�ĺ���ָ������
typedef wtp::ITraderApi* (*FuncCreateTrader)();
typedef void(*FuncDeleteTrader)(wtp::ITraderApi* &trader);
