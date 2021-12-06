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

typedef faster_hashset<std::string> CodeSet;

NS_OTP_BEGIN
class WTSTickData;
class WTSOrdDtlData;
class WTSOrdQueData;
class WTSTransData;
class WTSParams;
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
	 *	@bNeedSlice	�Ƿ���Ҫ��Ƭ,����Ǵ��ⲿ����Ŀ�����������,����Ҫ��Ƭ,������ڲ��㲥�ľͲ���Ҫ��Ƭ
	 */
	virtual void handleQuote(WTSTickData *quote, bool bNeedSlice)	= 0;

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
	virtual void handleParserLog(WTSLogLevel ll, const char* format, ...)	= 0;

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
	virtual bool init(WTSParams* config)			= 0;

	/*
	 *	�ͷŽ���ģ��
	 *	�����˳�ʱ
	 */
	virtual void release()							= 0;

	/*
	 *	��ʼ���ӷ�����
	 *	@����ֵ	���������Ƿ��ͳɹ�
	 */
	virtual bool connect()							= 0;

	/*
	 *	�Ͽ�����
	 *	@����ֵ	�����Ƿ��ͳɹ�
	 */
	virtual bool disconnect()						= 0;

	/*
	 *	�Ƿ�������
	 *	@����ֵ	�Ƿ�������
	 */
	virtual bool isConnected()						= 0;

	/*
	 *	���ĺ�Լ�б�
	 */
	virtual void subscribe(const CodeSet& setCodes)		= 0;

	/*
	 *	�˶���Լ�б�
	 */
	virtual void unsubscribe(const CodeSet& setCodes)	= 0;

	/*
	 *	ע��ص��ӿ�
	 */
	virtual void registerSpi(IParserSpi* listener)		= 0;
};

NS_OTP_END

//��ȡIDataMgr�ĺ���ָ������
typedef otp::IParserApi* (*FuncCreateParser)();
typedef void(*FuncDeleteParser)(otp::IParserApi* &parser);