/*!
 * \file StateMonitor.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief ״̬����������
 */
#pragma once
#include <vector>
#include "../Share/StdUtils.hpp"
#include "../Includes/FasterDefs.h"

typedef enum tagSimpleState
{
	SS_ORIGINAL,		//δ��ʼ��
	SS_INITIALIZED,		//�ѳ�ʼ��
	SS_RECEIVING,		//������
	SS_PAUSED,			//��Ϣ��
	SS_CLOSED,			//������
	SS_PROCING,			//������ҵ��
	SS_PROCED,			//�̺��Ѵ���
	SS_Holiday	= 99	//�ڼ���
} SimpleState;

typedef struct _StateInfo
{
	char		_session[16];
	uint32_t	_init_time;
	uint32_t	_close_time;
	uint32_t	_proc_time;
	SimpleState	_state;

	typedef struct _Section
	{
		uint32_t _from;
		uint32_t _end;
	} Section;
	std::vector<Section> _sections;

	bool isInSections(uint32_t curTime)
	{
		for (auto it = _sections.begin(); it != _sections.end(); it++)
		{
			const Section& sec = *it;
			if (sec._from <= curTime && curTime < sec._end)
				return true;
		}
		return false;
	}

	_StateInfo()
	{
		_session[0] = '\0';
		_init_time = 0;
		_close_time = 0;
		_proc_time = 0;
		_state = SS_ORIGINAL;
	}
} StateInfo;

typedef std::shared_ptr<StateInfo> StatePtr;
typedef faster_hashmap<std::string, StatePtr>	StateMap;

class WTSBaseDataMgr;
class DataManager;

class StateMonitor
{
public:
	StateMonitor();
	~StateMonitor();

public:
	bool		initialize(const char* filename, WTSBaseDataMgr* bdMgr, DataManager* dtMgr);

	bool		isAnyInState(SimpleState ss) const;
	bool		isAllInState(SimpleState ss) const;

	bool		isInState(const char* sid, SimpleState ss) const;

	void		run();
	void		stop();

private:
	StateMap		_map;
	WTSBaseDataMgr*	_bd_mgr;
	DataManager*	_dt_mgr;

	StdThreadPtr	_thrd;

	bool			_stopped;
};
