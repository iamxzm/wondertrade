#pragma once
#include "DataDefineAD.h"

#include "../WTSUtils/WtLMDB.hpp"

#include "../Includes/FasterDefs.h"
#include "../Includes/IDataWriter.h"
#include "../Share/StdUtils.hpp"
#include "../Share/BoostMappingFile.hpp"

#include <queue>

typedef std::shared_ptr<BoostMappingFile> BoostMFPtr;

NS_WTP_BEGIN
class WTSContractInfo;
NS_WTP_END

USING_NS_WTP;

class WtDataWriterAD : public IDataWriter
{
public:
	WtDataWriterAD();
	~WtDataWriterAD();	

private:
	template<typename HeaderType, typename T>
	void* resizeRTBlock(BoostMFPtr& mfPtr, uint32_t nCount);

public:
	virtual bool init(WTSVariant* params, IDataWriterSink* sink) override;
	virtual void release() override;

	virtual bool writeTick(WTSTickData* curTick, uint32_t procFlag) override;

	virtual WTSTickData* getCurTick(const char* code, const char* exchg = "") override;

private:
	IBaseDataMgr*		_bd_mgr;

	//Tick����
	StdUniqueMutex	_mtx_tick_cache;
	std::string		_cache_file_tick;
	faster_hashmap<std::string, uint32_t> _tick_cache_idx;
	BoostMFPtr		_tick_cache_file;
	RTTickCache*	_tick_cache_block;

	//m1����
	typedef struct _RTBarCacheWrapper
	{
		StdUniqueMutex	_mtx;
		std::string		_filename;
		faster_hashmap<std::string, uint32_t> _idx;
		BoostMFPtr		_file_ptr;
		RTBarCache*		_cache_block;

		_RTBarCacheWrapper():_cache_block(NULL),_file_ptr(NULL){}

		inline bool empty() const { return _cache_block == NULL; }
	} RTBarCacheWrapper;

	RTBarCacheWrapper _m1_cache;
	RTBarCacheWrapper _m5_cache;
	RTBarCacheWrapper _d1_cache;

	typedef std::function<void()> TaskInfo;
	std::queue<TaskInfo>	_tasks;
	StdThreadPtr			_task_thrd;
	StdUniqueMutex			_task_mtx;
	StdCondVariable			_task_cond;
	bool					_async_task;

	std::string		_base_dir;
	uint32_t		_log_group_size;

	bool			_terminated;

	bool			_disable_tick;
	bool			_disable_min1;
	bool			_disable_min5;
	bool			_disable_day;

private:
	//////////////////////////////////////////////////////////////////////////
	/*
	 *	�����LMDB�����ݿⶨ��
	 *	K�����ݣ�����ÿ���г�m1/m5/d1��������һ���������ݿ⣬·����./m1/CFFEX
	 *	Tick���ݣ�ÿ����Լһ�����ݿ⣬·����./ticks/CFFEX/IF2101
	 */
	typedef std::shared_ptr<WtLMDB> WtLMDBPtr;
	typedef faster_hashmap<std::string, WtLMDBPtr> WtLMDBMap;

	WtLMDBMap	_exchg_m1_dbs;
	WtLMDBMap	_exchg_m5_dbs;
	WtLMDBMap	_exchg_d1_dbs;

	//��exchg.code��Ϊkey����BINANCE.BTCUSDT
	WtLMDBMap	_tick_dbs;

	WtLMDBPtr	get_k_db(const char* exchg, WTSKlinePeriod period);

	WtLMDBPtr	get_t_db(const char* exchg, const char* code);

private:
	void loadCache();

	bool updateTickCache(WTSContractInfo* ct, WTSTickData* curTick, uint32_t procFlag);

	void updateBarCache(WTSContractInfo* ct, WTSTickData* curTick);

	void pipeToTicks(WTSContractInfo* ct, WTSTickData* curTick);

	void pipeToDayBars(WTSContractInfo* ct, const WTSBarStruct& bar);

	void pipeToM1Bars(WTSContractInfo* ct, const WTSBarStruct& bar);

	void pipeToM5Bars(WTSContractInfo* ct, const WTSBarStruct& bar);

	void pushTask(TaskInfo task);
};

