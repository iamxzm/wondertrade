#include "WtDataReader.h"

#include "../Includes/WTSVariant.hpp"
#include "../Share/StdUtils.hpp"
#include "../Share/TimeUtils.hpp"
#include "../Share/CodeHelper.hpp"
#include "../Share/StdUtils.hpp"
#include "../Share/DLLHelper.hpp"

#include "../Includes/WTSContractInfo.hpp"
#include "../Includes/IBaseDataMgr.h"
#include "../Includes/IHotMgr.h"
#include "../Includes/WTSDataDef.hpp"
#include "../Includes/WTSSessionInfo.hpp"

#include "../WTSTools/WTSCmpHelper.hpp"
#include "../WTSTools/WTSLogger.h"

#include <rapidjson/document.h>
namespace rj = rapidjson;


WtDataReader::WtDataReader()
	: _base_data_mgr(NULL)
	, _hot_mgr(NULL)
	, _stopped(false)
{
}


WtDataReader::~WtDataReader()
{
	_stopped = true;
	if (_thrd_check)
		_thrd_check->join();
}

void WtDataReader::init(WTSVariant* cfg, IBaseDataMgr* bdMgr, IHotMgr* hotMgr)
{
	_base_data_mgr = bdMgr;
	_hot_mgr = hotMgr;

	if (cfg == NULL)
		return ;

	_base_dir = cfg->getCString("path");
	_base_dir = StrUtil::standardisePath(_base_dir);

	bool bAdjLoaded = false;
	
	if (!bAdjLoaded && cfg->has("adjfactor"))
		loadStkAdjFactorsFromFile(cfg->getCString("adjfactor"));

	_thrd_check.reset(new StdThread([this]() {
		while(!_stopped)
		{
			std::this_thread::sleep_for(std::chrono::seconds(5));
			uint64_t now = TimeUtils::getLocalTimeNow();

			for(auto& m : _rt_tick_map)
			{
				//���5����֮��û�з��ʣ����ͷŵ�
				TickBlockPair& tPair = (TickBlockPair&)m.second;
				if(now > tPair._last_time + 300000 && tPair._block != NULL)
				{	
					StdUniqueLock lock(*tPair._mtx);
					tPair._block = NULL;
					tPair._file.reset();
				}
			}

			for (auto& m : _rt_ordque_map)
			{
				//���5����֮��û�з��ʣ����ͷŵ�
				OrdQueBlockPair& tPair = (OrdQueBlockPair&)m.second;
				if (now > tPair._last_time + 300000 && tPair._block != NULL)
				{
					StdUniqueLock lock(*tPair._mtx);
					tPair._block = NULL;
					tPair._file.reset();
				}
			}

			for (auto& m : _rt_orddtl_map)
			{
				//���5����֮��û�з��ʣ����ͷŵ�
				OrdDtlBlockPair& tPair = (OrdDtlBlockPair&)m.second;
				if (now > tPair._last_time + 300000 && tPair._block != NULL)
				{
					StdUniqueLock lock(*tPair._mtx);
					tPair._block = NULL;
					tPair._file.reset();
				}
			}

			for (auto& m : _rt_trans_map)
			{
				//���5����֮��û�з��ʣ����ͷŵ�
				TransBlockPair& tPair = (TransBlockPair&)m.second;
				if (now > tPair._last_time + 300000 && tPair._block != NULL)
				{
					StdUniqueLock lock(*tPair._mtx);
					tPair._block = NULL;
					tPair._file.reset();
				}
			}

			for (auto& m : _rt_min1_map)
			{
				//���5����֮��û�з��ʣ����ͷŵ�
				RTKlineBlockPair& tPair = (RTKlineBlockPair&)m.second;
				if (now > tPair._last_time + 300000 && tPair._block != NULL)
				{
					StdUniqueLock lock(*tPair._mtx);
					tPair._block = NULL;
					tPair._file.reset();
				}
			}

			for (auto& m : _rt_min5_map)
			{
				//���5����֮��û�з��ʣ����ͷŵ�
				RTKlineBlockPair& tPair = (RTKlineBlockPair&)m.second;
				if (now > tPair._last_time + 300000 && tPair._block != NULL)
				{
					StdUniqueLock lock(*tPair._mtx);
					tPair._block = NULL;
					tPair._file.reset();
				}
			}
		}
	}));
}


bool WtDataReader::loadStkAdjFactorsFromFile(const char* adjfile)
{
	if(!StdFile::exists(adjfile))
	{
		WTSLogger::error("��Ȩ�����ļ�%s������", adjfile);
		return false;
	}

	std::string content;
	StdFile::read_file_content(adjfile, content);

	rj::Document doc;
	doc.Parse(content.c_str());

	if(doc.HasParseError())
	{
		WTSLogger::error("��Ȩ�����ļ�%s����ʧ��", adjfile);
		return false;
	}

	uint32_t stk_cnt = 0;
	uint32_t fct_cnt = 0;
	for (auto& mExchg : doc.GetObject())
	{
		const char* exchg = mExchg.name.GetString();
		const rj::Value& itemExchg = mExchg.value;
		for(auto& mCode : itemExchg.GetObject())
		{
			const char* code = mCode.name.GetString();
			const rj::Value& ayFacts = mCode.value;
			if(!ayFacts.IsArray() )
				continue;

			std::string key = StrUtil::printf("%s.%s", exchg, code);
			stk_cnt++;

			AdjFactorList& fctrLst = _adj_factors[key];
			for (auto& fItem : ayFacts.GetArray())
			{
				AdjFactor adjFact;
				adjFact._date = fItem["date"].GetUint();
				adjFact._factor = fItem["factor"].GetDouble();

				fctrLst.emplace_back(adjFact);
				fct_cnt++;
			}
		}
	}

	WTSLogger::info("������%uֻ��Ʊ��%u����Ȩ��������", stk_cnt, fct_cnt);
	return true;
}

WTSArray* WtDataReader::readTickSlicesByRange(const char* stdCode, uint64_t stime, uint64_t etime /* = 0 */)
{
	CodeHelper::CodeInfo cInfo;
	CodeHelper::extractStdCode(stdCode, cInfo);
	std::string stdPID = StrUtil::printf("%s.%s", cInfo._exchg, cInfo._product);

	WTSSessionInfo* sInfo = _base_data_mgr->getSession(_base_data_mgr->getCommodity(cInfo._exchg, cInfo._code)->getSession());

	uint32_t rDate, rTime, rSecs;
	//20190807124533900
	rDate = (uint32_t)(etime / 1000000000);
	rTime = (uint32_t)(etime % 1000000000) / 100000;
	rSecs = (uint32_t)(etime % 100000);

	uint32_t lDate, lTime, lSecs;
	//20190807124533900
	lDate = (uint32_t)(stime / 1000000000);
	lTime = (uint32_t)(stime % 1000000000) / 100000;
	lSecs = (uint32_t)(stime % 100000);

	uint32_t endTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), rDate, rTime, false);
	uint32_t beginTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), lDate, lTime, false);
	uint32_t curTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), 0, 0, false);

	bool hasToday = (endTDate >= curTDate);

	WTSArray* ayTicks = WTSArray::create();

	WTSTickStruct sTick;
	sTick.action_date = lDate;
	sTick.action_time = lTime * 100000 + lSecs;
	
	uint32_t nowTDate = beginTDate;
	while(nowTDate < curTDate)
	{
		std::string curCode = cInfo._code;
		std::string hotCode;
		if (cInfo.isHot() && cInfo._category == CC_Future)
		{
			curCode = _hot_mgr->getRawCode(cInfo._exchg, cInfo._product, nowTDate);
			WTSLogger::info("Hot contract of %u confirmed: %s -> %s", curTDate, stdCode, curCode.c_str());
			hotCode = cInfo._product;
			hotCode += "_HOT";
		}
		else if (cInfo.isSecond() && cInfo._category == CC_Future)
		{
			curCode = _hot_mgr->getSecondRawCode(cInfo._exchg, cInfo._product, nowTDate);
			WTSLogger::info("Second contract of %u confirmed: %s -> %s", curTDate, stdCode, curCode.c_str());
			hotCode = cInfo._product;
			hotCode += "_2ND";
		}

		std::string key = StrUtil::printf("%s-%d", stdCode, nowTDate);

		auto it = _his_tick_map.find(key);
		bool bHasHisTick = (it != _his_tick_map.end());
		if(!bHasHisTick)
		{
			for(;;)
			{
				std::string filename;
				bool bHitHot = false;
				if (!hotCode.empty())
				{
					std::stringstream ss;
					ss << _base_dir << "his/ticks/" << cInfo._exchg << "/" << nowTDate << "/" << hotCode << ".dsb";
					filename = ss.str();
					if (StdFile::exists(filename.c_str()))
					{
						bHitHot = true;
					}
				}

				if (!bHitHot)
				{
					std::stringstream ss;
					ss << _base_dir << "his/ticks/" << cInfo._exchg << "/" << nowTDate << "/" << curCode << ".dsb";
					filename = ss.str();
					if (!StdFile::exists(filename.c_str()))
					{
						break;
					}
				}

				HisTBlockPair& tBlkPair = _his_tick_map[key];
				StdFile::read_file_content(filename.c_str(), tBlkPair._buffer);
				if (tBlkPair._buffer.size() < sizeof(HisTickBlock))
				{
					WTSLogger::error("��ʷTick�����ļ�%s��СУ��ʧ��", filename.c_str());
					tBlkPair._buffer.clear();
					break;
				}

				HisTickBlock* tBlock = (HisTickBlock*)tBlkPair._buffer.c_str();
				if (tBlock->_version == BLOCK_VERSION_CMP)
				{
					//ѹ���汾,Ҫ���¼���ļ���С
					HisTickBlockV2* tBlockV2 = (HisTickBlockV2*)tBlkPair._buffer.c_str();

					if (tBlkPair._buffer.size() != (sizeof(HisTickBlockV2) + tBlockV2->_size))
					{
						WTSLogger::error("��ʷTick�����ļ�%s��СУ��ʧ��", filename.c_str());
						break;
					}

					//��Ҫ��ѹ
					std::string buf = WTSCmpHelper::uncompress_data(tBlockV2->_data, (uint32_t)tBlockV2->_size);

					//��ԭ����bufferֻ����һ��ͷ��,��������tick����׷�ӵ�β��
					tBlkPair._buffer.resize(sizeof(HisTickBlock));
					tBlkPair._buffer.append(buf);
					tBlockV2 = (HisTickBlockV2*)tBlkPair._buffer.c_str();
					tBlockV2->_version = BLOCK_VERSION_RAW;
				}

				tBlkPair._block = (HisTickBlock*)tBlkPair._buffer.c_str();
				bHasHisTick = true;
				break;
			}
		}
		
		while(bHasHisTick)
		{
			//�Ƚ�ʱ��Ķ���
			WTSTickStruct eTick;
			if(nowTDate == endTDate)
			{
				eTick.action_date = rDate;
				eTick.action_time = rTime * 100000 + rSecs;
			}
			else
			{
				eTick.action_date = nowTDate;
				eTick.action_time = sInfo->getCloseTime() * 100000 + 59999;
			}

			HisTBlockPair& tBlkPair = _his_tick_map[key];
			if (tBlkPair._block == NULL)
				break;

			HisTickBlock* tBlock = tBlkPair._block;

			uint32_t tcnt = (tBlkPair._buffer.size() - sizeof(HisTickBlock)) / sizeof(WTSTickStruct);
			if (tcnt <= 0)
				break;

			WTSTickStruct* pTick = std::lower_bound(tBlock->_ticks, tBlock->_ticks + (tcnt - 1), eTick, [](const WTSTickStruct& a, const WTSTickStruct& b) {
				if (a.action_date != b.action_date)
					return a.action_date < b.action_date;
				else
					return a.action_time < b.action_time;
			});

			uint32_t eIdx = pTick - tBlock->_ticks;
			if (pTick->action_date > eTick.action_date || pTick->action_time >= eTick.action_time)
			{
				pTick--;
				eIdx--;
			}

			if (beginTDate != nowTDate)
			{
				//�����ʼ�Ľ����պ͵�ǰ�Ľ����ղ�һ�£��򷵻�ȫ����tick����
				WTSTickSlice* slice = WTSTickSlice::create(stdCode, tBlock->_ticks, eIdx + 1);
				ayTicks->append(slice, false);
			}
			else
			{
				//�����������ͬ���������ʼ��λ��
				pTick = std::lower_bound(tBlock->_ticks, tBlock->_ticks + eIdx, sTick, [](const WTSTickStruct& a, const WTSTickStruct& b) {
					if (a.action_date != b.action_date)
						return a.action_date < b.action_date;
					else
						return a.action_time < b.action_time;
				});

				uint32_t sIdx = pTick - tBlock->_ticks;
				WTSTickSlice* slice = WTSTickSlice::create(stdCode, tBlock->_ticks + sIdx, eIdx - sIdx + 1);
				ayTicks->append(slice, false);
			}

			break;
		}
		
		nowTDate = TimeUtils::getNextDate(nowTDate);
	}

	while(hasToday)
	{
		std::string curCode = cInfo._code;
		if (cInfo.isHot() && cInfo._category == CC_Future)
			curCode = _hot_mgr->getRawCode(cInfo._exchg, cInfo._product, curTDate);
		else if (cInfo.isSecond() && cInfo._category == CC_Future)
			curCode = _hot_mgr->getSecondRawCode(cInfo._exchg, cInfo._product, curTDate);

		TickBlockPair* tPair = getRTTickBlock(cInfo._exchg, curCode.c_str());
		if (tPair == NULL || tPair->_block->_size == 0)
			break;

		StdUniqueLock lock(*tPair->_mtx);
		RTTickBlock* tBlock = tPair->_block;
		WTSTickStruct eTick;
		if (curTDate == endTDate)
		{
			eTick.action_date = rDate;
			eTick.action_time = rTime * 100000 + rSecs;
		}
		else
		{
			eTick.action_date = curTDate;
			eTick.action_time = sInfo->getCloseTime() * 100000 + 59999;
		}

		WTSTickStruct* pTick = std::lower_bound(tBlock->_ticks, tBlock->_ticks + (tBlock->_size - 1), eTick, [](const WTSTickStruct& a, const WTSTickStruct& b) {
			if (a.action_date != b.action_date)
				return a.action_date < b.action_date;
			else
				return a.action_time < b.action_time;
		});

		uint32_t eIdx = pTick - tBlock->_ticks;

		//�����궨λ��tickʱ���Ŀ��ʱ���, ��ȫ������һ��
		if (pTick->action_date > eTick.action_date || pTick->action_time > eTick.action_time)
		{
			pTick--;
			eIdx--;
		}

		if (beginTDate != curTDate)
		{
			//�����ʼ�Ľ����պ͵�ǰ�Ľ����ղ�һ�£��򷵻�ȫ����tick����
			WTSTickSlice* slice = WTSTickSlice::create(stdCode, tBlock->_ticks, eIdx + 1);
			ayTicks->append(slice, false);
		}
		else
		{
			//�����������ͬ���������ʼ��λ��
			pTick = std::lower_bound(tBlock->_ticks, tBlock->_ticks + eIdx, sTick, [](const WTSTickStruct& a, const WTSTickStruct& b) {
				if (a.action_date != b.action_date)
					return a.action_date < b.action_date;
				else
					return a.action_time < b.action_time;
			});

			uint32_t sIdx = pTick - tBlock->_ticks;
			WTSTickSlice* slice = WTSTickSlice::create(stdCode, tBlock->_ticks + sIdx, eIdx - sIdx + 1);
			ayTicks->append(slice, false);
		}
		break;
	}

	return ayTicks;
}

WTSOrdQueSlice* WtDataReader::readOrdQueSliceByRange(const char* stdCode, uint64_t stime, uint64_t etime /* = 0 */)
{
	CodeHelper::CodeInfo cInfo;
	CodeHelper::extractStdCode(stdCode, cInfo);
	std::string stdPID = StrUtil::printf("%s.%s", cInfo._exchg, cInfo._product);

	uint32_t rDate, rTime, rSecs;
	//20190807124533900
	rDate = (uint32_t)(etime / 1000000000);
	rTime = (uint32_t)(etime % 1000000000) / 100000;
	rSecs = (uint32_t)(etime % 100000);

	uint32_t lDate, lTime, lSecs;
	//20190807124533900
	lDate = (uint32_t)(stime / 1000000000);
	lTime = (uint32_t)(stime % 1000000000) / 100000;
	lSecs = (uint32_t)(stime % 100000);

	uint32_t endTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), rDate, rTime, false);
	uint32_t beginTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), lDate, lTime, false);
	uint32_t curTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), 0, 0, false);

	bool isToday = (endTDate == curTDate);

	std::string curCode = cInfo._code;
	if (cInfo.isHot() && cInfo._category == CC_Future)
		curCode = _hot_mgr->getRawCode(cInfo._exchg, cInfo._product, endTDate);
	else if (cInfo.isSecond() && cInfo._category == CC_Future)
		curCode = _hot_mgr->getSecondRawCode(cInfo._exchg, cInfo._product, endTDate);

	//�Ƚ�ʱ��Ķ���
	WTSOrdQueStruct eTick;
	eTick.action_date = rDate;
	eTick.action_time = rTime * 100000 + rSecs;

	WTSOrdQueStruct sTick;
	sTick.action_date = lDate;
	sTick.action_time = lTime * 100000 + lSecs;

	if (isToday)
	{
		OrdQueBlockPair* tPair = getRTOrdQueBlock(cInfo._exchg, curCode.c_str());
		if (tPair == NULL)
			return NULL;

		RTOrdQueBlock* rtBlock = tPair->_block;

		WTSOrdQueStruct* pItem = std::lower_bound(rtBlock->_queues, rtBlock->_queues + (rtBlock->_size - 1), eTick, [](const WTSOrdQueStruct& a, const WTSOrdQueStruct& b) {
			if (a.action_date != b.action_date)
				return a.action_date < b.action_date;
			else
				return a.action_time < b.action_time;
		});

		uint32_t eIdx = pItem - rtBlock->_queues;

		//�����궨λ��tickʱ���Ŀ��ʱ���, ��ȫ������һ��
		if (pItem->action_date > eTick.action_date || pItem->action_time > eTick.action_time)
		{
			pItem--;
			eIdx--;
		}

		if (beginTDate != endTDate)
		{
			//�����ʼ�Ľ����պ͵�ǰ�Ľ����ղ�һ�£��򷵻�ȫ����tick����
			WTSOrdQueSlice* slice = WTSOrdQueSlice::create(stdCode, rtBlock->_queues, eIdx + 1);
			return slice;
		}
		else
		{
			//�����������ͬ���������ʼ��λ��
			pItem = std::lower_bound(rtBlock->_queues, rtBlock->_queues + eIdx, sTick, [](const WTSOrdQueStruct& a, const WTSOrdQueStruct& b) {
				if (a.action_date != b.action_date)
					return a.action_date < b.action_date;
				else
					return a.action_time < b.action_time;
			});

			uint32_t sIdx = pItem - rtBlock->_queues;
			WTSOrdQueSlice* slice = WTSOrdQueSlice::create(stdCode, rtBlock->_queues + sIdx, eIdx - sIdx + 1);
			return slice;
		}
	}
	else
	{
		std::string key = StrUtil::printf("%s-%d", stdCode, endTDate);

		auto it = _his_ordque_map.find(key);
		if (it == _his_ordque_map.end())
		{
			std::stringstream ss;
			ss << _base_dir << "his/queue/" << cInfo._exchg << "/" << endTDate << "/" << curCode << ".dsb";
			std::string filename = ss.str();
			if (!StdFile::exists(filename.c_str()))
				return NULL;

			HisOrdQueBlockPair& hisBlkPair = _his_ordque_map[key];
			StdFile::read_file_content(filename.c_str(), hisBlkPair._buffer);
			if (hisBlkPair._buffer.size() < sizeof(HisOrdQueBlockV2))
			{
				WTSLogger::error("��ʷί�ж��������ļ�%s��СУ��ʧ��", filename.c_str());
				hisBlkPair._buffer.clear();
				return NULL;
			}

			HisOrdQueBlockV2* tBlockV2 = (HisOrdQueBlockV2*)hisBlkPair._buffer.c_str();

			if (hisBlkPair._buffer.size() != (sizeof(HisOrdQueBlockV2) + tBlockV2->_size))
			{
				WTSLogger::error("��ʷί�ж��������ļ�%s��СУ��ʧ��", filename.c_str());
				return NULL;
			}

			//��Ҫ��ѹ
			std::string buf = WTSCmpHelper::uncompress_data(tBlockV2->_data, (uint32_t)tBlockV2->_size);

			//��ԭ����bufferֻ����һ��ͷ��,��������tick����׷�ӵ�β��
			hisBlkPair._buffer.resize(sizeof(HisOrdQueBlock));
			hisBlkPair._buffer.append(buf);
			tBlockV2->_version = BLOCK_VERSION_RAW;

			hisBlkPair._block = (HisOrdQueBlock*)hisBlkPair._buffer.c_str();
		}

		HisOrdQueBlockPair& tBlkPair = _his_ordque_map[key];
		if (tBlkPair._block == NULL)
			return NULL;

		HisOrdQueBlock* tBlock = tBlkPair._block;

		uint32_t tcnt = (tBlkPair._buffer.size() - sizeof(HisOrdQueBlock)) / sizeof(WTSOrdQueStruct);
		if (tcnt <= 0)
			return NULL;

		WTSOrdQueStruct* pItem = std::lower_bound(tBlock->_items, tBlock->_items + (tcnt - 1), eTick, [](const WTSOrdQueStruct& a, const WTSOrdQueStruct& b) {
			if (a.action_date != b.action_date)
				return a.action_date < b.action_date;
			else
				return a.action_time < b.action_time;
		});

		uint32_t eIdx = pItem - tBlock->_items;
		if (pItem->action_date > eTick.action_date || pItem->action_time >= eTick.action_time)
		{
			pItem--;
			eIdx--;
		}


		if (beginTDate != endTDate)
		{
			//�����ʼ�Ľ����պ͵�ǰ�Ľ����ղ�һ�£��򷵻�ȫ����tick����
			WTSOrdQueSlice* slice = WTSOrdQueSlice::create(stdCode, tBlock->_items, eIdx + 1);
			return slice;
		}
		else
		{
			//�����������ͬ���������ʼ��λ��
			pItem = std::lower_bound(tBlock->_items, tBlock->_items + eIdx, sTick, [](const WTSOrdQueStruct& a, const WTSOrdQueStruct& b) {
				if (a.action_date != b.action_date)
					return a.action_date < b.action_date;
				else
					return a.action_time < b.action_time;
			});

			uint32_t sIdx = pItem - tBlock->_items;
			WTSOrdQueSlice* slice = WTSOrdQueSlice::create(stdCode, tBlock->_items + sIdx, eIdx - sIdx + 1);
			return slice;
		}
	}
}

WTSOrdDtlSlice* WtDataReader::readOrdDtlSliceByRange(const char* stdCode, uint64_t stime, uint64_t etime /* = 0 */)
{
	CodeHelper::CodeInfo cInfo;
	CodeHelper::extractStdCode(stdCode, cInfo);
	std::string stdPID = StrUtil::printf("%s.%s", cInfo._exchg, cInfo._product);

	uint32_t rDate, rTime, rSecs;
	//20190807124533900
	rDate = (uint32_t)(etime / 1000000000);
	rTime = (uint32_t)(etime % 1000000000) / 100000;
	rSecs = (uint32_t)(etime % 100000);

	uint32_t lDate, lTime, lSecs;
	//20190807124533900
	lDate = (uint32_t)(stime / 1000000000);
	lTime = (uint32_t)(stime % 1000000000) / 100000;
	lSecs = (uint32_t)(stime % 100000);

	uint32_t endTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), rDate, rTime, false);
	uint32_t beginTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), lDate, lTime, false);
	uint32_t curTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), 0, 0, false);

	bool isToday = (endTDate == curTDate);

	std::string curCode = cInfo._code;
	if (cInfo.isHot() && cInfo._category == CC_Future)
		curCode = _hot_mgr->getRawCode(cInfo._exchg, cInfo._product, endTDate);
	else if (cInfo.isSecond() && cInfo._category == CC_Future)
		curCode = _hot_mgr->getSecondRawCode(cInfo._exchg, cInfo._product, endTDate);

	//�Ƚ�ʱ��Ķ���
	WTSOrdDtlStruct eTick;
	eTick.action_date = rDate;
	eTick.action_time = rTime * 100000 + rSecs;

	WTSOrdDtlStruct sTick;
	sTick.action_date = lDate;
	sTick.action_time = lTime * 100000 + lSecs;

	if (isToday)
	{
		OrdDtlBlockPair* tPair = getRTOrdDtlBlock(cInfo._exchg, curCode.c_str());
		if (tPair == NULL)
			return NULL;

		RTOrdDtlBlock* rtBlock = tPair->_block;

		WTSOrdDtlStruct* pItem = std::lower_bound(rtBlock->_details, rtBlock->_details + (rtBlock->_size - 1), eTick, [](const WTSOrdDtlStruct& a, const WTSOrdDtlStruct& b) {
			if (a.action_date != b.action_date)
				return a.action_date < b.action_date;
			else
				return a.action_time < b.action_time;
		});

		uint32_t eIdx = pItem - rtBlock->_details;

		//�����궨λ��tickʱ���Ŀ��ʱ���, ��ȫ������һ��
		if (pItem->action_date > eTick.action_date || pItem->action_time > eTick.action_time)
		{
			pItem--;
			eIdx--;
		}

		if (beginTDate != endTDate)
		{
			//�����ʼ�Ľ����պ͵�ǰ�Ľ����ղ�һ�£��򷵻�ȫ����tick����
			WTSOrdDtlSlice* slice = WTSOrdDtlSlice::create(stdCode, rtBlock->_details, eIdx + 1);
			return slice;
		}
		else
		{
			//�����������ͬ���������ʼ��λ��
			pItem = std::lower_bound(rtBlock->_details, rtBlock->_details + eIdx, sTick, [](const WTSOrdDtlStruct& a, const WTSOrdDtlStruct& b) {
				if (a.action_date != b.action_date)
					return a.action_date < b.action_date;
				else
					return a.action_time < b.action_time;
			});

			uint32_t sIdx = pItem - rtBlock->_details;
			WTSOrdDtlSlice* slice = WTSOrdDtlSlice::create(stdCode, rtBlock->_details + sIdx, eIdx - sIdx + 1);
			return slice;
		}
	}
	else
	{
		std::string key = StrUtil::printf("%s-%d", stdCode, endTDate);

		auto it = _his_ordque_map.find(key);
		if (it == _his_ordque_map.end())
		{
			std::stringstream ss;
			ss << _base_dir << "his/orders/" << cInfo._exchg << "/" << endTDate << "/" << curCode << ".dsb";
			std::string filename = ss.str();
			if (!StdFile::exists(filename.c_str()))
				return NULL;

			HisOrdDtlBlockPair& hisBlkPair = _his_orddtl_map[key];
			StdFile::read_file_content(filename.c_str(), hisBlkPair._buffer);
			if (hisBlkPair._buffer.size() < sizeof(HisOrdDtlBlockV2))
			{
				WTSLogger::error("��ʷ���ί�������ļ�%s��СУ��ʧ��", filename.c_str());
				hisBlkPair._buffer.clear();
				return NULL;
			}

			HisOrdDtlBlockV2* tBlockV2 = (HisOrdDtlBlockV2*)hisBlkPair._buffer.c_str();

			if (hisBlkPair._buffer.size() != (sizeof(HisOrdDtlBlockV2) + tBlockV2->_size))
			{
				WTSLogger::error("��ʷ���ί�������ļ�%s��СУ��ʧ��", filename.c_str());
				return NULL;
			}

			//��Ҫ��ѹ
			std::string buf = WTSCmpHelper::uncompress_data(tBlockV2->_data, (uint32_t)tBlockV2->_size);

			//��ԭ����bufferֻ����һ��ͷ��,��������tick����׷�ӵ�β��
			hisBlkPair._buffer.resize(sizeof(HisOrdDtlBlock));
			hisBlkPair._buffer.append(buf);
			tBlockV2->_version = BLOCK_VERSION_RAW;

			hisBlkPair._block = (HisOrdDtlBlock*)hisBlkPair._buffer.c_str();
		}

		HisOrdDtlBlockPair& tBlkPair = _his_orddtl_map[key];
		if (tBlkPair._block == NULL)
			return NULL;

		HisOrdDtlBlock* tBlock = tBlkPair._block;

		uint32_t tcnt = (tBlkPair._buffer.size() - sizeof(HisOrdDtlBlock)) / sizeof(WTSOrdDtlStruct);
		if (tcnt <= 0)
			return NULL;

		WTSOrdDtlStruct* pItem = std::lower_bound(tBlock->_items, tBlock->_items + (tcnt - 1), eTick, [](const WTSOrdDtlStruct& a, const WTSOrdDtlStruct& b) {
			if (a.action_date != b.action_date)
				return a.action_date < b.action_date;
			else
				return a.action_time < b.action_time;
		});

		uint32_t eIdx = pItem - tBlock->_items;
		if (pItem->action_date > eTick.action_date || pItem->action_time >= eTick.action_time)
		{
			pItem--;
			eIdx--;
		}

		if (beginTDate != endTDate)
		{
			//�����ʼ�Ľ����պ͵�ǰ�Ľ����ղ�һ�£��򷵻�ȫ����tick����
			WTSOrdDtlSlice* slice = WTSOrdDtlSlice::create(stdCode, tBlock->_items, eIdx + 1);
			return slice;
		}
		else
		{
			//�����������ͬ���������ʼ��λ��
			pItem = std::lower_bound(tBlock->_items, tBlock->_items + eIdx, sTick, [](const WTSOrdDtlStruct& a, const WTSOrdDtlStruct& b) {
				if (a.action_date != b.action_date)
					return a.action_date < b.action_date;
				else
					return a.action_time < b.action_time;
			});

			uint32_t sIdx = pItem - tBlock->_items;
			WTSOrdDtlSlice* slice = WTSOrdDtlSlice::create(stdCode, tBlock->_items + sIdx, eIdx - sIdx + 1);
			return slice;
		}
	}
}

WTSTransSlice* WtDataReader::readTransSliceByRange(const char* stdCode, uint64_t stime, uint64_t etime /* = 0 */)
{
	CodeHelper::CodeInfo cInfo;
	CodeHelper::extractStdCode(stdCode, cInfo);
	std::string stdPID = StrUtil::printf("%s.%s", cInfo._exchg, cInfo._product);

	uint32_t rDate, rTime, rSecs;
	//20190807124533900
	rDate = (uint32_t)(etime / 1000000000);
	rTime = (uint32_t)(etime % 1000000000) / 100000;
	rSecs = (uint32_t)(etime % 100000);

	uint32_t lDate, lTime, lSecs;
	//20190807124533900
	lDate = (uint32_t)(stime / 1000000000);
	lTime = (uint32_t)(stime % 1000000000) / 100000;
	lSecs = (uint32_t)(stime % 100000);

	uint32_t endTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), rDate, rTime, false);
	uint32_t beginTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), lDate, lTime, false);
	uint32_t curTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), 0, 0, false);

	bool isToday = (endTDate == curTDate);

	std::string curCode = cInfo._code;
	if (cInfo.isHot() && cInfo._category == CC_Future)
		curCode = _hot_mgr->getRawCode(cInfo._exchg, cInfo._product, endTDate);
	else if (cInfo.isSecond() && cInfo._category == CC_Future)
		curCode = _hot_mgr->getSecondRawCode(cInfo._exchg, cInfo._product, endTDate);

	//�Ƚ�ʱ��Ķ���
	WTSTransStruct eTick;
	eTick.action_date = rDate;
	eTick.action_time = rTime * 100000 + rSecs;

	WTSTransStruct sTick;
	sTick.action_date = lDate;
	sTick.action_time = lTime * 100000 + lSecs;

	if (isToday)
	{
		TransBlockPair* tPair = getRTTransBlock(cInfo._exchg, curCode.c_str());
		if (tPair == NULL)
			return NULL;

		RTTransBlock* rtBlock = tPair->_block;

		WTSTransStruct* pItem = std::lower_bound(rtBlock->_trans, rtBlock->_trans + (rtBlock->_size - 1), eTick, [](const WTSTransStruct& a, const WTSTransStruct& b) {
			if (a.action_date != b.action_date)
				return a.action_date < b.action_date;
			else
				return a.action_time < b.action_time;
		});

		uint32_t eIdx = pItem - rtBlock->_trans;

		//�����궨λ��tickʱ���Ŀ��ʱ���, ��ȫ������һ��
		if (pItem->action_date > eTick.action_date || pItem->action_time > eTick.action_time)
		{
			pItem--;
			eIdx--;
		}

		if (beginTDate != endTDate)
		{
			//�����ʼ�Ľ����պ͵�ǰ�Ľ����ղ�һ�£��򷵻�ȫ����tick����
			WTSTransSlice* slice = WTSTransSlice::create(stdCode, rtBlock->_trans, eIdx + 1);
			return slice;
		}
		else
		{
			//�����������ͬ���������ʼ��λ��
			pItem = std::lower_bound(rtBlock->_trans, rtBlock->_trans + eIdx, sTick, [](const WTSTransStruct& a, const WTSTransStruct& b) {
				if (a.action_date != b.action_date)
					return a.action_date < b.action_date;
				else
					return a.action_time < b.action_time;
			});

			uint32_t sIdx = pItem - rtBlock->_trans;
			WTSTransSlice* slice = WTSTransSlice::create(stdCode, rtBlock->_trans + sIdx, eIdx - sIdx + 1);
			return slice;
		}
	}
	else
	{
		std::string key = StrUtil::printf("%s-%d", stdCode, endTDate);

		auto it = _his_ordque_map.find(key);
		if (it == _his_ordque_map.end())
		{
			std::stringstream ss;
			ss << _base_dir << "his/trans/" << cInfo._exchg << "/" << endTDate << "/" << curCode << ".dsb";
			std::string filename = ss.str();
			if (!StdFile::exists(filename.c_str()))
				return NULL;

			HisTransBlockPair& hisBlkPair = _his_trans_map[key];
			StdFile::read_file_content(filename.c_str(), hisBlkPair._buffer);
			if (hisBlkPair._buffer.size() < sizeof(HisTransBlockV2))
			{
				WTSLogger::error("��ʷ��ʳɽ������ļ�%s��СУ��ʧ��", filename.c_str());
				hisBlkPair._buffer.clear();
				return NULL;
			}

			HisTransBlockV2* tBlockV2 = (HisTransBlockV2*)hisBlkPair._buffer.c_str();

			if (hisBlkPair._buffer.size() != (sizeof(HisTransBlockV2) + tBlockV2->_size))
			{
				WTSLogger::error("��ʷ��ʳɽ������ļ�%s��СУ��ʧ��", filename.c_str());
				return NULL;
			}

			//��Ҫ��ѹ
			std::string buf = WTSCmpHelper::uncompress_data(tBlockV2->_data, (uint32_t)tBlockV2->_size);

			//��ԭ����bufferֻ����һ��ͷ��,��������tick����׷�ӵ�β��
			hisBlkPair._buffer.resize(sizeof(HisTransBlock));
			hisBlkPair._buffer.append(buf);
			tBlockV2->_version = BLOCK_VERSION_RAW;

			hisBlkPair._block = (HisTransBlock*)hisBlkPair._buffer.c_str();
		}

		HisTransBlockPair& tBlkPair = _his_trans_map[key];
		if (tBlkPair._block == NULL)
			return NULL;

		HisTransBlock* tBlock = tBlkPair._block;

		uint32_t tcnt = (tBlkPair._buffer.size() - sizeof(HisTransBlock)) / sizeof(WTSTransStruct);
		if (tcnt <= 0)
			return NULL;

		WTSTransStruct* pItem = std::lower_bound(tBlock->_items, tBlock->_items + (tcnt - 1), eTick, [](const WTSTransStruct& a, const WTSTransStruct& b) {
			if (a.action_date != b.action_date)
				return a.action_date < b.action_date;
			else
				return a.action_time < b.action_time;
		});

		uint32_t eIdx = pItem - tBlock->_items;
		if (pItem->action_date > eTick.action_date || pItem->action_time >= eTick.action_time)
		{
			pItem--;
			eIdx--;
		}

		if (beginTDate != endTDate)
		{
			//�����ʼ�Ľ����պ͵�ǰ�Ľ����ղ�һ�£��򷵻�ȫ����tick����
			WTSTransSlice* slice = WTSTransSlice::create(stdCode, tBlock->_items, eIdx + 1);
			return slice;
		}
		else
		{
			//�����������ͬ���������ʼ��λ��
			pItem = std::lower_bound(tBlock->_items, tBlock->_items + eIdx, sTick, [](const WTSTransStruct& a, const WTSTransStruct& b) {
				if (a.action_date != b.action_date)
					return a.action_date < b.action_date;
				else
					return a.action_time < b.action_time;
			});

			uint32_t sIdx = pItem - tBlock->_items;
			WTSTransSlice* slice = WTSTransSlice::create(stdCode, tBlock->_items + sIdx, eIdx - sIdx + 1);
			return slice;
		}
	}
}

bool WtDataReader::cacheHisBarsFromFile(const std::string& key, const char* stdCode, WTSKlinePeriod period)
{
	CodeHelper::CodeInfo cInfo;
	CodeHelper::extractStdCode(stdCode, cInfo);
	std::string stdPID = StrUtil::printf("%s.%s", cInfo._exchg, cInfo._product);

	uint32_t curDate = TimeUtils::getCurDate();
	uint32_t curTime = TimeUtils::getCurMin() / 100;

	uint32_t endTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), curDate, curTime, false);

	std::string pname;
	switch (period)
	{
	case KP_Minute1: pname = "min1"; break;
	case KP_Minute5: pname = "min5"; break;
	default: pname = "day"; break;
	}

	BarsList& barList = _bars_cache[key];
	barList._code = stdCode;
	barList._period = period;
	barList._exchg = cInfo._exchg;

	std::vector<std::vector<WTSBarStruct>*> barsSections;

	uint32_t realCnt = 0;
	if (!cInfo.isFlat() && cInfo._category == CC_Future)//����Ƕ�ȡ�ڻ�������������
	{
		const char* hot_flag = cInfo.isHot() ? "HOT" : "2ND";

		//�Ȱ���HOT������ж�ȡ, ��rb.HOT
		std::vector<WTSBarStruct>* hotAy = NULL;
		uint32_t lastHotTime = 0;
		for (;;)
		{
			std::stringstream ss;
			ss << _base_dir << "his/" << pname << "/" << cInfo._exchg << "/" << cInfo._exchg << "." << cInfo._product << "_" << hot_flag << ".dsb";
			std::string filename = ss.str();
			if (!StdFile::exists(filename.c_str()))
				break;

			std::string content;
			StdFile::read_file_content(filename.c_str(), content);
			if (content.size() < sizeof(HisKlineBlock))
			{
				WTSLogger::error("Sizechecking of his kline data file %s failed", filename.c_str());
				break;
			}

			HisKlineBlock* kBlock = (HisKlineBlock*)content.c_str();
			uint32_t barcnt = 0;
			if(kBlock->_version == BLOCK_VERSION_CMP)
			{
				if (content.size() < sizeof(HisKlineBlockV2))
				{
					WTSLogger::error("Sizechecking of his kline data file %s failed", filename.c_str());
					break;
				}

				HisKlineBlockV2* kBlockV2 = (HisKlineBlockV2*)content.c_str();
				if (kBlockV2->_size == 0)
					break;

				std::string rawData = WTSCmpHelper::uncompress_data(kBlockV2->_data, (uint32_t)kBlockV2->_size);
				barcnt = rawData.size() / sizeof(WTSBarStruct);
				if (barcnt <= 0)
					break;

				hotAy = new std::vector<WTSBarStruct>();
				hotAy->resize(barcnt);
				memcpy(hotAy->data(), rawData.data(), rawData.size());	
			}
			else
			{
				barcnt = (content.size() - sizeof(HisKlineBlock)) / sizeof(WTSBarStruct);
				if (barcnt <= 0)
					break;

				HisKlineBlock* kBlock = (HisKlineBlock*)content.c_str();
				hotAy = new std::vector<WTSBarStruct>();
				hotAy->resize(barcnt);
				memcpy(hotAy->data(), kBlock->_bars, sizeof(WTSBarStruct)*barcnt);
			}		

			if (period != KP_DAY)
				lastHotTime = hotAy->at(barcnt - 1).time;
			else
				lastHotTime = hotAy->at(barcnt - 1).date;

			WTSLogger::info("%u items of back %s data of hot contract %s directly loaded", barcnt, pname.c_str(), stdCode);
			break;
		}

		HotSections secs;
		if (cInfo.isHot())
		{
			if (!_hot_mgr->splitHotSecions(cInfo._exchg, cInfo._product, 19900102, endTDate, secs))
				return false;
		}
		else if (cInfo.isSecond())
		{
			if (!_hot_mgr->splitSecondSecions(cInfo._exchg, cInfo._product, 19900102, endTDate, secs))
				return false;
		}

		if (secs.empty())
			return false;

		bool bAllCovered = false;
		for (auto it = secs.rbegin(); it != secs.rend() && left > 0; it++)
		{
			//const char* curCode = it->first.c_str();
			//uint32_t rightDt = it->second.second;
			//uint32_t leftDt = it->second.first;
			const HotSection& hotSec = *it;
			const char* curCode = hotSec._code.c_str();
			uint32_t rightDt = hotSec._e_date;
			uint32_t leftDt = hotSec._s_date;

			//Ҫ�Ƚ�����ת��Ϊ�߽�ʱ��
			WTSBarStruct sBar, eBar;
			if (period != KP_DAY)
			{
				uint64_t sTime = _base_data_mgr->getBoundaryTime(stdPID.c_str(), leftDt, false, true);
				uint64_t eTime = _base_data_mgr->getBoundaryTime(stdPID.c_str(), rightDt, false, false);

				sBar.date = leftDt;
				sBar.time = ((uint32_t)(sTime / 10000) - 19900000) * 10000 + (uint32_t)(sTime % 10000);

				if(sBar.time < lastHotTime)	//����߽�ʱ��С�����������һ��Bar��ʱ��, ˵���Ѿ��н�����, ����Ҫ�ٴ�����
				{
					bAllCovered = true;
					sBar.time = lastHotTime + 1;
				}

				eBar.date = rightDt;
				eBar.time = ((uint32_t)(eTime / 10000) - 19900000) * 10000 + (uint32_t)(eTime % 10000);

				if (eBar.time <= lastHotTime)	//�ұ߽�ʱ��С�����һ��Hotʱ��, ˵��ȫ��������, û�����ҵı�Ҫ��
					break;
			}
			else
			{
				sBar.date = leftDt;
				if (sBar.date < lastHotTime)	//����߽�ʱ��С�����������һ��Bar��ʱ��, ˵���Ѿ��н�����, ����Ҫ�ٴ�����
				{
					bAllCovered = true;
					sBar.date = lastHotTime + 1;
				}

				eBar.date = rightDt;

				if (eBar.date <= lastHotTime)
					break;
			}

			std::stringstream ss;
			ss << _base_dir << "his/" << pname << "/" << cInfo._exchg << "/" << curCode << ".dsb";
			std::string filename = ss.str();
			if (!StdFile::exists(filename.c_str()))
				continue;

			{
				std::string content;
				StdFile::read_file_content(filename.c_str(), content);
				if (content.size() < sizeof(HisKlineBlock))
				{
					WTSLogger::error("Sizechecking of his kline data file %s failed", filename.c_str());
					return false;
				}

				HisKlineBlock* kBlock = (HisKlineBlock*)content.c_str();
				WTSBarStruct* firstBar = NULL;
				uint32_t barcnt = 0;
				std::string rawData;
				if(kBlock->_version == BLOCK_VERSION_CMP)
				{
					if (content.size() < sizeof(HisKlineBlockV2))
					{
						WTSLogger::error("Sizechecking of his kline data file %s failed", filename.c_str());
						break;
					}

					HisKlineBlockV2* kBlockV2 = (HisKlineBlockV2*)content.c_str();
					if (kBlockV2->_size == 0)
						break;

					rawData = WTSCmpHelper::uncompress_data(kBlockV2->_data, (uint32_t)kBlockV2->_size);
					barcnt = rawData.size() / sizeof(WTSBarStruct);
					if (barcnt <= 0)
						break;

					firstBar = (WTSBarStruct*)rawData.data();
				}
				else
				{
					barcnt = (content.size() - sizeof(HisKlineBlock)) / sizeof(WTSBarStruct);
					if (barcnt <= 0)
						continue;

					firstBar = kBlock->_bars;
				}

				WTSBarStruct* pBar = std::lower_bound(firstBar, firstBar + (barcnt - 1), sBar, [period](const WTSBarStruct& a, const WTSBarStruct& b){
					if (period == KP_DAY)
					{
						return a.date < b.date;
					}
					else
					{
						return a.time < b.time;
					}
				});

				uint32_t sIdx = pBar - firstBar;
				if ((period == KP_DAY && pBar->date < sBar.date) || (period != KP_DAY && pBar->time < sBar.time))	//���ڱ߽�ʱ��
				{
					//���ڱ߽�ʱ��, ˵��û��������, ��Ϊlower_bound�᷵�ش��ڵ���Ŀ��λ�õ�����
					continue;
				}

				pBar = std::lower_bound(firstBar + sIdx, firstBar + (barcnt - 1), eBar, [period](const WTSBarStruct& a, const WTSBarStruct& b){
					if (period == KP_DAY)
					{
						return a.date < b.date;
					}
					else
					{
						return a.time < b.time;
					}
				});
				uint32_t eIdx = pBar - firstBar;
				if ((period == KP_DAY && pBar->date > eBar.date) || (period != KP_DAY && pBar->time > eBar.time))
				{
					pBar--;
					eIdx--;
				}

				if (eIdx < sIdx)
					continue;

				uint32_t curCnt = eIdx - sIdx + 1;
				std::vector<WTSBarStruct>* tempAy = new std::vector<WTSBarStruct>();
				tempAy->resize(curCnt);
				memcpy(tempAy->data(), &firstBar[sIdx], sizeof(WTSBarStruct)*curCnt);
				realCnt += curCnt;

				barsSections.emplace_back(tempAy);

				if(bAllCovered)
					break;
			}
		}

		if (hotAy)
		{
			barsSections.emplace_back(hotAy);
			realCnt += hotAy->size();
		}
	}
	else if(cInfo.isExright() && cInfo._category == CC_Stock)//����Ƕ�ȡ��Ʊ��Ȩ����
	{
		std::vector<WTSBarStruct>* hotAy = NULL;
		uint32_t lastQTime = 0;
		
		do
		{
			//��ֱ�Ӷ�ȡ��Ȩ������ʷ����,·����/his/day/sse/SH600000Q.dsb
			char flag = cInfo._exright == 1 ? 'Q' : 'H';
			std::stringstream ss;
			ss << _base_dir << "his/" << pname << "/" << cInfo._exchg << "/" << cInfo._code << flag << ".dsb";
			std::string filename = ss.str();
			if (!StdFile::exists(filename.c_str()))
				break;

			std::string content;
			StdFile::read_file_content(filename.c_str(), content);
			if (content.size() < sizeof(HisKlineBlock))
			{
				WTSLogger::error("Sizechecking of his kline data file %s failed", filename.c_str());
				break;
			}

			HisKlineBlock* kBlock = (HisKlineBlock*)content.c_str();
			uint32_t barcnt = 0;
			if (kBlock->_version == BLOCK_VERSION_CMP)
			{
				if (content.size() < sizeof(HisKlineBlockV2))
				{
					WTSLogger::error("Sizechecking of his kline data file %s failed", filename.c_str());
					break;
				}

				HisKlineBlockV2* kBlockV2 = (HisKlineBlockV2*)content.c_str();
				if (kBlockV2->_size == 0)
					break;

				std::string rawData = WTSCmpHelper::uncompress_data(kBlockV2->_data, (uint32_t)kBlockV2->_size);
				barcnt = rawData.size() / sizeof(WTSBarStruct);
				if (barcnt <= 0)
					break;

				hotAy = new std::vector<WTSBarStruct>();
				hotAy->resize(barcnt);
				memcpy(hotAy->data(), rawData.data(), rawData.size());
			}
			else
			{
				barcnt = (content.size() - sizeof(HisKlineBlock)) / sizeof(WTSBarStruct);
				if (barcnt <= 0)
					break;

				HisKlineBlock* kBlock = (HisKlineBlock*)content.c_str();
				hotAy = new std::vector<WTSBarStruct>();
				hotAy->resize(barcnt);
				memcpy(hotAy->data(), kBlock->_bars, sizeof(WTSBarStruct)*barcnt);
			}

			if (period != KP_DAY)
				lastQTime = hotAy->at(barcnt - 1).time;
			else
				lastQTime = hotAy->at(barcnt - 1).date;

			WTSLogger::info("%u history exrighted %s data of %s directly cached", barcnt, pname.c_str(), stdCode);
			break;
		} while (false);

		bool bAllCovered = false;
		do
		{
			//const char* curCode = it->first.c_str();
			//uint32_t rightDt = it->second.second;
			//uint32_t leftDt = it->second.first;
			const char* curCode = cInfo._code;

			//Ҫ�Ƚ�����ת��Ϊ�߽�ʱ��
			WTSBarStruct sBar;
			if (period != KP_DAY)
			{
				sBar.date = TimeUtils::minBarToDate(lastQTime);

				sBar.time = lastQTime + 1;
			}
			else
			{
				sBar.date = lastQTime + 1;
			}

			std::stringstream ss;
			ss << _base_dir << "his/" << pname << "/" << cInfo._exchg << "/" << curCode << ".dsb";
			std::string filename = ss.str();
			if (!StdFile::exists(filename.c_str()))
				continue;

			{
				std::string content;
				StdFile::read_file_content(filename.c_str(), content);
				if (content.size() < sizeof(HisKlineBlock))
				{
					WTSLogger::error("Sizechecking of his kline data file %s failed", filename.c_str());
					return false;
				}

				HisKlineBlock* kBlock = (HisKlineBlock*)content.c_str();
				WTSBarStruct* firstBar = NULL;
				uint32_t barcnt = 0;
				std::string rawData;
				if (kBlock->_version == BLOCK_VERSION_CMP)
				{
					if (content.size() < sizeof(HisKlineBlockV2))
					{
						WTSLogger::error("Sizechecking of his kline data file %s failed", filename.c_str());
						break;
					}

					HisKlineBlockV2* kBlockV2 = (HisKlineBlockV2*)content.c_str();
					if (kBlockV2->_size == 0)
						break;

					rawData = WTSCmpHelper::uncompress_data(kBlockV2->_data, (uint32_t)kBlockV2->_size);
					barcnt = rawData.size() / sizeof(WTSBarStruct);
					if (barcnt <= 0)
						break;

					firstBar = (WTSBarStruct*)rawData.data();
				}
				else
				{
					barcnt = (content.size() - sizeof(HisKlineBlock)) / sizeof(WTSBarStruct);
					if (barcnt <= 0)
						continue;

					firstBar = kBlock->_bars;
				}

				WTSBarStruct* pBar = std::lower_bound(firstBar, firstBar + (barcnt - 1), sBar, [period](const WTSBarStruct& a, const WTSBarStruct& b){
					if (period == KP_DAY)
					{
						return a.date < b.date;
					}
					else
					{
						return a.time < b.time;
					}
				});

				if(pBar != NULL)
				{
					uint32_t sIdx = pBar - firstBar;
					uint32_t curCnt = barcnt - sIdx;
					std::vector<WTSBarStruct>* tempAy = new std::vector<WTSBarStruct>();
					tempAy->resize(curCnt);
					memcpy(tempAy->data(), &firstBar[sIdx], sizeof(WTSBarStruct)*curCnt);
					realCnt += curCnt;

					auto& ayFactors = getAdjFactors(cInfo._code, cInfo._exchg);
					if(!ayFactors.empty())
					{
						//��ǰ��Ȩ����
						int32_t lastIdx = curCnt;
						WTSBarStruct bar;
						firstBar = tempAy->data();
						for (auto& adjFact : ayFactors)
						{
							bar.date = adjFact._date;
							double factor = adjFact._factor;

							WTSBarStruct* pBar = NULL;
							pBar = std::lower_bound(firstBar, firstBar + lastIdx - 1, bar, [period](const WTSBarStruct& a, const WTSBarStruct& b) {
								return a.date < b.date;
							});

							if (pBar->date < bar.date)
								continue;

							WTSBarStruct* endBar = pBar;
							if (pBar != NULL)
							{
								int32_t curIdx = pBar - firstBar;
								while (pBar && curIdx < lastIdx)
								{
									pBar->open /= factor;
									pBar->high /= factor;
									pBar->low /= factor;
									pBar->close /= factor;

									pBar++;
									curIdx++;
								}
								lastIdx = endBar - firstBar;
							}

							if (lastIdx == 0)
								break;
						}
					}

					barsSections.emplace_back(tempAy);
				}
			}
		} while (false);

		if (hotAy)
		{
			barsSections.emplace_back(hotAy);
			realCnt += hotAy->size();
		}
	}
	else
	{
		//��ȡ��ʷ��
		std::stringstream ss;
		ss << _base_dir << "his/" << pname << "/" << cInfo._exchg << "/" << cInfo._code << ".dsb";
		std::string filename = ss.str();
		if (StdFile::exists(filename.c_str()))
		{
			//����и�ʽ������ʷ�����ļ�, ��ֱ�Ӷ�ȡ
			std::string content;
			StdFile::read_file_content(filename.c_str(), content);
			if (content.size() < sizeof(HisKlineBlock))
			{
				WTSLogger::error("Sizechecking of his kline data file %s failed", filename.c_str());
				return false;
			}

			HisKlineBlock* kBlock = (HisKlineBlock*)content.c_str();
			WTSBarStruct* firstBar = NULL;
			uint32_t barcnt = 0;
			std::string rawData;
			if (kBlock->_version == BLOCK_VERSION_CMP)
			{
				if (content.size() < sizeof(HisKlineBlockV2))
				{
					WTSLogger::error("Sizechecking of his kline data file %s failed", filename.c_str());
					return false;
				}

				HisKlineBlockV2* kBlockV2 = (HisKlineBlockV2*)content.c_str();
				if (kBlockV2->_size == 0)
					return false;

				rawData = WTSCmpHelper::uncompress_data(kBlockV2->_data, (uint32_t)kBlockV2->_size);
				barcnt = rawData.size() / sizeof(WTSBarStruct);
				if (barcnt <= 0)
					return false;

				firstBar = (WTSBarStruct*)rawData.data();
			}
			else
			{
				barcnt = (content.size() - sizeof(HisKlineBlock)) / sizeof(WTSBarStruct);
				if (barcnt <= 0)
					return false;

				firstBar = kBlock->_bars;
			}

			if (barcnt > 0)
			{
				
				uint32_t sIdx = 0;
				uint32_t idx = barcnt - 1;
				uint32_t curCnt = (idx - sIdx + 1);

				std::vector<WTSBarStruct>* tempAy = new std::vector<WTSBarStruct>();
				tempAy->resize(curCnt);
				memcpy(tempAy->data(), &firstBar[sIdx], sizeof(WTSBarStruct)*curCnt);
				realCnt += curCnt;

				barsSections.emplace_back(tempAy);
			}
		}
	}

	if (realCnt > 0)
	{
		barList._bars.resize(realCnt);

		uint32_t curIdx = 0;
		for (auto it = barsSections.rbegin(); it != barsSections.rend(); it++)
		{
			std::vector<WTSBarStruct>* tempAy = *it;
			memcpy(barList._bars.data() + curIdx, tempAy->data(), tempAy->size()*sizeof(WTSBarStruct));
			curIdx += tempAy->size();
			delete tempAy;
		}
		barsSections.clear();
	}

	WTSLogger::info("%u history %s data of %s cached", realCnt, pname.c_str(), stdCode);
	return true;
}

WTSBarStruct* WtDataReader::indexBarFromCacheByRange(const std::string& key, uint64_t stime, uint64_t etime, uint32_t& count, bool isDay /* = false */)
{
	uint32_t rDate, rTime, lDate, lTime;
	rDate = (uint32_t)(etime / 10000);
	rTime = (uint32_t)(etime % 10000);
	lDate = (uint32_t)(stime / 10000);
	lTime = (uint32_t)(stime % 10000);

	BarsList& barsList = _bars_cache[key];
	if (barsList._bars.empty())
		return NULL;
	
	uint32_t eIdx,sIdx;
	{
		//�����δ��ʼ��, ��Ҫ���¶�λ
		uint64_t nowTime = (uint64_t)rDate * 10000 + rTime;

		WTSBarStruct eBar;
		eBar.date = rDate;
		eBar.time = (rDate - 19900000) * 10000 + rTime;

		WTSBarStruct sBar;
		sBar.date = lDate;
		sBar.time = (lDate - 19900000) * 10000 + lTime;

		auto eit = std::lower_bound(barsList._bars.begin(), barsList._bars.end(), eBar, [isDay](const WTSBarStruct& a, const WTSBarStruct& b){
			if (isDay)
				return a.date < b.date;
			else
				return a.time < b.time;
		});


		if (eit == barsList._bars.end())
			eIdx = barsList._bars.size() - 1;
		else
		{
			if ((isDay && eit->date > eBar.date) || (!isDay && eit->time > eBar.time))
			{
				eit--;
			}

			eIdx = eit - barsList._bars.begin();
		}

		auto sit = std::lower_bound(barsList._bars.begin(), eit, sBar, [isDay](const WTSBarStruct& a, const WTSBarStruct& b) {
			if (isDay)
				return a.date < b.date;
			else
				return a.time < b.time;
		});
		sIdx = sit - barsList._bars.begin();
	}

	uint32_t curCnt = eIdx - sIdx + 1;
	count = curCnt;
	return &barsList._bars[sIdx];
}

WTSBarStruct* WtDataReader::indexBarFromCacheByCount(const std::string& key, uint64_t etime, uint32_t& count, bool isDay /* = false */)
{
	uint32_t rDate, rTime;
	rDate = (uint32_t)(etime / 10000);
	rTime = (uint32_t)(etime % 10000);

	BarsList& barsList = _bars_cache[key];
	if (barsList._bars.empty())
		return NULL;

	uint32_t eIdx, sIdx;
	WTSBarStruct eBar;
	eBar.date = rDate;
	eBar.time = (rDate - 19900000) * 10000 + rTime;

	auto eit = std::lower_bound(barsList._bars.begin(), barsList._bars.end(), eBar, [isDay](const WTSBarStruct& a, const WTSBarStruct& b) {
		if (isDay)
			return a.date < b.date;
		else
			return a.time < b.time;
	});


	if (eit == barsList._bars.end())
		eIdx = barsList._bars.size() - 1;
	else
	{
		if ((isDay && eit->date > eBar.date) || (!isDay && eit->time > eBar.time))
		{
			eit--;
		}

		eIdx = eit - barsList._bars.begin();
	}

	uint32_t curCnt = min(eIdx + 1, count);
	sIdx = eIdx + 1 - curCnt;
	count = curCnt;
	return &barsList._bars[sIdx];
}

uint32_t WtDataReader::readBarsFromCacheByRange(const std::string& key, uint64_t stime, uint64_t etime, std::vector<WTSBarStruct>& ayBars, bool isDay /* = false */)
{
	uint32_t rDate, rTime, lDate, lTime;
	rDate = (uint32_t)(etime / 10000);
	rTime = (uint32_t)(etime % 10000);
	lDate = (uint32_t)(stime / 10000);
	lTime = (uint32_t)(stime % 10000);

	BarsList& barsList = _bars_cache[key];
	uint32_t eIdx,sIdx;
	{
		WTSBarStruct eBar;
		eBar.date = rDate;
		eBar.time = (rDate - 19900000) * 10000 + rTime;

		WTSBarStruct sBar;
		sBar.date = lDate;
		sBar.time = (lDate - 19900000) * 10000 + lTime;

		auto eit = std::lower_bound(barsList._bars.begin(), barsList._bars.end(), eBar, [isDay](const WTSBarStruct& a, const WTSBarStruct& b){
			if (isDay)
				return a.date < b.date;
			else
				return a.time < b.time;
		});
		

		if(eit == barsList._bars.end())
			eIdx = barsList._bars.size() - 1;
		else
		{
			if ((isDay && eit->date > eBar.date) || (!isDay && eit->time > eBar.time))
			{
				if (eit == barsList._bars.begin())
					return 0;
				
				eit--;
			}

			eIdx = eit - barsList._bars.begin();
		}

		auto sit = std::lower_bound(barsList._bars.begin(), eit, sBar, [isDay](const WTSBarStruct& a, const WTSBarStruct& b) {
			if (isDay)
				return a.date < b.date;
			else
				return a.time < b.time;
		});
		sIdx = sit - barsList._bars.begin();
	}

	uint32_t curCnt = eIdx - sIdx + 1;
	if(curCnt > 0)
	{
		ayBars.resize(curCnt);
		memcpy(ayBars.data(), &barsList._bars[sIdx], sizeof(WTSBarStruct)*curCnt);
	}
	return curCnt;
}

WTSKlineSlice* WtDataReader::readKlineSliceByRange(const char* stdCode, WTSKlinePeriod period, uint64_t stime, uint64_t etime /* = 0 */)
{
	CodeHelper::CodeInfo cInfo;
	CodeHelper::extractStdCode(stdCode, cInfo);
	std::string stdPID = StrUtil::printf("%s.%s", cInfo._exchg, cInfo._product);

	std::string key = StrUtil::printf("%s#%u", stdCode, period);
	auto it = _bars_cache.find(key);
	bool bHasHisData = false;
	if (it == _bars_cache.end())
	{
		bHasHisData = cacheHisBarsFromFile(key, stdCode, period);
	}
	else
	{
		bHasHisData = true;
	}

	if (etime == 0)
		etime = 203012312359;

	uint32_t rDate, rTime, lDate, lTime;
	rDate = (uint32_t)(etime / 10000);
	rTime = (uint32_t)(etime % 10000);
	lDate = (uint32_t)(stime / 10000);
	lTime = (uint32_t)(stime % 10000);

	uint32_t endTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), rDate, rTime, false);
	uint32_t curTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), 0, 0, false);
	
	WTSBarStruct* hisHead = NULL;
	WTSBarStruct* rtHead = NULL;
	uint32_t hisCnt = 0;
	uint32_t rtCnt = 0;

	std::string pname;
	switch (period)
	{
	case KP_Minute1: pname = "min1"; break;
	case KP_Minute5: pname = "min5"; break;
	default: pname = "day"; break;
	}

	bool isDay = period == KP_DAY;

	//�Ƿ���������
	bool bHasToday = (endTDate >= curTDate);
	std::string raw_code = cInfo._code;

	if (cInfo.isHot() && cInfo._category == CC_Future)
	{
		raw_code = _hot_mgr->getRawCode(cInfo._exchg, cInfo._product, curTDate);
		WTSLogger::info( "Hot contract of %u confirmed: %s -> %s", curTDate, stdCode, raw_code.c_str());
	}
	else if (cInfo.isSecond() && cInfo._category == CC_Future)
	{
		raw_code = _hot_mgr->getSecondRawCode(cInfo._exchg, cInfo._product, curTDate);
		WTSLogger::info( "Second contract of %u confirmed: %s -> %s", curTDate, stdCode, raw_code.c_str());
	}
	else
	{
		raw_code = cInfo._code;
	}

	WTSBarStruct eBar;
	eBar.date = rDate;
	eBar.time = (rDate - 19900000) * 10000 + rTime;

	WTSBarStruct sBar;
	sBar.date = lDate;
	sBar.time = (lDate - 19900000) * 10000 + lTime;

	bool bNeedHisData = true;

	if (bHasToday)
	{
		const char* curCode = raw_code.c_str();

		//��ȡʵʱ��
		RTKlineBlockPair* kPair = getRTKilneBlock(cInfo._exchg, curCode, period);
		if (kPair != NULL)
		{
			StdUniqueLock lock(*kPair->_mtx);
			//��ȡ���յ�����
			WTSBarStruct* pBar = std::lower_bound(kPair->_block->_bars, kPair->_block->_bars + (kPair->_block->_size - 1), eBar, [isDay](const WTSBarStruct& a, const WTSBarStruct& b){
				if (isDay)
					return a.date < b.date;
				else
					return a.time < b.time;
			});
			uint32_t idx = pBar - kPair->_block->_bars;
			if ((isDay && pBar->date > eBar.date) || (!isDay && pBar->time > eBar.time))
			{
				pBar--;
				idx--;
			}

			pBar = &kPair->_block->_bars[0];
			//�����һ��ʵʱK�ߵ�ʱ����ڿ�ʼ���ڣ���ʵʱK��Ҫȫ��������ȥ
			if ((isDay && pBar->date > sBar.date) || (!isDay && pBar->time > sBar.time))
			{
				rtHead = &kPair->_block->_bars[0];
				rtCnt = idx+1;
			}
			else
			{
				pBar = std::lower_bound(kPair->_block->_bars, kPair->_block->_bars + idx, sBar, [isDay](const WTSBarStruct& a, const WTSBarStruct& b) {
					if (isDay)
						return a.date < b.date;
					else
						return a.time < b.time;
				});

				uint32_t sIdx = pBar - kPair->_block->_bars;
				rtHead = pBar;
				rtCnt = idx - sIdx + 1;
				bNeedHisData = false;
			}
		}
	}

	if (bNeedHisData)
	{
		hisHead = indexBarFromCacheByRange(key, stime, etime, hisCnt, period == KP_DAY);
	}

	if (hisCnt + rtCnt > 0)
	{
		WTSKlineSlice* slice = WTSKlineSlice::create(stdCode, period, 1, hisHead, hisCnt, rtHead, rtCnt);
		return slice;
	}

	return NULL;
}


WtDataReader::TickBlockPair* WtDataReader::getRTTickBlock(const char* exchg, const char* code)
{
	std::string key = StrUtil::printf("%s.%s", exchg, code);

	std::string path = StrUtil::printf("%srt/ticks/%s/%s.dmb", _base_dir.c_str(), exchg, code);
	if (!StdFile::exists(path.c_str()))
		return NULL;

	TickBlockPair& block = _rt_tick_map[key];
	if (block._file == NULL || block._block == NULL)
	{
		if (block._file == NULL)
		{
			block._file.reset(new BoostMappingFile());
		}

		if (!block._file->map(path.c_str(), boost::interprocess::read_only, boost::interprocess::read_only))
			return NULL;

		block._block = (RTTickBlock*)block._file->addr();
		block._last_cap = block._block->_capacity;
	}
	else if (block._last_cap != block._block->_capacity)
	{
		//˵���ļ���С�ѱ�, ��Ҫ����ӳ��
		block._file.reset(new BoostMappingFile());
		block._last_cap = 0;
		block._block = NULL;

		if (!block._file->map(path.c_str(), boost::interprocess::read_only, boost::interprocess::read_only))
			return NULL;

		block._block = (RTTickBlock*)block._file->addr();
		block._last_cap = block._block->_capacity;
	}

	block._last_time = TimeUtils::getLocalTimeNow();
	return &block;
}

WtDataReader::OrdDtlBlockPair* WtDataReader::getRTOrdDtlBlock(const char* exchg, const char* code)
{
	std::string key = StrUtil::printf("%s.%s", exchg, code);

	std::string path = StrUtil::printf("%srt/orders/%s/%s.dmb", _base_dir.c_str(), exchg, code);
	if (!StdFile::exists(path.c_str()))
		return NULL;

	OrdDtlBlockPair& block = _rt_orddtl_map[key];
	if (block._file == NULL || block._block == NULL)
	{
		if (block._file == NULL)
		{
			block._file.reset(new BoostMappingFile());
		}

		if (!block._file->map(path.c_str(), boost::interprocess::read_only, boost::interprocess::read_only))
			return NULL;

		block._block = (RTOrdDtlBlock*)block._file->addr();
		block._last_cap = block._block->_capacity;
	}
	else if (block._last_cap != block._block->_capacity)
	{
		//˵���ļ���С�ѱ�, ��Ҫ����ӳ��
		block._file.reset(new BoostMappingFile());
		block._last_cap = 0;
		block._block = NULL;

		if (!block._file->map(path.c_str(), boost::interprocess::read_only, boost::interprocess::read_only))
			return NULL;

		block._block = (RTOrdDtlBlock*)block._file->addr();
		block._last_cap = block._block->_capacity;
	}

	block._last_time = TimeUtils::getLocalTimeNow();
	return &block;
}

WtDataReader::OrdQueBlockPair* WtDataReader::getRTOrdQueBlock(const char* exchg, const char* code)
{
	std::string key = StrUtil::printf("%s.%s", exchg, code);

	std::string path = StrUtil::printf("%srt/queue/%s/%s.dmb", _base_dir.c_str(), exchg, code);
	if (!StdFile::exists(path.c_str()))
		return NULL;

	OrdQueBlockPair& block = _rt_ordque_map[key];
	if (block._file == NULL || block._block == NULL)
	{
		if (block._file == NULL)
		{
			block._file.reset(new BoostMappingFile());
		}

		if (!block._file->map(path.c_str(), boost::interprocess::read_only, boost::interprocess::read_only))
			return NULL;

		block._block = (RTOrdQueBlock*)block._file->addr();
		block._last_cap = block._block->_capacity;
	}
	else if (block._last_cap != block._block->_capacity)
	{
		//˵���ļ���С�ѱ�, ��Ҫ����ӳ��
		block._file.reset(new BoostMappingFile());
		block._last_cap = 0;
		block._block = NULL;

		if (!block._file->map(path.c_str(), boost::interprocess::read_only, boost::interprocess::read_only))
			return NULL;

		block._block = (RTOrdQueBlock*)block._file->addr();
		block._last_cap = block._block->_capacity;
	}

	block._last_time = TimeUtils::getLocalTimeNow();
	return &block;
}

WtDataReader::TransBlockPair* WtDataReader::getRTTransBlock(const char* exchg, const char* code)
{
	std::string key = StrUtil::printf("%s.%s", exchg, code);

	std::string path = StrUtil::printf("%srt/trans/%s/%s.dmb", _base_dir.c_str(), exchg, code);
	if (!StdFile::exists(path.c_str()))
		return NULL;

	TransBlockPair& block = _rt_trans_map[key];
	if (block._file == NULL || block._block == NULL)
	{
		if (block._file == NULL)
		{
			block._file.reset(new BoostMappingFile());
		}

		if (!block._file->map(path.c_str(), boost::interprocess::read_only, boost::interprocess::read_only))
			return NULL;

		block._block = (RTTransBlock*)block._file->addr();
		block._last_cap = block._block->_capacity;
	}
	else if (block._last_cap != block._block->_capacity)
	{
		//˵���ļ���С�ѱ�, ��Ҫ����ӳ��
		block._file.reset(new BoostMappingFile());
		block._last_cap = 0;
		block._block = NULL;

		if (!block._file->map(path.c_str(), boost::interprocess::read_only, boost::interprocess::read_only))
			return NULL;

		block._block = (RTTransBlock*)block._file->addr();
		block._last_cap = block._block->_capacity;
	}

	block._last_time = TimeUtils::getLocalTimeNow();
	return &block;
}

WtDataReader::RTKlineBlockPair* WtDataReader::getRTKilneBlock(const char* exchg, const char* code, WTSKlinePeriod period)
{
	if (period != KP_Minute1 && period != KP_Minute5)
		return NULL;

	std::string key = StrUtil::printf("%s.%s", exchg, code);

	RTKBlockFilesMap* cache_map = NULL;
	std::string subdir = "";
	BlockType bType;
	switch (period)
	{
	case KP_Minute1:
		cache_map = &_rt_min1_map;
		subdir = "min1";
		bType = BT_RT_Minute1;
		break;
	case KP_Minute5:
		cache_map = &_rt_min5_map;
		subdir = "min5";
		bType = BT_RT_Minute5;
		break;
	default: break;
	}

	std::string path = StrUtil::printf("%srt/%s/%s/%s.dmb", _base_dir.c_str(), subdir.c_str(), exchg, code);
	if (!StdFile::exists(path.c_str()))
		return NULL;

	RTKlineBlockPair& block = (*cache_map)[key];
	if (block._file == NULL || block._block == NULL)
	{
		if (block._file == NULL)
		{
			block._file.reset(new BoostMappingFile());
		}

		if (!block._file->map(path.c_str(), boost::interprocess::read_only, boost::interprocess::read_only))
			return NULL;

		block._block = (RTKlineBlock*)block._file->addr();
		block._last_cap = block._block->_capacity;
	}
	else if (block._last_cap != block._block->_capacity)
	{
		//˵���ļ���С�ѱ�, ��Ҫ����ӳ��
		block._file.reset(new BoostMappingFile());
		block._last_cap = 0;
		block._block = NULL;

		if (!block._file->map(path.c_str(), boost::interprocess::read_only, boost::interprocess::read_only))
			return NULL;

		block._block = (RTKlineBlock*)block._file->addr();
		block._last_cap = block._block->_capacity;
	}

	block._last_time = TimeUtils::getLocalTimeNow();
	return &block;
}

WTSKlineSlice* WtDataReader::readKlineSliceByCount(const char* stdCode, WTSKlinePeriod period, uint32_t count, uint64_t etime /* = 0 */)
{
	CodeHelper::CodeInfo cInfo;
	CodeHelper::extractStdCode(stdCode, cInfo);
	std::string stdPID = StrUtil::printf("%s.%s", cInfo._exchg, cInfo._product);

	std::string key = StrUtil::printf("%s#%u", stdCode, period);
	auto it = _bars_cache.find(key);
	bool bHasHisData = false;
	if (it == _bars_cache.end())
	{
		bHasHisData = cacheHisBarsFromFile(key, stdCode, period);
	}
	else
	{
		bHasHisData = true;
	}

	if (etime == 0)
		etime = 203012312359;

	uint32_t rDate, rTime;
	rDate = (uint32_t)(etime / 10000);
	rTime = (uint32_t)(etime % 10000);

	uint32_t endTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), rDate, rTime, false);
	uint32_t curTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), 0, 0, false);

	WTSBarStruct* hisHead = NULL;
	WTSBarStruct* rtHead = NULL;
	uint32_t hisCnt = 0;
	uint32_t rtCnt = 0;

	std::string pname;
	switch (period)
	{
	case KP_Minute1: pname = "min1"; break;
	case KP_Minute5: pname = "min5"; break;
	default: pname = "day"; break;
	}

	bool isDay = period == KP_DAY;

	//�Ƿ���������
	bool bHasToday = (endTDate >= curTDate);
	std::string raw_code = cInfo._code;

	if (cInfo.isHot() && cInfo._category == CC_Future)
	{
		raw_code = _hot_mgr->getRawCode(cInfo._exchg, cInfo._product, curTDate);
		WTSLogger::info("Hot contract of %u confirmed: %s -> %s", curTDate, stdCode, raw_code.c_str());
	}
	else if (cInfo.isSecond() && cInfo._category == CC_Future)
	{
		raw_code = _hot_mgr->getSecondRawCode(cInfo._exchg, cInfo._product, curTDate);
		WTSLogger::info("Second contract of %u confirmed: %s -> %s", curTDate, stdCode, raw_code.c_str());
	}
	else
	{
		raw_code = cInfo._code;
	}

	WTSBarStruct eBar;
	eBar.date = rDate;
	eBar.time = (rDate - 19900000) * 10000 + rTime;


	bool bNeedHisData = true;

	if (bHasToday)
	{
		const char* curCode = raw_code.c_str();

		//��ȡʵʱ��
		RTKlineBlockPair* kPair = getRTKilneBlock(cInfo._exchg, curCode, period);
		if (kPair != NULL)
		{
			StdUniqueLock lock(*kPair->_mtx);
			//��ȡ���յ�����
			WTSBarStruct* pBar = std::lower_bound(kPair->_block->_bars, kPair->_block->_bars + (kPair->_block->_size - 1), eBar, [isDay](const WTSBarStruct& a, const WTSBarStruct& b) {
				if (isDay)
					return a.date < b.date;
				else
					return a.time < b.time;
			});
			uint32_t idx = pBar - kPair->_block->_bars;
			if ((isDay && pBar->date > eBar.date) || (!isDay && pBar->time > eBar.time))
			{
				pBar--;
				idx--;
			}

			//�����һ��ʵʱK�ߵ�ʱ����ڿ�ʼ���ڣ���ʵʱK��Ҫȫ��������ȥ
			rtCnt = min(idx + 1, count);
			uint32_t sIdx = idx + 1 - rtCnt;
			rtHead = kPair->_block->_bars + sIdx;
			bNeedHisData = (rtCnt < count);
		}
	}

	if (bNeedHisData)
	{
		hisCnt = count - rtCnt;
		hisHead = indexBarFromCacheByCount(key, etime, hisCnt, period == KP_DAY);
	}

	if (hisCnt + rtCnt > 0)
	{
		WTSKlineSlice* slice = WTSKlineSlice::create(stdCode, period, 1, hisHead, hisCnt, rtHead, rtCnt);
		return slice;
	}

	return NULL;
}

WTSArray* WtDataReader::readTickSlicesByCount(const char* stdCode, uint32_t count, uint64_t etime /* = 0 */)
{
	CodeHelper::CodeInfo cInfo;
	CodeHelper::extractStdCode(stdCode, cInfo);
	std::string stdPID = StrUtil::printf("%s.%s", cInfo._exchg, cInfo._product);

	WTSSessionInfo* sInfo = _base_data_mgr->getSession(_base_data_mgr->getCommodity(cInfo._exchg, cInfo._code)->getSession());

	uint32_t rDate, rTime, rSecs;
	//20190807124533900
	rDate = (uint32_t)(etime / 1000000000);
	rTime = (uint32_t)(etime % 1000000000) / 100000;
	rSecs = (uint32_t)(etime % 100000);


	uint32_t endTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), rDate, rTime, false);
	uint32_t curTDate = _base_data_mgr->calcTradingDate(stdPID.c_str(), 0, 0, false);

	bool hasToday = (endTDate >= curTDate);

	WTSArray* ayTicks = WTSArray::create();

	uint32_t left = count;
	while (hasToday)
	{
		std::string curCode = cInfo._code;
		if (cInfo.isHot() && cInfo._category == CC_Future)
		{
			curCode = _hot_mgr->getRawCode(cInfo._exchg, cInfo._product, curTDate);
			WTSLogger::info("Hot contract of %u confirmed: %s -> %s", curTDate, stdCode, curCode.c_str());
		}
		else if (cInfo.isSecond() && cInfo._category == CC_Future)
		{
			curCode = _hot_mgr->getSecondRawCode(cInfo._exchg, cInfo._product, curTDate);
			WTSLogger::info("Second contract of %u confirmed: %s -> %s", curTDate, stdCode, curCode.c_str());
		}

		TickBlockPair* tPair = getRTTickBlock(cInfo._exchg, curCode.c_str());
		if (tPair == NULL || tPair->_block->_size == 0)
			break;

		StdUniqueLock lock(*tPair->_mtx);
		RTTickBlock* tBlock = tPair->_block;
		WTSTickStruct eTick;
		if (curTDate == endTDate)
		{
			eTick.action_date = rDate;
			eTick.action_time = rTime * 100000 + rSecs;
		}
		else
		{
			eTick.action_date = curTDate;
			eTick.action_time = sInfo->getCloseTime() * 100000 + 59999;
		}

		WTSTickStruct* pTick = std::lower_bound(tBlock->_ticks, tBlock->_ticks + (tBlock->_size - 1), eTick, [](const WTSTickStruct& a, const WTSTickStruct& b) {
			if (a.action_date != b.action_date)
				return a.action_date < b.action_date;
			else
				return a.action_time < b.action_time;
		});

		uint32_t eIdx = pTick - tBlock->_ticks;

		//�����궨λ��tickʱ���Ŀ��ʱ���, ��ȫ������һ��
		if (pTick->action_date > eTick.action_date || pTick->action_time > eTick.action_time)
		{
			pTick--;
			eIdx--;
		}

		uint32_t thisCnt = min(eIdx + 1, left);
		uint32_t sIdx = eIdx + 1 - thisCnt;
		WTSTickSlice* slice = WTSTickSlice::create(stdCode, tBlock->_ticks + sIdx, thisCnt);
		ayTicks->append(slice, false);
		left -= thisCnt;
		break;
	}

	uint32_t nowTDate = min(endTDate, curTDate);
	if (nowTDate == curTDate)
		nowTDate = TimeUtils::getNextDate(nowTDate, -1);
	uint32_t missingCnt = 0;
	while (left > 0)
	{
		if(missingCnt >= 30)
			break;

		std::string curCode = cInfo._code;
		std::string hotCode;
		if (cInfo.isHot() && cInfo._category == CC_Future)
		{
			curCode = _hot_mgr->getRawCode(cInfo._exchg, cInfo._product, nowTDate);
			hotCode = cInfo._product;
			hotCode += "_HOT";
			WTSLogger::info("Hot contract of %u confirmed: %s -> %s", curTDate, stdCode, curCode.c_str());
		}
		else if (cInfo.isSecond() && cInfo._category == CC_Future)
		{
			curCode = _hot_mgr->getSecondRawCode(cInfo._exchg, cInfo._product, nowTDate);
			hotCode = cInfo._product;
			hotCode += "_2ND";
			WTSLogger::info("Second contract of %u confirmed: %s -> %s", curTDate, stdCode, curCode.c_str());
		}

		std::string key = StrUtil::printf("%s-%d", stdCode, nowTDate);

		auto it = _his_tick_map.find(key);
		bool bHasHisTick = (it != _his_tick_map.end());
		if (!bHasHisTick)
		{
			for (;;)
			{
				std::string filename;
				bool bHitHot = false;
				if(!hotCode.empty())
				{
					std::stringstream ss;
					ss << _base_dir << "his/ticks/" << cInfo._exchg << "/" << nowTDate << "/" << hotCode << ".dsb";
					filename = ss.str();
					if (StdFile::exists(filename.c_str()))
					{
						bHitHot = true;
					}
				}

				if(!bHitHot)
				{
					std::stringstream ss;
					ss << _base_dir << "his/ticks/" << cInfo._exchg << "/" << nowTDate << "/" << curCode << ".dsb";
					filename = ss.str();
					if (!StdFile::exists(filename.c_str()))
					{
						missingCnt++;
						break;
					}
				}

				missingCnt = 0;

				HisTBlockPair& tBlkPair = _his_tick_map[key];
				StdFile::read_file_content(filename.c_str(), tBlkPair._buffer);
				if (tBlkPair._buffer.size() < sizeof(HisTickBlock))
				{
					WTSLogger::error("Sizechecking of his tick data file %s failed", filename.c_str());
					tBlkPair._buffer.clear();
					break;
				}

				HisTickBlock* tBlock = (HisTickBlock*)tBlkPair._buffer.c_str();
				if (tBlock->_version == BLOCK_VERSION_CMP)
				{
					//ѹ���汾,Ҫ���¼���ļ���С
					HisTickBlockV2* tBlockV2 = (HisTickBlockV2*)tBlkPair._buffer.c_str();

					if (tBlkPair._buffer.size() != (sizeof(HisTickBlockV2) + tBlockV2->_size))
					{
						WTSLogger::error("Sizechecking of his tick data file %s failed", filename.c_str());
						break;
					}

					//��Ҫ��ѹ
					std::string buf = WTSCmpHelper::uncompress_data(tBlockV2->_data, (uint32_t)tBlockV2->_size);

					//��ԭ����bufferֻ����һ��ͷ��,��������tick����׷�ӵ�β��
					tBlkPair._buffer.resize(sizeof(HisTickBlock));
					tBlkPair._buffer.append(buf);
					tBlockV2 = (HisTickBlockV2*)tBlkPair._buffer.c_str();
					tBlockV2->_version = BLOCK_VERSION_RAW;
				}

				tBlkPair._block = (HisTickBlock*)tBlkPair._buffer.c_str();
				bHasHisTick = true;
				break;
			}
		}

		while (bHasHisTick)
		{
			//�Ƚ�ʱ��Ķ���
			WTSTickStruct eTick;
			if (nowTDate == endTDate)
			{
				eTick.action_date = rDate;
				eTick.action_time = rTime * 100000 + rSecs;
			}
			else
			{
				eTick.action_date = nowTDate;
				eTick.action_time = sInfo->getCloseTime() * 100000 + 59999;
			}

			HisTBlockPair& tBlkPair = _his_tick_map[key];
			if (tBlkPair._block == NULL)
				break;

			HisTickBlock* tBlock = tBlkPair._block;

			uint32_t tcnt = (tBlkPair._buffer.size() - sizeof(HisTickBlock)) / sizeof(WTSTickStruct);
			if (tcnt <= 0)
				break;

			WTSTickStruct* pTick = std::lower_bound(tBlock->_ticks, tBlock->_ticks + (tcnt - 1), eTick, [](const WTSTickStruct& a, const WTSTickStruct& b) {
				if (a.action_date != b.action_date)
					return a.action_date < b.action_date;
				else
					return a.action_time < b.action_time;
			});

			uint32_t eIdx = pTick - tBlock->_ticks;
			if (pTick->action_date > eTick.action_date || pTick->action_time >= eTick.action_time)
			{
				pTick--;
				eIdx--;
			}

			uint32_t thisCnt = min(eIdx + 1, left);
			uint32_t sIdx = eIdx + 1 - thisCnt;
			WTSTickSlice* slice = WTSTickSlice::create(stdCode, tBlock->_ticks + sIdx, thisCnt);
			ayTicks->append(slice, false);
			left -= thisCnt;
			break;
		}

		nowTDate = TimeUtils::getNextDate(nowTDate, -1);
	}

	std::reverse(ayTicks->begin(), ayTicks->end());
	return ayTicks;
}
