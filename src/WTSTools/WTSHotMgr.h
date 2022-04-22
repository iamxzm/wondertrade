/*!
 * \file WTSHotMgr.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief ������Լ������ʵ��
 */
#pragma once
#include "../Includes/IHotMgr.h"
#include "../Includes/FasterDefs.h"
#include "../Includes/WTSCollection.hpp"
#include <string>

NS_WTP_BEGIN
	class IMarketMgr;
	class WTSHotItem;
NS_WTP_END

USING_NS_WTP;

//��������ӳ��
typedef WTSMap<uint32_t>		WTSDateHotMap;
//Ʒ������ӳ��
typedef WTSMap<ShortKey>		WTSProductHotMap;
//���г�����ӳ��
typedef WTSMap<ShortKey>		WTSExchgHotMap;

class WTSHotMgr : public IHotMgr
{
public:
	WTSHotMgr();
	~WTSHotMgr();

public:
	bool loadHots(const char* filename);
	bool loadSeconds(const char* filename);
	void release();

	void getHotCodes(const char* exchg, std::map<std::string, std::string> &mapHots);
	bool hasHotCodes();

	inline bool isInitialized() const {return m_bInitialized;}

public:
	virtual const char* getRawCode(const char* exchg, const char* pid, uint32_t dt = 0) override;

	virtual const char* getPrevRawCode(const char* exchg, const char* pid, uint32_t dt = 0) override;

	virtual const char* getHotCode(const char* exchg, const char* rawCode, uint32_t dt = 0) override;

	virtual bool	isHot(const char* exchg, const char* rawCode, uint32_t dt = 0) override;

	virtual bool	splitHotSecions(const char* exchg, const char* pid, uint32_t sDt, uint32_t eDt, HotSections& sections) override;

	//////////////////////////////////////////////////////////////////////////
	//�������ӿ�
	virtual const char* getSecondRawCode(const char* exchg, const char* pid, uint32_t dt = 0) override;

	virtual const char* getPrevSecondRawCode(const char* exchg, const char* pid, uint32_t dt = 0) override;

	virtual const char* getSecondCode(const char* exchg, const char* rawCode, uint32_t dt = 0) override;

	virtual bool		isSecond(const char* exchg, const char* rawCode, uint32_t d = 0) override;

	virtual bool		splitSecondSecions(const char* exchg, const char* hotCode, uint32_t sDt, uint32_t eDt, HotSections& sections) override;
private:
	WTSExchgHotMap*	m_pExchgHotMap;
	WTSExchgHotMap*	m_pExchgScndMap;
	faster_hashmap<ShortKey, std::string>	m_curHotCodes;
	faster_hashmap<ShortKey, std::string>	m_curSecCodes;
	bool			m_bInitialized;
};

