/*!
 * \file HftStrategyDefs.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#pragma once
#include <string>
#include <stdint.h>

#include "../Includes/WTSMarcos.h"

NS_WTP_BEGIN
class WTSVariant;
class IHftStraCtx;
class WTSTickData;
class WTSOrdDtlData;
class WTSOrdQueData;
class WTSTransData;
struct WTSBarStruct;
NS_WTP_END

USING_NS_WTP;

class HftStrategy
{
public:
	HftStrategy(const char* id) :_id(id){}
	virtual ~HftStrategy(){}

public:
	/*
	 *	ִ�е�Ԫ����
	 */
	virtual const char* getName() = 0;

	/*
	 *	����ִ������������
	 */
	virtual const char* getFactName() = 0;

	/*
	 *	��ʼ��
	 */
	virtual bool init(WTSVariant* cfg){ return true; }

	virtual const char* id() const { return _id.c_str(); }

	//�ص�����
	virtual void on_init(IHftStraCtx* ctx) = 0;
	virtual void on_session_begin(IHftStraCtx* ctx, uint32_t uTDate) {}
	virtual void on_session_end(IHftStraCtx* ctx, uint32_t uTDate) {}

	virtual void on_tick(IHftStraCtx* ctx, const char* code, WTSTickData* newTick) {}
	virtual void on_order_queue(IHftStraCtx* ctx, const char* code, WTSOrdQueData* newOrdQue) {}
	virtual void on_order_detail (IHftStraCtx* ctx, const char* code, WTSOrdDtlData* newOrdDtl) {}
	virtual void on_transaction(IHftStraCtx* ctx, const char* code, WTSTransData* newTrans) {}
	virtual void on_bar(IHftStraCtx* ctx, const char* code, const char* period, uint32_t times, WTSBarStruct* newBar) {}

	virtual void on_trade(IHftStraCtx* ctx, uint32_t localid, const char* stdCode, bool isBuy, double vol, double price, const char* userTag) {}
	virtual void on_position(IHftStraCtx* ctx, const char* stdCode, bool isLong, double prevol, double preavail, double newvol, double newavail) {}
	virtual void on_order(IHftStraCtx* ctx, uint32_t localid, const char* stdCode, bool isBuy, double totalQty, double leftQty, double price, bool isCanceled, const char* userTag) {}
	virtual void on_channel_ready(IHftStraCtx* ctx) {}
	virtual void on_channel_lost(IHftStraCtx* ctx) {}
	virtual void on_entrust(uint32_t localid, bool bSuccess, const char* message, const char* userTag) {}

protected:
	std::string _id;
};

//////////////////////////////////////////////////////////////////////////
//���Թ����ӿ�
typedef void(*FuncEnumHftStrategyCallback)(const char* factName, const char* straName, bool isLast);

class IHftStrategyFact
{
public:
	IHftStrategyFact(){}
	virtual ~IHftStrategyFact(){}

public:
	/*
	 *	��ȡ������
	 */
	virtual const char* getName() = 0;

	/*
	 *	ö�ٲ���
	 */
	virtual void enumStrategy(FuncEnumHftStrategyCallback cb) = 0;

	/*
	 *	�������ƴ���ִ�е�Ԫ
	 */
	virtual HftStrategy* createStrategy(const char* name, const char* id) = 0;

	/*
	 *	ɾ��ִ�е�Ԫ
	 */
	virtual bool deleteStrategy(HftStrategy* stra) = 0;
};

//����ִ�й���
typedef IHftStrategyFact* (*FuncCreateHftStraFact)();
//ɾ��ִ�й���
typedef void(*FuncDeleteHftStraFact)(IHftStrategyFact* &fact);
