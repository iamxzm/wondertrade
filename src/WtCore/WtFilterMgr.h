#pragma once
#include <string>
#include "../Includes/FasterDefs.h"
#include "../Includes/WTSMarcos.h"

NS_WTP_BEGIN

class EventNotifier;

class WtFilterMgr
{
public:
	WtFilterMgr():_filter_timestamp(0), _notifier(NULL){}

	void		set_notifier(EventNotifier* notifier) { _notifier = notifier; }

	/*
	 *	�����źŹ�����
	 */
	void		load_filters(const char* fileName = "");

	/*
	 *	�Ƿ���Ϊ���Ա����˵���
	 *	����������Ǻ��ԵĻ�, �ͻ᷵��true, ������ض����λ, �ͻ᷵��false, ��Ŀ���λ��һ�����������Ĺ���ֵ
	 *
	 *	@sname		��������
	 *	@targetPos	Ŀ���λ, �Ը�����Ϊ׼
	 *	@isDiff		�Ƿ�������
	 *
	 *	return		�Ƿ���˵���, ������˵���, �óֲ־Ͳ������������Ŀ���λ
	 */
	bool		is_filtered_by_strategy(const char* straName, double& targetPos, bool isDiff = false);

	/*
	 *	�Ƿ���Ϊ���뱻���˵���
	 *	����������Ǻ��ԵĻ�, �ͻ᷵��true, ������ض����λ, �ͻ᷵��false, ��Ŀ���λ��һ�����������Ĺ���ֵ
	 *
	 *	@stdCode	��׼��Լ����
	 *	@targetPos	Ŀ���λ, �Ը�����Ϊ׼
	 *
	 *	return		�Ƿ���˵���, ������˵���, �óֲ־Ͳ������������Ŀ���λ
	 */
	bool		is_filtered_by_code(const char* stdCode, double& targetPos);

	/*
	 *	�Ƿ�ִ�������������˵���
	 *
	 *	@channelid	����ͨ��ID
	 * 
	 *	return		�Ƿ񱻹��˵��ˣ�������˵��ˣ���ִ�����Ͳ�ִ���κ��ź���
	 */
	bool		is_filtered_by_executer(const char* execid);

private:
	//////////////////////////////////////////////////////////////////////////
	//�źŹ�����
	typedef enum tagFilterAction
	{
		FA_Ignore,		//����, ��ά��ԭ�в�λ
		FA_Redirect,	//�ض���ֲ�, ��ͬ����ָ��Ŀ���λ
		FA_None = 99
	} FilterAction;

	typedef struct _FilterItem
	{
		std::string		_key;		//�ؼ���
		FilterAction	_action;	//���˲���
		double			_target;	//Ŀ���λ, ֻ�е�_actionΪFA_Redirect����Ч
	} FilterItem;

	typedef faster_hashmap<LongKey, FilterItem>	FilterMap;
	FilterMap		_stra_filters;	//���Թ�����

	FilterMap		_code_filters;	//���������, ������Լ�����Ʒ�ִ���, ͬһʱ��ֻ��һ����Ч, ��Լ�������ȼ�����Ʒ�ִ���

	//����ͨ��������
	typedef faster_hashmap<LongKey, bool>	ExecuterFilters;
	ExecuterFilters	_exec_filters;

	std::string		_filter_file;	//�����������ļ�
	uint64_t		_filter_timestamp;	//�������ļ�ʱ���

	EventNotifier*	_notifier;
};

NS_WTP_END

