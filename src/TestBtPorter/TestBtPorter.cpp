// TestPorter.cpp : 定义控制台应用程序的入口点。
//
#include "../WtBtPorter/WtBtPorter.h"

#include "../Includes/WTSStruct.h"
#include "../Share/DLLHelper.hpp"
#include "../Share/StdUtils.hpp"

void on_getbar(CtxHandler ctxid, const char* code, const char* period, WTSBarStruct* bar, WtUInt32 count, bool isLast)
{
	if (bar)
		printf("on_getbar@%u\r\n", bar->time);
	else
		int x = 1;
}

void on_gettick(CtxHandler ctxid, const char* code, WTSTickStruct* tick, WtUInt32 count, bool isLast)
{
	if (tick)
		printf("on_gettick@%u\r\n", tick->action_time);
}

void on_init(CtxHandler ctxid)
{
	//cta_get_bars(ctxid, "CFFEX.IF.HOT", "d1", 30, true, on_getbar);
	//cta_get_bars(ctxid, "SHFE.ag.HOT", "m1", 30, true, on_getbar);
	//cta_sub_ticks(ctxid, "SHFE.ag.HOT");
	//cta_get_ticks(ctxid, "SHFE.ag.HOT", 100, on_gettick);
	hft_get_ticks(ctxid, "SHFE.ag.HOT", 100, on_gettick);
	//cta_log_text(ctxid, "this is a test message");
}

void on_tick(CtxHandler ctxid, const char* stdCode, WTSTickStruct* newTick)
{
	printf("on_tick newTick:%u %u %f\r\n",newTick->action_date,newTick->action_time,newTick->price);
}

void on_calc(CtxHandler ctxid, WtUInt32 curDate, WtUInt32 curTime)
{
	printf("on_calc @ %u.%u\r\n", curDate, curTime);
	//cta_get_ticks(ctxid, "CFFEX.IF.HOT", 100, on_gettick);
}

void on_calc_done(CtxHandler ctxid, WtUInt32 curDate, WtUInt32 curTime)
{
	printf("on_calc_done @ %u.%u\r\n", curDate, curTime);
	//cta_get_ticks(ctxid, "CFFEX.IF.HOT", 100, on_gettick);
}


void on_bar(CtxHandler ctxid, const char* code, const char* period, WTSBarStruct* newBar)
{
	printf("on_bar%u\r\n",newBar->time);
}

void on_session_event(CtxHandler cHandle, WtUInt32 curTDate, bool isBegin)
{

}

void cbChnl(CtxHandler cHandle, const char* trader, WtUInt32 evtid)
{
}

void cbOrd(CtxHandler cHandle, WtUInt32 localid, const char* stdCode, bool isBuy, double totalQty, double leftQty, double price, bool isCanceled, const char* userTag)
{
}

void cbTrd(CtxHandler cHandle, WtUInt32 localid, const char* stdCode, bool isBuy, double vol, double price, const char* userTag)
{}

void cbEntrust(CtxHandler cHandle, WtUInt32 localid, const char* stdCode, bool bSuccess, const char* message, const char* userTag)
{}

void cbOrdDtl(CtxHandler cHandle, const char* stdCode, WTSOrdDtlStruct* ordDtl)
{}

void cbOrdQue(CtxHandler cHandle, const char* stdCode, WTSOrdQueStruct* ordQue)
{}

void cbTrans(CtxHandler cHandle, const char* stdCode, WTSTransStruct* trans)
{}

void run_bt()
{
#ifdef _WIN32
	DLLHelper::load_library("WtBtPorter.dll");
#else
	DLLHelper::load_library("libWtBtPorter.so");
#endif
	//register_cta_callbacks(on_init, on_tick, on_calc, on_bar, on_session_event, on_calc_done);
	register_hft_callbacks(on_init, on_tick, on_bar, cbChnl, cbOrd, cbTrd, cbEntrust, cbOrdDtl, cbOrdQue, cbTrans, on_session_event);

	//auto id = init_cta_mocker("test", 0, true);
	auto id = init_hft_mocker("test", true);

	init_backtest("logcfg.json", true);

	config_backtest("config.json", true);

	run_backtest(true, true);

	for(int i = 0; i < 20; i++)
	{
		printf("%d\r\n", i);
		//cta_step(id);
		hft_step(id);

		//if (i == 5) { stop_backtest(); }
			
	}

	printf("press enter key to exit\n");
	getchar();
	release_backtest();
}


int main()
{
	run_bt();
	return 0;
}

