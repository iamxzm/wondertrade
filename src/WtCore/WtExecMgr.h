#pragma once
#include <functional>
#include "WtLocalExecuter.h"

NS_OTP_BEGIN
class WtFilterMgr;

typedef std::function<void(ExecCmdPtr)> EnumExecuterCb;

class WtExecuterMgr : private boost::noncopyable
{
public:
	WtExecuterMgr():_filter_mgr(NULL){}

	inline void set_filter_mgr(WtFilterMgr* mgr) { _filter_mgr = mgr; }

	inline void	add_executer(ExecCmdPtr executer)
	{
		_executers.emplace_back(executer);
	}

	void	enum_executer(EnumExecuterCb cb);

	void	set_positions(faster_hashmap<std::string, double> target_pos);
	void	handle_pos_change(const char* stdCode, double targetPos);
	void	handle_tick(const char* stdCode, WTSTickData* curTick);

private:
	typedef std::vector<ExecCmdPtr> ExecuterList;
	ExecuterList	_executers;

	WtFilterMgr*	_filter_mgr;
};
NS_OTP_END
