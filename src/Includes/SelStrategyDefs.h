/*!
* \file MfStrategyDefs.h
* \project	WonderTrader
*
* \author Wesley
* \date 2020/03/30
*
* \brief
*/
#pragma once
#include <string>
#include <stdint.h>

#include "../Includes/WTSMarcos.h"

NS_WTP_BEGIN
class WTSVariant;
class ISelStraCtx;
class WTSTickData;
struct WTSBarStruct;
NS_WTP_END

USING_NS_WTP;

class SelStrategy
{
public:
	SelStrategy(const char* id) :_id(id){}
	virtual ~SelStrategy(){}

public:
	/*
	*	ִ�е�Ԫ����
	*/
	virtual const char* getName() = 0;

	/*
	*	����ִ������������
	*/
	virtual const char* getFactName() = 0;

	/*
	*	��ʼ��
	*/
	virtual bool init(WTSVariant* cfg){ return true; }

	virtual const char* id() const { return _id.c_str(); }

	/*
	*	��ʼ���ص�
	*/
	virtual void on_init(ISelStraCtx* ctx){}

	/*
	 *	�����տ�ʼ
	 */
	virtual void on_session_begin(ISelStraCtx* ctx, uint32_t uTDate) {}

	/*
	 *	�����ս���
	 */
	virtual void on_session_end(ISelStraCtx* ctx, uint32_t uTDate) {}

	/*
	*	�����߼�ִ�����
	*/
	virtual void on_schedule(ISelStraCtx* ctx, uint32_t uDate, uint32_t uTime){}

	/*
	*	tick����
	*/
	virtual void on_tick(ISelStraCtx* ctx, const char* stdCode, WTSTickData* newTick){}

	/*
	*	K�߱պ�
	*/
	virtual void on_bar(ISelStraCtx* ctx, const char* stdCode, const char* period, WTSBarStruct* newBar){}

protected:
	std::string _id;
};

//////////////////////////////////////////////////////////////////////////
//���Թ����ӿ�
typedef void(*FuncEnumSelStrategyCallback)(const char* factName, const char* straName, bool isLast);

class ISelStrategyFact
{
public:
	ISelStrategyFact(){}
	virtual ~ISelStrategyFact(){}

public:
	/*
	*	��ȡ������
	*/
	virtual const char* getName() = 0;

	/*
	*	ö�ٲ���
	*/
	virtual void enumStrategy(FuncEnumSelStrategyCallback cb) = 0;

	/*
	*	�������ƴ���K�߼������
	*/
	virtual SelStrategy* createStrategy(const char* name, const char* id) = 0;


	/*
	*	ɾ������
	*/
	virtual bool deleteStrategy(SelStrategy* stra) = 0;
};

//��������
typedef ISelStrategyFact* (*FuncCreateSelStraFact)();
//ɾ������
typedef void(*FuncDeleteSelStraFact)(ISelStrategyFact* &fact);
