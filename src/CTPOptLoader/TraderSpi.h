#pragma once
//v3.5.8P4
#include "../API/CTPOpt3.5.8/ThostFtdcTraderApi.h"

class CTraderSpi : public CThostFtdcTraderSpi
{
public:
	///���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ��,�÷��������á�
	virtual void OnFrontConnected();

	virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///��¼������Ӧ
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
	///�����ѯ��Լ��Ӧ
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///����Ӧ��
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	
	///���ͻ����뽻�׺�̨ͨ�����ӶϿ�ʱ,�÷��������á���������������,API���Զ���������,�ͻ��˿ɲ�������
	virtual void OnFrontDisconnected(int nReason);

private:
	void ReqAuth();
	///�û���¼����
	void ReqUserLogin();
	///�����ѯ��Լ
	void ReqQryInstrument();

	// �Ƿ��յ��ɹ�����Ӧ
	bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);


	void DumpToJson();

protected:
	int	m_lTradingDate;
	int m_ReqCount;
};