#pragma once
#include <string>
#include <stdint.h>
#include <boost/circular_buffer.hpp>

#include "DataDefineAD.h"

#include "../WTSUtils/WtLMDB.hpp"
#include "../Includes/FasterDefs.h"
#include "../Includes/IDataReader.h"

#include "../Share/StdUtils.hpp"
#include "../Share/BoostMappingFile.hpp"

NS_WTP_BEGIN

typedef std::shared_ptr<BoostMappingFile> BoostMFPtr;

class WtDataReaderAD : public IDataReader
{
public:
	WtDataReaderAD();
	virtual ~WtDataReaderAD();


public:
	virtual void init(WTSVariant* cfg, IDataReaderSink* sink, IHisDataLoader* loader = NULL) override;

	virtual void onMinuteEnd(uint32_t uDate, uint32_t uTime, uint32_t endTDate = 0) override;

	virtual WTSTickSlice*	readTickSlice(const char* stdCode, uint32_t count, uint64_t etime = 0) override;

	virtual WTSKlineSlice*	readKlineSlice(const char* stdCode, WTSKlinePeriod period, uint32_t count, uint64_t etime = 0) override;

private:
	std::string		_base_dir;
	IBaseDataMgr*	_base_data_mgr;
	IHotMgr*		_hot_mgr;

	//m1����
	typedef struct _RTBarCacheWrapper
	{
		StdUniqueMutex	_mtx;
		std::string		_filename;
		faster_hashmap<std::string, uint32_t> _idx;
		BoostMFPtr		_file_ptr;
		RTBarCache*		_cache_block;
		uint32_t		_last_size;

		_RTBarCacheWrapper() :_cache_block(NULL), _file_ptr(NULL), _last_size(0){}

		inline bool empty() const { return _cache_block == NULL; }
	} RTBarCacheWrapper;

	RTBarCacheWrapper _m1_cache;
	RTBarCacheWrapper _m5_cache;
	RTBarCacheWrapper _d1_cache;

	typedef struct _BarsList
	{
		std::string		_exchg;
		std::string		_code;
		WTSKlinePeriod	_period;
		//���һ���Ƿ�ӻ������ȡ�ģ�������´θ��µ�ʱ��Ҫ��lmdb����һ�Σ����һ���ٰ���ԭ���߼�����
		bool			_last_from_cache;
		uint64_t		_last_req_time;

		boost::circular_buffer<WTSBarStruct>	_bars;

		_BarsList():_last_from_cache(false),_last_req_time(0){}
	} BarsList;

	typedef struct _TicksList
	{
		std::string		_exchg;
		std::string		_code;
		uint64_t		_last_req_time;

		boost::circular_buffer<WTSTickStruct>	_ticks;

		_TicksList():_last_req_time(0){}
	} TicksList;

	typedef faster_hashmap<std::string, BarsList> BarsCache;
	BarsCache	_bars_cache;

	typedef faster_hashmap<std::string, TicksList> TicksCache;
	TicksCache	_ticks_cache;

	uint64_t	_last_time;	

private:
	/*
	 *	����ʷ���ݷ��뻺��
	 */
	bool	cacheBarsFromStorage(const std::string& key, const char* stdCode, WTSKlinePeriod period, uint32_t count);

	/*
	 *	��LMDB�и��»��������
	 */
	void	update_cache_from_lmdb(BarsList& barsList, const char* exchg, const char* code, WTSKlinePeriod period, uint32_t& lastBarTime);

	std::string	read_bars_to_buffer(const char* exchg, const char* code, WTSKlinePeriod period);

	WTSBarStruct* get_rt_cache_bar(const char* exchg, const char* code, WTSKlinePeriod period);

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
};

NS_WTP_END
