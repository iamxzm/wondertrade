/*!
 * \file ILogHandler.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief ��־ת��ģ��ӿڶ���
 */
#pragma once
#include "WTSMarcos.h"
#include "WTSTypes.h"

NS_WTP_BEGIN
class ILogHandler
{
public:
	virtual void handleLogAppend(WTSLogLevel ll, const char* msg)	= 0;
};
NS_WTP_END