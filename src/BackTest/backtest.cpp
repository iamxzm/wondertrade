#include "backtest.h"

void GetBar(const char* code, const char* period, BarStruct* bar, unsigned long, bool isLast)
{

}

void GetTick(const char* code, TickStruct* tick, unsigned long count, bool isLast)
{

}

void Tick(const char* code, TickStruct* tick)
{

}

void Calc(unsigned long curDate, unsigned long curTime)
{

}

void Calc_Done(unsigned long curDate, unsigned long curTime)
{

}

void Bar(const char* code, const char* period, BarStruct* bar)
{

}

void Session_Event(unsigned long curTDate, bool isBegin)
{

}

void CbChnl(const char* trader, unsigned long id)
{

}

void CbOrd(unsigned long localid, const char* stdCode, bool isBuy, double totalQty, double leftQty, double price, bool isCanceled, const char* userTag)
{

}

void CbTrd(unsigned long localid, const char* stdCode, bool isBuy, double vol, double price, const char* userTag)
{

}

void CbEntrust(unsigned long localid, const char* stdCode, bool bSuccess, const char* message, const char* userTag)
{

}

void CbOrdDtl(const char* stdCode, OrdDtlStruct* ordDtl)
{

}

void CbOrdQue(const char* stdCode, OrdQueStruct* ordQue)
{

}

void CbTrans(const char* stdCode, TransStruct* trans)
{

}

int main()
{
	init();
	return 0;
}