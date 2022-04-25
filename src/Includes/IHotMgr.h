/*!
 * \file IHotMgr.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief ������Լ�������ӿڶ���
 */
#pragma once
#include "WTSMarcos.h"
#include <vector>
#include <string>
#include <stdint.h>

typedef struct _HotSection
{
	std::string	_code;
	uint32_t	_s_date;
	uint32_t	_e_date;

	_HotSection(const char* code, uint32_t sdate, uint32_t edate)
		: _s_date(sdate), _e_date(edate), _code(code)
	{
	
	}

} HotSection;
typedef std::vector<HotSection>	HotSections;

NS_WTP_BEGIN

#define HOTS_MARKET		"HOTS_MARKET"
#define SECONDS_MARKET	"SECONDS_MARKET"

class IHotMgr
{
public:
	/*
	 *	��ȡ���´���
	 *	@pid	Ʒ�ִ���
	 *	@dt		����(������)
	 */
	virtual const char* getRawCode(const char* exchg, const char* pid, uint32_t dt)	= 0;

	/*
	 *	��ȡ������һ������һ������,����һ��������Լ�ķ��´���
	 *	@pid	Ʒ�ִ���
	 *	@dt		����(������)
	 */
	virtual const char* getPrevRawCode(const char* exchg, const char* pid, uint32_t dt) = 0;

	/*
	 *	��ȡ��������
	 *	@rawCode	���´���
	 *	@dt			����(������)
	 */
	virtual const char* getHotCode(const char* exchg, const char* rawCode, uint32_t dt) = 0;

	/*
	 *	�Ƿ�������Լ
	 *	@rawCode	���´���
	 *	@dt			����(������)
	 */
	virtual bool		isHot(const char* exchg, const char* rawCode, uint32_t dt) = 0;

	/*
	 *	�ָ�������,��������Լ��ĳ��ʱ�εķ��º�Լȫ�����ȡ��
	 */
	virtual bool		splitHotSecions(const char* exchg, const char* hotCode, uint32_t sDt, uint32_t eDt, HotSections& sections) = 0;

	/*
	 *	��ȡ���������´���
	 *	@pid	Ʒ�ִ���
	 *	@dt		����(������)
	 */
	virtual const char* getSecondRawCode(const char* exchg, const char* pid, uint32_t dt) = 0;

	/*
	 *	��ȡ��������һ������һ������,����һ����������Լ�ķ��´���
	 *	@pid	Ʒ�ִ���
	 *	@dt		����(������)
	 */
	virtual const char* getPrevSecondRawCode(const char* exchg, const char* pid, uint32_t dt) = 0;

	/*
	 *	��ȡ����������
	 *	@rawCode	���´���
	 *	@dt			����(������)
	 */
	virtual const char* getSecondCode(const char* exchg, const char* rawCode, uint32_t dt) = 0;

	/*
	 *	�Ƿ��������Լ
	 *	@rawCode	���´���
	 *	@dt			����(������)
	 */
	virtual bool		isSecond(const char* exchg, const char* rawCode, uint32_t dt) = 0;

	/*
	 *	�ָ��������,����������Լ��ĳ��ʱ�εķ��º�Լȫ�����ȡ��
	 */
	virtual bool		splitSecondSecions(const char* exchg, const char* hotCode, uint32_t sDt, uint32_t eDt, HotSections& sections) = 0;
};
NS_WTP_END
