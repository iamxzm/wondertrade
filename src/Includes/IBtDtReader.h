/*!
 * \file IBtDtReader.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#pragma once
#include <string>

#include "../Includes/WTSTypes.h"

NS_WTP_BEGIN
class WTSVariant;

/*
 *	@brief ���ݶ�ȡģ��ص��ӿ�
 *	@details ��Ҫ�������ݶ�ȡģ�������ģ��ص�
 */
class IBtDtReaderSink
{
public:
	/*
	 *	@brief	������ݶ�ȡģ�����־
	 */
	virtual void		reader_log(WTSLogLevel ll, const char* message) = 0;
};

/*
 *	@brief	������ݶ�ȡ�ӿ�
 *
 *	�����ģ���ṩ��������(tick��K��)��ȡ�ӿ�
 */
class IBtDtReader
{
public:
	IBtDtReader() :_sink(NULL) {}
	virtual ~IBtDtReader(){}

public:
	virtual void init(WTSVariant* cfg, IBtDtReaderSink* sink) { _sink = sink; }

	virtual bool read_raw_bars(const char* exchg, const char* code, WTSKlinePeriod period, std::string& buffer) = 0;
	virtual bool read_raw_ticks(const char* exchg, const char* code, uint32_t uDate, std::string& buffer) = 0;

protected:
	IBtDtReaderSink*	_sink;
};

//�������ݴ洢����
typedef IBtDtReader* (*FuncCreateBtDtReader)();
//ɾ�����ݴ洢����
typedef void(*FuncDeleteBtDtReader)(IBtDtReader* store);

NS_WTP_END