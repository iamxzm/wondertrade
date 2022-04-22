#include "WtStraDualThrust.h"

#include "../Includes/ICtaStraCtx.h"

#include "../Includes/WTSContractInfo.hpp"
#include "../Includes/WTSVariant.hpp"
#include "../Includes/WTSDataDef.hpp"
#include "../Share/decimal.h"

extern const char* FACT_NAME;

WtStraDualThrust::WtStraDualThrust(const char* id)
	: CtaStrategy(id)
{
}


WtStraDualThrust::~WtStraDualThrust()
{
}

const char* WtStraDualThrust::getFactName()
{
	return FACT_NAME;
}

const char* WtStraDualThrust::getName()
{
	return "DualThrust";
}

bool WtStraDualThrust::init(WTSVariant* cfg)
{
	if (cfg == NULL)
		return false;

	_days = cfg->getUInt32("days");
	_k1 = cfg->getDouble("k1");
	_k2 = cfg->getDouble("k2");

	_period = cfg->getCString("period");
	_count = cfg->getUInt32("count");
	_code = cfg->getCString("code");

	_isstk = cfg->getBoolean("stock");

	return true;
}

void WtStraDualThrust::on_schedule(ICtaStraCtx* ctx, uint32_t curDate, uint32_t curTime)
{
	std::string code = _code;
	if (_isstk)
		code += "Q";
	WTSKlineSlice *kline = ctx->stra_get_bars(code.c_str(), _period.c_str(), _count, true);
	if(kline == NULL)
	{
		//����������һЩ��־
		return;
	}

	if (kline->size() == 0)
	{
		kline->release();
		return;
	}

	uint32_t trdUnit = 1;
	if (_isstk)
		trdUnit = 100;


	int32_t days = (int32_t)_days;

	double hh = kline->maxprice(-days, -2);
	double ll = kline->minprice(-days, -2);

	WTSValueArray* closes = kline->extractData(KFT_CLOSE);
	double hc = closes->maxvalue(-days, -2);
	double lc = closes->minvalue(-days, -2);
	double curPx = closes->at(-1);
	closes->release();///!!!����ͷ�һ��Ҫ��

	double openPx = kline->open(-1);
	double highPx = kline->high(-1);
	double lowPx = kline->low(-1);

	double upper_bound = openPx + _k1 * (max(hh - lc, hc - ll));
	double lower_bound = openPx - _k2 * max(hh - lc, hc - ll);

	WTSCommodityInfo* commInfo = ctx->stra_get_comminfo(_code.c_str());

	double curPos = ctx->stra_get_position(_code.c_str()) / trdUnit;
	if(decimal::eq(curPos,0))
	{
		if(highPx >= upper_bound)
		{
			ctx->stra_enter_long(_code.c_str(), 2 * trdUnit, "DT_EnterLong");
			//����ͻ��
			ctx->stra_log_info("����ͻ��%.2f>=%.2f,��ֽ���", highPx, upper_bound);
		}
		else if (lowPx <= lower_bound && !_isstk)
		{
			ctx->stra_enter_short(_code.c_str(), 2 * trdUnit, "DT_EnterShort");
			//����ͻ��
			ctx->stra_log_info("����ͻ��%.2f<=%.2f,�ղֽ���", lowPx, lower_bound);
		}
	}
	//else if(curPos > 0)
	else if (decimal::gt(curPos, 0))
	{
		if(lowPx <= lower_bound)
		{
			//��ֳ���
			ctx->stra_exit_long(_code.c_str(), 2 * trdUnit, "DT_ExitLong");
			ctx->stra_log_info("����ͻ��%.2f<=%.2f,��ֳ���", lowPx, lower_bound);
		}
	}
	//else if(curPos < 0)
	else if (decimal::lt(curPos, 0))
	{
		if (highPx >= upper_bound && !_isstk)
		{
			//�ղֳ���
			ctx->stra_exit_short(_code.c_str(), 2 * trdUnit, "DT_ExitShort");
			ctx->stra_log_info("����ͻ��%.2f>=%.2f,�ղֳ���", highPx, upper_bound);
		}
	}

	ctx->stra_save_user_data("test", "waht");

	//����ͷ�һ��Ҫ��
	kline->release();
}

void WtStraDualThrust::on_init(ICtaStraCtx* ctx)
{
	std::string code = _code;
	if (_isstk)
		code += "Q";
	WTSKlineSlice *kline = ctx->stra_get_bars(code.c_str(), _period.c_str(), _count, true);
	if (kline == NULL)
	{
		//����������һЩ��־
		return;
	}

	kline->release();
}

void WtStraDualThrust::on_tick(ICtaStraCtx* ctx, const char* stdCode, WTSTickData* newTick)
{
	//û��ʲôҪ����
}
