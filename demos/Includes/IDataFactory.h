/*!
 * \file IDataFactory.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief ����ƴ�ӹ����ӿڶ���
 */
#pragma once
#include <stdint.h>
#include "../Includes/WTSTypes.h"

//USING_NS_OTP;

NS_OTP_BEGIN
class WTSKlineData;
class WTSHisTrendData;
class WTSTickData;
class WTSSessionInfo;
class WTSKlineSlice;
class WTSContractInfo;
struct WTSBarStruct;
struct WTSTickStruct;
class WTSTickSlice;

/*
 *	���ݹ���
 *	��Ҫ���ڸ������ݵ�ƴ��
 *	����ֻ����һ���ӿ�
 */
class IDataFactory
{
public:
	/*
	 *	����tick���ݸ���K��
	 *	@klineData	K������
	 *	@tick		tick����
	 *	@sInfo		����ʱ��ģ��
	 */
	virtual WTSBarStruct*	updateKlineData(WTSKlineData* klineData, WTSTickData* tick, WTSSessionInfo* sInfo)						= 0;

	/*
	 *	���û�������K�����ݸ���K��
	 *	@klineData		K������
	 *	@newBasicBar	��������K������
	 *	@sInfo			����ʱ��ģ��
	 */
	virtual WTSBarStruct*	updateKlineData(WTSKlineData* klineData, WTSBarStruct* newBasicBar, WTSSessionInfo* sInfo)				= 0;

	/*
	 *	�ӻ�������K��������ȡ�ǻ������ڵ�K������
	 *	@baseKline	��������K��
	 *	@period		�������ڣ�m1/m5/day
	 *	@times		���ڱ���
	 *	@sInfo		����ʱ��ģ��
	 *	@bIncludeOpen	�Ƿ����δ�պϵ�K��
	 */
	virtual WTSKlineData*	extractKlineData(WTSKlineSlice* baseKline, WTSKlinePeriod period, uint32_t times, WTSSessionInfo* sInfo, bool bIncludeOpen = true) = 0;

	/*
	 *	��tick������ȡ�����ڵ�K������
	 *	@ayTicks	tick����
	 *	@seconds	Ŀ������
	 *	@sInfo		����ʱ��ģ��
	 *	@bUnixTime	tickʱ����Ƿ���unixtime
	 */
	virtual WTSKlineData*	extractKlineData(WTSTickSlice* ayTicks, uint32_t seconds, WTSSessionInfo* sInfo, bool bUnixTime = false) = 0;

	/*
	 *	�ϲ�K��
	 *	@klineData	Ŀ��K��
	 *	@newKline	���ϲ���K��
	 */
	virtual bool			mergeKlineData(WTSKlineData* klineData, WTSKlineData* newKline)											= 0;
};

NS_OTP_END
