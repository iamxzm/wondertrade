﻿/*!
 * \file WTSDataDef.hpp
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief Wt行情数据定义文件,包括tick、bar、orderqueue、orderdetail、transaction等数据
 */
#pragma once
#include <stdlib.h>
#include <vector>
#include <deque>
#include <string.h>
#include<chrono>

#include "WTSObject.hpp"

#include "WTSTypes.h"
#include "WTSMarcos.h"
#include "WTSStruct.h"
#include "WTSCollection.hpp"

using namespace std;

#pragma warning(disable:4267)


NS_OTP_BEGIN
/*
 *	数值数组的内部封装
 *	采用std::vector实现
 *	包含数据格式化字符串
 *	数值的数据类型为double
 */
class WTSValueArray : public WTSObject
{
protected:
	vector<double>	m_vecData;

public:
	/*
	 *	创建一个数值数组对象
	 *	@decimal 保留的小数点位数
	 */
	static WTSValueArray* create()
	{
		WTSValueArray* pRet = new WTSValueArray;
		pRet->m_vecData.clear();
		return pRet;
	}

	/*
	 *	读取数组的长度
	 */
	inline uint32_t	size() const{ return m_vecData.size(); }
	inline bool		empty() const{ return m_vecData.empty(); }

	/*
	 *	读取指定位置的数据
	 *	如果超出范围,则返回INVALID_VALUE
	 */
	inline double		at(uint32_t idx) const
	{
		idx = translateIdx(idx);

		if(idx < 0 || idx >= m_vecData.size())
			return INVALID_DOUBLE;

		return m_vecData[idx];
	}

	inline int32_t		translateIdx(int32_t idx) const
	{
		if(idx < 0)
		{
			return m_vecData.size()+idx;
		}

		return idx;
	}

	/*
	 *	找到指定范围内的最大值
	 *	如果超出范围,则返回INVALID_VALUE
	 */
	double		maxvalue(int32_t head, int32_t tail, bool isAbs = false) const
	{
		head = translateIdx(head);
		tail = translateIdx(tail);

		uint32_t begin = min(head, tail);
		uint32_t end = max(head, tail);

		if(begin <0 || begin >= m_vecData.size() || end < 0 || end > m_vecData.size())
			return INVALID_DOUBLE;

		double maxValue = INVALID_DOUBLE;
		for(uint32_t i = begin; i <= end; i++)
		{
			if(m_vecData[i] == INVALID_DOUBLE)
				continue;

			if(maxValue == INVALID_DOUBLE)
				maxValue = isAbs?abs(m_vecData[i]):m_vecData[i];
			else
				maxValue = max(maxValue, isAbs?abs(m_vecData[i]):m_vecData[i]);
		}

		//if (maxValue == INVALID_DOUBLE)
		//	maxValue = 0.0;

		return maxValue;
	}

	/*
	 *	找到指定范围内的最小值
	 *	如果超出范围,则返回INVALID_VALUE
	 */
	double		minvalue(int32_t head, int32_t tail, bool isAbs = false) const
	{
		head = translateIdx(head);
		tail = translateIdx(tail);

		uint32_t begin = min(head, tail);
		uint32_t end = max(head, tail);

		if(begin <0 || begin >= m_vecData.size() || end < 0 || end > m_vecData.size())
			return INVALID_DOUBLE;

		double minValue = INVALID_DOUBLE;
		for(uint32_t i = begin; i <= end; i++)
		{
			if (m_vecData[i] == INVALID_DOUBLE)
				continue;

			if(minValue == INVALID_DOUBLE)
				minValue = isAbs?abs(m_vecData[i]):m_vecData[i];
			else
				minValue = min(minValue, isAbs?abs(m_vecData[i]):m_vecData[i]);
		}

		//if (minValue == INVALID_DOUBLE)
		//	minValue = 0.0;

		return minValue;
	}

	/*
	 *	在数组末尾添加数据
	 */
	inline void		append(double val)
	{
		m_vecData.emplace_back(val);
	}

	/*
	 *	设置指定位置的数据
	 */
	inline void		set(uint32_t idx, double val)
	{
		if(idx < 0 || idx >= m_vecData.size())
			return;

		m_vecData[idx] = val;
	}

	/*
	 *	重新分配数组大小,并设置默认值
	 */
	inline void		resize(uint32_t uSize, double val = INVALID_DOUBLE)
	{
		m_vecData.resize(uSize, val);
	}

	/*
	 *	重载操作符[]
	 *	用法同getValue接口
	 */
	inline double&		operator[](uint32_t idx)
	{
		return m_vecData[idx];
	}

	inline double		operator[](uint32_t idx) const
	{
		return m_vecData[idx];
	}

	inline std::vector<double>& getDataRef()
	{
		return m_vecData;
	}
};

/*
 *	K线数据切片
 *	这个比较特殊,因为要拼接当日和历史的
 *	所以有两个开始地址
 */
class WTSKlineSlice : public WTSObject
{
private:
	char			m_strCode[MAX_INSTRUMENT_LENGTH];
	WTSKlinePeriod	m_kpPeriod;
	uint32_t		m_uTimes;
	WTSBarStruct*	m_bsHisBegin;
	int32_t			m_iHisCnt;
	WTSBarStruct*	m_bsRtBegin;
	int32_t			m_iRtCnt;

protected:
	WTSKlineSlice()
		:m_kpPeriod(KP_Minute1)
		, m_uTimes(1)
		, m_iHisCnt(0)
		, m_bsHisBegin(NULL)
		, m_iRtCnt(0)
		, m_bsRtBegin(NULL)
	{

	}

	inline int32_t		translateIdx(int32_t idx) const
	{
		int32_t totalCnt = m_iHisCnt + m_iRtCnt;
		if (idx < 0)
		{
			return max(0, totalCnt + idx);
		}

		return idx;
	}


public:
	static WTSKlineSlice* create(const char* code, WTSKlinePeriod period, uint32_t times, WTSBarStruct* hisHead, int32_t hisCnt, WTSBarStruct* rtHead = NULL, int32_t rtCnt = 0)
	{
		if (hisHead == NULL && rtHead == NULL)
			return NULL;

		if (hisCnt == 0 && rtCnt == 0)
			return NULL;

		WTSKlineSlice *pRet = new WTSKlineSlice;
		strcpy(pRet->m_strCode, code);
		pRet->m_kpPeriod = period;
		pRet->m_uTimes = times;
		pRet->m_bsHisBegin = hisHead;
		pRet->m_iHisCnt = hisCnt;

		pRet->m_bsRtBegin = rtHead;
		pRet->m_iRtCnt = rtCnt;

		return pRet;
	}

	inline WTSBarStruct*	get_his_addr()
	{
		return m_bsHisBegin;
	}

	inline int32_t	get_his_count()
	{
		return m_iHisCnt;
	}

	inline WTSBarStruct*	get_rt_addr()
	{
		return m_bsRtBegin;
	}

	inline int32_t	get_rt_count()
	{
		return m_iRtCnt;
	}

	inline WTSBarStruct*	at(int32_t idx)
	{
		idx = translateIdx(idx);

		if (idx < 0 || idx >= size())
			return NULL;
		
		return idx < m_iHisCnt ? &m_bsHisBegin[idx] : &m_bsRtBegin[idx - m_iHisCnt];
	}

	inline const WTSBarStruct*	at(int32_t idx) const
	{
		idx = translateIdx(idx);

		if (idx < 0 || idx >= size())
			return NULL;

		return idx < m_iHisCnt ? &m_bsHisBegin[idx] : &m_bsRtBegin[idx - m_iHisCnt];
	}


	/*
	*	查找指定范围内的最大价格
	*	@head 起始位置
	*	@tail 结束位置
	*	如果位置超出范围,返回INVALID_VALUE
	*/
	double		maxprice(int32_t head, int32_t tail) const
	{
		head = translateIdx(head);
		tail = translateIdx(tail);

		int32_t begin = max(0,min(head, tail));
		int32_t end = min(max(head, tail), size() - 1);

		double maxValue = this->at(begin)->high;
		for (int32_t i = begin; i <= end; i++)
		{
			maxValue = max(maxValue, at(i)->high);
		}
		return maxValue;
	}

	/*
	*	查找指定范围内的最小价格
	*	@head 起始位置
	*	@tail 结束位置
	*	如果位置超出范围,返回INVALID_VALUE
	*/
	double		minprice(int32_t head, int32_t tail) const
	{
		head = translateIdx(head);
		tail = translateIdx(tail);

		int32_t begin = max(0, min(head, tail));
		int32_t end = min(max(head, tail), size() - 1);

		double minValue = at(begin)->low;
		for (int32_t i = begin; i <= end; i++)
		{
			minValue = min(minValue, at(i)->low);
		}

		return minValue;
	}

	/*
	*	返回K线的大小
	*/
	inline int32_t	size() const{ return m_iHisCnt + m_iRtCnt; }
	inline bool	empty() const{ return (m_iHisCnt + m_iRtCnt) == 0; }

	/*
	*	返回K线对象的合约代码
	*/
	inline const char*	code() const{ return m_strCode; }
	inline void		setCode(const char* code){ strcpy(m_strCode, code); }


	/*
	*	读取指定位置的开盘价
	*	如果超出范围则返回INVALID_VALUE
	*/
	double	open(int32_t idx) const
	{
		idx = translateIdx(idx);

		if (idx < 0 || idx >= size())
			return INVALID_DOUBLE;

		if (idx < m_iHisCnt)
			if (m_bsHisBegin == NULL)
				return INVALID_DOUBLE;
			else
				return (m_bsHisBegin + idx)->open;
		else if (idx < size())
			if (m_bsRtBegin == NULL)
				return INVALID_DOUBLE;
			else
				return (m_bsRtBegin + (idx - m_iHisCnt))->open;

		return INVALID_DOUBLE;
	}

	/*
	*	读取指定位置的最高价
	*	如果超出范围则返回INVALID_VALUE
	*/
	double	high(int32_t idx) const
	{
		idx = translateIdx(idx);

		if (idx < 0 || idx >= size())
			return INVALID_DOUBLE;

		if (idx < m_iHisCnt)
			if (m_bsHisBegin == NULL)
				return INVALID_DOUBLE;
			else
				return (m_bsHisBegin + idx)->high;
		else if (idx < size())
			if (m_bsRtBegin == NULL)
				return INVALID_DOUBLE;
			else
				return (m_bsRtBegin + (idx - m_iHisCnt))->high;

		return INVALID_DOUBLE;
	}

	/*
	*	读取指定位置的最低价
	*	如果超出范围则返回INVALID_VALUE
	*/
	double	low(int32_t idx) const
	{
		idx = translateIdx(idx);

		if (idx < 0 || idx >= size())
			return INVALID_DOUBLE;

		if (idx < m_iHisCnt)
			if (m_bsHisBegin == NULL)
				return INVALID_DOUBLE;
			else
				return (m_bsHisBegin + idx)->low;
		else if (idx < size())
			if (m_bsRtBegin == NULL)
				return INVALID_DOUBLE;
			else
				return (m_bsRtBegin + (idx - m_iHisCnt))->low;

		return INVALID_DOUBLE;
	}

	/*
	*	读取指定位置的收盘价
	*	如果超出范围则返回INVALID_VALUE
	*/
	double	close(int32_t idx) const
	{
		idx = translateIdx(idx);

		if (idx < 0 || idx >= size())
			return INVALID_DOUBLE;

		if (idx < m_iHisCnt)
			if (m_bsHisBegin == NULL)
				return INVALID_DOUBLE;
			else
				return (m_bsHisBegin + idx)->close;
		else if (idx < size())
			if (m_bsRtBegin == NULL)
				return INVALID_DOUBLE;
			else
				return (m_bsRtBegin + (idx - m_iHisCnt))->close;

		return INVALID_DOUBLE;
	}

	/*
	*	读取指定位置的成交量
	*	如果超出范围则返回INVALID_VALUE
	*/
	uint32_t	volume(int32_t idx) const
	{
		idx = translateIdx(idx);

		if (idx < 0 || idx >= size())
			return INVALID_UINT32;

		if (idx < m_iHisCnt)
			if (m_bsHisBegin == NULL)
				return INVALID_UINT32;
			else
				return (m_bsHisBegin + idx)->vol;
		else if (idx < size())
			if (m_bsRtBegin == NULL)
				return INVALID_UINT32;
			else
				return (m_bsRtBegin + (idx - m_iHisCnt))->vol;

		return INVALID_UINT32;
	}

	/*
	*	读取指定位置的总持
	*	如果超出范围则返回INVALID_VALUE
	*/
	uint32_t	openinterest(int32_t idx) const
	{
		idx = translateIdx(idx);

		if (idx < 0 || idx >= size())
			return INVALID_UINT32;

		if (idx < m_iHisCnt)
			if (m_bsHisBegin == NULL)
				return INVALID_UINT32;
			else
				return (m_bsHisBegin + idx)->hold;
		else if (idx < size())
			if (m_bsRtBegin == NULL)
				return INVALID_UINT32;
			else
				return (m_bsRtBegin + (idx - m_iHisCnt))->hold;

		return INVALID_UINT32;
	}

	/*
	*	读取指定位置的增仓
	*	如果超出范围则返回INVALID_VALUE
	*/
	int32_t	additional(int32_t idx) const
	{
		idx = translateIdx(idx);

		if (idx < 0 || idx >= size())
			return INVALID_INT32;

		if (idx < m_iHisCnt)
			if (m_bsHisBegin == NULL)
				return INVALID_INT32;
			else
				return (m_bsHisBegin + idx)->add;
		else if (idx < size())
			if (m_bsRtBegin == NULL)
				return INVALID_INT32;
			else
				return (m_bsRtBegin + (idx - m_iHisCnt))->add;

		return INVALID_INT32;
	}

	/*
	*	读取指定位置的成交额
	*	如果超出范围则返回INVALID_VALUE
	*/
	double	money(int32_t idx) const
	{
		idx = translateIdx(idx);

		if (idx < 0 || idx >= size())
			return INVALID_DOUBLE;

		if (idx < m_iHisCnt)
			if (m_bsHisBegin == NULL)
				return INVALID_DOUBLE;
			else
				return (m_bsHisBegin + idx)->money;
		else if (idx < size())
			if (m_bsRtBegin == NULL)
				return INVALID_DOUBLE;
			else
				return (m_bsRtBegin + (idx - m_iHisCnt))->money;

		return INVALID_DOUBLE;
	}

	/*
	*	读取指定位置的日期
	*	如果超出范围则返回INVALID_VALUE
	*/
	uint32_t	date(int32_t idx) const
	{
		idx = translateIdx(idx);

		if (idx < 0 || idx >= size())
			return INVALID_UINT32;

		if (idx < m_iHisCnt)
			if (m_bsHisBegin == NULL)
				return INVALID_UINT32;
			else
				return (m_bsHisBegin + idx)->date;
		else if (idx < size())
			if (m_bsRtBegin == NULL)
				return INVALID_UINT32;
			else
				return (m_bsRtBegin + (idx - m_iHisCnt))->date;

		return INVALID_UINT32;
	}

	/*
	*	读取指定位置的时间
	*	如果超出范围则返回INVALID_VALUE
	*/
	uint32_t	time(int32_t idx) const
	{
		idx = translateIdx(idx);

		if (idx < 0 || idx >= size())
			return INVALID_UINT32;

		if (idx < m_iHisCnt)
			if (m_bsHisBegin == NULL)
				return INVALID_UINT32;
			else
				return (m_bsHisBegin + idx)->time;
		else if (idx < size())
			if (m_bsRtBegin == NULL)
				return INVALID_UINT32;
			else
				return (m_bsRtBegin + (idx - m_iHisCnt))->time;

		return INVALID_UINT32;
	}

	/*
	*	将指定范围内的某个特定字段的数据全部抓取出来
	*	并保存的一个数值数组中
	*	如果超出范围,则返回NULL
	*	@type 支持的类型有KT_OPEN、KT_HIGH、KT_LOW、KT_CLOSE,KFT_VOLUME、KT_DATE
	*/
	WTSValueArray*	extractData(WTSKlineFieldType type, int32_t head = 0, int32_t tail = -1) const
	{
		if (m_bsHisBegin == NULL && m_bsRtBegin == NULL)
			return NULL;

		if (m_iHisCnt == 0 && m_iRtCnt == 0)
			return NULL;

		head = translateIdx(head);
		tail = translateIdx(tail);

		int32_t begin = max(0, min(head, tail));
		int32_t end = min(max(head, tail), size() - 1);

		WTSValueArray *vArray = NULL;

		vArray = WTSValueArray::create();

		for (int32_t i = begin; i <= end; i++)
		{
			const WTSBarStruct& day = i < m_iHisCnt ? m_bsHisBegin[i] : m_bsRtBegin[i - m_iHisCnt];
			switch (type)
			{
			case KFT_OPEN:
				vArray->append(day.open);
				break;
			case KFT_HIGH:
				vArray->append(day.high);
				break;
			case KFT_LOW:
				vArray->append(day.low);
				break;
			case KFT_CLOSE:
				vArray->append(day.close);
				break;
			case KFT_VOLUME:
				vArray->append(day.vol);
				break;
			case KFT_SVOLUME:
				if (day.vol > INT_MAX)
					vArray->append(1 * ((day.close > day.open) ? 1 : -1));
				else
					vArray->append((int32_t)day.vol * ((day.close > day.open) ? 1 : -1));
				break;
			case KFT_DATE:
				vArray->append(day.date);
				break;
			}
		}

		return vArray;
	}
};

/*
 *	K线数据
 *	K线数据的内部数据使用WTSBarStruct
 *	WTSBarStruct是一个结构体
 *	因为K线数据单独使用的可能性较低
 *	所以不做WTSObject派生类的封装
 */
class WTSKlineData : public WTSObject
{
public:
	typedef std::vector<WTSBarStruct> WTSBarList;

protected:
	char			m_strCode[32];
	WTSKlinePeriod	m_kpPeriod;
	uint32_t		m_uTimes;
	bool			m_bUnixTime;	//是否是时间戳格式,目前只在秒线上有效
	WTSBarList		m_vecBarData;
	bool			m_bClosed;		//是否是闭合K线

protected:
	WTSKlineData()
		:m_kpPeriod(KP_Minute1)
		,m_uTimes(1)
		,m_bUnixTime(false)
		,m_bClosed(true)
	{

	}

	inline int32_t		translateIdx(int32_t idx) const
	{
		if(idx < 0)
		{
			return max(0, (int32_t)m_vecBarData.size() + idx);
		}

		return idx;
	}

public:
	/*
	 *	创建一个K线数据对象
	 *	@code 要创建的合约代码
	 *	@size 初始分配的数据长度
	 */
	static WTSKlineData* create(const char* code, uint32_t size)
	{
		WTSKlineData *pRet = new WTSKlineData;
		pRet->m_vecBarData.resize(size);
		strcpy(pRet->m_strCode, code);

		return pRet;
	}

	inline void setClosed(bool bClosed){ m_bClosed = bClosed; }
	inline bool isClosed() const{ return m_bClosed; }

	/*
	 *	设置周期和步长
	 *	@period	基础周期
	 *	@times 倍数
	 */
	inline void	setPeriod(WTSKlinePeriod period, uint32_t times = 1){ m_kpPeriod = period; m_uTimes = times; }

	inline void	setUnixTime(bool bEnabled = true){ m_bUnixTime = bEnabled; }

	inline WTSKlinePeriod	period() const{ return m_kpPeriod; }
	inline uint32_t		times() const{ return m_uTimes; }
	inline bool			isUnixTime() const{ return m_bUnixTime; }

	/*
	 *	查找指定范围内的最大价格
	 *	@head 起始位置
	 *	@tail 结束位置
	 *	如果位置超出范围,返回INVALID_VALUE
	 */
	inline double		maxprice(int32_t head, int32_t tail) const
	{
		head = translateIdx(head);
		tail = translateIdx(tail);

		uint32_t begin = min(head, tail);
		uint32_t end = max(head, tail);

		if(begin >= m_vecBarData.size() || end > m_vecBarData.size())
			return INVALID_DOUBLE;

		double maxValue = m_vecBarData[begin].high;
		for(uint32_t i = begin; i <= end; i++)
		{
			maxValue = max(maxValue, m_vecBarData[i].high);
		}

		return maxValue;
	}

	/*
	 *	查找指定范围内的最小价格
	 *	@head 起始位置
	 *	@tail 结束位置
	 *	如果位置超出范围,返回INVALID_VALUE
	 */
	inline double		minprice(int32_t head, int32_t tail) const
	{
		head = translateIdx(head);
		tail = translateIdx(tail);

		uint32_t begin = min(head, tail);
		uint32_t end = max(head, tail);

		if(begin >= m_vecBarData.size() || end > m_vecBarData.size())
			return INVALID_DOUBLE;

		double minValue = m_vecBarData[begin].low;
		for(uint32_t i = begin; i <= end; i++)
		{
			minValue = min(minValue, m_vecBarData[i].low);
		}

		return minValue;
	}
	
	/*
	 *	返回K线的大小
	 */
	inline uint32_t	size() const{return m_vecBarData.size();}
	inline bool IsEmpty() const{ return m_vecBarData.empty(); }

	/*
	 *	返回K线对象的合约代码
	 */
	inline const char*	code() const{ return m_strCode; }
	inline void		setCode(const char* code){ strcpy(m_strCode, code); }

	/*
	 *	读取指定位置的开盘价
	 *	如果超出范围则返回INVALID_VALUE
	 */
	inline double	open(int32_t idx) const
	{
		idx = translateIdx(idx);

		if(idx < 0 || idx >= (int32_t)m_vecBarData.size())
			return INVALID_DOUBLE;

		return m_vecBarData[idx].open;
	}

	/*
	 *	读取指定位置的最高价
	 *	如果超出范围则返回INVALID_VALUE
	 */
	inline double	high(int32_t idx) const
	{
		idx = translateIdx(idx);

		if(idx < 0 || idx >= (int32_t)m_vecBarData.size())
			return INVALID_DOUBLE;

		return m_vecBarData[idx].high;
	}

	/*
	 *	读取指定位置的最低价
	 *	如果超出范围则返回INVALID_VALUE
	 */
	inline double	low(int32_t idx) const
	{
		idx = translateIdx(idx);

		if(idx < 0 || idx >= (int32_t)m_vecBarData.size())
			return INVALID_DOUBLE;

		return m_vecBarData[idx].low;
	}

	/*
	 *	读取指定位置的收盘价
	 *	如果超出范围则返回INVALID_VALUE
	 */
	inline double	close(int32_t idx) const
	{
		idx = translateIdx(idx);

		if(idx < 0 || idx >= (int32_t)m_vecBarData.size())
			return INVALID_DOUBLE;

		return m_vecBarData[idx].close;
	}

	/*
	 *	读取指定位置的成交量
	 *	如果超出范围则返回INVALID_VALUE
	 */
	inline uint32_t	volume(int32_t idx) const
	{
		idx = translateIdx(idx);

		if(idx < 0 || idx >= (int32_t)m_vecBarData.size())
			return INVALID_UINT32;

		return m_vecBarData[idx].vol;
	}

	/*
	 *	读取指定位置的总持
	 *	如果超出范围则返回INVALID_VALUE
	 */
	inline uint32_t	openinterest(int32_t idx) const
	{
		idx = translateIdx(idx);

		if(idx < 0 || idx >= (int32_t)m_vecBarData.size())
			return INVALID_UINT32;

		return m_vecBarData[idx].hold;
	}

	/*
	 *	读取指定位置的增仓
	 *	如果超出范围则返回INVALID_VALUE
	 */
	inline int32_t	additional(int32_t idx) const
	{
		idx = translateIdx(idx);

		if(idx < 0 || idx >= (int32_t)m_vecBarData.size())
			return INVALID_INT32;

		return m_vecBarData[idx].add;
	}	

	/*
	 *	读取指定位置的成交额
	 *	如果超出范围则返回INVALID_VALUE
	 */
	inline double	money(int32_t idx) const
	{
		idx = translateIdx(idx);

		if(idx < 0 || idx >= (int32_t)m_vecBarData.size())
			return INVALID_DOUBLE;

		return m_vecBarData[idx].money;
	}

	/*
	 *	读取指定位置的日期
	 *	如果超出范围则返回INVALID_VALUE
	 */
	inline uint32_t	date(int32_t idx) const
	{
		idx = translateIdx(idx);

		if(idx < 0 || idx >= (int32_t)m_vecBarData.size())
			return INVALID_UINT32;

		return m_vecBarData[idx].date;
	}

	/*
	 *	读取指定位置的时间
	 *	如果超出范围则返回INVALID_VALUE
	 */
	inline uint32_t	time(int32_t idx) const
	{
		idx = translateIdx(idx);

		if(idx < 0 || idx >= (int32_t)m_vecBarData.size())
			return INVALID_UINT32;

		return m_vecBarData[idx].time;
	}

	/*
	 *	将指定范围内的某个特定字段的数据全部抓取出来
	 *	并保存的一个数值数组中
	 *	如果超出范围,则返回NULL
	 *	@type 支持的类型有KT_OPEN、KT_HIGH、KT_LOW、KT_CLOSE,KFT_VOLUME、KT_DATE
	 */
	WTSValueArray*	extractData(WTSKlineFieldType type, int32_t head = 0, int32_t tail = -1) const
	{
		head = translateIdx(head);
		tail = translateIdx(tail);

		uint32_t begin = min(head, tail);
		uint32_t end = max(head, tail);

		if(begin >= m_vecBarData.size() || end >= (int32_t)m_vecBarData.size())
			return NULL;

		WTSValueArray *vArray = NULL;

		vArray = WTSValueArray::create();

		for(uint32_t i = 0; i < m_vecBarData.size(); i++)
		{
			const WTSBarStruct& day = m_vecBarData.at(i);
			switch(type)
			{
			case KFT_OPEN:
				vArray->append(day.open);
				break;
			case KFT_HIGH:
				vArray->append(day.high);
				break;
			case KFT_LOW:
				vArray->append(day.low);
				break;
			case KFT_CLOSE:
				vArray->append(day.close);
				break;
			case KFT_VOLUME:
				vArray->append(day.vol);
				break;
			case KFT_SVOLUME:
				if(day.vol > INT_MAX)
					vArray->append(1 * ((day.close > day.open) ? 1 : -1));
				else
					vArray->append((int32_t)day.vol * ((day.close > day.open)?1:-1));
				break;
			case KFT_DATE:
				vArray->append(day.date);
				break;
			}
		}

		return vArray;
	}

public:
	/*
	 *	获取K线内部vector的引用
	 */
	inline WTSBarList& getDataRef(){ return m_vecBarData; }

	inline WTSBarStruct*	at(int32_t idx)
	{
		idx = translateIdx(idx);

		if(idx < 0 || idx >= (int32_t)m_vecBarData.size())
			return NULL;
		return &m_vecBarData[idx];
	}

	/*
	 *	释放K线数据
	 *	并delete所有的日线数据,清空vector
	 */
	virtual void release()
	{
		if(isSingleRefs())
		{
			m_vecBarData.clear();
		}		

		WTSObject::release();
	}

	/*
	 *	追加一条K线
	 */
	inline void	appendBar(const WTSBarStruct& bar)
	{
		if(m_vecBarData.empty())
		{
			m_vecBarData.emplace_back(bar);
		}
		else
		{
			WTSBarStruct* lastBar = at(-1);
			if(lastBar->date==bar.date && lastBar->time==bar.time)
			{
				memcpy(lastBar, &bar, sizeof(WTSBarStruct));
			}
			else
			{
				m_vecBarData.emplace_back(bar);
			}
		}
	}
};



/*
 *	Tick数据对象
 *	内部封装WTSTickStruct
 *	封装的主要目的是出于跨语言的考虑
 */
class WTSTickData : public WTSObject
{
public:
	/*
	 *	创建一个tick数据对象
	 *	@stdCode 合约代码
	 */
	static inline WTSTickData* create(const char* stdCode)
	{
		WTSTickData* pRet = new WTSTickData;
		strcpy(pRet->m_tickStruct.code, stdCode);

		return pRet;
	}

	/*
	 *	根据tick结构体创建一个tick数据对象
	 *	@tickData tick结构体
	 */
	static inline WTSTickData* create(WTSTickStruct& tickData)
	{
		WTSTickData* pRet = new WTSTickData;
		memcpy(&pRet->m_tickStruct, &tickData, sizeof(WTSTickStruct));

		return pRet;
	}

	inline void setCode(const char* code)
	{
		strcpy(m_tickStruct.code, code);
	}

	/*
	 *	读取合约代码
	 */
	inline const char* code() const{ return m_tickStruct.code; }

	/*
	 *	读取市场代码
	 */
	inline const char*	exchg() const{ return m_tickStruct.exchg; }

	/*
	 *	读取最新价
	 */
	inline double	price() const{ return m_tickStruct.price; }

	inline double	open() const{ return m_tickStruct.open; }

	/*
	 *	最高价
	 */
	inline double	high() const{ return m_tickStruct.high; }

	/*
	 *	最低价
	 */
	inline double	low() const{ return m_tickStruct.low; }

	//昨收价,如果是期货则是昨结算
	inline double	preclose() const{ return m_tickStruct.pre_close; }
	inline double	presettle() const{ return m_tickStruct.pre_settle; }
	inline int32_t	preinterest() const{ return m_tickStruct.pre_interest; }

	inline double	upperlimit() const{ return m_tickStruct.upper_limit; }
	inline double	lowerlimit() const{ return m_tickStruct.lower_limit; }
	//成交量
	inline uint32_t	totalvolume() const{ return m_tickStruct.total_volume; }

	//成交量
	inline uint32_t	volume() const{ return m_tickStruct.volume; }

	//结算价
	inline double	settlepx() const{ return m_tickStruct.settle_price; }

	//总持
	inline uint32_t	openinterest() const{ return m_tickStruct.open_interest; }

	inline int32_t	additional() const{ return m_tickStruct.diff_interest; }

	//成交额
	inline double	totalturnover() const{ return m_tickStruct.total_turnover; }

	//成交额
	inline double	turnover() const{ return m_tickStruct.turn_over; }

	//交易日
	inline uint32_t	tradingdate() const{ return m_tickStruct.trading_date; }

	//数据发生日期
	inline uint32_t	actiondate() const{ return m_tickStruct.action_date; }

	//数据发生时间
	inline uint32_t	actiontime() const{ return m_tickStruct.action_time; }


	/*
	 *	读取指定档位的委买价
	 *	@idx 0-9
	 */
	inline double		bidprice(int idx) const
	{
		if(idx < 0 || idx >= 10) 
			return -1;

		return m_tickStruct.bid_prices[idx];
	}

	/*
	 *	读取指定档位的委卖价
	 *	@idx 0-9
	 */
	inline double		askprice(int idx) const
	{
		if(idx < 0 || idx >= 10) 
			return -1;

		return m_tickStruct.ask_prices[idx];
	}

	/*
	 *	读取指定档位的委买量
	 *	@idx 0-9
	 */
	inline uint32_t	bidqty(int idx) const
	{
		if(idx < 0 || idx >= 10) 
			return -1;

		return m_tickStruct.bid_qty[idx];
	}

	/*
	 *	读取指定档位的委卖量
	 *	@idx 0-9
	 */
	inline uint32_t	askqty(int idx) const
	{
		if(idx < 0 || idx >= 10) 
			return -1;

		return m_tickStruct.ask_qty[idx];
	}

	/*
	 *	返回tick结构体的引用
	 */
	inline WTSTickStruct&	getTickStruct(){ return m_tickStruct; }

	inline uint64_t getLocalTime() const { return m_uLocalTime; }

private:
	WTSTickStruct	m_tickStruct;
	uint64_t		m_uLocalTime;	//本地时间

	WTSTickData():m_uLocalTime(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()){}
};

/*
 *	K线数据封装,派生于通用的基础类,便于传递
 */
class WTSBarData : public WTSObject
{
public:
	inline static WTSBarData* create()
	{
		WTSBarData* pRet = new WTSBarData;
		return pRet;
	}

	inline static WTSBarData* create(WTSBarStruct& barData, uint16_t market, const char* code)
	{
		WTSBarData* pRet = new WTSBarData;
		pRet->m_market = market;
		pRet->m_strCode = code;
		memcpy(&pRet->m_barStruct, &barData, sizeof(WTSBarStruct));

		return pRet;
	}

	inline WTSBarStruct&	getBarStruct(){return m_barStruct;}

	inline uint16_t	getMarket(){return m_market;}
	inline const char* getCode(){return m_strCode.c_str();}

private:
	WTSBarStruct	m_barStruct;
	uint16_t		m_market;
	std::string		m_strCode;
};

class WTSOrdQueData : public WTSObject
{
public:
	static inline WTSOrdQueData* create(const char* code)
	{
		WTSOrdQueData* pRet = new WTSOrdQueData;
		strcpy(pRet->m_oqStruct.code, code);
		return pRet;
	}

	static inline WTSOrdQueData* create(WTSOrdQueStruct& ordQueData)
	{
		WTSOrdQueData* pRet = new WTSOrdQueData;
		memcpy(&pRet->m_oqStruct, &ordQueData, sizeof(WTSOrdQueStruct));

		return pRet;
	}

	inline WTSOrdQueStruct& getOrdQueStruct(){return m_oqStruct;}

	inline const char* exchg() const{ return m_oqStruct.exchg; }
	inline const char* code() const{ return m_oqStruct.code; }
	inline uint32_t tradingdate() const{ return m_oqStruct.trading_date; }
	inline uint32_t actiondate() const{ return m_oqStruct.action_date; }
	inline uint32_t actiontime() const { return m_oqStruct.action_time; }

	inline void		setCode(const char* code) { strcpy(m_oqStruct.code, code); }

private:
	WTSOrdQueStruct	m_oqStruct;
};

class WTSOrdDtlData : public WTSObject
{
public:
	static inline WTSOrdDtlData* create(const char* code)
	{
		WTSOrdDtlData* pRet = new WTSOrdDtlData;
		strcpy(pRet->m_odStruct.code, code);
		return pRet;
	}

	static inline WTSOrdDtlData* create(WTSOrdDtlStruct& odData)
	{
		WTSOrdDtlData* pRet = new WTSOrdDtlData;
		memcpy(&pRet->m_odStruct, &odData, sizeof(WTSOrdDtlStruct));

		return pRet;
	}

	inline WTSOrdDtlStruct& getOrdDtlStruct(){ return m_odStruct; }

	inline const char* exchg() const{ return m_odStruct.exchg; }
	inline const char* code() const{ return m_odStruct.code; }
	inline uint32_t tradingdate() const{ return m_odStruct.trading_date; }
	inline uint32_t actiondate() const{ return m_odStruct.action_date; }
	inline uint32_t actiontime() const { return m_odStruct.action_time; }

	inline void		setCode(const char* code) { strcpy(m_odStruct.code, code); }

private:
	WTSOrdDtlStruct	m_odStruct;
};

class WTSTransData : public WTSObject
{
public:
	static inline WTSTransData* create(const char* code)
	{
		WTSTransData* pRet = new WTSTransData;
		strcpy(pRet->m_tsStruct.code, code);
		return pRet;
	}

	static inline WTSTransData* create(WTSTransStruct& transData)
	{
		WTSTransData* pRet = new WTSTransData;
		memcpy(&pRet->m_tsStruct, &transData, sizeof(WTSTransStruct));

		return pRet;
	}

	inline const char* exchg() const{ return m_tsStruct.exchg; }
	inline const char* code() const{ return m_tsStruct.code; }
	inline uint32_t tradingdate() const{ return m_tsStruct.trading_date; }
	inline uint32_t actiondate() const{ return m_tsStruct.action_date; }
	inline uint32_t actiontime() const { return m_tsStruct.action_time; }

	inline WTSTransStruct& getTransStruct(){ return m_tsStruct; }

	inline void		setCode(const char* code) { strcpy(m_tsStruct.code, code); }

private:
	WTSTransStruct	m_tsStruct;
};

/*
 *	@brief 历史Tick数据数组
 *	@details 内部使用WTSArray作为容器
 */
class WTSHisTickData : public WTSObject
{
protected:
	char						m_strCode[32];
	std::vector<WTSTickStruct>	m_ayTicks;
	bool						m_bValidOnly;

	WTSHisTickData() :m_bValidOnly(false){}

public:
	/*
	 *	@brief 创建指定大小的tick数组对象
	 *	@details 内部的数组预先分配大小
	 *
	 *	@param stdCode 合约代码
	 *	@param nSize 预先分配的大小
	 */
	static inline WTSHisTickData* create(const char* stdCode, unsigned int nSize = 0, bool bValidOnly = false)
	{
		WTSHisTickData *pRet = new WTSHisTickData;
		strcpy(pRet->m_strCode, stdCode);
		pRet->m_ayTicks.resize(nSize);
		pRet->m_bValidOnly = bValidOnly;

		return pRet;
	}

	/*
	 *	@brief 根据tick数组对象创建历史tick数据对象
	 *	@details 内部的tick数组不用再分配了

	 *	@param ayTicks tick数组对象指针
	 */
	static inline WTSHisTickData* create(const char* stdCode, const std::vector<WTSTickStruct>& ayTicks, bool bValidOnly = false)
	{
		WTSHisTickData *pRet = new WTSHisTickData;
		strcpy(pRet->m_strCode, stdCode);
		pRet->m_ayTicks = ayTicks;
		pRet->m_bValidOnly = bValidOnly;

		return pRet;
	}

	//读取tick数据的条数
	inline uint32_t	size() const{ return m_ayTicks.size(); }
	inline bool		empty() const{ return m_ayTicks.empty(); }

	//读取该数据对应的合约代码
	inline const char*		code() const{ return m_strCode; }

	/*
	 *	获取指定位置的tick数据
	 *	
	 */
	inline WTSTickStruct*	at(uint32_t idx)
	{
		if (m_ayTicks.empty() || idx >= m_ayTicks.size())
			return NULL;

		return &m_ayTicks[idx];
	}

	inline std::vector<WTSTickStruct>& getDataRef() { return m_ayTicks; }

	inline bool isValidOnly() const{ return m_bValidOnly; }

	/*
	*	追加一条Tick
	*/
	inline void	appendTick(const WTSTickStruct& ts)
	{
		m_ayTicks.emplace_back(ts);
	}
};

//////////////////////////////////////////////////////////////////////////
/*
 *	@brief Tick数据切片,从连续的tick缓存中做的切片
 *	@details 切片并没有真实的复制内存,而只是取了开始和结尾的下标
 *	这样使用虽然更快,但是使用场景要非常小心,因为他依赖于基础数据对象
 */
class WTSTickSlice : public WTSObject
{
private:
	char			m_strCode[MAX_INSTRUMENT_LENGTH];
	WTSTickStruct*	m_ptrBegin;
	uint32_t		m_uCount;

protected:
	WTSTickSlice():m_ptrBegin(NULL),m_uCount(0){}
	inline int32_t		translateIdx(int32_t idx) const
	{
		if (idx < 0)
		{
			return max(0, (int32_t)m_uCount + idx);
		}

		return idx;
	}

public:
	static inline WTSTickSlice* create(const char* code, WTSTickStruct* firstTick, uint32_t count)
	{
		if (count == 0 || firstTick == NULL)
			return NULL;

		WTSTickSlice* slice = new WTSTickSlice();
		strcpy(slice->m_strCode, code);
		slice->m_ptrBegin = firstTick;
		slice->m_uCount = count;

		return slice;
	}

	inline uint32_t size() const{ return m_uCount; }

	inline bool empty() const{ return (m_uCount == 0) || (m_ptrBegin == NULL); }

	inline const WTSTickStruct* at(int32_t idx)
	{
		if (m_ptrBegin == NULL)
			return NULL;
		idx = translateIdx(idx);
		return m_ptrBegin + idx;
	}
};

//////////////////////////////////////////////////////////////////////////
/*
 *	@brief 逐笔委托数据切片,从连续的逐笔委托缓存中做的切片
 *	@details 切片并没有真实的复制内存,而只是取了开始和结尾的下标
 *	这样使用虽然更快,但是使用场景要非常小心,因为他依赖于基础数据对象
 */
class WTSOrdDtlSlice : public WTSObject
{
private:
	char				m_strCode[MAX_INSTRUMENT_LENGTH];
	WTSOrdDtlStruct*	m_ptrBegin;
	uint32_t			m_uCount;

protected:
	WTSOrdDtlSlice() :m_ptrBegin(NULL), m_uCount(0) {}
	inline int32_t		translateIdx(int32_t idx) const
	{
		if (idx < 0)
		{
			return max(0, (int32_t)m_uCount + idx);
		}

		return idx;
	}

public:
	static inline WTSOrdDtlSlice* create(const char* code, WTSOrdDtlStruct* firstItem, uint32_t count)
	{
		if (count == 0 || firstItem == NULL)
			return NULL;

		WTSOrdDtlSlice* slice = new WTSOrdDtlSlice();
		strcpy(slice->m_strCode, code);
		slice->m_ptrBegin = firstItem;
		slice->m_uCount = count;

		return slice;
	}

	inline uint32_t size() const { return m_uCount; }

	inline bool empty() const { return (m_uCount == 0) || (m_ptrBegin == NULL); }

	inline const WTSOrdDtlStruct* at(int32_t idx)
	{
		if (m_ptrBegin == NULL)
			return NULL;
		idx = translateIdx(idx);
		return m_ptrBegin + idx;
	}
};

//////////////////////////////////////////////////////////////////////////
/*
 *	@brief 委托队列数据切片,从连续的委托队列缓存中做的切片
 *	@details 切片并没有真实的复制内存,而只是取了开始和结尾的下标
 *	这样使用虽然更快,但是使用场景要非常小心,因为他依赖于基础数据对象
 */
class WTSOrdQueSlice : public WTSObject
{
private:
	char				m_strCode[MAX_INSTRUMENT_LENGTH];
	WTSOrdQueStruct*	m_ptrBegin;
	uint32_t			m_uCount;

protected:
	WTSOrdQueSlice() :m_ptrBegin(NULL), m_uCount(0) {}
	inline int32_t		translateIdx(int32_t idx) const
	{
		if (idx < 0)
		{
			return max(0, (int32_t)m_uCount + idx);
		}

		return idx;
	}

public:
	static inline WTSOrdQueSlice* create(const char* code, WTSOrdQueStruct* firstItem, uint32_t count)
	{
		if (count == 0 || firstItem == NULL)
			return NULL;

		WTSOrdQueSlice* slice = new WTSOrdQueSlice();
		strcpy(slice->m_strCode, code);
		slice->m_ptrBegin = firstItem;
		slice->m_uCount = count;

		return slice;
	}

	inline uint32_t size() const { return m_uCount; }

	inline bool empty() const { return (m_uCount == 0) || (m_ptrBegin == NULL); }

	inline const WTSOrdQueStruct* at(int32_t idx)
	{
		if (m_ptrBegin == NULL)
			return NULL;
		idx = translateIdx(idx);
		return m_ptrBegin + idx;
	}
};

//////////////////////////////////////////////////////////////////////////
/*
 *	@brief 逐笔成交数据切片,从连续的逐笔成交缓存中做的切片
 *	@details 切片并没有真实的复制内存,而只是取了开始和结尾的下标
 *	这样使用虽然更快,但是使用场景要非常小心,因为他依赖于基础数据对象
 */
class WTSTransSlice : public WTSObject
{
private:
	char			m_strCode[MAX_INSTRUMENT_LENGTH];
	WTSTransStruct*	m_ptrBegin;
	uint32_t		m_uCount;

protected:
	WTSTransSlice() :m_ptrBegin(NULL), m_uCount(0) {}
	inline int32_t		translateIdx(int32_t idx) const
	{
		if (idx < 0)
		{
			return max(0, (int32_t)m_uCount + idx);
		}

		return idx;
	}

public:
	static inline WTSTransSlice* create(const char* code, WTSTransStruct* firstItem, uint32_t count)
	{
		if (count == 0 || firstItem == NULL)
			return NULL;

		WTSTransSlice* slice = new WTSTransSlice();
		strcpy(slice->m_strCode, code);
		slice->m_ptrBegin = firstItem;
		slice->m_uCount = count;

		return slice;
	}

	inline uint32_t size() const { return m_uCount; }

	inline bool empty() const { return (m_uCount == 0) || (m_ptrBegin == NULL); }

	inline const WTSTransStruct* at(int32_t idx)
	{
		if (m_ptrBegin == NULL)
			return NULL;
		idx = translateIdx(idx);
		return m_ptrBegin + idx;
	}
};

NS_OTP_END