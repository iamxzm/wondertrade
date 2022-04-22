/*!
 * \file WtExecuter.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#pragma once
#include <boost/core/noncopyable.hpp>

#include "ITrdNotifySink.h"
#include "IExecCommand.h"
#include "../Includes/ExecuteDefs.h"

#include "../Share/DLLHelper.hpp"
#include "../Share/threadpool.hpp"

NS_OTP_BEGIN
class WTSVariant;
class IDataManager;
class TraderAdapter;
class IHotMgr;

//////////////////////////////////////////////////////////////////////////
//ִ�е�Ԫ��װ
//��Ϊִ�е�Ԫ��dll�ﴴ����, �������װ�Ļ�, ֱ��delete���ܻ�������
//����Ҫ�ѹ���ָ��һ���װ������, ֱ�ӵ��ù���ʵ����deleteUnit�����ͷ�ִ�е�Ԫ
class ExeUnitWrapper
{
public:
	ExeUnitWrapper(ExecuteUnit* unitPtr, IExecuterFact* fact):_unit(unitPtr),_fact(fact){}
	~ExeUnitWrapper()
	{
		if(_unit)
		{
			_fact->deleteExeUnit(_unit);
		}
	}

	ExecuteUnit* self(){ return _unit; }


private:
	ExecuteUnit*	_unit;
	IExecuterFact*	_fact;
};

typedef std::shared_ptr<ExeUnitWrapper>	ExecuteUnitPtr;

//////////////////////////////////////////////////////////////////////////
//ִ����������
class WtExecuterFactory : private boost::noncopyable
{
public:
	~WtExecuterFactory() {}

public:
	bool loadFactories(const char* path);

	ExecuteUnitPtr createExeUnit(const char* name);
	ExecuteUnitPtr createExeUnit(const char* factname, const char* unitname);

private:
	typedef struct _ExeFactInfo
	{
		std::string		_module_path;
		DllHandle		_module_inst;
		IExecuterFact*	_fact;
		FuncCreateExeFact	_creator;
		FuncDeleteExeFact	_remover;
	} ExeFactInfo;
	typedef faster_hashmap<std::string, ExeFactInfo> ExeFactMap;

	ExeFactMap	_factories;
};

//����ִ����
class WtLocalExecuter : public ExecuteContext,
		public ITrdNotifySink, public IExecCommand
{
public:
	typedef faster_hashmap<std::string, ExecuteUnitPtr> ExecuteUnitMap;

public:
	WtLocalExecuter(WtExecuterFactory* factory, const char* name, IDataManager* dataMgr);
	virtual ~WtLocalExecuter();

public:
	/*
	 *	��ʼ��ִ����
	 *	�����ʼ������
	 */
	bool init(WTSVariant* params);


	inline void setTrader(TraderAdapter* adapter)
	{
		_trader = adapter;
	}

private:
	ExecuteUnitPtr	getUnit(const char* code, bool bAutoCreate = true);
public:
	//////////////////////////////////////////////////////////////////////////
	//ExecuteContext
	virtual WTSTickSlice* getTicks(const char* code, uint32_t count, uint64_t etime = 0) override;

	virtual WTSTickData*	grabLastTick(const char* code) override;

	virtual double		getPosition(const char* code, int32_t flag = 3) override;
	virtual OrderMap*	getOrders(const char* code) override;
	virtual double		getUndoneQty(const char* code) override;

	virtual OrderIDs	buy(const char* code, double price, double qty, bool bForceClose = false) override;
	virtual OrderIDs	sell(const char* code, double price, double qty, bool bForceClose = false) override;
	virtual bool		cancel(uint32_t localid) override;
	virtual OrderIDs	cancel(const char* code, bool isBuy, double qty) override;
	virtual void		writeLog(const char* fmt, ...) override;

	virtual WTSCommodityInfo*	getCommodityInfo(const char* stdCode) override;
	virtual WTSSessionInfo*		getSessionInfo(const char* stdCode) override;

	virtual uint64_t	getCurTime() override;

public:
	/*
	 *	����Ŀ���λ
	 */
	virtual void set_position(const faster_hashmap<std::string, double>& targets) override;


	/*
	 *	��Լ��λ�䶯
	 */
	virtual void on_position_changed(const char* stdCode, double targetPos) override;

	/*
	 *	ʵʱ����ص�
	 */
	virtual void on_tick(const char* stdCode, WTSTickData* newTick) override;

	/*
	 *	�ɽ��ر�
	 */
	virtual void on_trade(uint32_t localid, const char* stdCode, bool isBuy, double vol, double price) override;

	/*
	 *	�����ر�
	 */
	virtual void on_order(uint32_t localid, const char* stdCode, bool isBuy, double totalQty, double leftQty, double price, bool isCanceled = false) override;

	/*
	 *	
	 */
	virtual void on_position(const char* stdCode, bool isLong, double prevol, double preavail, double newvol, double newavail, uint32_t tradingday) override;

	/*
	 *	
	 */
	virtual void on_entrust(uint32_t localid, const char* stdCode, bool bSuccess, const char* message) override;

	/*
	 *	����ͨ������
	 */
	virtual void on_channel_ready() override;

	/*
	 *	����ͨ����ʧ
	 */
	virtual void on_channel_lost() override;


private:
	ExecuteUnitMap		_unit_map;
	TraderAdapter*		_trader;
	WtExecuterFactory*	_factory;
	IDataManager*		_data_mgr;
	WTSVariant*			_config;

	double			_scale;
	bool			_channel_ready;

	faster_hashmap<std::string, double> _target_pos;

	typedef std::shared_ptr<boost::threadpool::pool> ThreadPoolPtr;
	ThreadPoolPtr		_pool;
};

typedef std::shared_ptr<IExecCommand> ExecCmdPtr;
typedef std::shared_ptr<WtLocalExecuter> WtExecuterPtr;

NS_OTP_END
