/*!
 * \file HftStraContext.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#pragma once
#include "UftStraBaseCtx.h"


USING_NS_WTP;

class UftStrategy;

class UftStraContext : public UftStraBaseCtx
{
public:
	UftStraContext(WtUftEngine* engine, const char* name);
	virtual ~UftStraContext();

	void set_strategy(UftStrategy* stra){ _strategy = stra; }
	UftStrategy* get_stragety() { return _strategy; }

public:
	virtual void on_init() override;

	virtual void on_tick(const char* code, WTSTickData* newTick) override;

	virtual void on_order_queue(const char* stdCode, WTSOrdQueData* newOrdQue) override;

	virtual void on_order_detail(const char* stdCode, WTSOrdDtlData* newOrdDtl) override;

	virtual void on_transaction(const char* stdCode, WTSTransData* newTrans) override;

	virtual void on_bar(const char* code, const char* period, uint32_t times, WTSBarStruct* newBar) override;

	virtual void on_trade(uint32_t localid, const char* stdCode, bool isLong, uint32_t offset, double vol, double price) override;

	virtual void on_order(uint32_t localid, const char* stdCode, bool isLong, uint32_t offset, double totalQty, double leftQty, double price, bool isCanceled = false) override;

	virtual void on_channel_ready() override;

	virtual void on_channel_lost() override;

	virtual void on_entrust(uint32_t localid, const char* stdCode, bool bSuccess, const char* message) override;

	virtual void on_position(const char* stdCode, bool isLong, double prevol, double preavail, double newvol, double newavail, uint32_t tradingday) override;

	virtual void on_session_begin(uint32_t uTDate) override;
	virtual void on_session_end(uint32_t uTDate) override;


private:
	UftStrategy*		_strategy;
};

