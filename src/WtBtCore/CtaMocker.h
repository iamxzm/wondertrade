/*!
 * \file CtaMocker.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#pragma once
#include <sstream>
#include <atomic>
#include <math.h>
#include "HisDataReplayer.h"

#include "../Includes/FasterDefs.h"
#include "../Includes/ICtaStraCtx.h"
#include "../Includes/CtaStrategyDefs.h"
#include "../Includes/WTSDataDef.hpp"

#include "../Share/DLLHelper.hpp"
#include "../Share/StdUtils.hpp"

NS_OTP_BEGIN
class EventNotifier;
NS_OTP_END

USING_NS_OTP;

class HisDataReplayer;
class CtaStrategy;

const char COND_ACTION_OL = 0;	//����
const char COND_ACTION_CL = 1;	//ƽ��
const char COND_ACTION_OS = 2;	//����
const char COND_ACTION_CS = 3;	//ƽ��
const char COND_ACTION_SP = 4;	//ֱ�����ò�λ

typedef struct _CondEntrust
{
	WTSCompareField _field;
	WTSCompareType	_alg;
	double			_target;

	double			_qty;

	char			_action;	//0-����,1-ƽ��,2-����,3-ƽ��

	char			_code[MAX_INSTRUMENT_LENGTH];
	char			_usertag[32];


	_CondEntrust()
	{
		memset(this, 0, sizeof(_CondEntrust));
	}

} CondEntrust;

typedef std::vector<CondEntrust>	CondList;
typedef faster_hashmap<std::string, CondList>	CondEntrustMap;


class CtaMocker : public ICtaStraCtx, public IDataSink
{
public:
	CtaMocker(HisDataReplayer* replayer, const char* name, int32_t slippage = 0, bool persistData = true, EventNotifier* notifier = NULL);
	virtual ~CtaMocker();

private:
	void	dump_outputs();
	inline void log_signal(const char* stdCode, double target, double price, uint64_t gentime, const char* usertag = "");
	inline void	log_trade(const char* stdCode, bool isLong, bool isOpen, uint64_t curTime, double price, double qty, const char* userTag = "", double fee = 0.0, uint32_t barNo = 0);
	inline void	log_close(const char* stdCode, bool isLong, uint64_t openTime, double openpx, uint64_t closeTime, double closepx, double qty,
		double profit, double maxprofit, double maxloss, double totalprofit = 0, const char* enterTag = "", const char* exitTag = "", uint32_t openBarNo = 0, uint32_t closeBarNo = 0);

	void	update_dyn_profit(const char* stdCode, double price);

	void	do_set_position(const char* stdCode, double qty, double price = 0.0, std::string instid = "",const char* userTag = "", bool bTriggered = false);//
	void	append_signal(const char* stdCode, double qty, const char* userTag = "", double price = 0.0);

	inline CondList& get_cond_entrusts(const char* stdCode);

	void set_dayaccount(const char* stdCode, WTSTickData* newTick, bool bEmitStrategy = true);
public:
	bool	init_cta_factory(WTSVariant* cfg);
	void	install_hook();
	void	enable_hook(bool bEnabled = true);
	bool	step_calc();

public:
	//////////////////////////////////////////////////////////////////////////
	//IDataSink
	virtual void	handle_tick(const char* stdCode, WTSTickData* curTick) override;
	virtual void	handle_bar_close(const char* stdCode, const char* period, uint32_t times, WTSBarStruct* newBar) override;
	virtual void	handle_schedule(uint32_t uDate, uint32_t uTime) override;

	virtual void	handle_init() override;
	virtual void	handle_session_begin(uint32_t curTDate) override;
	virtual void	handle_session_end(uint32_t curTDate) override;

	virtual void	handle_replay_done() override;

	//////////////////////////////////////////////////////////////////////////
	//ICtaStraCtx
	virtual uint32_t id() { return _context_id; }

	//�ص�����
	virtual void on_init() override;
	virtual void on_session_begin(uint32_t curTDate) override;
	virtual void on_session_end(uint32_t curTDate) override;
	virtual void on_tick(const char* stdCode, WTSTickData* newTick, bool bEmitStrategy = true) override;
	virtual void on_bar(const char* stdCode, const char* period, uint32_t times, WTSBarStruct* newBar) override;
	virtual bool on_schedule(uint32_t curDate, uint32_t curTime) override;
	virtual void enum_position(FuncEnumCtaPosCallBack cb) override;

	virtual void on_tick_updated(const char* stdCode, WTSTickData* newTick) override;
	virtual void on_bar_close(const char* stdCode, const char* period, WTSBarStruct* newBar) override;
	virtual void on_calculate(uint32_t curDate, uint32_t curTime) override;


	//////////////////////////////////////////////////////////////////////////
	//���Խӿ�
	virtual void stra_enter_long(const char* stdCode, double qty, const char* userTag = "", double limitprice = 0.0, double stopprice = 0.0) override;
	virtual void stra_enter_short(const char* stdCode, double qty, const char* userTag = "", double limitprice = 0.0, double stopprice = 0.0) override;
	virtual void stra_exit_long(const char* stdCode, double qty, const char* userTag = "", double limitprice = 0.0, double stopprice = 0.0) override;
	virtual void stra_exit_short(const char* stdCode, double qty, const char* userTag = "", double limitprice = 0.0, double stopprice = 0.0) override;

	virtual double stra_get_position(const char* stdCode, const char* userTag = "") override;
	virtual void stra_set_position(const char* stdCode, double qty, bool insert_mongo, const char* userTag = "", double limitprice = 0.0, double stopprice = 0.0) override;
	virtual double stra_get_price(const char* stdCode) override;

	virtual uint32_t stra_get_tdate() override;
	virtual uint32_t stra_get_date() override;
	virtual uint32_t stra_get_time() override;

	virtual double stra_get_fund_data(int flag = 0) override;

	virtual uint64_t stra_get_first_entertime(const char* stdCode) override;
	virtual uint64_t stra_get_last_entertime(const char* stdCode) override;
	virtual uint64_t stra_get_last_exittime(const char* stdCode) override;
	virtual double stra_get_last_enterprice(const char* stdCode) override;
	virtual double stra_get_position_avgpx(const char* stdCode) override;
	virtual double stra_get_position_profit(const char* stdCode) override;

	virtual uint64_t stra_get_detail_entertime(const char* stdCode, const char* userTag) override;
	virtual double stra_get_detail_cost(const char* stdCode, const char* userTag) override;
	virtual double stra_get_detail_profit(const char* stdCode, const char* userTag, int flag = 0) override;

	virtual WTSCommodityInfo* stra_get_comminfo(const char* stdCode) override;
	virtual WTSKlineSlice*	stra_get_bars(const char* stdCode, const char* period, uint32_t count, bool isMain = false) override;
	virtual WTSTickSlice*	stra_get_ticks(const char* stdCode, uint32_t count) override;
	virtual WTSTickData*	stra_get_last_tick(const char* stdCode) override;

	virtual void stra_sub_ticks(const char* stdCode) override;

	virtual void stra_log_info(const char* fmt, ...) override;
	virtual void stra_log_debug(const char* fmt, ...) override;
	virtual void stra_log_error(const char* fmt, ...) override;

	virtual void stra_save_user_data(const char* key, const char* val) override;

	virtual const char* stra_load_user_data(const char* key, const char* defVal = "") override;

protected:
	uint32_t			_context_id;
	HisDataReplayer*	_replayer;

	uint64_t		_total_calc_time;	//�ܼ���ʱ��
	uint32_t		_emit_times;		//�ܼ������

	int32_t			_slippage;			//�ɽ�����

	uint32_t		_schedule_times;	//���ȴ���

	const double init_money = 100000;			//��ʼ�ʽ�
	double      _balance = 0;					//�����ʲ�
	double		_total_money = init_money;		//ʣ���ʽ�
	double		_static_balance = init_money;			//�ڳ��ʲ�
	double		_close_price = 0;				//������
	double		_settlepx;						//������
	double        _used_margin = 0;			//ռ�ñ�֤��
	double        _margin_rate = 0.5;			//��֤�����
	uint64_t		_cur_multiplier = 100;		//��ǰ��Լ����

	double		_day_profit = 0;
	double		_total_profit = 0;	//��������
	double		_benchmark_rate_of_return = 0; //��׼������
	double		_daily_rate_of_return = 0;//����������
	double		_abnormal_rate_of_return = 0;//�ճ���������
	int			_win_or_lose_flag;

	bool			_new_trade_day = true;
	bool		_dayacc_insert_flag = true;
	bool		_changepos = true;
	uint32_t    _traderday = 0;


	std::string		_main_key;

	typedef struct _KlineTag
	{
		bool			_closed;

		_KlineTag() :_closed(false){}

	} KlineTag;
	typedef faster_hashmap<std::string, KlineTag> KlineTags;
	KlineTags	_kline_tags;

	typedef faster_hashmap<std::string, double> PriceMap;
	PriceMap		_price_map;

	typedef struct _DetailInfo
	{
		bool		_long;
		double		_price;
		double		_volume;
		uint64_t	_opentime;
		uint32_t	_opentdate;
		double		_max_profit;
		double		_max_loss;
		double		_profit;
		char		_opentag[32];
		uint32_t	_open_barno;
		double		_margin;

		_DetailInfo()
		{
			memset(this, 0, sizeof(_DetailInfo));
		}
	} DetailInfo;

	typedef struct _PosInfo
	{
		double		_volume;
		double		_closeprofit;
		double		_dynprofit;
		uint64_t	_last_entertime;
		uint64_t	_last_exittime;

		std::vector<DetailInfo> _details;

		_PosInfo()
		{
			_volume = 0;
			_closeprofit = 0;
			_dynprofit = 0;
		}
	} PosInfo;
	typedef faster_hashmap<std::string, PosInfo> PositionMap;
	PositionMap		_pos_map;
	double	_total_closeprofit;

	typedef struct _SigInfo
	{
		double		_volume;
		std::string	_usertag;
		double		_sigprice;
		double		_desprice;
		bool		_triggered;
		uint64_t	_gentime;

		_SigInfo()
		{
			_volume = 0;
			_sigprice = 0;
			_desprice = 0;
			_triggered = false;
			_gentime = 0;
		}
	}SigInfo;
	typedef faster_hashmap<std::string, SigInfo>	SignalMap;
	SignalMap		_sig_map;

	std::stringstream	_trade_logs;
	std::stringstream	_close_logs;
	std::stringstream	_fund_logs;
	std::stringstream	_sig_logs;

	CondEntrustMap	_condtions;

	//�Ƿ��ڵ����еı��
	bool			_is_in_schedule;	//�Ƿ����Զ�������

	//�û�����
	typedef faster_hashmap<std::string, std::string> StringHashMap;
	StringHashMap	_user_datas;
	bool			_ud_modified;

	typedef struct _StraFundInfo
	{
		double	_total_profit;
		double	_total_dynprofit;
		double	_total_fees;

		_StraFundInfo()
		{
			memset(this, 0, sizeof(_StraFundInfo));
		}
	} StraFundInfo;

	StraFundInfo		_fund_info;

	typedef struct _StraFactInfo
	{
		std::string		_module_path;
		DllHandle		_module_inst;
		ICtaStrategyFact*	_fact;
		FuncCreateStraFact	_creator;
		FuncDeleteStraFact	_remover;

		_StraFactInfo()
		{
			_module_inst = NULL;
			_fact = NULL;
		}

		~_StraFactInfo()
		{
			if (_fact)
				_remover(_fact);
		}
	} StraFactInfo;
	StraFactInfo	_factory;

	CtaStrategy*	_strategy;
	EventNotifier*	_notifier;

	StdUniqueMutex	_mtx_calc;
	StdCondVariable	_cond_calc;
	bool			_has_hook;		//������Ϊ�����Ƿ����ù���
	bool			_hook_valid;	//���Ǹ����Ƿ����첽�ز�ģʽ��ȷ�������Ƿ����
	std::atomic<uint32_t>		_cur_step;	//��ʱ���������ڿ���״̬

	bool			_in_backtest;
	bool			_wait_calc;

	bool			_persist_data;

	void insert_his_position(DetailInfo dInfo, PosInfo pInfo, double fee,
		std::string exch_id, std::string inst_id, uint64_t curTime);
	void insert_his_trades(DetailInfo dInfo, PosInfo pInfo, double fee,
		std::string exch_id, std::string inst_id, uint64_t curTime);

	bool _insert_mongo;
	//mongocxx::uri _uri;
	//mongocxx::client _client;
	/*mongocxx::database _db;
	mongocxx::collection	_poscoll_1;*/
	/*mongocxx::collection	_poscoll_2;
	mongocxx::collection	_poscoll_3;*/
};