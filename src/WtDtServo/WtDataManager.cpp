/*!
 * \file WtDataManager.cpp
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#include "WtDataManager.h"
#include "WtDtRunner.h"
#include "WtDataReader.h"
#include "WtHelper.h"

#include "../Includes/WTSDataDef.hpp"
#include "../Includes/WTSVariant.hpp"
#include "../Includes/IBaseDataMgr.h"
#include "../Includes/WTSContractInfo.hpp"

#include "../Share/StrUtil.hpp"
#include "../Share/DLLHelper.hpp"
#include "../Share/StdUtils.hpp"
#include "../Share/TimeUtils.hpp"
#include "../Share/CodeHelper.hpp"

#include "../WTSTools/WTSLogger.h"
#include "../WTSTools/WTSDataFactory.h"


WTSDataFactory g_dataFact;

WtDataManager::WtDataManager()
	: _bd_mgr(NULL)
	, _hot_mgr(NULL)
	, _runner(NULL)
{
}


WtDataManager::~WtDataManager()
{
	for(auto& m : _bars_cache)
	{
		if (m.second._bars != NULL)
			m.second._bars->release();
	}
	_bars_cache.clear();
}

bool WtDataManager::initStore(WTSVariant* cfg)
{
	if (cfg == NULL)
		return false;

	_reader.init(cfg, _bd_mgr, _hot_mgr);

	return true;
}

bool WtDataManager::init(WTSVariant* cfg, WtDtRunner* runner)
{
	_runner = runner;
	if (_runner)
	{
		_bd_mgr = &_runner->getBaseDataMgr();
		_hot_mgr = &_runner->getHotMgr();
	}

	return initStore(cfg->get("store"));
}

WTSArray* WtDataManager::get_tick_slices_by_range(const char* stdCode,uint64_t stime, uint64_t etime /* = 0 */)
{
	stime = stime * 100000;
	etime = etime * 100000;
	return _reader.readTickSlicesByRange(stdCode, stime, etime);
}

WTSOrdQueSlice* WtDataManager::get_order_queue_slice(const char* stdCode,uint64_t stime, uint64_t etime /* = 0 */)
{
	stime = stime * 100000;
	etime = etime * 100000;
	return _reader.readOrdQueSliceByRange(stdCode, stime, etime);
}

WTSOrdDtlSlice* WtDataManager::get_order_detail_slice(const char* stdCode,uint64_t stime, uint64_t etime /* = 0 */)
{
	stime = stime * 100000;
	etime = etime * 100000;
	return _reader.readOrdDtlSliceByRange(stdCode, stime, etime);
}

WTSTransSlice* WtDataManager::get_transaction_slice(const char* stdCode,uint64_t stime, uint64_t etime /* = 0 */)
{
	stime = stime * 100000;
	etime = etime * 100000;
	return _reader.readTransSliceByRange(stdCode, stime, etime);
}

WTSSessionInfo* WtDataManager::get_session_info(const char* sid, bool isCode /* = false */)
{
	if (!isCode)
		return _bd_mgr->getSession(sid);

	WTSCommodityInfo* cInfo = _bd_mgr->getCommodity(CodeHelper::stdCodeToStdCommID(sid).c_str());
	if (cInfo == NULL)
		return NULL;

	return _bd_mgr->getSession(cInfo->getSession());
}

WTSKlineSlice* WtDataManager::get_kline_slice_by_range(const char* stdCode, WTSKlinePeriod period, uint32_t times,uint64_t stime, uint64_t etime /* = 0 */)
{
	std::string key = StrUtil::printf("%s-%u", stdCode, period);

	if (times == 1)
	{
		return _reader.readKlineSliceByRange(stdCode, period, stime, etime);
	}

	//ֻ�зǻ������ڵĻ��������Ĳ���
	WTSSessionInfo* sInfo = get_session_info(stdCode, true);
	key = StrUtil::printf("%s-%u-%u", stdCode, period, times);
	BarCache& barCache = _bars_cache[key];
	barCache._period = period;
	barCache._times = times;
	if(barCache._bars == NULL)
	{
		//��һ�ν�ȫ�����ݻ��浽�ڴ���
		WTSKlineSlice* rawData = _reader.readKlineSliceByCount(stdCode, period, UINT_MAX, 0);
		if (rawData != NULL)
		{
			WTSKlineData* kData = g_dataFact.extractKlineData(rawData, period, times, sInfo, false);
			barCache._bars = kData;

			//������Σ���ɾ�����һ��K��
			//����ͨ���պϱ���жϣ���Ϊ��ȡ�Ļ������ڿ��ܱ���û�бպ�
			if (barCache._bars->size() > 0)
			{
				auto& bars = barCache._bars->getDataRef();
				bars.erase(bars.begin() + bars.size() - 1, bars.end());
			}

			if (period == KP_DAY)
				barCache._last_bartime = kData->date(-1);
			else
			{
				uint64_t lasttime = kData->time(-1);
				barCache._last_bartime = 199000000000 + lasttime;
			}

			rawData->release();
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		//��������������
		WTSKlineSlice* rawData = _reader.readKlineSliceByRange(stdCode, period, barCache._last_bartime, 0);
		if (rawData != NULL)
		{
			for(int32_t idx = 0; idx < rawData->size(); idx ++)
			{
				uint64_t barTime = 0;
				if (period == KP_DAY)
					barTime = rawData->date(0);
				else
					barTime = 199000000000 + rawData->time(0);
				
				//ֻ��ʱ���ϴμ�¼�����һ��ʱ�䣬�ſ������ڸ���K��
				if(barTime <= barCache._last_bartime)
					continue;

				g_dataFact.updateKlineData(barCache._bars, rawData->at(idx), sInfo);
			}

			//������Σ���ɾ�����һ��K��
			//����ͨ���պϱ���жϣ���Ϊ��ȡ�Ļ������ڿ��ܱ���û�бպ�
			if(barCache._bars->size() > 0)
			{
				auto& bars = barCache._bars->getDataRef();
				bars.erase(bars.begin() + bars.size() - 1, bars.end());
			}

			if (period == KP_DAY)
				barCache._last_bartime = barCache._bars->date(-1);
			else
			{
				uint64_t lasttime = barCache._bars->time(-1);
				barCache._last_bartime = 199000000000 + lasttime;
			}
			

			rawData->release();
		}
	}

	//��󵽻����ж�λ
	bool isDay = period == KP_DAY;
	uint32_t rDate, rTime, lDate, lTime;
	rDate = (uint32_t)(etime / 10000);
	rTime = (uint32_t)(etime % 10000);
	lDate = (uint32_t)(stime / 10000);
	lTime = (uint32_t)(stime % 10000);

	WTSBarStruct eBar;
	eBar.date = rDate;
	eBar.time = (rDate - 19900000) * 10000 + rTime;

	WTSBarStruct sBar;
	sBar.date = lDate;
	sBar.time = (lDate - 19900000) * 10000 + lTime;

	uint32_t eIdx, sIdx;
	auto& bars = barCache._bars->getDataRef();
	auto eit = std::lower_bound(bars.begin(), bars.end(), eBar, [isDay](const WTSBarStruct& a, const WTSBarStruct& b) {
		if (isDay)
			return a.date < b.date;
		else
			return a.time < b.time;
	});


	if (eit == bars.end())
		eIdx = bars.size() - 1;
	else
	{
		if ((isDay && eit->date > eBar.date) || (!isDay && eit->time > eBar.time))
		{
			eit--;
		}

		eIdx = eit - bars.begin();
	}

	auto sit = std::lower_bound(bars.begin(), eit, sBar, [isDay](const WTSBarStruct& a, const WTSBarStruct& b) {
		if (isDay)
			return a.date < b.date;
		else
			return a.time < b.time;
	});
	sIdx = sit - bars.begin();
	uint32_t rtCnt = eIdx - sIdx + 1;
	WTSBarStruct* rtHead = barCache._bars->at(sIdx);
	WTSKlineSlice* slice = WTSKlineSlice::create(stdCode, period, times, NULL, 0, rtHead, rtCnt);
	return slice;
}

WTSKlineSlice* WtDataManager::get_kline_slice_by_count(const char* stdCode, WTSKlinePeriod period, uint32_t times, uint32_t count, uint64_t etime /* = 0 */)
{
	std::string key = StrUtil::printf("%s-%u", stdCode, period);

	if (times == 1)
	{
		return _reader.readKlineSliceByCount(stdCode, period, count, etime);
	}

	//ֻ�зǻ������ڵĻ��������Ĳ���
	WTSSessionInfo* sInfo = get_session_info(stdCode, true);
	key = StrUtil::printf("%s-%u-%u", stdCode, period, times);
	BarCache& barCache = _bars_cache[key];
	barCache._period = period;
	barCache._times = times;

	char* tag = "unknown";
	switch (period)
	{
	case KP_Minute1:tag = "min1"; break;
	case KP_Minute5:tag = "min5"; break;
	case KP_DAY:tag = "day"; break;
	default:break;
	}

	if (barCache._bars == NULL)
	{
		//��һ�ν�ȫ�����ݻ��浽�ڴ���
		WTSLogger::info("Caching all %s bars of %s...", tag, stdCode);
		WTSKlineSlice* rawData = _reader.readKlineSliceByCount(stdCode, period, UINT_MAX, 0);
		if (rawData != NULL)
		{
			WTSLogger::info("Resampling %u %s bars by %u TO 1 of %s...", rawData->size(), tag, times, stdCode);
			WTSKlineData* kData = g_dataFact.extractKlineData(rawData, period, times, sInfo, true);
			barCache._bars = kData;

			//����������ߣ�Ҫ�������һ��K���Ƿ�պϵ����
			//������ñ��صķ������������ʱ��������һ��K�ߵ�ʱ�䣬����Ϊ�����պ���
			if (period != KP_DAY)
			{
				uint64_t last_bartime = 0;
				last_bartime = 199000000000 + kData->time(-1);

				uint64_t now = TimeUtils::getYYYYMMDDhhmmss() / 100;
				if (now <= last_bartime && barCache._bars->size() > 0)
				{
					auto& bars = barCache._bars->getDataRef();
					bars.erase(bars.begin() + bars.size() - 1, bars.end());
				}
			}


			if (period == KP_DAY)
				barCache._last_bartime = kData->date(-1);
			else
			{
				uint64_t lasttime = kData->time(-1);
				barCache._last_bartime = 199000000000 + lasttime;
			}

			rawData->release();
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		//��������������
		WTSKlineSlice* rawData = _reader.readKlineSliceByRange(stdCode, period, barCache._last_bartime, 0);
		if (rawData != NULL)
		{
			WTSLogger::info("%u %s bars updated of %s, adding to cache...", rawData->size(), tag, stdCode);
			for (int32_t idx = 0; idx < rawData->size(); idx++)
			{
				uint64_t barTime = 0;
				if (period == KP_DAY)
					barTime = rawData->date(0);
				else
					barTime = 199000000000 + rawData->time(0);

				//ֻ��ʱ���ϴμ�¼�����һ��ʱ�䣬�ſ������ڸ���K��
				if (barTime <= barCache._last_bartime)
					continue;

				g_dataFact.updateKlineData(barCache._bars, rawData->at(idx), sInfo);
			}

			//����������ߣ�Ҫ�������һ��K���Ƿ�պϵ����
			//������ñ��صķ������������ʱ��������һ��K�ߵ�ʱ�䣬����Ϊ�����պ���
			if (period != KP_DAY)
			{
				uint64_t last_bartime = 0;
				last_bartime = 199000000000 + barCache._bars->time(-1);

				uint64_t now = TimeUtils::getYYYYMMDDhhmmss() / 100;
				if (now <= last_bartime && barCache._bars->size() > 0)
				{
					auto& bars = barCache._bars->getDataRef();
					bars.erase(bars.begin() + bars.size() - 1, bars.end());
				}
			}

			if (period == KP_DAY)
				barCache._last_bartime = barCache._bars->date(-1);
			else
			{
				uint64_t lasttime = barCache._bars->time(-1);
				barCache._last_bartime = 199000000000 + lasttime;
			}


			rawData->release();
		}
	}

	//��󵽻����ж�λ
	bool isDay = period == KP_DAY;
	uint32_t rDate, rTime;
	rDate = (uint32_t)(etime / 10000);
	rTime = (uint32_t)(etime % 10000);

	WTSBarStruct eBar;
	eBar.date = rDate;
	eBar.time = (rDate - 19900000) * 10000 + rTime;

	uint32_t eIdx, sIdx;
	auto& bars = barCache._bars->getDataRef();
	auto eit = std::lower_bound(bars.begin(), bars.end(), eBar, [isDay](const WTSBarStruct& a, const WTSBarStruct& b) {
		if (isDay)
			return a.date < b.date;
		else
			return a.time < b.time;
	});


	if (eit == bars.end())
		eIdx = bars.size() - 1;
	else
	{
		if ((isDay && eit->date > eBar.date) || (!isDay && eit->time > eBar.time))
		{
			eit--;
		}

		eIdx = eit - bars.begin();
	}

	sIdx = (eIdx + 1 >= count) ? (eIdx + 1 - count) : 0;
	uint32_t rtCnt = eIdx - sIdx + 1;
	WTSBarStruct* rtHead = barCache._bars->at(sIdx);
	WTSKlineSlice* slice = WTSKlineSlice::create(stdCode, period, times, NULL, 0, rtHead, rtCnt);
	return slice;
}

WTSArray* WtDataManager::get_tick_slices_by_count(const char* stdCode, uint32_t count, uint64_t etime /* = 0 */)
{
	etime = etime * 100000;
	return _reader.readTickSlicesByCount(stdCode, count, etime);
}