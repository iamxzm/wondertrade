/*!
 * \file WtSimpExeUnit.cpp
 *
 * \author Wesley
 * \date 2020/03/30
 *
 * 
 */
#include "WtSimpExeUnit.h"

#include "../Includes/WTSVariant.hpp"
#include "../Includes/WTSContractInfo.hpp"
#include "../Includes/WTSSessionInfo.hpp"
#include "../Share/decimal.h"

extern const char* FACT_NAME;


WtSimpExeUnit::WtSimpExeUnit()
	: _last_tick(NULL)
	, _comm_info(NULL)
	, _price_mode(0)
	, _price_offset(0)
	, _expire_secs(0)
	, _cancel_cnt(0)
	, _target_pos(0)
	, _unsent_qty(0)
	, _cancel_times(0)
	, _last_tick_time(0)
{
}


WtSimpExeUnit::~WtSimpExeUnit()
{
	if (_last_tick)
		_last_tick->release();

	if (_comm_info)
		_comm_info->release();
}

const char* WtSimpExeUnit::getFactName()
{
	return FACT_NAME;
}

const char* WtSimpExeUnit::getName()
{
	return "WtSimpExeUnit";
}

const char* PriceModeNames[] = 
{
	"BESTPX",		//���ż�
	"LASTPX",		//���¼�
	"MARKET",		//���ּ�
	"AUTOPX"		//�Զ�
};
void WtSimpExeUnit::init(ExecuteContext* ctx, const char* stdCode, WTSVariant* cfg)
{
	ExecuteUnit::init(ctx, stdCode, cfg);

	_comm_info = ctx->getCommodityInfo(stdCode);
	if (_comm_info)
		_comm_info->retain();

	_sess_info = ctx->getSessionInfo(stdCode);
	if (_sess_info)
		_sess_info->retain();

	_price_offset = cfg->getInt32("offset");
	_expire_secs = cfg->getUInt32("expire");
	_price_mode = cfg->getInt32("pricemode");	//�۸�����,0-���¼�,-1-���ż�,1-���ּ�,2-�Զ�,Ĭ��Ϊ0

	ctx->writeLog("ExecUnit %s inited, order price: %s �� %d ticks, order expired in %u secs", stdCode, PriceModeNames[_price_mode+1], _price_offset, _expire_secs);
}

void WtSimpExeUnit::on_order(uint32_t localid, const char* stdCode, bool isBuy, double leftover, double price, bool isCanceled)
{
	{
		if (!_orders_mon.has_order(localid))
			return;

		if (isCanceled || leftover == 0)
		{
			_orders_mon.erase_order(localid);
			if (_cancel_cnt > 0)
				_cancel_cnt--;

			_ctx->writeLog("@ %d cancelcnt -> %u", __LINE__, _cancel_cnt);
		}

		if (leftover == 0 && !isCanceled)
			_cancel_times = 0;
	}

	//����г���,Ҳ�������¼���
	if (isCanceled)
	{
		//_ctx->writeLog("%s�Ķ���%u�ѳ���,���´���ִ���߼�", stdCode, localid);
		_ctx->writeLog("Order %u of %s canceled, recalc will be done", localid, stdCode);
		_cancel_times++;
		doCalculate();
	}
}

void WtSimpExeUnit::on_channel_ready()
{
	double undone = _ctx->getUndoneQty(_code.c_str());

	if(!decimal::eq(undone, 0) && !_orders_mon.has_order())
	{
		//��˵����δ��ɵ����ڼ��֮��,�ȳ���
		//_ctx->writeLog("%s�в��ڹ����е�δ��ɵ� %f ,ȫ������", _code.c_str(), undone);
		_ctx->writeLog("Live orders with qty %f of %s found, cancel all", undone, _code.c_str());

		bool isBuy = (undone > 0);
		OrderIDs ids = _ctx->cancel(_code.c_str(), isBuy);
		_orders_mon.push_order(ids.data(), ids.size(), _ctx->getCurTime());
		_cancel_cnt += ids.size();

		_ctx->writeLog("%s cancelcnt -> %u", __FUNCTION__, _cancel_cnt);
	}


	doCalculate();
}

void WtSimpExeUnit::on_channel_lost()
{
	
}

void WtSimpExeUnit::on_tick(WTSTickData* newTick)
{
	if (newTick == NULL || _code.compare(newTick->code()) != 0)
		return;

	bool isFirstTick = false;
	//���ԭ����tick��Ϊ��,��Ҫ�ͷŵ�
	if (_last_tick)
	{
		_last_tick->release();
	}
	else
	{
		//�������ʱ�䲻�ڽ���ʱ��,�������һ���Ǽ��Ͼ��۵��������,�µ���ʧ��,����ֱ�ӹ��˵��������
		if (_sess_info != NULL && !_sess_info->isInTradingTime(newTick->actiontime() / 100000))
			return;

		isFirstTick = true;
	}

	//�µ�tick����,Ҫ����
	_last_tick = newTick;
	_last_tick->retain();

	/*
	 *	������Կ���һ��
	 *	���д����һ�ζ���ȥ�ĵ��Ӳ����ﵽĿ���λ
	 *	��ô���µ��������ݽ�����ʱ������ٴδ��������߼�
	 */

	if(isFirstTick)	//����ǵ�һ��tick,����Ŀ���λ,���������µ�
	{
		double newVol = _target_pos;
		const char* stdCode = _code.c_str();

		double undone = _ctx->getUndoneQty(stdCode);
		double realPos = _ctx->getPosition(stdCode);

		if (!decimal::eq(newVol, undone + realPos))
		{
			doCalculate();
		}
	}
	else if(_expire_secs != 0 && _orders_mon.has_order() && _cancel_cnt==0)
	{
		uint64_t now = _ctx->getCurTime();

		_orders_mon.check_orders(_expire_secs, now, [this](uint32_t localid) {
			if (_ctx->cancel(localid))
			{
				_cancel_cnt++;
				_ctx->writeLog("@ %d cancelcnt -> %u", __LINE__, _cancel_cnt);
			}
		});
	}
}

void WtSimpExeUnit::on_trade(uint32_t localid, const char* stdCode, bool isBuy, double vol, double price)
{
	//���ô���,������ontick�ﴥ����
}

void WtSimpExeUnit::on_entrust(uint32_t localid, const char* stdCode, bool bSuccess, const char* message)
{
	if (!bSuccess)
	{
		//��������ҷ���ȥ�Ķ���,�ҾͲ�����
		if (!_orders_mon.has_order(localid))
			return;

		_orders_mon.erase_order(localid);

		doCalculate();
	}
}

void WtSimpExeUnit::doCalculate()
{
	if (_cancel_cnt != 0)
		return;

	double newVol = _target_pos;
	const char* stdCode = _code.c_str();

	double undone = _ctx->getUndoneQty(stdCode);
	double realPos = _ctx->getPosition(stdCode);

	//����з���δ��ɵ�,��ֱ�ӳ���
	//���Ŀ���λΪ0,�ҵ�ǰ�ֲ�Ϊ0,����ȫ���ҵ�
	if (decimal::lt(newVol * undone, 0))
	{
		bool isBuy = decimal::gt(undone, 0);
		OrderIDs ids = _ctx->cancel(stdCode, isBuy);
		_orders_mon.push_order(ids.data(), ids.size(), _ctx->getCurTime());
		_cancel_cnt += ids.size();
		_ctx->writeLog("@ %d cancelcnt -> %u", __LINE__, _cancel_cnt);
		return;
	}
	//else if(newVol == 0 && undone != 0)
	else if (decimal::eq(newVol,0) && !decimal::eq(undone, 0))
	{
		//���Ŀ���λΪ0,��δ��ɲ�Ϊ0
		//��ô��Ŀǰ��λΪ0,���� Ŀǰ��λ��δ�������������ͬ
		//����ҲҪȫ������
		//if (realPos == 0 || (realPos * undone > 0))
		if (decimal::eq(realPos, 0) || decimal::gt(realPos * undone, 0))
		{
			bool isBuy = decimal::gt(undone, 0);
			OrderIDs ids = _ctx->cancel(stdCode, isBuy);
			_orders_mon.push_order(ids.data(), ids.size(), _ctx->getCurTime());
			_cancel_cnt += ids.size();
			_ctx->writeLog("@ %d cancelcnt -> %u", __LINE__, _cancel_cnt);
			return;
		}
	}

	//�������ͬ���,���������
	double curPos = realPos + undone;
	//if (curPos == newVol)
	if (decimal::eq(curPos, newVol))
		return;

	if(_last_tick == NULL)
	{
		//grabLastTick���Զ��������ü���,����Ҫ��retain
		_last_tick = _ctx->grabLastTick(stdCode);
	}

	if (_last_tick == NULL)
	{
		//_ctx->writeLog("%sû������tick����,�˳�ִ���߼�", _code.c_str());
		_ctx->writeLog("No lastest tick data of %s, execute later", _code.c_str());
		return;
	}

	//�������ϴ�û�и��µ�tick���������Ȳ��µ�����ֹ����ǰ�����µ�����ͨ������
	uint64_t curTickTime = (uint64_t)_last_tick->actiondate() * 1000000000 + _last_tick->actiontime();
	if(curTickTime <= _last_tick_time)
	{
		_ctx->writeLog("No tick data of %s updated, execute later", _code.c_str());
		return;
	}
	_last_tick_time = curTickTime;

	double buyPx, sellPx;
	if(_price_mode == -1)
	{
		buyPx = _last_tick->bidprice(0) + _comm_info->getPriceTick() * _price_offset;
		sellPx = _last_tick->askprice(0) - _comm_info->getPriceTick() * _price_offset;
	}
	else if(_price_mode == 0)
	{
		buyPx = _last_tick->price() + _comm_info->getPriceTick() * _price_offset;
		sellPx = _last_tick->price() - _comm_info->getPriceTick() * _price_offset;
	}
	else if(_price_mode == 1)
	{
		buyPx = _last_tick->askprice(0) + _comm_info->getPriceTick() * _price_offset;
		sellPx = _last_tick->bidprice(0) - _comm_info->getPriceTick() * _price_offset;
	}
	else if(_price_mode == 2)
	{
		double mp = (_last_tick->bidqty(0) - _last_tick->askqty(0))*1.0 / (_last_tick->bidqty(0) + _last_tick->askqty(0));
		bool isUp = (mp > 0);
		if(isUp)
		{
			buyPx = _last_tick->askprice(0) + _comm_info->getPriceTick() * _cancel_times;
			sellPx = _last_tick->askprice(0) - _comm_info->getPriceTick() * _cancel_times;
		}
		else
		{
			buyPx = _last_tick->bidprice(0) + _comm_info->getPriceTick() * _cancel_times;
			sellPx = _last_tick->bidprice(0) - _comm_info->getPriceTick() * _cancel_times;
		}
	}

	//����ǵ�ͣ��
	bool isCanCancel = true;
	if (!decimal::eq(_last_tick->upperlimit(), 0) && decimal::gt(buyPx, _last_tick->upperlimit()))
	{
		//_ctx->writeLog("%s�������%f������Ϊ��ͣ��%f", _code.c_str(), buyPx, _last_tick->upperlimit());
		_ctx->writeLog("Buy price %f of %s modified to upper limit price", buyPx, _code.c_str(), _last_tick->upperlimit());
		buyPx = _last_tick->upperlimit();
		isCanCancel = false;	//����۸�����Ϊ�ǵ�ͣ�ۣ��������ɳ���
	}
	
	if (!decimal::eq(_last_tick->lowerlimit(), 0) && decimal::lt(sellPx, _last_tick->lowerlimit()))
	{
		//_ctx->writeLog("%s��������%f������Ϊ��ͣ��%f", _code.c_str(), sellPx, _last_tick->lowerlimit());
		_ctx->writeLog("Sell price %f of %s modified to lower limit price", buyPx, _code.c_str(), _last_tick->upperlimit());
		sellPx = _last_tick->lowerlimit();
		isCanCancel = false;	//����۸�����Ϊ�ǵ�ͣ�ۣ��������ɳ���
	}

	//if (newVol > curPos)
	if (decimal::gt(newVol, curPos))
	{
		OrderIDs ids = _ctx->buy(stdCode, buyPx, newVol - curPos);
		_orders_mon.push_order(ids.data(), ids.size(), _ctx->getCurTime(), isCanCancel);
	}
	else
	{
		OrderIDs ids  = _ctx->sell(stdCode, sellPx, curPos - newVol);
		_orders_mon.push_order(ids.data(), ids.size(), _ctx->getCurTime(), isCanCancel);
	}
}

void WtSimpExeUnit::set_position(const char* stdCode, double newVol)
{
	if (_code.compare(stdCode) != 0)
		return;

	if (_target_pos != newVol)
	{
		_target_pos = newVol;
	}

	doCalculate();
}
