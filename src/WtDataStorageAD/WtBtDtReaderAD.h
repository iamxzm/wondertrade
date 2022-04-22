#pragma once
#include <string>

#include "../Includes/FasterDefs.h"
#include "../Includes/IBtDtReader.h"

#include "../WTSUtils/WtLMDB.hpp"

NS_WTP_BEGIN

class WtBtDtReaderAD : public IBtDtReader
{
public:
	WtBtDtReaderAD();
	virtual ~WtBtDtReaderAD();	

//////////////////////////////////////////////////////////////////////////
//IBtDtReader
public:
	virtual void init(WTSVariant* cfg, IBtDtReaderSink* sink);

	virtual bool read_raw_bars(const char* exchg, const char* code, WTSKlinePeriod period, std::string& buffer) override;
	virtual bool read_raw_ticks(const char* exchg, const char* code, uint32_t uDate, std::string& buffer) override;

private:
	std::string		_base_dir;

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