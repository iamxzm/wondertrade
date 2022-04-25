/*!
 * \file IParserApi.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief �������ģ��ӿڶ����ļ�
 */
#pragma once
#include <string>
#include <stdint.h>
#include "WTSTypes.h"
#include "FasterDefs.h"

NS_WTP_BEGIN
class WTSTickData;
class WTSOrdDtlData;
class WTSOrdQueData;
class WTSTransData;
class WTSVariant;
class WTSArray;
class IBaseDataMgr;

/*
 *	�������ģ��ص��ӿ�
 */
class IParserSpi
{
public:
	/*
	 *	����ģ���¼�
	 *	@e	�¼�����,�����ӡ��Ͽ�����¼���ǳ�
	 *	@ec	������,0Ϊû�д���
	 */
	virtual void handleEvent(WTSParserEvent e, int32_t ec){}

	/*
	 *	�����Լ�б�
	 *	@aySymbols	��Լ�б�,����Ԫ��ΪWTSContractInfo,WTSArray���÷���ο�����
	 */
	virtual void handleSymbolList(const WTSArray* aySymbols)		= 0;

	/*
	 *	����ʵʱ����
	 *	@quote		ʵʱ����
	 *	@procFlag	�����ǣ�0-��Ƭ���飬���账��(ParserUDP)��1-�������գ���Ҫ��Ƭ(���ڸ�·ͨ��)��2-������գ���Ҫ�����ۼӣ���Ҫ������ߡ�tick��m1��m5�����Զ��ۼӵģ�����������飩
	 */
	virtual void handleQuote(WTSTickData *quote, uint32_t procFlag)	= 0;

	/*
	 *	����ί�ж������ݣ���Ʊlevel2��
	 *	@ordQueData	ί�ж�������
	 */
	virtual void handleOrderQueue(WTSOrdQueData* ordQueData){}

	/*
	 *	�������ί�����ݣ���Ʊlevel2��
	 *	@ordDetailData	���ί������
	 */
	virtual void handleOrderDetail(WTSOrdDtlData* ordDetailData){}

	/*
	 *	������ʳɽ�����
	 *	@transData	��ʳɽ�����
	 */
	virtual void handleTransaction(WTSTransData* transData){}

	/*
	 *	�������ģ�����־
	 *	@ll			��־����
	 *	@message	��־����
	 */
	virtual void handleParserLog(WTSLogLevel ll, const char* message)	= 0;

public:
	virtual IBaseDataMgr*	getBaseDataMgr()	= 0;
};

/*
 *	�������ģ��ӿ�
 */
class IParserApi
{
public:
	virtual ~IParserApi(){}

public:
	/*
	 *	��ʼ������ģ��
	 *	@config	ģ������
	 *	����ֵ	�Ƿ��ʼ���ɹ�
	 */
	virtual bool init(WTSVariant* config) { return false; }

	/*
	 *	�ͷŽ���ģ��
	 *	�����˳�ʱ
	 */
	virtual void release(){}

	/*
	 *	��ʼ���ӷ�����
	 *	@����ֵ	���������Ƿ��ͳɹ�
	 */
	virtual bool connect() { return false; }

	/*
	 *	�Ͽ�����
	 *	@����ֵ	�����Ƿ��ͳɹ�
	 */
	virtual bool disconnect() { return false; }

	/*
	 *	�Ƿ�������
	 *	@����ֵ	�Ƿ�������
	 */
	virtual bool isConnected() { return false; }

	/*
	 *	���ĺ�Լ�б�
	 */
	virtual void subscribe(const CodeSet& setCodes){}

	/*
	 *	�˶���Լ�б�
	 */
	virtual void unsubscribe(const CodeSet& setCodes){}

	/*
	 *	ע��ص��ӿ�
	 */
	virtual void registerSpi(IParserSpi* spi) {}
};

NS_WTP_END

//��ȡIDataMgr�ĺ���ָ������
typedef wtp::IParserApi* (*FuncCreateParser)();
typedef void(*FuncDeleteParser)(wtp::IParserApi* &parser);