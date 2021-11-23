/*!
 * \file ParserXTP.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#pragma once
#include "../Includes/IParserApi.h"
#include "../Share/DLLHelper.hpp"
//XTP v1.1.19.2
#include "./XTPQuoteApi/xtp_quote_api.h"
#include <map>

NS_OTP_BEGIN
class WTSTickData;
NS_OTP_END

USING_NS_OTP;

class ParserXTP :	public IParserApi, public XTP::API::QuoteSpi
{
public:
	ParserXTP();
	virtual ~ParserXTP();

public:
	enum LoginStatus
	{
		LS_NOTLOGIN,
		LS_LOGINING,
		LS_LOGINED
	};

//IQuoteParser �ӿ�
public:
	virtual bool init(WTSParams* config) override;

	virtual void release() override;

	virtual bool connect() override;

	virtual bool disconnect() override;

	virtual bool isConnected() override;

	virtual void subscribe(const CodeSet &vecSymbols) override;
	virtual void unsubscribe(const CodeSet &vecSymbols) override;

	virtual void registerSpi(IParserSpi* listener) override;


//CThostFtdcMdSpi �ӿ�
public:
	///���ͻ����뽻�׺�̨ͨ�����ӶϿ�ʱ,�÷��������á���������������,API���Զ���������,�ͻ��˿ɲ�������
	///@param reason ����ԭ��
	///        0x1001 �����ʧ��
	///        0x1002 ����дʧ��
	///        0x2001 ����������ʱ
	///        0x2002 ��������ʧ��
	///        0x2003 �յ�������
	virtual void OnDisconnected(int reason) override;

	///����Ӧ��
	virtual void OnError(XTPRI *error_info) override;


	virtual void OnSubMarketData(XTPST *ticker, XTPRI *error_info, bool is_last) override;
	virtual void OnUnSubMarketData(XTPST *ticker, XTPRI *error_info, bool is_last) override;
	virtual void OnDepthMarketData(XTPMD *market_data, int64_t bid1_qty[], int32_t bid1_count, int32_t max_bid1_count, int64_t ask1_qty[], int32_t ask1_count, int32_t max_ask1_count) override;

private:
	/*
	 *	���͵�¼����
	 */
	void DoLogin();
	/*
	 *	����Ʒ������
	 */
	void DoSubscribeMD();
	/*
	 *	��������Ϣ
	 */
	bool IsErrorRspInfo(XTPRI *error_info);


private:
	uint32_t			m_uTradingDate;
	LoginStatus			m_loginState;
	XTP::API::QuoteApi*	m_pUserAPI;

	std::string			m_strHost;
	int32_t				m_iPort;;
	uint32_t			m_uClientID;
	std::string			m_strUser;
	std::string			m_strPass;
	std::string			m_strFlowDir;

	XTP_PROTOCOL_TYPE	m_iProtocol;
	uint32_t			m_uHBInterval;
	uint32_t			m_uBuffSize;

	CodeSet				m_fitSHSubs;
	CodeSet				m_fitSZSubs;

	int					m_iRequestID;

	IParserSpi*	m_sink;
	IBaseDataMgr*		m_pBaseDataMgr;

	DllHandle		m_hInst;
	typedef XTP::API::QuoteApi* (*XTPCreater)(uint8_t, const char *, XTP_LOG_LEVEL log_level);
	//typedef CUstpFtdcMduserApi* (*FemasCreator)(const char *);
	XTPCreater		m_funcCreator;
};

