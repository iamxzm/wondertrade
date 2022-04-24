/*!
 * \file WTSDataFactory.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief ����ƴ�ӹ����ඨ��
 */
#pragma once
#include "../Includes/IDataFactory.h"

USING_NS_WTP;

class WTSDataFactory : public IDataFactory
{
public:
	/*
	 *	����tick���ݸ���K��
	 *	@klineData	K������
	 *	@tick		tick����
	 *	@sInfo		����ʱ��ģ��
	 */
	virtual WTSBarStruct*	updateKlineData(WTSKlineData* klineData, WTSTickData* tick, WTSSessionInfo* sInfo);

	/*
	 *	���û�������K�����ݸ���K��
	 *	@klineData		K������
	 *	@newBasicBar	��������K������
	 *	@sInfo			����ʱ��ģ��
	 */
	virtual WTSBarStruct*	updateKlineData(WTSKlineData* klineData, WTSBarStruct* newBasicBar, WTSSessionInfo* sInfo);

	/*
	 *	�ӻ�������K��������ȡ�ǻ������ڵ�K������
	 *	@baseKline	��������K��
	 *	@period		�������ڣ�m1/m5/day
	 *	@times		���ڱ���
	 *	@sInfo		����ʱ��ģ��
	 *	@bIncludeOpen	�Ƿ����δ�պϵ�K��
	 */
	virtual WTSKlineData*	extractKlineData(WTSKlineSlice* baseKline, WTSKlinePeriod period, uint32_t times, WTSSessionInfo* sInfo, bool bIncludeOpen = true);

	/*
	 *	��tick������ȡ�����ڵ�K������
	 *	@ayTicks	tick����
	 *	@seconds	Ŀ������
	 *	@sInfo		����ʱ��ģ��
	 *	@bUnixTime	tickʱ����Ƿ���unixtime
	 */
	virtual WTSKlineData*	extractKlineData(WTSTickSlice* ayTicks, uint32_t seconds, WTSSessionInfo* sInfo, bool bUnixTime = false);

	/*
	 *	�ϲ�K��
	 *	@klineData	Ŀ��K��
	 *	@newKline	���ϲ���K��
	 */
	virtual bool			mergeKlineData(WTSKlineData* klineData, WTSKlineData* newKline);

protected:
	WTSBarStruct* updateMin1Data(WTSSessionInfo* sInfo, WTSKlineData* klineData, WTSTickData* tick);
	WTSBarStruct* updateMin5Data(WTSSessionInfo* sInfo, WTSKlineData* klineData, WTSTickData* tick);
	WTSBarStruct* updateDayData(WTSSessionInfo* sInfo, WTSKlineData* klineData, WTSTickData* tick);
	WTSBarStruct* updateSecData(WTSSessionInfo* sInfo, WTSKlineData* klineData, WTSTickData* tick);

	WTSBarStruct* updateMin1Data(WTSSessionInfo* sInfo, WTSKlineData* klineData, WTSBarStruct* newBasicBar);
	WTSBarStruct* updateMin5Data(WTSSessionInfo* sInfo, WTSKlineData* klineData, WTSBarStruct* newBasicBar);

	WTSKlineData* extractMin1Data(WTSKlineSlice* baseKline, uint32_t times, WTSSessionInfo* sInfo, bool bIncludeOpen = true);
	WTSKlineData* extractMin5Data(WTSKlineSlice* baseKline, uint32_t times, WTSSessionInfo* sInfo, bool bIncludeOpen = true);
	WTSKlineData* extractDayData(WTSKlineSlice* baseKline, uint32_t times, bool bIncludeOpen = true);

protected:
	static uint32_t getPrevMinute(uint32_t curMinute, int period = 1);
};

