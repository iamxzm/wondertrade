/*!
 * \file IUftStraCtx.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#pragma once
#include <stdint.h>
#include <string>
#include "ExecuteDefs.h"

#include "../Includes/WTSMarcos.h"

NS_WTP_BEGIN
class WTSCommodityInfo;
class WTSTickSlice;
class WTSKlineSlice;
class WTSTickData;
struct WTSBarStruct;

/*
 *	�������
 */
static const int UFT_OrderFlag_Nor = 0;
static const int UFT_OrderFlag_FAK = 1;
static const int UFT_OrderFlag_FOK = 2;

class IUftStraCtx
{
public:
	IUftStraCtx(const char* name) :_name(name) {}
	virtual ~IUftStraCtx() {}

	const char* name() const { return _name.c_str(); }

public:
	virtual uint32_t id() = 0;

	//�ص�����
	virtual void on_init() = 0;
	virtual void on_tick(const char* stdCode, WTSTickData* newTick) = 0;
	virtual void on_order_queue(const char* stdCode, WTSOrdQueData* newOrdQue) = 0;
	virtual void on_order_detail(const char* stdCode, WTSOrdDtlData* newOrdDtl) = 0;
	virtual void on_transaction(const char* stdCode, WTSTransData* newTrans) = 0;
	virtual void on_bar(const char* stdCode, const char* period, uint32_t times, WTSBarStruct* newBar) {}
	virtual void on_session_begin(uint32_t uTDate) {}
	virtual void on_session_end(uint32_t uTDate) {}

	/*
	 *	�ز�����¼�
	 *	ֻ�ڻز��²Żᴥ��
	 */
	virtual void	on_bactest_end() {};

	virtual void	on_tick_updated(const char* stdCode, WTSTickData* newTick) {}
	virtual void	on_ordque_updated(const char* stdCode, WTSOrdQueData* newOrdQue) {}
	virtual void	on_orddtl_updated(const char* stdCode, WTSOrdDtlData* newOrdDtl) {}
	virtual void	on_trans_updated(const char* stdCode, WTSTransData* newTrans) {}

	//���Խӿ�

	virtual uint32_t	stra_get_date() = 0;
	virtual uint32_t	stra_get_time() = 0;
	virtual uint32_t	stra_get_secs() = 0;

	/*
	 *	�����ӿ�
	 *	@localid	���ص���
	 */
	virtual bool		stra_cancel(uint32_t localid) = 0;
	
	/*
	 *	һ�������ӿ�
	 *	@stdCode	��Լ����
	 */
	virtual OrderIDs	stra_cancel_all(const char* stdCode) = 0;

	/*
	 *	�µ��ӿ�: ����
	 *
	 *	@stdCode	��Լ����
	 *	@price		�µ��۸�0�����м۵�
	 *	@qty		�µ�����
	 *	@flag		�µ���־: 0-normal��1-fak��2-fok��Ĭ��0
	 */
	virtual OrderIDs	stra_buy(const char* stdCode, double price, double qty, int flag = 0) { return OrderIDs(); }

	/*
	 *	�µ��ӿ�: ����
	 *
	 *	@stdCode	��Լ����
	 *	@price		�µ��۸�0�����м۵�
	 *	@qty		�µ�����
	 *	@flag		�µ���־: 0-normal��1-fak��2-fok��Ĭ��0
	 */
	virtual OrderIDs	stra_sell(const char* stdCode, double price, double qty, int flag = 0) { return OrderIDs(); }

	/*
	 *	����
	 *	@stdCode	���룬��ʽ��SSE.600000
	 *	@price		ί�м۸�
	 *	@qty		�µ�����
	 *	@flag		�µ���־: 0-normal��1-fak��2-fok��Ĭ��0
	 */
	virtual uint32_t	stra_enter_long(const char* stdCode, double price, double qty, int flag = 0) { return 0; }

	/*
	 *	����
	 *	@stdCode	���룬��ʽ��SSE.600000
	 *	@price		ί�м۸�
	 *	@qty		�µ�����
	 *	@flag		�µ���־: 0-normal��1-fak��2-fok��Ĭ��0
	 */
	virtual uint32_t	stra_enter_short(const char* stdCode, double price, double qty, int flag = 0) { return 0; }

	/*
	 *	ƽ��
	 *	@stdCode	���룬��ʽ��SSE.600000
	 *	@price		ί�м۸�
	 *	@qty		�µ�����
	 *	@isToday	�Ƿ��֣�SHFE��INEר��
	 *	@flag		�µ���־: 0-normal��1-fak��2-fok��Ĭ��0
	 */
	virtual uint32_t	stra_exit_long(const char* stdCode, double price, double qty, bool isToday = false, int flag = 0) { return 0; }

	/*
	 *	ƽ��
	 *	@stdCode	���룬��ʽ��SSE.600000
	 *	@price		ί�м۸�
	 *	@qty		�µ�����
	 *	@isToday	�Ƿ��֣�SHFE��INEר��
	 *	@flag		�µ���־: 0-normal��1-fak��2-fok��Ĭ��0
	 */
	virtual uint32_t	stra_exit_short(const char* stdCode, double price, double qty, bool isToday = false, int flag = 0) { return 0; }

	/*
	 *	��ȡƷ����Ϣ
	 *	@stdCode	���룬��ʽ��SSE.600000
	 */
	virtual WTSCommodityInfo* stra_get_comminfo(const char* stdCode) = 0;

	/*
	 *	��ȡK�ߣ���δʵ��
	 *	@stdCode	���룬��ʽ��SSE.600000
	 *	@period		���ڣ���m1/m5/d1
	 *	@count		����
	 */
	virtual WTSKlineSlice*	stra_get_bars(const char* stdCode, const char* period, uint32_t count) = 0;

	/*
	 *	��ȡtick����δʵ��
	 *	@stdCode	���룬��ʽ��SSE.600000
	 *	@count		����
	 */
	virtual WTSTickSlice*	stra_get_ticks(const char* stdCode, uint32_t count) = 0;

	/*
	 *	��ȡί����ϸ����δʵ��
	 *	@stdCode	���룬��ʽ��SSE.600000
	 *	@count		����
	 */
	virtual WTSOrdDtlSlice*	stra_get_order_detail(const char* stdCode, uint32_t count) = 0;

	/*
	 *	��ȡί�ж��У���δʵ��
	 *	@stdCode	���룬��ʽ��SSE.600000
	 *	@count		����
	 */
	virtual WTSOrdQueSlice*	stra_get_order_queue(const char* stdCode, uint32_t count) = 0;

	/*
	 *	��ȡ��ʳɽ�����δʵ��
	 *	@stdCode	���룬��ʽ��SSE.600000
	 *	@count		����
	 */
	virtual WTSTransSlice*	stra_get_transaction(const char* stdCode, uint32_t count) = 0;

	/*
	 *	��ȡ���һ��tick
	 *	@stdCode	���룬��ʽ��SSE.600000
	 */
	virtual WTSTickData*	stra_get_last_tick(const char* stdCode) = 0;

	/*
	 *	��ȡ�ֲ�
	 *	@stdCode	���룬��ʽ��SSE.600000
	 *	@bOnlyValid	��ȡ���óֲ�
	 *	@iFlag		��ȡ��ǣ�1-��ͷ��2-��ͷ��3-��ͷ��
	 */
	virtual double stra_get_position(const char* stdCode, bool bOnlyValid = false, int32_t iFlag = 3) = 0;

	/*
	 *	ö�ٳֲ֣���ͨ��on_position�ص�����
	 *	@stdCode	���룬��ʽ��SSE.600000��������գ���ö��ȫ����
	 *	
	 */
	virtual double stra_enum_position(const char* stdCode) = 0;

	/*
	 *	��ȡ���¼۸�
	 *	@stdCode	���룬��ʽ��SSE.600000
	 */
	virtual double stra_get_price(const char* stdCode) = 0;

	/*
	 *	��ȡδ�������������Ϊ��������Ϊ��
	 *	@stdCode	���룬��ʽ��SSE.600000
	 */
	virtual double stra_get_undone(const char* stdCode) = 0;

	/*
	 *	���Ľӿ�
	 */
	virtual void stra_sub_ticks(const char* stdCode) = 0;
	virtual void stra_sub_order_queues(const char* stdCode) = 0;
	virtual void stra_sub_order_details(const char* stdCode) = 0;
	virtual void stra_sub_transactions(const char* stdCode) = 0;

	/*
	 *	�����־
	 */
	virtual void stra_log_info(const char* message) = 0;
	virtual void stra_log_debug(const char* message) = 0;
	virtual void stra_log_error(const char* message) = 0;

protected:
	std::string _name;
};

NS_WTP_END