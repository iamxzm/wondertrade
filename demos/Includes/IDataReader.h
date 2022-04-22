/*!
 * \file IDataReader.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#pragma once
#include <stdint.h>

#include "../Includes/WTSMarcos.h"
#include "../Includes/WTSTypes.h"

NS_OTP_BEGIN
class WTSKlineData;
class WTSKlineSlice;
class WTSTickSlice;
class WTSOrdQueSlice;
class WTSOrdDtlSlice;
class WTSTransSlice;
struct WTSBarStruct;
class WTSVariant;
class IBaseDataMgr;
class IHotMgr;

/*
 *	@brief ���ݶ�ȡģ��ص��ӿ�
 *	@details ��Ҫ�������ݶ�ȡģ�������ģ��ص�
 */
class IDataReaderSink
{
public:
	/*
	 *	@brief	K�߱պ��¼��ص�
	 *	
	 *	@param stdCode	��׼Ʒ�ִ���,��SSE.600000,SHFE.au.2005
	 *	@param period	K������
	 *	@param newBar	�պϵ�K�߽ṹָ��
	 */
	virtual void on_bar(const char* stdCode, WTSKlinePeriod period, WTSBarStruct* newBar) = 0;

	/*
	 *	@brief	���л����K��ȫ�����µ��¼��ص�
	 *
	 *	@param updateTime	K�߸���ʱ��,��ȷ������,��202004101500
	 */
	virtual void on_all_bar_updated(uint32_t updateTime) = 0;

	/*
	 *	@brief	��ȡ�������ݹ���ӿ�ָ��
	 */
	virtual IBaseDataMgr*	get_basedata_mgr() = 0;

	 /*
	  *	@brief	��ȡ�����л��������ӿ�ָ��
	  */
	virtual IHotMgr*		get_hot_mgr() = 0;

	/*
	 *	@brief	��ȡ��ǰ����,��ʽͼ20100410
	 */
	virtual uint32_t	get_date() = 0;

	/*
	 *	@brief	��ȡ��ǰ1�����ߵ�ʱ��
	 *	@details ����ķ�����ʱ���Ǵ������1������ʱ��,��������9:00:32��,��ʵ�¼�Ϊ0900,���Ƕ�Ӧ��1������ʱ��Ϊ0901
	 */
	virtual uint32_t	get_min_time() = 0;

	/*
	 *	@brief	��ȡ��ǰ������,��ȷ������,��37,500
	 */
	virtual uint32_t	get_secs() = 0;

	/*
	 *	@brief	������ݶ�ȡģ�����־
	 */
	virtual void		reader_log(WTSLogLevel ll, const char* fmt, ...) = 0;
};

/*
 *	@brief	���ݶ�ȡ�ӿ�
 *
 *	�����ģ���ṩ��������(tick��K��)��ȡ�ӿ�
 */
class IDataReader
{
public:
	IDataReader(){}
	virtual ~IDataReader(){}

public:
	/*
	 *	@brief ��ʼ�����ݶ�ȡģ��
	 *
	 *	@param cfg	ģ��������
	 *	@param sink	ģ��ص��ӿ�
	 */
	virtual void init(WTSVariant* cfg, IDataReaderSink* sink){ _sink = sink; }

	/*
	 *	@brief	�����߱պ��¼�����ӿ�
	 *	
	 *	@param uDate	�պϵķ���������,��20200410,���ﲻ�ǽ�����
	 *	@param uTime	�պϵķ����ߵķ���ʱ��,��1115
	 *	@param endTDate	����պϵķ������ǽ��������һ��������,��endTDateΪ��ǰ������,��20200410,�������Ϊ0
	 */
	virtual void onMinuteEnd(uint32_t uDate, uint32_t uTime, uint32_t endTDate = 0) = 0;

	/*
	 *	@brief	��ȡtick������Ƭ
	 *	@details ��Ƭ���Ḵ������,ֻ�ѻ����е�����ָ�봫�ݳ���,���Խ�����Ƭ
	 *
	 *	@param stdCode	��׼Ʒ�ִ���,��SSE.600000,SHFE.au.2005
	 *	@param count	Ҫ��ȡ��tick����
	 *	@param etime	����ʱ��,��ȷ������,��ʽ��yyyyMMddhhmmssmmm,���Ҫ��ȡ�����һ��,etimeΪ0,Ĭ��Ϊ0
	 */
	virtual WTSTickSlice*	readTickSlice(const char* stdCode, uint32_t count, uint64_t etime = 0) = 0;

	/*
	 *	@brief	��ȡ���ί��������Ƭ
	 *	@details ��Ƭ���Ḵ������,ֻ�ѻ����е�����ָ�봫�ݳ���,���Խ�����Ƭ
	 *
	 *	@param stdCode	��׼Ʒ�ִ���,��SSE.600000,SHFE.au.2005
	 *	@param count	Ҫ��ȡ��tick����
	 *	@param etime	����ʱ��,��ȷ������,��ʽ��yyyyMMddhhmmssmmm,���Ҫ��ȡ�����һ��,etimeΪ0,Ĭ��Ϊ0
	 */
	virtual WTSOrdDtlSlice*	readOrdDtlSlice(const char* stdCode, uint32_t count, uint64_t etime = 0) = 0;

	/*
	 *	@brief	��ȡί�ж���������Ƭ
	 *	@details ��Ƭ���Ḵ������,ֻ�ѻ����е�����ָ�봫�ݳ���,���Խ�����Ƭ
	 *
	 *	@param stdCode	��׼Ʒ�ִ���,��SSE.600000,SHFE.au.2005
	 *	@param count	Ҫ��ȡ��tick����
	 *	@param etime	����ʱ��,��ȷ������,��ʽ��yyyyMMddhhmmssmmm,���Ҫ��ȡ�����һ��,etimeΪ0,Ĭ��Ϊ0
	 */
	virtual WTSOrdQueSlice*	readOrdQueSlice(const char* stdCode, uint32_t count, uint64_t etime = 0) = 0;

	/*
	 *	@brief	��ȡ��ʳɽ�������Ƭ
	 *	@details ��Ƭ���Ḵ������,ֻ�ѻ����е�����ָ�봫�ݳ���,���Խ�����Ƭ
	 *
	 *	@param stdCode	��׼Ʒ�ִ���,��SSE.600000,SHFE.au.2005
	 *	@param count	Ҫ��ȡ��tick����
	 *	@param etime	����ʱ��,��ȷ������,��ʽ��yyyyMMddhhmmssmmm,���Ҫ��ȡ�����һ��,etimeΪ0,Ĭ��Ϊ0
	 */
	virtual WTSTransSlice*	readTransSlice(const char* stdCode, uint32_t count, uint64_t etime = 0) = 0;

	/*
	 *	@brief ��ȡK������,������һ���洢������
	 *	@details	��Ƭ���Ḵ������,ֻ�ѻ����е�����ָ�봫�ݳ���,���Խ�����Ƭ
	 *
	 *	@param	stdCode	��׼Ʒ�ִ���,��SSE.600000,SHFE.au.2005
	 *	@param	period	K������
	 *	@param	count	Ҫ��ȡ��K������
	 *	@param	etime	����ʱ��,��ʽyyyyMMddhhmm
	 */
	virtual WTSKlineSlice*	readKlineSlice(const char* stdCode, WTSKlinePeriod period, uint32_t count, uint64_t etime = 0) = 0;

protected:
	IDataReaderSink* _sink;
};

//�������ݴ洢����
typedef IDataReader* (*FuncCreateDataReader)();
//ɾ�����ݴ洢����
typedef void(*FuncDeleteDataReader)(IDataReader* store);

NS_OTP_END