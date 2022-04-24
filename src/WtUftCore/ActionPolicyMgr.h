/*!
 * \file ActionPolicyMgr.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#pragma once
#include <vector>
#include <stdint.h>
#include <string.h>

#include "../Includes/FasterDefs.h"


NS_WTP_BEGIN
class WTSVariant;

typedef enum tagActionType
{
	AT_Unknown = 8888,
	AT_Open = 9999,		//����
	AT_Close,			//ƽ��
	AT_CloseToday,		//ƽ��
	AT_CloseYestoday	//ƽ��
} ActionType;

typedef struct _ActionRule
{
	ActionType	_atype;		//��������
	uint32_t	_limit;		//��������
	uint32_t	_limit_l;	//��ͷ��������
	uint32_t	_limit_s;	//��ͷ��������
	bool		_pure;		//��Ҫ���AT_CloseToday��AT_CloseYestoday�������ж��Ƿ��Ǿ���ֻ��߾����

	_ActionRule()
	{
		memset(this, 0, sizeof(_ActionRule));
	}
} ActionRule;

typedef std::vector<ActionRule>	ActionRuleGroup;

class ActionPolicyMgr
{
public:
	ActionPolicyMgr();
	~ActionPolicyMgr();

public:
	bool init(const char* filename);

	const ActionRuleGroup& getActionRules(const char* pid);

private:
	typedef faster_hashmap<ShortKey, ActionRuleGroup> RulesMap;
	RulesMap	_rules;	//�����

	faster_hashmap<ShortKey, std::string> _comm_rule_map;	//Ʒ�ֹ���ӳ��
};

NS_WTP_END
