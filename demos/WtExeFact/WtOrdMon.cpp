#include "WtOrdMon.h"

void WtOrdMon::push_order(const uint32_t* ids, uint32_t cnt, uint64_t curTime, bool bCanCancel /* = true */)
{
	StdLocker<StdRecurMutex> lock(_mtx_ords);
	for (uint32_t idx = 0; idx < cnt; idx++)
	{
		uint32_t localid = ids[idx];
		OrderPair& ordInfo = _orders[localid];
		ordInfo.first = curTime;
		ordInfo.second = bCanCancel;
	}
}

void WtOrdMon::erase_order(uint32_t localid)
{
	auto it = _orders.find(localid);
	if (it == _orders.end())
		return;

	_orders.erase(it);
}

void WtOrdMon::check_orders(uint32_t expiresecs, uint64_t curTime, EnumOrderCallback callback)
{
	if (_orders.empty())
		return;

	StdLocker<StdRecurMutex> lock(_mtx_ords);
	for (auto& m : _orders)
	{
		uint32_t localid = m.first;
		OrderPair& ordInfo = m.second;
		if(ordInfo.second)
			continue;

		auto entertm = ordInfo.first;
		if (curTime - entertm < expiresecs * 1000)
			continue;

		callback(m.first);
	}
}

