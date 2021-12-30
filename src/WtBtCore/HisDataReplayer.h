/*!
 * \file HisDataReplayer.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#pragma once
#include <string>
#include "DataDefine.h"
#include "../WtDataWriter/MysqlDB.hpp"

#include "../Includes/FasterDefs.h"
#include "../Includes/WTSMarcos.h"
#include "../Includes/WTSTypes.h"

#include "../WTSTools/WTSHotMgr.h"
#include "../WTSTools/WTSBaseDataMgr.h"
#include <rapidjson/document.h>
#include <numeric>
namespace rj = rapidjson;

NS_OTP_BEGIN
class WTSTickData;
class WTSVariant;
class WTSKlineSlice;
class WTSTickSlice;
class WTSOrdDtlSlice;
class WTSOrdQueSlice;
class WTSTransSlice;
class WTSSessionInfo;
class WTSCommodityInfo;

class WTSOrdDtlData;
class WTSOrdQueData;
class WTSTransData;

class EventNotifier;
NS_OTP_END

typedef std::shared_ptr<MysqlDb>	MysqlDbPtr;

USING_NS_OTP;

class IDataSink
{
public:
	virtual void	handle_tick(const char* stdCode, WTSTickData* curTick) = 0;
	virtual void	handle_order_queue(const char* stdCode, WTSOrdQueData* curOrdQue) {};
	virtual void	handle_order_detail(const char* stdCode, WTSOrdDtlData* curOrdDtl) {};
	virtual void	handle_transaction(const char* stdCode, WTSTransData* curTrans) {};
	virtual void	handle_bar_close(const char* stdCode, const char* period, uint32_t times, WTSBarStruct* newBar) = 0;
	virtual void	handle_schedule(uint32_t uDate, uint32_t uTime) = 0;

	virtual void	handle_init() = 0;
	virtual void	handle_session_begin(uint32_t curTDate) = 0;
	virtual void	handle_session_end(uint32_t curTDate) = 0;
	virtual void	handle_replay_done() {}
};

class HisDataReplayer
{

private:
	template <typename T>
	class HftDataList
	{
	public:
		std::string		_code;
		uint32_t		_date;
		uint32_t		_cursor;
		uint32_t		_count;

		std::vector<T> _items;

		HftDataList() :_cursor(UINT_MAX), _count(0), _date(0){}
	};

	typedef faster_hashmap<std::string, HftDataList<WTSTickStruct>>		TickCache;
	typedef faster_hashmap<std::string, HftDataList<WTSOrdDtlStruct>>	OrdDtlCache;
	typedef faster_hashmap<std::string, HftDataList<WTSOrdQueStruct>>	OrdQueCache;
	typedef faster_hashmap<std::string, HftDataList<WTSTransStruct>>	TransCache;


	typedef struct _BarsList
	{
		std::string		_code;
		WTSKlinePeriod	_period;
		uint32_t		_cursor;
		uint32_t		_count;
		uint32_t		_times;

		std::vector<WTSBarStruct>	_bars;
		double			_factor;	//���һ����Ȩ����

		_BarsList() :_cursor(UINT_MAX), _count(0), _times(1), _factor(1){}
	} BarsList;

	typedef faster_hashmap<std::string, BarsList>	BarsCache;

	typedef enum tagTaskPeriodType
	{
		TPT_None,		//���ظ�
		TPT_Minute = 4,	//����������
		TPT_Daily = 8,	//ÿ��������
		TPT_Weekly,		//ÿ��,�����ڼ��յĻ�Ҫ˳��
		TPT_Monthly,	//ÿ��,�����ڼ���˳��
		TPT_Yearly		//ÿ��,�����ڼ���˳��
	}TaskPeriodType;

	typedef struct _TaskInfo
	{
		uint32_t	_id;
		char		_name[16];		//������
		char		_trdtpl[16];	//������ģ��
		char		_session[16];	//����ʱ��ģ��
		uint32_t	_day;			//����,�������ڱ仯,ÿ��Ϊ0,ÿ��Ϊ0~6,��Ӧ���յ�����,ÿ��Ϊ1~31,ÿ��Ϊ0101~1231
		uint32_t	_time;			//ʱ��,��ȷ������
		bool		_strict_time;	//�Ƿ����ϸ�ʱ��,�ϸ�ʱ�伴ֻ��ʱ����ȲŻ�ִ��,�����ϸ�ʱ��,����ڵ��ڴ���ʱ�䶼��ִ��

		uint64_t	_last_exe_time;	//�ϴ�ִ��ʱ��,��ҪΪ�˷�ֹ�ظ�ִ��

		TaskPeriodType	_period;	//��������
	} TaskInfo;

	typedef std::shared_ptr<TaskInfo> TaskInfoPtr;



public:
	HisDataReplayer();
	~HisDataReplayer();

private:
	/*
	 *	���Զ��������ļ�������ʷ����
	 */
	bool		cacheRawBarsFromBin(const std::string& key, const char* stdCode, WTSKlinePeriod period, bool bForBars = true);

	/*
	 *	��csv�ļ�������ʷ����
	 */
	bool		cacheRawBarsFromCSV(const std::string& key, const char* stdCode, WTSKlinePeriod period, bool bForBars = true);

	/*
	 *	�����ݿ⻺����ʷ����
	 */
	bool		cacheRawBarsFromDB(const std::string& key, const char* stdCode, WTSKlinePeriod period, bool bForBars = true);

	/*
	 *	���Զ��������ļ�������ʷtick����
	 */
	bool		cacheRawTicksFromBin(const std::string& key, const char* stdCode, uint32_t uDate);

	/*
	 *	��csv�ļ�������ʷtick����
	 */
	bool		cacheRawTicksFromCSV(const std::string& key, const char* stdCode, uint32_t uDate);
	/*
	 *	��mongodb������ʷtick����
	 */
	bool		cacheRawTicksFromDB(const std::string& key, const char* stdCode, uint32_t uDate);

	void		onMinuteEnd(uint32_t uDate, uint32_t uTime, uint32_t endTDate = 0, bool tickSimulated = true);

	void		loadFees(const char* filename);

	bool		replayHftDatas(uint64_t stime, uint64_t etime);

	uint64_t	replayHftDatasByDay(uint32_t curTDate);

	void		replayUnbars(uint64_t stime, uint64_t etime, uint32_t endTDate = 0);

	inline bool		checkTicks(const char* stdCode, uint32_t uDate);

	inline bool		checkOrderDetails(const char* stdCode, uint32_t uDate);

	inline bool		checkOrderQueues(const char* stdCode, uint32_t uDate);

	inline bool		checkTransactions(const char* stdCode, uint32_t uDate);

	void		checkUnbars();

	bool		loadStkAdjFactors(const char* adjfile);

	bool		loadStkAdjFactorsFromDB();

	void		initDB();

	bool		checkAllTicks(uint32_t uDate);

	inline	uint64_t	getNextTickTime(uint32_t curTDate, uint64_t stime = UINT64_MAX);
	inline	uint64_t	getNextOrdQueTime(uint32_t curTDate, uint64_t stime = UINT64_MAX);
	inline	uint64_t	getNextOrdDtlTime(uint32_t curTDate, uint64_t stime = UINT64_MAX);
	inline	uint64_t	getNextTransTime(uint32_t curTDate, uint64_t stime = UINT64_MAX);

	void		reset();


	void		dump_btstate(const char* stdCode, WTSKlinePeriod period, uint32_t times, uint64_t stime, uint64_t etime, double progress, int64_t elapse);
	void		notify_state(const char* stdCode, WTSKlinePeriod period, uint32_t times, uint64_t stime, uint64_t etime, double progress);

	uint32_t	locate_barindex(const std::string& key, uint64_t curTime, bool bUpperBound = false);

	void	run_by_bars(bool bNeedDump = false);
	void	run_by_tasks(bool bNeedDump = false);
	void	run_by_ticks(bool bNeedDump = false);

public:
	bool init(WTSVariant* cfg, EventNotifier* notifier = NULL);

	bool prepare();
	void run(bool bNeedDump = false);
	
	void stop();

	void clear_cache();

	inline void set_time_range(uint64_t stime, uint64_t etime)
	{
		_begin_time = stime;
		_end_time = etime;
	}

	inline void enable_tick(bool bEnabled = true)
	{
		_tick_enabled = bEnabled;
	}

	inline void register_sink(IDataSink* listener, const char* sinkName) 
	{
		_listener = listener; 
		_stra_name = sinkName;
	}

	void register_task(uint32_t taskid, uint32_t date, uint32_t time, const char* period, const char* trdtpl = "CHINA", const char* session = "TRADING");

	WTSKlineSlice* get_kline_slice(const char* stdCode, const char* period, uint32_t count, uint32_t times = 1, bool isMain = false);

	WTSTickSlice* get_tick_slice(const char* stdCode, uint32_t count, uint64_t etime = 0);

	WTSOrdDtlSlice* get_order_detail_slice(const char* stdCode, uint32_t count, uint64_t etime = 0);

	WTSOrdQueSlice* get_order_queue_slice(const char* stdCode, uint32_t count, uint64_t etime = 0);

	WTSTransSlice* get_transaction_slice(const char* stdCode, uint32_t count, uint64_t etime = 0);

	WTSTickData* get_last_tick(const char* stdCode);

	uint32_t get_date() const{ return _cur_date; }
	uint32_t get_min_time() const{ return _cur_time; }
	uint32_t get_raw_time() const{ return _cur_time; }
	uint32_t get_secs() const{ return _cur_secs; }
	uint32_t get_trading_date() const{ return _cur_tdate; }

	double calc_fee(const char* stdCode, double price, double qty, uint32_t offset);
	WTSSessionInfo*		get_session_info(const char* sid, bool isCode = false);
	WTSCommodityInfo*	get_commodity_info(const char* stdCode);
	double get_cur_price(const char* stdCode);

	void sub_tick(uint32_t sid, const char* stdCode);
	void sub_order_queue(uint32_t sid, const char* stdCode);
	void sub_order_detail(uint32_t sid, const char* stdCode);
	void sub_transaction(uint32_t sid, const char* stdCode);

	inline bool	is_tick_enabled() const{ return _tick_enabled; }

	inline bool	is_tick_simulated() const { return _tick_simulated; }

	inline void update_price(const char* stdCode, double price)
	{
		_price_map[stdCode] = price;
	}

private:
	IDataSink*		_listener;
	std::string		_stra_name;

	TickCache		_ticks_cache;	//tick����
	OrdDtlCache		_orddtl_cache;	//order detail����
	OrdQueCache		_ordque_cache;	//order queue����
	TransCache		_trans_cache;	//transaction����

	BarsCache		_bars_cache;	//K�߻���
	BarsCache		_unbars_cache;	//δ���ĵ�K�߻���

	TaskInfoPtr		_task;

	std::string		_main_key;
	std::string		_min_period;	//��СK������,�����Ҫ����δ����Ʒ�ֵ��źŴ�����
	bool			_tick_enabled;	//�Ƿ�����tick�ز�
	bool			_tick_simulated;	//�Ƿ���Ҫģ��tick
	std::map<std::string, WTSTickStruct>	_day_cache;	//ÿ��Tick����,��tick�ط�δ����ʱ,���õ��û���
	std::map<std::string, std::string>		_ticker_keys;

	uint32_t		_cur_date;
	uint32_t		_cur_time;
	uint32_t		_cur_secs;
	uint32_t		_cur_tdate;
	uint32_t		_closed_tdate;
	uint32_t		_opened_tdate;

	WTSBaseDataMgr	_bd_mgr;
	WTSHotMgr		_hot_mgr;

	std::string		_base_dir;
	std::string		_mode;
	uint64_t		_begin_time;
	uint64_t		_end_time;

	bool			_running;
	bool			_terminated;
	//////////////////////////////////////////////////////////////////////////
	//������ģ��
	typedef struct _FeeItem
	{
		double	_open;
		double	_close;
		double	_close_today;
		bool	_by_volume;

		_FeeItem()
		{
			memset(this, 0, sizeof(_FeeItem));
		}
	} FeeItem;
	typedef faster_hashmap<std::string, FeeItem>	FeeMap;
	FeeMap		_fee_map;

	//////////////////////////////////////////////////////////////////////////
	//
	typedef faster_hashmap<std::string, double> PriceMap;
	PriceMap		_price_map;

	//////////////////////////////////////////////////////////////////////////
	//
	typedef faster_hashset<uint32_t> SIDSet;
	typedef faster_hashmap<std::string, SIDSet>	StraSubMap;
	StraSubMap		_tick_sub_map;		//tick���ݶ��ı�
	StraSubMap		_ordque_sub_map;	//orderqueue���ݶ��ı�
	StraSubMap		_orddtl_sub_map;	//orderdetail���ݶ��ı�
	StraSubMap		_trans_sub_map;		//transaction���ݶ��ı�

	//��Ȩ����
	typedef struct _AdjFactor
	{
		uint32_t	_date;
		double		_factor;
	} AdjFactor;
	typedef std::vector<AdjFactor> AdjFactorList;
	typedef faster_hashmap<std::string, AdjFactorList>	AdjFactorMap;
	AdjFactorMap	_adj_factors;

	inline const AdjFactorList& getAdjFactors(const char* code, const char* exchg)
	{
		char key[20] = { 0 };
		sprintf(key, "%s.%s", exchg, code);
		return _adj_factors[key];
	}

	typedef struct _DBConfig
	{
		bool	_active;
		char	_host[64];
		int32_t	_port;
		char	_dbname[32];
		char	_user[32];
		char	_pass[32];

		_DBConfig() { memset(this, 0, sizeof(_DBConfig)); }
	} DBConfig;

	DBConfig	_db_conf;
	MysqlDbPtr	_db_conn;

	EventNotifier*	_notifier;

	//������Լ���ڷ�Χ
	typedef struct _DateList
	{
		uint32_t _sdate;
		uint32_t _edate;
		uint32_t _cnt_date;
	} DateList;

	std::map<std::string, DateList> _datelistmap;
};

