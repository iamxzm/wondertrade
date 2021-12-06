/*!
 * \file WTSTypes.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief WonderTrader�����������Ͷ����ļ�
 */
#pragma once
#include "WTSMarcos.h"

NS_OTP_BEGIN

/*
 *	��Լ����
 */
typedef enum tagContractCategory
{
	CC_Stock,			//��Ʊ
	CC_Future,			//�ڻ�
	CC_FutOption,		//�ڻ���Ȩ
	CC_Combination,		//���
	CC_Spot,			//����
	CC_EFP,				//��ת��
	CC_SpotOption,		//�ֻ���Ȩ
	CC_ETFOption		//������Ȩ
} ContractCategory;

/*
 *	��Ȩ����
 */
typedef enum tagOptionType
{
	OT_None = 0,
	OT_Call = '1',		//������Ȩ
	OT_Put	= '2'		//������Ȩ
} OptionType;

/*
 *	ƽ������
 */
typedef enum tagCoverMode
{
	CM_OpenCover,		//��ƽ
	CM_CoverToday,		//��ƽ��ƽ��
	CM_UNFINISHED,		//ƽδ�˽��
	CM_None			//�����ֿ�ƽ
} CoverMode;

/*
*	�۸�ģʽ
*/
typedef enum tagPriceMode
{
	PM_Both,		//�м��޼۶�֧��
	PM_Limit,		//ֻ֧���޼�
	PM_Market		//ֻ֧���м�
} PriceMode;

/*
 *	K����������
 *	�����ߡ��͡��ա����������
 */
typedef enum tagKlineFieldType
{
	KFT_OPEN,
	KFT_HIGH,
	KFT_LOW,
	KFT_CLOSE,
	KFT_DATE,
	KFT_VOLUME,
	KFT_SVOLUME
} WTSKlineFieldType;

/*
 *	K������
 */
typedef enum tagKlinePeriod
{
	KP_Tick,
	KP_Minute1,
	KP_Minute5,
	KP_DAY,
	KP_Week,
	KP_Month
} WTSKlinePeriod;

/*
 *	��־����
 */
typedef enum tagLogLevel
{
	LL_ALL	= 100,
	LL_DEBUG,
	LL_INFO,
	LL_WARN,
	LL_ERROR,
	LL_FATAL,
	LL_NONE
} WTSLogLevel;

/*
 *	�۸�����
 */
typedef enum tagPriceType
{
	WPT_ANYPRICE	= '1',	//�м۵�
	WPT_LIMITPRICE,			//�޼۵�
	WPT_BESTPRICE,			//���ż�
	WPT_LASTPRICE,			//���¼�
	//WTP_LASTPRICEPLUSONETICKS,		//���¼�+1ticks
	//WPT_LASTPRICEPLUSTWOTICKS,		//���¼�+2ticks
	//WPT_LASTPRICEPLUSTHREETICKS,	//���¼�+3ticks
	//WPT_ASKPRICE1,					//��һ��
	//WPT_ASKPRICE1PLUSONETICKS,		//��һ��+1ticks
	//WPT_ASKPRICE1PLUSTWOTICKS,		//��һ��+2ticks
	//WPT_ASKPRICE1PLUSTHREETICKS,	//��һ��+3ticks
	//WPT_BIDPRICE1,					//��һ��
	//WPT_BIDPRICE1PLUSONETICKS,		//��һ��+1ticks
	//WPT_BIDPRICE1PLUSTWOTICKS,		//��һ��+2ticks
	//WPT_BIDPRICE1PLUSTHREETICKS,	//��һ��+3ticks
} WTSPriceType;

/*
 *	ʱ������
 */
typedef enum tagTimeCondition
{
	WTC_IOC		= '1',	//�������,������
	WTC_GFS,			//������Ч
	WTC_GFD,			//������Ч
} WTSTimeCondition;

/*
 *	��ƽ����
 */
typedef enum tagOffsetType
{
	WOT_OPEN			= '0',	//����
	WOT_CLOSE,					//ƽ��,����Ϊƽ��
	WOT_FORCECLOSE,				//ǿƽ
	WOT_CLOSETODAY,				//ƽ��
	WOT_CLOSEYESTERDAY,			//ƽ��
} WTSOffsetType;

/*
 *	��շ���
 */
typedef enum tagDirectionType
{
	WDT_LONG			= '0',	//����
	WDT_SHORT,					//����
	WDT_NET						//��
} WTSDirectionType;

/*
 *	ҵ������
 */
typedef enum tagBusinessType
{
	BT_CASH		= '0',	//��ͨ����,
	BT_ETF		= '1',	//ETF����
	BT_EXECUTE	= '2',	//��Ȩ��Ȩ
	BT_QUOTE	= '3',	//��Ȩ����
	BT_FORQUOTE = '4',	//��Ȩѯ��
	BT_FREEZE	= '5',	//��Ȩ����
	BT_CREDIT	= '6',	//������ȯ
	BT_UNKNOWN			//δ֪ҵ������
} WTSBusinessType;

/*
 *	������������
 */
typedef enum tagActionFlag
{
	WAF_CANCEL			= '0',	//����
	WAF_MODIFY			= '3',	//�޸�
} WTSActionFlag;

/*
 *	����״̬
 */
typedef enum tagOrderState
{
	WOS_AllTraded				= '0',	//ȫ���ɽ�
	WOS_PartTraded_Queuing,				//���ֳɽ�,���ڶ�����
	WOS_PartTraded_NotQueuing,			//���ֳɽ�,δ�ڶ���
	WOS_NotTraded_Queuing,				//δ�ɽ�
	WOS_NotTraded_NotQueuing,			//δ�ɽ�,δ�ڶ���
	WOS_Canceled,						//�ѳ���
	WOS_Submitting				= 'a',	//�����ύ
	WOS_Nottouched,						//δ����
} WTSOrderState;

/*
 *	��������
 */
typedef enum tagOrderType
{
	WORT_Normal			= 0,		//��������
	WORT_Exception,					//�쳣����
	WORT_System,					//ϵͳ����
	WORT_Hedge						//�Գ嶩��
} WTSOrderType;

/*
 *	�ɽ�����
 */
typedef enum tagTrageType
{
	WTT_Common				= '0',	//��ͨ
	WTT_OptionExecution		= '1',	//��Ȩִ��
	WTT_OTC					= '2',	//OTC�ɽ�
	WTT_EFPDerived			= '3',	//��ת�������ɽ�
	WTT_CombinationDerived	= '4'	//��������ɽ�
} WTSTradeType;


/*
 *	�������
 */
typedef enum tagErrorCode
{
	WEC_NONE			=	0,		//û�д���
	WEC_ORDERINSERT,				//�µ�����
	WEC_ORDERCANCEL,				//��������
	WEC_EXECINSERT,					//��Ȩָ�����
	WEC_EXECCANCEL,					//��Ȩ��������
	WEC_UNKNOWN			=	9999	//δ֪����
} WTSErroCode;

/*
 *	�Ƚ��ֶ�
 */
typedef enum tagCompareField
{
	WCF_NEWPRICE			=	0,	//���¼�
	WCF_BIDPRICE,					//��һ��
	WCF_ASKPRICE,					//��һ��
	WCF_PRICEDIFF,					//�۲�,ֹӯֹ��ר��
	WCF_NONE				=	9	//���Ƚ�
} WTSCompareField;

/*
 *	�Ƚ�����
 */
typedef enum tagCompareType
{
	WCT_Equal			= 0,		//����
	WCT_Larger,						//����
	WCT_Smaller,					//С��
	WCT_LargerOrEqual,				//���ڵ���
	WCT_SmallerOrEqual				//С�ڵ���
}WTSCompareType;

/*
 *	����������¼�
 */
typedef enum tagParserEvent
{
	WPE_Connect			= 0,		//�����¼�
	WPE_Close,						//�ر��¼�
	WPE_Login,						//��¼
	WPE_Logout						//ע��
}WTSParserEvent;

/*
 *	����ģ���¼�
 */
typedef enum tagTraderEvent
{
	WTE_Connect			= 0,		//�����¼�
	WTE_Close,						//�ر��¼�
	WTE_Login,						//��¼
	WTE_Logout						//ע��
}WTSTraderEvent;

/*
 *	ָ������
 */
typedef enum tagExpressType
{
	WET_Unique,
	WET_SubExp
} WTSExpressType;

/*
 *	ָ��������
 */
typedef enum tagExpressLineType
{
	WELT_Polyline,	//����	
	WELT_VolStick,	//����
	WELT_StickLine,	//��״��
	WELT_AStickLine,	//��״�߾���ֵ
} WTSExpressLineType;

//ָ���߷��
typedef enum tagExpLineStyle
{
	ELS_LINE_VISIBLE = 0x00000001,	//�����ɼ�
	ELS_TITLE_VISIBLE = 0x00000002	//����ɼ�
} ExpLineStyle;

/*
 *	������������
 */
typedef enum tagBSDirectType
{
	BDT_Buy		= 'B',	//����	
	BDT_Sell	= 'S',	//����
	BDT_Unknown = ' ',	//δ֪
	BDT_Borrow	= 'G',	//����
	BDT_Lend	= 'F'	//���
} WTSBSDirectType;

/*
 *	�ɽ�����
 */
typedef enum tagTransType
{
	TT_Unknown	= 'U',	//δ֪����
	TT_Match	= 'M',	//��ϳɽ�
	TT_Cancel	= 'C'	//����
} WTSTransType;

/*
 *	
 */
typedef enum tagOrdDetailType
{
	ODT_Unknown		= 0,	//δ֪����
	ODT_BestPrice	= 'U',	//��������
	ODT_AnyPrice	= '1',	//�м�
	ODT_LimitPrice	= '2'	//�޼�
} WTSOrdDetailType;

NS_OTP_END