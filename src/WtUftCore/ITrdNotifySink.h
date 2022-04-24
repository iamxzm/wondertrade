/*!
 * \file ITrdNotifySink.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#pragma once
#include <stdint.h>
#include "../Includes/WTSMarcos.h"

NS_WTP_BEGIN

class ITrdNotifySink
{
public:
	/*
	 *	�ɽ��ر�
	 */
	virtual void on_trade(uint32_t localid, const char* stdCode, bool isLong, uint32_t offset, double vol, double price) = 0;

	/*
	 *	�����ر�
	 */
	virtual void on_order(uint32_t localid, const char* stdCode, bool isLong, uint32_t offset, double totalQty, double leftQty, double price, bool isCanceled = false) = 0;

	/*
	 *	�ֲָ��»ص�
	 */
	virtual void on_position(const char* stdCode, bool isLong, double prevol, double preavail, double newvol, double newavail, uint32_t tradingday) {}

	/*
	 *	����ͨ������
	 */
	virtual void on_channel_ready() = 0;

	/*
	 *	����ͨ����ʧ
	 */
	virtual void on_channel_lost() = 0;

	/*
	 *	�µ��ر�
	 */
	virtual void on_entrust(uint32_t localid, const char* stdCode, bool bSuccess, const char* message){}
};

NS_WTP_END