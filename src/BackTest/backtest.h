#include "../WtBtPorter/WtBtPorter.h"

#include "../Includes/WTSStruct.h"
#include "../Share/DLLHelper.hpp"
#include "../Share/StdUtils.hpp"

const char* _stdCode;
int cnt;

struct BarStruct
{
public:
	BarStruct()
	{
		memset(this, 0, sizeof(BarStruct));
	}

	uint32_t	date;
	uint32_t	time;
	double		open;		//开
	double		high;		//高
	double		low;		//低
	double		close;		//收
	double		settle;		//结算
	double		money;		//成交金额

	uint32_t	vol;	//成交量
	uint32_t	hold;	//总持
	int32_t		add;	//增仓
};

struct TickStruct
{
	char		exchg[10];
	char		code[32];

	double		price;				//最新价
	double		open;				//开盘价
	double		high;				//最高价
	double		low;				//最低价
	double		settle_price;		//结算价

	double		upper_limit;		//涨停价
	double		lower_limit;		//跌停价

	uint32_t	total_volume;		//总成交量
	uint32_t	volume;				//成交量
	double		total_turnover;		//总成交额
	double		turn_over;			//成交额
	uint32_t	open_interest;		//总持
	int32_t		diff_interest;		//增仓

	uint32_t	trading_date;		//交易日,如20140327
	uint32_t	action_date;		//自然日期,如20140327
	uint32_t	action_time;		//发生时间,精确到毫秒,如105932000

	double		pre_close;			//昨收价
	double		pre_settle;			//昨结算
	int32_t		pre_interest;		//上日总持

	double		bid_prices[10];		//委买价格
	double		ask_prices[10];		//委卖价格
	uint32_t	bid_qty[10];		//委买量
	uint32_t	ask_qty[10];		//委卖量
	TickStruct()
	{
		memset(this, 0, sizeof(TickStruct));
	}
};

enum BSDirectType
{
	Buy	='B',	//买入
	Sell = 'S',	//卖出
	B_Unknown = ' ',	//未知
	Borrow = 'G',	//借入
	Lend = 'F'	//借出
};

enum TransType
{
	T_Unknown = 'U',	//未知类型
	Match = 'M',	//撮合成交
	TCancel = 'C'	//撤单
};

enum OrdDetailType
{
	O_Unknown = 0,	//未知类型
	BestPrice = 'U',	//本方最优
	AnyPrice = '1',	//市价
	LimitPrice = '2'	//限价
};

struct OrdQueStruct
{
	char		exchg[10];
	char		code[32];

	uint32_t	trading_date;		//交易日,如20140327
	uint32_t	action_date;		//自然日期,如20140327
	uint32_t	action_time;		//发生时间,精确到毫秒,如105932000

	BSDirectType	side;			//委托方向
	double			price;			//委托价格
	uint32_t		order_items;	//订单个数
	uint32_t		qsize;			//队列长度
	uint32_t		volumes[50];	//委托明细

	OrdQueStruct()
	{
		memset(this, 0, sizeof(OrdQueStruct));
	}
};

struct OrdDtlStruct
{
	char		exchg[10];
	char		code[32];

	uint32_t	trading_date;		//交易日,如20140327
	uint32_t	action_date;		//自然日期,如20140327
	uint32_t	action_time;		//发生时间,精确到毫秒,如105932000

	uint32_t		index;			//委托编号(从1开始,递增1)
	BSDirectType	side;			//委托方向
	double			price;			//委托价格
	uint32_t		volume;			//委托数量
	OrdDetailType	otype;		//委托类型

	OrdDtlStruct()
	{
		memset(this, 0, sizeof(OrdDtlStruct));
	}
};

struct TransStruct
{
	char		exchg[10];
	char		code[32];

	uint32_t	trading_date;		//交易日,如20140327
	uint32_t	action_date;		//自然日期,如20140327
	uint32_t	action_time;		//发生时间,精确到毫秒,如105932000

	uint32_t		index;			//成交编号(从1开始,递增1)
	TransType	ttype;			//成交类型: 'C', 0
	BSDirectType	side;			//BS标志

	double			price;			//成交价格
	uint32_t		volume;			//成交数量
	int32_t			askorder;		//叫卖序号
	int32_t			bidorder;		//叫买序号

	TransStruct()
	{
		memset(this, 0, sizeof(TransStruct));
	}
};

void GetBar(const char* code, const char* period, BarStruct* bar, unsigned long count, bool isLast);

void GetTick(const char* code, TickStruct* tick, unsigned long count, bool isLast);

void Tick(const char* code, TickStruct* tick);

void Calc(unsigned long curDate, unsigned long curTime);

void Calc_Done(unsigned long curDate, unsigned long curTime);

void Bar(const char* code, const char* period, BarStruct* bar);

void Session_Event(unsigned long curTDate, bool isBegin);

void CbChnl(const char* trader, unsigned long id);

void CbOrd(unsigned long localid, const char* stdCode, bool isBuy, double totalQty, double leftQty, double price, bool isCanceled, const char* userTag);

void CbTrd(unsigned long localid, const char* stdCode, bool isBuy, double vol, double price, const char* userTag);

void CbEntrust(unsigned long localid, const char* stdCode, bool bSuccess, const char* message, const char* userTag);

void CbOrdDtl(const char* stdCode, OrdDtlStruct* ordDtl);

void CbOrdQue(const char* stdCode, OrdQueStruct* ordQue);

void CbTrans(const char* stdCode, TransStruct* trans);

bool Hft_Cancel(unsigned long localid)
{
	unsigned long cHandle = 0;
	return hft_cancel(cHandle, localid);
}

std::string Hft_Cancel_All(const char* stdCode, bool isBuy)
{
	unsigned long cHandle = 0;
	return hft_cancel_all(cHandle, stdCode, isBuy);
}

std::string Hft_Buy(const char* stdCode, double price, double qty, const char* userTag)
{
	unsigned long cHandle = 0;
	return hft_buy(cHandle, stdCode, price, qty, userTag);
}

std::string Hft_Sell(const char* stdCode, double price, double qty, const char* userTag)
{
	unsigned long cHandle = 0;
	return hft_sell(cHandle, stdCode, price, qty, userTag);
}

BarStruct* BarStrTras(BarStruct* bar,WTSBarStruct* wtbar)
{
	bar->date = wtbar->date;
	bar->time = wtbar->time;
	bar->open = wtbar->open;
	bar->high = wtbar->high;
	bar->low = wtbar->low;
	bar->close = wtbar->close;
	bar->settle = wtbar->settle;
	bar->money = wtbar->money;
	bar->vol = wtbar->vol;
	bar->hold = wtbar->hold;
	bar->add = wtbar->add;
	return bar;
}

TickStruct* TickStrTras(TickStruct* tick, WTSTickStruct* wttick)
{
	tick->exchg[0] = wttick->exchg[0];
	tick->code[0] = wttick->code[0];
	tick->price = wttick->price;
	tick->open = wttick->open;
	tick->high = wttick->high;
	tick->low = wttick->low;
	tick->settle_price = wttick->settle_price;
	tick->upper_limit = wttick->upper_limit;
	tick->lower_limit = wttick->lower_limit;
	tick->total_volume = wttick->total_volume;
	tick->volume = wttick->volume;
	tick->total_turnover = wttick->total_turnover;
	tick->turn_over = wttick->turn_over;
	tick->open_interest = wttick->open_interest;
	tick->diff_interest = wttick->diff_interest;
	tick->trading_date = wttick->trading_date;
	tick->action_date = wttick->action_date;
	tick->action_time = wttick->action_time;
	tick->pre_close = wttick->pre_close;
	tick->pre_settle = wttick->pre_settle;
	tick->pre_interest = wttick->pre_interest;
	for (int i=0;i<sizeof(wttick->bid_prices)/sizeof(double);i++)
	{
		tick->bid_prices[i] = wttick->bid_prices[i];
		tick->ask_prices[i] = wttick->ask_prices[i];
		tick->bid_qty[i] = wttick->bid_qty[i];
		tick->ask_qty[i] = wttick->ask_qty[i];
	}
	return tick;
}

OrdDtlStruct* OrdStrTrans(OrdDtlStruct* ordstr, WTSOrdDtlStruct* wtordstr)
{
	strcpy(ordstr->exchg, wtordstr->exchg);
	strcpy(ordstr->code, wtordstr->code);
	ordstr->trading_date = wtordstr->trading_date;
	ordstr->action_date = wtordstr->action_date;
	ordstr->action_time = wtordstr->action_time;
	ordstr->index = wtordstr->index;
	
	switch (wtordstr->side)
	{
	case BDT_Buy:
		ordstr->side = Buy;
		break;
	case BDT_Sell:
		ordstr->side = Sell;
		break;
	case BDT_Borrow:
		ordstr->side = Borrow;
		break;
	case BDT_Lend:
		ordstr->side = Lend;
		break;
	default:
		ordstr->side = B_Unknown;
		break;
	}
	ordstr->price = wtordstr->price;
	ordstr->volume = wtordstr->volume;
	switch (wtordstr->otype)
	{
	case ODT_BestPrice:
		ordstr->otype = BestPrice;
		break;
	case ODT_AnyPrice:
		ordstr->otype = AnyPrice;
		break;
	case ODT_LimitPrice:
		ordstr->otype = LimitPrice;
		break;
	default:
		ordstr->otype = O_Unknown;
		break;
	}
	return ordstr;
}

OrdQueStruct* OrdQueSTras(OrdQueStruct* ordstr, WTSOrdQueStruct* wtordstr)
{
	strcpy(ordstr->exchg, wtordstr->exchg);
	strcpy(ordstr->code, wtordstr->code);
	ordstr->trading_date = wtordstr->trading_date;
	ordstr->action_date = wtordstr->action_date;
	ordstr->action_time = wtordstr->action_time;
	switch (wtordstr->side)
	{
	case BDT_Buy:
		ordstr->side = Buy;
		break;
	case BDT_Sell:
		ordstr->side = Sell;
		break;
	case BDT_Borrow:
		ordstr->side = Borrow;
		break;
	case BDT_Lend:
		ordstr->side = Lend;
		break;
	default:
		ordstr->side = B_Unknown;
		break;
	}
	ordstr->price = wtordstr->price;
	ordstr->order_items = wtordstr->order_items;
	ordstr->qsize = wtordstr->qsize;
	for (int i=0;i<sizeof(wtordstr->volumes)/sizeof(uint32_t);i++)
	{
		ordstr->volumes[i] = wtordstr->volumes[i];
	}
	return ordstr;
}

TransStruct* TranstrTras(TransStruct* trans, WTSTransStruct* wttrans)
{
	strcpy(trans->exchg, wttrans->exchg);
	strcpy(trans->code, wttrans->code);
	trans->trading_date = wttrans->trading_date;
	trans->action_date = wttrans->action_date;
	trans->action_time = wttrans->action_time;
	trans->index = wttrans->index;
	switch (wttrans->ttype)
	{
	case TT_Match:
		trans->ttype = Match;
		break;
	case TT_Cancel:
		trans->ttype = TCancel;
		break;
	default:
		trans->ttype = T_Unknown;
		break;
	}
	switch (wttrans->side)
	{
	case BDT_Buy:
		trans->side = Buy;
		break;
	case BDT_Sell:
		trans->side = Sell;
		break;
	case BDT_Borrow:
		trans->side = Borrow;
		break;
	case BDT_Lend:
		trans->side = Lend;
		break;
	default:
		trans->side = B_Unknown;
		break;
	}
	trans->price = wttrans->price;
	trans->volume = wttrans->volume;
	trans->askorder = wttrans->askorder;
	trans->bidorder = wttrans->bidorder;
}

void on_getbar(CtxHandler ctxid, const char* code, const char* period, WTSBarStruct* bar, WtUInt32 count, bool isLast)
{
	BarStruct barstruct;
	BarStrTras(&barstruct, bar);
	GetBar(code, period, &barstruct, count, isLast);
}

void on_gettick(CtxHandler ctxid, const char* code, WTSTickStruct* tick, WtUInt32 count, bool isLast)
{
	TickStruct tickstruct;
	TickStrTras(&tickstruct, tick);
	GetTick(code, &tickstruct, count, isLast);
}

void on_init(CtxHandler ctxid)
{
	hft_get_bars(ctxid, _stdCode, "m1", cnt, on_getbar);
	hft_sub_ticks(ctxid, _stdCode);
}

void on_tick(CtxHandler ctxid, const char* stdCode, WTSTickStruct* newTick)
{
	TickStruct tickstruct;
	TickStrTras(&tickstruct, newTick);
	Tick(stdCode, &tickstruct);
}

void on_calc(CtxHandler ctxid, WtUInt32 curDate, WtUInt32 curTime)
{
	Calc(curDate, curTime);
}

void on_calc_done(CtxHandler ctxid, WtUInt32 curDate, WtUInt32 curTime)
{
	//printf("on_calc_done @ %u.%u\r\n", curDate, curTime);
}

void on_bar(CtxHandler ctxid, const char* code, const char* period, WTSBarStruct* newBar)
{
	BarStruct barstruct;
	BarStrTras(&barstruct, newBar);
	Bar(code, period, &barstruct);
}

void on_session_event(CtxHandler cHandle, WtUInt32 curTDate, bool isBegin)
{
	Session_Event(curTDate, isBegin);
}

void cbChnl(CtxHandler cHandle, const char* trader, WtUInt32 evtid)
{
	CbChnl(trader, evtid);
}

void cbOrd(CtxHandler cHandle, WtUInt32 localid, const char* stdCode, bool isBuy, double totalQty, double leftQty, double price, bool isCanceled, const char* userTag)
{
	CbOrd(localid, stdCode, isBuy, totalQty, leftQty, price, isCanceled, userTag);
}

void cbTrd(CtxHandler cHandle, WtUInt32 localid, const char* stdCode, bool isBuy, double vol, double price, const char* userTag)
{
	CbTrd(localid, stdCode, isBuy, vol, price, userTag);
}

void cbEntrust(CtxHandler cHandle, WtUInt32 localid, const char* stdCode, bool bSuccess, const char* message, const char* userTag)
{
	CbEntrust(localid, stdCode, bSuccess, message, userTag);
}

void cbOrdDtl(CtxHandler cHandle, const char* stdCode, WTSOrdDtlStruct* ordDtl)
{
	OrdDtlStruct orddtstr;
	OrdStrTrans(&orddtstr, ordDtl);
	CbOrdDtl(stdCode, &orddtstr);
}

void cbOrdQue(CtxHandler cHandle, const char* stdCode, WTSOrdQueStruct* ordQue)
{
	OrdQueStruct ordquedtstr;
	OrdQueSTras(&ordquedtstr, ordQue);
	CbOrdQue(stdCode, &ordquedtstr);
}

void cbTrans(CtxHandler cHandle, const char* stdCode, WTSTransStruct* trans)
{
	TransStruct transtruct;
	TranstrTras(&transtruct, trans);
	CbTrans(stdCode, &transtruct);
}

void init(const char* stdcode,int count)
{
#ifdef _WIN32
	DLLHelper::load_library("WtBtPorter.dll");
#else
	DLLHelper::load_library("libWtBtPorter.so");
#endif
	std::string code = stdcode;
	code += ".HOT";
	_stdCode = code.c_str();
	cnt = count;
	register_hft_callbacks(on_init, on_tick, on_bar, cbChnl, cbOrd, cbTrd, cbEntrust, cbOrdDtl, cbOrdQue, cbTrans, on_session_event);

	auto id = init_hft_mocker("test", true);

	init_backtest("logcfg.json", true);

	config_backtest("config.json", true);

	run_backtest(true, false);//

	for (int i = 0; i < 20; i++)
	{
		printf("%d\r\n", i);
		hft_step(id);

		//if (i == 5) { stop_backtest(); }

	}
	/*printf("press enter key to exit\n");
	getchar();*/
	release_backtest();
}