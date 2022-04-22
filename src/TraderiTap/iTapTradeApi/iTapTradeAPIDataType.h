//���ļ�������TapTradeAPI ʹ�õ��������ͺ����ݽṹ
#ifndef ITAP_TRADE_API_DATA_TYPE_H
#define ITAP_TRADE_API_DATA_TYPE_H
#include "iTapAPICommDef.h"


namespace ITapTrade
{
#pragma pack(push, 1)


    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIACCOUNTTYPE	�˺�����
     *	@{
     */
    //=============================================================================
    //! �˺�����
    typedef TAPICHAR			TAPIAccountType;
    //! ���˿ͻ�
    const TAPIAccountType		TAPI_ACCOUNT_TYPE_PERSON				= 'P';
    //! �����ͻ�
    const TAPIAccountType		TAPI_ACCOUNT_TYPE_ORGANIZATION		    = 'O';
    //! ������
    const TAPIAccountType		TAPI_ACCOUNT_TYPE_AGENT					= 'A';
    //!Margin
    const TAPIAccountType		TAPI_ACCOUNT_TYPE_MARGIN				= 'M';
    //! Internal
    const TAPIAccountType		TAPI_ACCOUNT_TYPE_HOUSE					= 'H';
    //! ��Ʊ�˻�
    const TAPIAccountType		TAPI_ACCOUNT_TYPE_STOCK					= 'S';
    /** @}*/

    //=============================================================================
    /**
    *	\addtogroup G_DATATYPE_T_TAPIRIGHTIDTYPE	Ȩ�ޱ�������
    *	@{
    */
    //=============================================================================
    //! Ȩ�ޱ�������
    typedef TAPIINT32			TAPIRightIDType;
    //! ϵͳɾ��
    const TAPIRightIDType		TAPI_RIGHT_ORDER_DEL	= 30001;
    //! �������
    const TAPIRightIDType		TAPI_RIGHT_ORDER_CHECK	= 30002;
    //! ֻ�ɲ�ѯ
    const TAPIRightIDType		TAPI_RIGHT_ONLY_QRY		= 31000;
    //! ֻ�ɿ���
    const TAPIRightIDType		TAPI_RIGHT_ONLY_OPEN	= 31001;
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIUSERTYPETYPE	��¼�û��������
     *	@{
     */
    //=============================================================================
    //! ��¼�û��������
    typedef TAPIINT32			TAPIUserTypeType;
    //! Ͷ�����û�
    const TAPIUserTypeType		TAPI_USERTYPE_CLIENT					=10000;
    //! ������
    const TAPIUserTypeType		TAPI_USERTYPE_BROKER					=20000;
    //! ����Ա
    const TAPIUserTypeType		TAPI_USERTYPE_TRADER					=30000;
    //! ���
    const TAPIUserTypeType		TAPI_USERTYPE_RISK						=40000;
    //! ����Ա
    const TAPIUserTypeType		TAPI_USERTYPE_MANAGER					=50000;
    //! ����
    const TAPIUserTypeType		TAPI_USERTYPE_QUOTE						=60000;
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIACCOUNTSTATE	�˺�״̬
     *	@{
     */
    //=============================================================================
    //! �˺�״̬
    typedef TAPICHAR			TAPIAccountState;
    //! ����
    const TAPIAccountState		TAPI_ACCOUNT_STATE_NORMAL				= 'N';
    //! ����
    const TAPIAccountState		TAPI_ACCOUNT_STATE_CANCEL				= 'C';
    //! ����
    const TAPIAccountState		TAPI_ACCOUNT_STATE_SLEEP				= 'S';
	//������
	const TAPIAccountState		TAPI_ACCOUNT_STATE_FROZEN				= 'F';
    /** @}*/



    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIACCOUNTRIGHTTYPE	�˻�����״̬
     *	@{
     */
    //=============================================================================
    //! �ͻ�����״̬����
    typedef TAPICHAR			TAPIAccountRightType;
    //! ��������
    const	TAPIAccountRightType	TAPI_ACCOUNT_TRADING_RIGHT_NORMAL	= '0';
    //! ��ֹ����	
    const	TAPIAccountRightType	TAPI_ACCOUNT_TRADING_RIGHT_NOTRADE	= '1';
    //! ֻ��ƽ��
    const	TAPIAccountRightType	TAPI_ACCOUNT_TRADING_RIGHT_CLOSE	= '2';
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIORDERTYPETYPE	ί������
     *	@{
     */
    //=============================================================================
    //! ί������
    typedef TAPICHAR				TAPIOrderTypeType;
    //! �м�
    const TAPIOrderTypeType			TAPI_ORDER_TYPE_MARKET				= '1';
    //! �޼�
    const TAPIOrderTypeType			TAPI_ORDER_TYPE_LIMIT				= '2';
    //! �м�ֹ��
    const TAPIOrderTypeType			TAPI_ORDER_TYPE_STOP_MARKET			= '3';
    //! �޼�ֹ��
    const TAPIOrderTypeType			TAPI_ORDER_TYPE_STOP_LIMIT			= '4';
    //! ��Ȩ��Ȩ
    const TAPIOrderTypeType			TAPI_ORDER_TYPE_OPT_EXEC			= '5';
    //! ��Ȩ��Ȩ
    const TAPIOrderTypeType			TAPI_ORDER_TYPE_OPT_ABANDON			= '6';
    //! ѯ��
    const TAPIOrderTypeType			TAPI_ORDER_TYPE_REQQUOT				= '7';
    //! Ӧ��
    const TAPIOrderTypeType			TAPI_ORDER_TYPE_RSPQUOT				= '8';
    //! ��ɽ��
	const TAPIOrderTypeType			TAPI_ORDER_TYPE_ICEBERG				= '9';
	//! Ӱ�ӵ�
	const TAPIOrderTypeType			TAPI_ORDER_TYPE_GHOST				= 'A';
	//�۽������۵�
	const TAPIOrderTypeType			TAPI_ORDER_TYPE_HKEX_AUCTION		= 'B';
	//����
	const TAPIOrderTypeType			TAPI_ORDER_TYPE_SWAP				= 'C';
	//֤ȯ����
	const TAPIOrderTypeType			TAPI_ORDER_TYPE_LOCK				= 'D';
	//֤ȯ����
	const TAPIOrderTypeType			TAPI_ORDER_TYPE_UNLOCK				= 'E';
	//��ǿ�޼۵�
	const TAPIOrderTypeType			TAPI_ORDER_TYPE_ENHANCE				= 'F';
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIORDERSOURCETYPE	ί����Դ
     *	@{
     */
    //=============================================================================
    //! ί����Դ
    typedef TAPICHAR				TAPIOrderSourceType;
    //! �������ӵ�
    const TAPIOrderSourceType		TAPI_ORDER_SOURCE_SELF_ETRADER			= '1';
    //! ������ӵ�
    const TAPIOrderSourceType		TAPI_ORDER_SOURCE_PROXY_ETRADER			= '2';
    //! �ⲿ���ӵ�(�ⲿ����ϵͳ�µ�����ϵͳ¼��)
    const TAPIOrderSourceType		TAPI_ORDER_SOURCE_JTRADER				= '3';
    //! �˹�¼�뵥(�ⲿ������ʽ�µ�����ϵͳ¼��)
    const TAPIOrderSourceType		TAPI_ORDER_SOURCE_MANUAL				= '4';
    //! carry��
    const TAPIOrderSourceType		TAPI_ORDER_SOURCE_CARRY					= '5';
    //! ��ʽ������
    const TAPIOrderSourceType		TAPI_ORDER_SOURCE_PROGRAM				= '6';
    //! ������Ȩ
    const TAPIOrderSourceType		TAPI_ORDER_SOURCE_DELIVERY				= '7';
    //! ��Ȩ����
    const TAPIOrderSourceType		TAPI_ORDER_SOURCE_ABANDON				= '8';
    //! ͨ����
    const TAPIOrderSourceType		TAPI_ORDER_SOURCE_CHANNEL				= '9';
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPITIMEINFORCETYPE	ί����Ч����
     *	@{
     */
    //=============================================================================
    //! ί����Ч����
    typedef TAPICHAR				TAPITimeInForceType;
    //! ������Ч
    const TAPITimeInForceType		TAPI_ORDER_TIMEINFORCE_GFD					= '0';
    //! ȡ��ǰ��Ч
    const TAPITimeInForceType		TAPI_ORDER_TIMEINFORCE_GTC					= '1';
    //! ָ������ǰ��Ч
    const TAPITimeInForceType		TAPI_ORDER_TIMEINFORCE_GTD					= '2';
    //! FAK��IOC
    const TAPITimeInForceType		TAPI_ORDER_TIMEINFORCE_FAK					= '3';
    //! FOK
    const TAPITimeInForceType		TAPI_ORDER_TIMEINFORCE_FOK					= '4';
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPISIDETYPE	��������
     *	@{
     */
    //=============================================================================
    //! ��������
    typedef TAPICHAR				TAPISideType;
    //! ��
    const TAPISideType				TAPI_SIDE_NONE							= 'N';
    //! ����
    const TAPISideType				TAPI_SIDE_BUY							= 'B';
    //! ����
    const TAPISideType				TAPI_SIDE_SELL							= 'S';
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIPOSITIONEFFECTTYPE	��ƽ����
     *	@{
     */
    //=============================================================================
    //! ��ƽ����
    typedef TAPICHAR				TAPIPositionEffectType;
    //! ���ֿ�ƽ
    const TAPIPositionEffectType	TAPI_PositionEffect_NONE				= 'N';
    //! ����
    const TAPIPositionEffectType	TAPI_PositionEffect_OPEN				= 'O';
    //! ƽ��
    const TAPIPositionEffectType	TAPI_PositionEffect_COVER			= 'C';
    //! ƽ����
    const TAPIPositionEffectType	TAPI_PositionEffect_COVER_TODAY		= 'T';
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIHEDGEFLAGTYPE	Ͷ����ֵ����
     *	@{
     */
    //=============================================================================
    //! Ͷ����ֵ����
    typedef TAPICHAR				TAPIHedgeFlagType;
    //! ��
    const TAPIHedgeFlagType			TAPI_HEDGEFLAG_NONE					= 'N';
    //! Ͷ��
    const TAPIHedgeFlagType			TAPI_HEDGEFLAG_T					= 'T';
    //! ��ֵ
    const TAPIHedgeFlagType			TAPI_HEDGEFLAG_B					= 'B';
	//! ����
	const TAPIHedgeFlagType			TAPI_HEDGEFLAG_R					= 'R';


    /** @}*/
    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIORDERSTATETYPE	ί��״̬����
     *	@{
     */
    //=============================================================================
    //! ί��״̬����
    typedef TAPICHAR				TAPIOrderStateType;
    //! �ն��ύ
    const TAPIOrderStateType		TAPI_ORDER_STATE_SUBMIT				= '0';
    //! ������
    const TAPIOrderStateType		TAPI_ORDER_STATE_ACCEPT				= '1';
    //! ���Դ�����
    const TAPIOrderStateType		TAPI_ORDER_STATE_TRIGGERING			= '2';
    //! ������������
    const TAPIOrderStateType		TAPI_ORDER_STATE_EXCTRIGGERING		= '3';
    //! ���Ŷ�
    const TAPIOrderStateType		TAPI_ORDER_STATE_QUEUED				= '4';
    //! ���ֳɽ�
    const TAPIOrderStateType		TAPI_ORDER_STATE_PARTFINISHED		= '5';
    //! ��ȫ�ɽ�
    const TAPIOrderStateType		TAPI_ORDER_STATE_FINISHED			= '6';
    //! ������(�Ŷ���ʱ״̬)
    const TAPIOrderStateType		TAPI_ORDER_STATE_CANCELING			= '7';
    //! ���޸�(�Ŷ���ʱ״̬)
    const TAPIOrderStateType		TAPI_ORDER_STATE_MODIFYING			= '8';
    //! ��ȫ����
    const TAPIOrderStateType		TAPI_ORDER_STATE_CANCELED			= '9';
    //! �ѳ��൥
    const TAPIOrderStateType		TAPI_ORDER_STATE_LEFTDELETED		= 'A';
    //! ָ��ʧ��
    const TAPIOrderStateType		TAPI_ORDER_STATE_FAIL				= 'B';
    //! ����ɾ��
    const TAPIOrderStateType		TAPI_ORDER_STATE_DELETED			= 'C';
    //! �ѹ���
    const TAPIOrderStateType		TAPI_ORDER_STATE_SUPPENDED			= 'D';
    //! ����ɾ��
    const TAPIOrderStateType		TAPI_ORDER_STATE_DELETEDFOREXPIRE	= 'E';
    //! ����Ч����ѯ�۳ɹ�
    const TAPIOrderStateType		TAPI_ORDER_STATE_EFFECT				= 'F';
    //! �����롪����Ȩ����Ȩ������������ɹ�
    const TAPIOrderStateType		TAPI_ORDER_STATE_APPLY				= 'G';
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPICALCULATEMODETYPE	���㷽ʽ
     *	@{
     */
    //=============================================================================
    //! ���㷽ʽ
    typedef TAPICHAR               TAPICalculateModeType;
    
	//!����+������������ѣ�����0.01����Ϊ���С��0.01����Ϊ�������磺0.001Ϊ������ȡ1%��
	const TAPICalculateModeType		TAPI_CALULATE_MODE_COMBINE				= '0';
	//! ����
    const TAPICalculateModeType		TAPI_CALCULATE_MODE_PERCENTAGE          = '1';
    //! ����
    const TAPICalculateModeType		TAPI_CALCULATE_MODE_QUOTA               = '2';
    //! ��ֵ����	
    const TAPICalculateModeType		TAPI_CALCULATE_MODE_CHAPERCENTAGE		= '3';
    //! ��ֵ����
    const TAPICalculateModeType		TAPI_CALCULATE_MODE_CHAQUOTA			= '4';
    //! �ۿ�
    const TAPICalculateModeType		TAPI_CALCULATE_MODE_DISCOUNT			= '5';
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIMATCHSOURCETYPE	�ɽ���Դ
     *	@{
     */
    //=============================================================================
    //! �ɽ���Դ
    typedef TAPICHAR				TAPIMatchSourceType;
    //! ȫ��
    const TAPIMatchSourceType		TAPI_MATCH_SOURCE_ALL                   = '0';
    //! �������ӵ�
    const TAPIMatchSourceType		TAPI_MATCH_SOURCE_SELF_ETRADER          = '1';
    //! ������ӵ�
    const TAPIMatchSourceType		TAPI_MATCH_SOURCE_PROXY_ETRADER         = '2';
    //! �ⲿ���ӵ�
    const TAPIMatchSourceType		TAPI_MATCH_SOURCE_JTRADER				= '3';
    //! �˹�¼�뵥
    const TAPIMatchSourceType		TAPI_MATCH_SOURCE_MANUAL				= '4';
    //! carry��
    const TAPIMatchSourceType		TAPI_MATCH_SOURCE_CARRY					= '5';
    //! ��ʽ����
    const TAPIMatchSourceType		TAPI_MATCH_SOURCE_PROGRAM				= '6';
    //! ������Ȩ
    const TAPIMatchSourceType		TAPI_MATCH_SOURCE_DELIVERY				= '7';
    //! ��Ȩ����
    const TAPIMatchSourceType		TAPI_MATCH_SOURCE_ABANDON				= '8';
    //! ͨ����
    const TAPIMatchSourceType		TAPI_MATCH_SOURCE_CHANNEL				= '9';
	//!Bloomberg�µ�
	const TAPIMatchSourceType		TAPI_MATCH_SOURCE_BLOOMBERG				= 'B';
	//!�����ֻ���
	const TAPIMatchSourceType		TAPI_MATCH_SOURCE_AUTOPHONE				= 'A';
	//!GiveUp�ɽ�
	const TAPIMatchSourceType		TAPI_MATCH_SOURCE_GIVEUP				= 'C';
	//!��Ȩ�ɽ�
	const TAPIMatchSourceType		TAPI_MATCH_SOURCE_EXERCISE				= 'E';

    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIOPENCLOSEMODETYPE	��ƽ��ʽ
     *	@{
     */
    //=============================================================================
    //! ��ƽ��ʽ
    typedef TAPICHAR				TAPIOpenCloseModeType;
    //! �����ֿ�ƽ
    const TAPIOpenCloseModeType		TAPI_CLOSE_MODE_NONE				= 'N';
    //! ƽ��δ�˽�
    const TAPIOpenCloseModeType		TAPI_CLOSE_MODE_UNFINISHED			= 'U';
    //! ���ֿ��ֺ�ƽ��
    const TAPIOpenCloseModeType		TAPI_CLOSE_MODE_OPENCOVER			= 'C';
    //! ���ֿ��֡�ƽ�ֺ�ƽ��
    const TAPIOpenCloseModeType		TAPI_CLOSE_MODE_CLOSETODAY			= 'T';
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIFUTUREALGTYPE	�ڻ��㷨
     *	@{
     */
    //=============================================================================
    //! �ڻ��㷨
    typedef TAPICHAR				TAPIFutureAlgType;
    //! ���
    const TAPIFutureAlgType			TAPI_FUTURES_ALG_ZHUBI                  = '1';
    //! ����
    const TAPIFutureAlgType			TAPI_FUTURES_ALG_DINGSHI                = '2';
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIOPTIONALGTYPE	��Ȩ�㷨
     *	@{
     */
    //=============================================================================
    //! ��Ȩ�㷨
    typedef TAPICHAR				TAPIOptionAlgType;
    //! �ڻ���ʽ
    const TAPIOptionAlgType         TAPI_OPTION_ALG_FUTURES                 = '1';
    //! ��Ȩ��ʽ
    const TAPIOptionAlgType         TAPI_OPTION_ALG_OPTION                  = '2';
    /** @}*/



    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIBANKACCOUNTLWFLAGTYPE	����ұ�ʶ
     *	@{
     */
    //=============================================================================
    //! ����ұ�ʶ
    typedef TAPICHAR				TAPIBankAccountLWFlagType;
    //! ����������˻�
    const TAPIBankAccountLWFlagType	TAPI_LWFlag_L					= 'L';
    //! �ͻ���������˻�
    const TAPIBankAccountLWFlagType	TAPI_LWFlag_W					= 'W';
    /** @}*/


    //=============================================================================
    /**
    *	\addtogroup G_DATATYPE_T_TAPICASHADJUSTTYPETYPE	�ʽ��������
    *	@{
    */
    //=============================================================================
    //! �ʽ��������
    typedef TAPICHAR						TAPICashAdjustTypeType;
    //! �����ѵ���
    const TAPICashAdjustTypeType			TAPI_CASHINOUT_MODE_FEEADJUST = '0';
    //! ӯ������
    const TAPICashAdjustTypeType			TAPI_CASHINOUT_MODE_YKADJUST = '1';
    //! ��Ѻ�ʽ�
    const TAPICashAdjustTypeType			TAPI_CASHINOUT_MODE_PLEDGE = '2';
    //! ��Ϣ����
    const TAPICashAdjustTypeType			TAPI_CASHINOUT_MODE_INTERESTREVENUE = '3';
    //! ���۷���
    const TAPICashAdjustTypeType			TAPI_CASHINOUT_MODE_COLLECTIONCOST = '4';
    //! ����
    const TAPICashAdjustTypeType			TAPI_CASHINOUT_MODE_OTHER = '5';
    //! ��˾�䲦��
    const TAPICashAdjustTypeType			TAPI_CASHINOUT_MODE_COMPANY = '6';
    /** @}*/



    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIMARGINCALCULATEMODETYPE	�ڻ���֤��ʽ
     *	@{
     */
    //=============================================================================
    //! �ڻ���֤��ʽ
    typedef TAPICHAR				TAPIMarginCalculateModeType;
    //! �ֱ�
    const TAPIMarginCalculateModeType TAPI_DEPOSITCALCULATE_MODE_FEN     = '1';
    //! ����
    const TAPIMarginCalculateModeType TAPI_DEPOSITCALCULATE_MODE_SUO     = '2';
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIOPTIONMARGINCALCULATEMODETYPE	��Ȩ��֤��ʽ
     *	@{
     */
    //=============================================================================
    //! ��Ȩ��֤��ʽ,�ݴ��жϸ�Ʒ����Ȩ���ú������ü��㹫ʽ���㱣֤��
    typedef TAPICHAR				TAPIOptionMarginCalculateModeType;
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPICMBDIRECTTYPE	��Ϸ���
     *	@{
     */
    //=============================================================================
    //! ��Ϸ���,Ʒ��������Ϻ�Լ����������͵ڼ�����ͬ
    typedef TAPICHAR				TAPICmbDirectType;
    //! �͵�һ��һ��
    const TAPICmbDirectType         TAPI_CMB_DIRECT_FIRST                    = '1';
    //! �͵ڶ���һ��
    const TAPICmbDirectType         TAPI_CMB_DIRECT_SECOND                   = '2';
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIDELIVERYMODETYPE	������Ȩ��ʽ
     *	@{
     */
    //=============================================================================
    //! ������Ȩ��ʽ,�ڻ�����Ȩ�˽�ķ�ʽ
    typedef TAPICHAR				TAPIDeliveryModeType;
    //! ʵ�ｻ��
    const TAPIDeliveryModeType		TAPI_DELIVERY_MODE_GOODS			= 'G';
    //! �ֽ𽻸�
    const TAPIDeliveryModeType		TAPI_DELIVERY_MODE_CASH				= 'C';
    //! ��Ȩ��Ȩ
    const TAPIDeliveryModeType		TAPI_DELIVERY_MODE_EXECUTE			= 'E';
    //! ��Ȩ����
    const TAPIDeliveryModeType		TAPI_DELIVERY_MODE_ABANDON			= 'A';
    //! �۽�����Ȩ
    const TAPIDeliveryModeType		TAPI_DELIVERY_MODE_HKF				= 'H';
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPICONTRACTTYPETYPE	��Լ����
     *	@{
     */
    //=============================================================================
    //! ��Լ����
    typedef TAPICHAR				TAPIContractTypeType;
    //! ���������Լ
    const TAPIContractTypeType		TAPI_CONTRACT_TYPE_TRADEQUOTE		='1';
    //! �����Լ
    const TAPIContractTypeType		TAPI_CONTRACT_TYPE_QUOTE			='2';
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPITACTICSTYPETYPE	���Ե�����
     *	@{
     */
    //=============================================================================
    //! ���Ե�����
    typedef TAPICHAR					TAPITacticsTypeType;
    //! ��
    const TAPITacticsTypeType			TAPI_TACTICS_TYPE_NONE				= 'N';
    //! Ԥ����(��)
    const TAPITacticsTypeType			TAPI_TACTICS_TYPE_READY				= 'M';
    //! �Զ���
    const TAPITacticsTypeType			TAPI_TACTICS_TYPE_ATUO				= 'A';
    //! ������
    const TAPITacticsTypeType			TAPI_TACTICS_TYPE_CONDITION			= 'C';
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIORDERACT	������������
     *	@{
     */
    //=============================================================================
    //! ������������
    typedef TAPICHAR		TAPIORDERACT;
    //! ����
    const TAPIORDERACT APIORDER_INSERT			= '1';
    //! �ĵ�
    const TAPIORDERACT APIORDER_MODIFY			= '2';
    //! ����
    const TAPIORDERACT APIORDER_DELETE			= '3';
    //! ����
    const TAPIORDERACT APIORDER_SUSPEND			= '4';
    //! ����
    const TAPIORDERACT APIORDER_ACTIVATE		= '5';
    //! ɾ��
    const TAPIORDERACT APIORDER_SYSTEM_DELETE	= '6';
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPITRIGGERCONDITIONTYPE	������������
     *	@{
     */
    //=============================================================================
    //! ������������
    typedef TAPICHAR				TAPITriggerConditionType;
    //! ��
    const TAPITriggerConditionType	TAPI_TRIGGER_CONDITION_NONE			= 'N';
    //! ���ڵ���
    const TAPITriggerConditionType	TAPI_TRIGGER_CONDITION_GREAT		= 'G';
    //! С�ڵ���
    const TAPITriggerConditionType	TAPI_TRIGGER_CONDITION_LITTLE		= 'L';
    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPITRIGGERPRICETYPETYPE	�����۸�����
     *	@{
     */
    //=============================================================================
    //! �����۸�����
    typedef TAPICHAR				TAPITriggerPriceTypeType;
    //! ��
    const TAPITriggerPriceTypeType	TAPI_TRIGGER_PRICE_NONE				= 'N';
    //! ���
    const TAPITriggerPriceTypeType	TAPI_TRIGGER_PRICE_BUY				= 'B';
    //! ����
    const TAPITriggerPriceTypeType	TAPI_TRIGGER_PRICE_SELL				= 'S';
    //! ���¼�
    const TAPITriggerPriceTypeType	TAPI_TRIGGER_PRICE_LAST				= 'L';
    /** @}*/


    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPITRADINGSTATETYPE	����״̬
     *	@{
     */
    //=============================================================================
    //! ����״̬
    typedef TAPICHAR               TAPITradingStateType;
    //! ���Ͼ���
    const TAPITradingStateType      TAPI_TRADE_STATE_BID                = '1';
    //! ���Ͼ��۴��
    const TAPITradingStateType      TAPI_TRADE_STATE_MATCH              = '2';
    //! ��������
    const TAPITradingStateType      TAPI_TRADE_STATE_CONTINUOUS         = '3';
    //! ������ͣ
    const TAPITradingStateType      TAPI_TRADE_STATE_PAUSED             = '4';
    //! ����
    const TAPITradingStateType      TAPI_TRADE_STATE_CLOSE              = '5';
    //! ���д���ʱ��
    const TAPITradingStateType      TAPI_TRADE_STATE_DEALLAST           = '6';
    //! ����δ��
    const TAPITradingStateType		TAPI_TRADE_STATE_GWDISCONNECT		= '0';
    //! δ֪״̬
    const TAPITradingStateType		TAPI_TRADE_STATE_UNKNOWN			= 'N';
    //! ����ʼ��
    const TAPITradingStateType		TAPI_TRADE_STATE_INITIALIZE			= 'I';
    //! ׼������
    const TAPITradingStateType		TAPI_TRADE_STATE_READY				= 'R';
    /** @}*/



    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPINOTICEIGNOREFLAGTYPE	���Ժ�̨����֪ͨ���
     *	@{
     */
    //=============================================================================
    //! ���Ժ�̨����֪ͨ���
    typedef TAPIUINT32              TAPINoticeIgnoreFlagType;
    //! ����������Ϣ
    const TAPINoticeIgnoreFlagType TAPI_NOTICE_IGNORE_NONE				= 0x00000000;
    //! ������������
    const TAPINoticeIgnoreFlagType TAPI_NOTICE_IGNORE_ALL				= 0xFFFFFFFF;
    //! �����ʽ�����:OnRtnFund
    const TAPINoticeIgnoreFlagType TAPI_NOTICE_IGNORE_FUND				= 0x00000001;
    //! ����ί������:OnRtnOrder
    const TAPINoticeIgnoreFlagType TAPI_NOTICE_IGNORE_ORDER				= 0x00000002;
    //! ���Գɽ�����:OnRtnFill
    const TAPINoticeIgnoreFlagType TAPI_NOTICE_IGNORE_FILL				= 0x00000004;
    //! ���Գֲ�����:OnRtnPosition
    const TAPINoticeIgnoreFlagType TAPI_NOTICE_IGNORE_POSITION			= 0x00000008;
    //! ����ƽ������:OnRtnClose
    const TAPINoticeIgnoreFlagType TAPI_NOTICE_IGNORE_CLOSE				= 0x00000010;
    //! ���Գֲ�ӯ������:OnRtnPositionProfit
    const TAPINoticeIgnoreFlagType TAPI_NOTICE_IGNORE_POSITIONPROFIT	= 0x00000020;
    /** @}*/


    //=============================================================================
    /**
     *	\addtogroup G_DATATYPE_T_TAPIORDERQRYTYPETYPE	ί�в�ѯ����
     *	@{
     */
    //=============================================================================
    //! ί�в�ѯ����
    typedef TAPICHAR              TAPIOrderQryTypeType;
    //! ��������ί��
    const TAPIOrderQryTypeType TAPI_ORDER_QRY_TYPE_ALL				= 'A';
    //! ֻ����δ������ί��
    const TAPIOrderQryTypeType TAPI_ORDER_QRY_TYPE_UNENDED			= 'U';
    /** @}*/

	//=============================================================================
	/**
	 *	\addtogroup G_DATATYPE_T_CONTACTINFO	������֤��¼����
	 *	@{
	 */
	 //=============================================================================
	//! ������֤��¼����
	typedef TAPICHAR					TAPILoginTypeType;
	//! ������¼�����θ��豸��
	const TAPILoginTypeType				TAPI_LOGINTYPE_NORMAL		= 'N';
	//! ��ʱ��¼
	const TAPILoginTypeType				TAPI_LOGINTYPE_TEMPORARY	= 'T';
	/** @}*/
    //! ------------------------------------------------------------------------------------------

    //! ��¼��֤��Ϣ
    struct TapAPITradeLoginAuth
    {
            TAPISTR_20					UserNo;					///< �û���
            TAPIYNFLAG					ISModifyPassword;		///< �Ƿ��޸�����	
            TAPISTR_20					Password;				///< ����
            TAPISTR_20					NewPassword;			///< ������
    };

    //! ��¼������Ϣ
    struct TapAPITradeLoginRspInfo
    {
            TAPISTR_20					UserNo;							///< �û����
            TAPIUserTypeType			UserType;						///< �û�����
            TAPISTR_20					UserName;						///< �û���
            TAPISTR_50					ReservedInfo;					///< �������ĺͺ�̨�汾��
            TAPISTR_40					LastLoginIP;					///< �ϴε�¼IP
            TAPIUINT32					LastLoginProt;					///< �ϴε�¼�˿�
            TAPIDATETIME				LastLoginTime;					///< �ϴε�¼ʱ��
            TAPIDATETIME				LastLogoutTime;					///< �ϴ��˳�ʱ��
            TAPIDATE					TradeDate;						///< ��ǰ��������
            TAPIDATETIME				LastSettleTime;					///< �ϴν���ʱ��
            TAPIDATETIME				StartTime;						///< ϵͳ����ʱ��
            TAPIDATETIME				NextSecondDate;					///< �´ζ�����֤����
    };
	//!���������֤��Ȩ��Ӧ��
	struct  TapAPIRequestVertificateCodeRsp
	{
		TAPISecondSerialIDType SecondSerialID;							///< ������֤��Ȩ�����
		TAPIINT32 Effective;											///< ������֤��Ȩ����Ч�ڣ��֣���
	};

	//!��֤������֤������ṹ
	struct TapAPISecondCertificationReq
	{
		TAPISTR_10							VertificateCode;		///<������֤��
		TAPILoginTypeType					LoginType;				///<������֤��¼����
	};

    //! �˺������Ϣ��ѯ����
    struct TapAPIAccQryReq
    {
    };

    //! �ʽ��˺���Ϣ
    struct TapAPIAccountInfo
    {
            TAPISTR_20              AccountNo;                              ///< �ʽ��˺�
            TAPIAccountType			AccountType;                            ///< �˺�����
            TAPIAccountState		AccountState;                           ///< �˺�״̬
            TAPIAccountRightType		AccountTradeRight;					///<����״̬
            TAPISTR_10				CommodityGroupNo;						///<�ɽ���Ʒ����.
            TAPISTR_20				AccountShortName;                       ///< �˺ż��
            TAPISTR_20				AccountEnShortName;						///<�˺�Ӣ�ļ��
    };

    //! �ͻ��µ�����ṹ
    struct TapAPINewOrder
    {
            TAPISTR_20					AccountNo;						///< �ͻ��ʽ��ʺţ�����

            TAPISTR_10					ExchangeNo;						///< ��������ţ�����
            TAPICommodityType			CommodityType;					///< Ʒ�����ͣ�����
            TAPISTR_10					CommodityNo;					///< Ʒ�ֱ������ͣ�����
            TAPISTR_10					ContractNo;						///< ��Լ1������
            TAPISTR_10					StrikePrice;					///< ִ�м۸�1����Ȩ��д
            TAPICallOrPutFlagType		CallOrPutFlag;					///< ���ſ���1 Ĭ��N
            TAPISTR_10					ContractNo2;					///< ��Լ2��Ĭ�Ͽ�
            TAPISTR_10					StrikePrice2;					///< ִ�м۸�2��Ĭ�Ͽ�
            TAPICallOrPutFlagType		CallOrPutFlag2;					///< ���ſ���2 Ĭ��N

            TAPIOrderTypeType			OrderType;						///< ί������ ����
            TAPIOrderSourceType			OrderSource;					///< ί����Դ��Ĭ�ϳ��򵥡�
            TAPITimeInForceType			TimeInForce;					///< ί����Ч����,Ĭ�ϵ�����Ч
            TAPIDATETIME				ExpireTime;						///< ��Ч����(GTD�����ʹ��)

            TAPIYNFLAG					IsRiskOrder;					///< �Ƿ���ձ�����Ĭ�ϷǷ��ձ���
            TAPISideType				OrderSide;						///< ��������
            TAPIPositionEffectType		PositionEffect;					///< ��ƽ��־1,Ĭ��N
            TAPIPositionEffectType		PositionEffect2;				///< ��ƽ��־2��Ĭ��N
            TAPISTR_50					InquiryNo;						///< ѯ�ۺ�
            TAPIHedgeFlagType			HedgeFlag;						///< Ͷ����ֵ��Ĭ��N
            TAPIREAL64					OrderPrice;						///< ί�м۸�1
            TAPIREAL64					OrderPrice2;					///< ί�м۸�2��������Ӧ��ʹ��
            TAPIREAL64					StopPrice;						///< �����۸�
            TAPIUINT32					OrderQty;						///< ί������������
            TAPIUINT32					OrderMinQty;					///< ��С�ɽ�����Ĭ��1

            TAPIUINT32					MinClipSize;					///< ��ɽ����С�����
            TAPIUINT32					MaxClipSize;					///< ��ɽ����������

            TAPIINT32					RefInt;							///< ���Ͳο�ֵ
            TAPIREAL64					RefDouble;						///< ����ο�ֵ
            TAPISTR_50					RefString;						///< �ַ����ο�ֵ

			TAPIClientIDType			ClientID;						///< �ͻ����˺ţ�����������˺ţ��������ϱ����˺�
            TAPITacticsTypeType			TacticsType;					///< ���Ե����ͣ�Ĭ��N
            TAPITriggerConditionType	TriggerCondition;				///< ����������Ĭ��N
            TAPITriggerPriceTypeType	TriggerPriceType;				///< �����۸����ͣ�Ĭ��N
            TAPIYNFLAG					AddOneIsValid;					///< �Ƿ�T+1��Ч,Ĭ��T+1��Ч��
	public:
		TapAPINewOrder()
		{
			memset(this, 0, sizeof(TapAPINewOrder));
			CallOrPutFlag = TAPI_CALLPUT_FLAG_NONE;
			CallOrPutFlag2 = TAPI_CALLPUT_FLAG_NONE;
			OrderSource = TAPI_ORDER_SOURCE_PROGRAM;
			TimeInForce=TAPI_ORDER_TIMEINFORCE_GFD;
			IsRiskOrder = APIYNFLAG_NO;
			PositionEffect = TAPI_PositionEffect_NONE;
			PositionEffect2 = TAPI_PositionEffect_NONE;
			HedgeFlag = TAPI_HEDGEFLAG_NONE;
			OrderMinQty = 1;
			TacticsType = TAPI_TACTICS_TYPE_NONE;
			TriggerCondition = TAPI_TRIGGER_CONDITION_NONE;
			TriggerPriceType = TAPI_TRIGGER_PRICE_NONE;
			AddOneIsValid = APIYNFLAG_YES;
		}
    };


    //! ί��������Ϣ
    struct TapAPIOrderInfo
    {
            TAPISTR_20					AccountNo;						///< �ͻ��ʽ��ʺ�

            TAPISTR_10					ExchangeNo;						///< ���������
            TAPICommodityType			CommodityType;					///< Ʒ������
            TAPISTR_10					CommodityNo;					///< Ʒ�ֱ�������
            TAPISTR_10					ContractNo;						///< ��Լ1
            TAPISTR_10					StrikePrice;					///< ִ�м۸�1
            TAPICallOrPutFlagType		CallOrPutFlag;					///< ���ſ���1
            TAPISTR_10					ContractNo2;					///< ��Լ2
            TAPISTR_10					StrikePrice2;					///< ִ�м۸�2
            TAPICallOrPutFlagType		CallOrPutFlag2;					///< ���ſ���2

            TAPIOrderTypeType			OrderType;						///< ί������
            TAPIOrderSourceType			OrderSource;					///< ί����Դ
            TAPITimeInForceType			TimeInForce;					///< ί����Ч����
            TAPIDATETIME				ExpireTime;						///< ��Ч����(GTD�����ʹ��)

            TAPIYNFLAG					IsRiskOrder;					///< �Ƿ���ձ���
            TAPISideType				OrderSide;						///< ��������
            TAPIPositionEffectType		PositionEffect;					///< ��ƽ��־1
            TAPIPositionEffectType		PositionEffect2;				///< ��ƽ��־2
            TAPISTR_50					InquiryNo;						///< ѯ�ۺ�
            TAPIHedgeFlagType			HedgeFlag;						///< Ͷ����ֵ
            TAPIREAL64					OrderPrice;						///< ί�м۸�1
            TAPIREAL64					OrderPrice2;					///< ί�м۸�2��������Ӧ��ʹ��
            TAPIREAL64					StopPrice;						///< �����۸�
            TAPIUINT32					OrderQty;						///< ί������
            TAPIUINT32					OrderMinQty;					///< ��С�ɽ���

            TAPIINT32					RefInt;							///< ���Ͳο�ֵ
            TAPIREAL64					RefDouble;						///< ����ο�ֵ
            TAPISTR_50					RefString;						///< �ַ����ο�ֵ

            TAPIUINT32					MinClipSize;					///< ��ɽ����С�����
            TAPIUINT32					MaxClipSize;					///< ��ɽ����������
            TAPISTR_50					LicenseNo;						///< �����Ȩ��



            TAPICHAR					ServerFlag;						///< ��������ʶ
            TAPISTR_20					OrderNo;						///< ί�б���
            TAPISTR_50                  ClientOrderNo;					///< �ͻ��˱���ί�б��
			TAPIClientIDType            ClientID;						///< �ͻ����˺�.
            TAPITacticsTypeType			TacticsType;					///< ���Ե�����
            TAPITriggerConditionType	TriggerCondition;				///< ��������
            TAPITriggerPriceTypeType	TriggerPriceType;				///< �����۸�����
            TAPIYNFLAG					AddOneIsValid;					///< �Ƿ�T+1��Ч

            TAPISTR_40					ClientLocalIP;					///< �ն˱���IP
            TAPIMACTYPE					ClientMac;						///< �ն˱���Mac��ַ
            TAPISTR_40					ClientIP;						///< �ն������ַ.

            TAPIUINT32					OrderStreamID;					///< ί����ˮ��
            TAPISTR_10					UpperNo;						///< ���ֺ�
            TAPISTR_10					UpperChannelNo;					///< ����ͨ����

            TAPISTR_20					OrderLocalNo;					///< ���غ�
            TAPIUINT32					UpperStreamID;					///< ��������

            TAPISTR_50					OrderSystemNo;					///< ϵͳ��
            TAPISTR_50					OrderExchangeSystemNo;			///< ������ϵͳ�� 
            TAPISTR_50					OrderParentSystemNo;			///< ����ϵͳ��

            TAPISTR_20					OrderInsertUserNo;				///< �µ���
            TAPIDATETIME				OrderInsertTime;				///< �µ�ʱ��
            TAPISTR_20					OrderCommandUserNo;				///< ¼��������
            TAPISTR_20					OrderUpdateUserNo;				///< ί�и�����
            TAPIDATETIME				OrderUpdateTime;				///< ί�и���ʱ��

            TAPIOrderStateType			OrderState;						///< ί��״̬

            TAPIREAL64					OrderMatchPrice;				///< �ɽ���1
            TAPIREAL64					OrderMatchPrice2;				///< �ɽ���2
            TAPIUINT32					OrderMatchQty;					///< �ɽ���1
            TAPIUINT32					OrderMatchQty2;					///< �ɽ���2

            TAPIUINT32					ErrorCode;						///< ���һ�β���������Ϣ��
            TAPISTR_50					ErrorText;						///< ������Ϣ

            TAPIYNFLAG					IsBackInput;					///< �Ƿ�Ϊ¼��ί�е�
            TAPIYNFLAG					IsDeleted;						///< ί�гɽ�ɾ����
            TAPIYNFLAG					IsAddOne;						///< �Ƿ�ΪT+1��

    };

    //! ����֪ͨ�ṹ
    struct TapAPIOrderInfoNotice
    {
            TAPIUINT32					SessionID;						///< �ỰID
            TAPIUINT32					ErrorCode; 						///< ������
            TapAPIOrderInfo*			OrderInfo;						///< ί��������Ϣ
    };

    //! ��������Ӧ��ṹ
    struct TapAPIOrderActionRsp
    {
            TAPIORDERACT				ActionType;						///< ��������
            TapAPIOrderInfo*			OrderInfo;						///< ί����Ϣ
    };


    //! �ͻ��ĵ�����
	//!��������ServerFlag��OrderNo,�Լ�ί�мۺ�ί������ֹ��ۡ������ֶ�����û���á�
    struct TapAPIAmendOrder
    {
        TapAPINewOrder              ReqData;                        ///< ������������
        TAPICHAR					ServerFlag;						///< ��������ʶ
        TAPISTR_20                  OrderNo;                        ///< ί�б��
	public:
		TapAPIAmendOrder()
		{
			memset(this, 0, sizeof(TapAPIAmendOrder));
		}
    };

    //! �ͻ���������ṹ
	//!��������ServerFlag��OrderNo.
    struct TapAPIOrderCancelReq
    {
            TAPIINT32					RefInt;							///< ���Ͳο�ֵ
            TAPIREAL64					RefDouble;						///< ����ο�ֵ
            TAPISTR_50					RefString;						///< �ַ����ο�ֵ
            TAPICHAR					ServerFlag;						///< ��������ʶ
            TAPISTR_20					OrderNo;						///< ί�б���
    };

    //! ����ί������ṹ
    typedef TapAPIOrderCancelReq TapAPIOrderDeactivateReq;

    //! ����ί������ṹ
    typedef TapAPIOrderCancelReq TapAPIOrderActivateReq;

    //! ɾ��ί������ṹ
    typedef TapAPIOrderCancelReq TapAPIOrderDeleteReq;

    //! ί�в�ѯ����ṹ
    struct TapAPIOrderQryReq
    {
            TAPISTR_20					AccountNo;						///< �ʽ��˺�

            TAPISTR_10					ExchangeNo;						///< ���������
            TAPICommodityType			CommodityType;					///< Ʒ������
            TAPISTR_10					CommodityNo;					///< Ʒ�ֱ�������
            TAPIOrderTypeType			OrderType;						///< ί������
            TAPIOrderSourceType			OrderSource;					///< ί����Դ
            TAPITimeInForceType			TimeInForce;					///< ί����Ч����
            TAPIDATETIME				ExpireTime;						///< ��Ч����(GTD�����ʹ��)

            TAPIYNFLAG					IsRiskOrder;					///< �Ƿ���ձ���

            TAPICHAR					ServerFlag;						///< ��������ʶ
            TAPISTR_20                  OrderNo;                        ///< ί�б��

            TAPIYNFLAG					IsBackInput;					///< �Ƿ�Ϊ¼��ί�е�
            TAPIYNFLAG					IsDeleted;						///< ί�гɽ�ɾ����
            TAPIYNFLAG					IsAddOne;						///< �Ƿ�ΪT+1��
    };

    //! ί�����̲�ѯ
    struct TapAPIOrderProcessQryReq
    {
            TAPICHAR					ServerFlag;						///< ��������ʶ
            TAPISTR_20					OrderNo;						///< ί�б���
    };

    //! �ɽ���ѯ����ṹ
	struct TapAPIFillQryReq
	{
		TAPISTR_20					AccountNo;						///< �ͻ��ʽ��ʺ�

		TAPISTR_10					ExchangeNo;						///< ���������
		TAPICommodityType			CommodityType;					///< Ʒ������
		TAPISTR_10					CommodityNo;					///< Ʒ�ֱ�������
		TAPISTR_10					ContractNo;						///< ��Լ1
		TAPISTR_10					StrikePrice;					///< ִ�м۸�
		TAPICallOrPutFlagType		CallOrPutFlag;					///< ���ſ���

		TAPIMatchSourceType			MatchSource;					///< ί����Դ
		TAPISideType				MatchSide;						///< ��������
		TAPIPositionEffectType              PositionEffect;					///< ��ƽ��־1

		TAPICHAR					ServerFlag;						///< ��������ʶ
		TAPISTR_20					OrderNo;						///< ί�б���
		TAPISTR_10					UpperNo;						///< ���ֺ�
		TAPIYNFLAG					IsDeleted;						///< ί�гɽ�ɾ����
		TAPIYNFLAG					IsAddOne;						///< �Ƿ�ΪT+1��
	};

    //! �ɽ���Ϣ
    struct TapAPIFillInfo
    {
            TAPISTR_20					AccountNo;						///< �ͻ��ʽ��ʺ�

            TAPISTR_10					ExchangeNo;						///< ���������
            TAPICommodityType			CommodityType;					///< Ʒ������
            TAPISTR_10					CommodityNo;					///< Ʒ�ֱ�������
            TAPISTR_10					ContractNo;						///< ��Լ1
            TAPISTR_10					StrikePrice;					///< ִ�м۸�
            TAPICallOrPutFlagType		CallOrPutFlag;					///< ���ſ���

            TAPIMatchSourceType			MatchSource;					///< ί����Դ
            TAPISideType				MatchSide;						///< ��������
            TAPIPositionEffectType              PositionEffect;					///< ��ƽ��־1

            TAPICHAR					ServerFlag;						///< ��������ʶ
            TAPISTR_20					OrderNo;						///< ί�б���
            TAPISTR_50					OrderSystemNo;					///< ϵͳ��

            TAPISTR_20					MatchNo;						///< ���سɽ���
            TAPISTR_70					UpperMatchNo;					///< ���ֳɽ���
            TAPISTR_70					ExchangeMatchNo;				///< �������ɽ���

            TAPIDATETIME				MatchDateTime;					///< �ɽ�ʱ��
            TAPIDATETIME				UpperMatchDateTime;				///< ���ֳɽ�ʱ��

            TAPISTR_10					UpperNo;						///< ���ֺ�

            TAPIREAL64					MatchPrice;						///< �ɽ���
            TAPIUINT32					MatchQty;						///< �ɽ���

            TAPIYNFLAG					IsDeleted;						///< ί�гɽ�ɾ����
            TAPIYNFLAG					IsAddOne;						///< �Ƿ�ΪT+1��

            TAPISTR_10					FeeCurrencyGroup;				///< �ͻ������ѱ�����
            TAPISTR_10					FeeCurrency;					///< �ͻ������ѱ���
            TAPIREAL64					FeeValue;						///< ������
            TAPIYNFLAG					IsManualFee;					///< �˹��ͻ������ѱ��

            TAPIREAL64					ClosePrositionPrice;					///< ָ���۸�ƽ��
    };

    //! ƽ�ֲ�ѯ����ṹ
    struct TapAPICloseQryReq
    {
            TAPISTR_20					AccountNo;						///< �ͻ��ʽ��ʺ�

            TAPISTR_10					ExchangeNo;						///< ���������
            TAPICommodityType			CommodityType;					///< Ʒ������
            TAPISTR_10					CommodityNo;					///< Ʒ�ֱ�������
    };

    //! ƽ����Ϣ
    struct TapAPICloseInfo
    {
            TAPISTR_20					AccountNo;						///< �ͻ��ʽ��ʺ�

            TAPISTR_10					ExchangeNo;						///< ���������
            TAPICommodityType			CommodityType;					///< Ʒ������
            TAPISTR_10					CommodityNo;					///< Ʒ�ֱ�������
            TAPISTR_10					ContractNo;						///< ��Լ
            TAPISTR_10					StrikePrice;					///< ִ�м۸�
            TAPICallOrPutFlagType		CallOrPutFlag;					///< ���ſ���

            TAPISideType				CloseSide;						///< ƽ��һ�ߵ���������
            TAPIUINT32					CloseQty;						///< ƽ�ֳɽ���
            TAPIREAL64					OpenPrice;						///< ���ֳɽ���
            TAPIREAL64					ClosePrice;						///< ƽ�ֳɽ���

            TAPISTR_20					OpenMatchNo;					///< ���سɽ���
            TAPIDATETIME				OpenMatchDateTime;				///< �ɽ�ʱ��
            TAPISTR_20					CloseMatchNo;					///< ���سɽ���
            TAPIDATETIME				CloseMatchDateTime;				///< �ɽ�ʱ��

            TAPIUINT32                  CloseStreamId;					///< ƽ������

            TAPISTR_10					CommodityCurrencyGroup;			///< Ʒ�ֱ�����
            TAPISTR_10					CommodityCurrency;				///< Ʒ�ֱ���

            TAPIREAL64					CloseProfit;					///< ƽ��ӯ��
    };

    //! �ֲֲ�ѯ����ṹ
    struct TapAPIPositionQryReq
    {
            TAPISTR_20 AccountNo;
    };



    //! �ֲ���Ϣ
    struct TapAPIPositionInfo
    {
            TAPISTR_20					AccountNo;						///< �ͻ��ʽ��ʺ�

            TAPISTR_10					ExchangeNo;						///< ���������
            TAPICommodityType			CommodityType;					///< Ʒ������
            TAPISTR_10					CommodityNo;					///< Ʒ�ֱ�������
            TAPISTR_10					ContractNo;						///< ��Լ1
            TAPISTR_10					StrikePrice;					///< ִ�м۸�
            TAPICallOrPutFlagType		CallOrPutFlag;					///< ���ſ���

            TAPISideType				MatchSide;						///< ��������
            TAPIHedgeFlagType			HedgeFlag;						///< Ͷ����ֵ

            TAPISTR_70					PositionNo;						///< ���سֲֺţ���������д

            TAPICHAR					ServerFlag;						///< ��������ʶ
            TAPISTR_20					OrderNo;						///< ί�б���
            TAPISTR_20					MatchNo;						///< ���سɽ���
            TAPISTR_10					UpperNo;						///< ���ֺ�


            TAPIREAL64					PositionPrice;					///< �ֲּ�
            TAPIUINT32					PositionQty;					///< �ֲ���

        TAPIUINT32                  PositionStreamId;				///< �ֲ�����

            TAPISTR_10					CommodityCurrencyGroup;			///< Ʒ�ֱ�����
            TAPISTR_10					CommodityCurrency;				///< Ʒ�ֱ���

            TAPIREAL64					CalculatePrice;					///< ��ǰ����۸�

            TAPIREAL64					AccountInitialMargin;			///< �ͻ���ʼ��֤��
            TAPIREAL64					AccountMaintenanceMargin;		///< �ͻ�ά�ֱ�֤��
            TAPIREAL64					UpperInitialMargin;				///< ���ֳ�ʼ��֤��
            TAPIREAL64					UpperMaintenanceMargin;			///< ����ά�ֱ�֤��

            TAPIREAL64					PositionProfit;					///< �ֲ�ӯ��
            TAPIREAL64					LMEPositionProfit;				///< LME�ֲ�ӯ��
            TAPIREAL64					OptionMarketValue;				///< ��Ȩ��ֵ
			TAPIYNFLAG					IsHistory;						///< �Ƿ�Ϊ��֡�
    };

    //! �ͻ��ֲ�ӯ��
    struct TapAPIPositionProfit
    {
            TAPISTR_70					PositionNo;						///< ���سֲֺţ���������д
            TAPIUINT32					PositionStreamId;				///< �ֲ�����
            TAPIREAL64					PositionProfit;					///< �ֲ�ӯ��
            TAPIREAL64					LMEPositionProfit;				///< LME�ֲ�ӯ��
            TAPIREAL64					OptionMarketValue;				///< ��Ȩ��ֵ
            TAPIREAL64					CalculatePrice;					///< ����۸�
    };

    //! �ͻ��ֲ�ӯ��֪ͨ
    struct TapAPIPositionProfitNotice
    {
            TAPIYNFLAG					IsLast;							///< �Ƿ����һ��
            TapAPIPositionProfit*		Data;							///< �ͻ��ֲ�ӯ����Ϣ
    };

	struct TapAPIPositionSummary
	{
		TAPISTR_20					AccountNo;						///< �ͻ��ʽ��ʺ�

		TAPISTR_10					ExchangeNo;						///< ���������
		TAPICommodityType			CommodityType;					///< Ʒ������
		TAPISTR_10					CommodityNo;					///< Ʒ�ֱ�������
		TAPISTR_10					ContractNo;						///< ��Լ1
		TAPISTR_10					StrikePrice;					///< ִ�м۸�
		TAPICallOrPutFlagType		CallOrPutFlag;					///< ���ſ���

		TAPISideType				MatchSide;						///< ��������
		TAPIREAL64					PositionPrice;					///< �ֲ־��ۡ�
		TAPIUINT32					PositionQty;					///< �ֲ���
		TAPIUINT32					HisPositionQty;					///< ��ʷ�ֲ���
	};



    //! �ʽ��ѯ����
    struct TapAPIFundReq
    {
            TAPISTR_20					AccountNo;						///< �ͻ��ʽ��ʺ�
    };
    //! �ʽ��˺��ʽ���Ϣ
    struct TapAPIFundData
    {
            TAPISTR_20					AccountNo;						///< �ͻ��ʽ��˺�

            TAPISTR_10					CurrencyGroupNo;				///< �������
            TAPISTR_10					CurrencyNo;						///< ���ֺ�(Ϊ�ձ�ʾ����������ʽ�)
            TAPIREAL64					TradeRate;						///< ���׻���
            TAPIFutureAlgType			FutureAlg;                      ///< �ڻ��㷨
            TAPIOptionAlgType			OptionAlg;                      ///< ��Ȩ�㷨

            TAPIREAL64					PreBalance;						///< ���ս��
            TAPIREAL64					PreUnExpProfit;					///< ����δ����ƽӯ
            TAPIREAL64					PreLMEPositionProfit;			///< ����LME�ֲ�ƽӯ
            TAPIREAL64					PreEquity;						///< ����Ȩ��
            TAPIREAL64					PreAvailable1;					///< ���տ���
            TAPIREAL64					PreMarketEquity;				///< ������ֵȨ��

            TAPIREAL64					CashInValue;					///< ���
            TAPIREAL64					CashOutValue;					///< ����
            TAPIREAL64					CashAdjustValue;				///< �ʽ����
            TAPIREAL64					CashPledged;					///< ��Ѻ�ʽ�
            TAPIREAL64					FrozenFee;						///< ����������
            TAPIREAL64					FrozenDeposit;					///< ���ᱣ֤��
            TAPIREAL64					AccountFee;						///< �ͻ������Ѱ�������������
            TAPIREAL64					SwapInValue;					///< �����ʽ�
            TAPIREAL64					SwapOutValue;					///< ����ʽ�
            TAPIREAL64					PremiumIncome;					///< Ȩ������ȡ
            TAPIREAL64					PremiumPay;						///< Ȩ����֧��
            TAPIREAL64					CloseProfit;					///< ƽ��ӯ��
            TAPIREAL64					FrozenFund;						///< �����ʽ�
            TAPIREAL64					UnExpProfit;					///< δ����ƽӯ
            TAPIREAL64					ExpProfit;						///< ����ƽ��ӯ��
            TAPIREAL64					PositionProfit;					///< ����LME�ֲ�ӯ��
            TAPIREAL64					LmePositionProfit;				///< LME�ֲ�ӯ��
            TAPIREAL64					OptionMarketValue;				///< ��Ȩ��ֵ
            TAPIREAL64					AccountIntialMargin;			///< �ͻ���ʼ��֤��
            TAPIREAL64					AccountMaintenanceMargin;		///< �ͻ�ά�ֱ�֤��
            TAPIREAL64					UpperInitalMargin;				///< ���ֳ�ʼ��֤��
            TAPIREAL64					UpperMaintenanceMargin;			///< ����ά�ֱ�֤��
            TAPIREAL64					Discount;						///< LME����

            TAPIREAL64					Balance;						///< ���ս��
            TAPIREAL64					Equity;							///< ����Ȩ��
            TAPIREAL64					Available;						///< ���տ���
            TAPIREAL64					CanDraw;						///< ����ȡ
            TAPIREAL64					MarketEquity;					///< �˻���ֵ
            TAPIREAL64					AuthMoney;                      ///< �����ʽ�
    };

    //! ����Ʒ����Ϣ
    struct TapAPICommodityInfo
    {
			TAPISTR_10							ExchangeNo;						//����������
			TAPICommodityType					CommodityType;					//Ʒ������
			TAPISTR_10							CommodityNo;					//Ʒ�ֱ��

			TAPISTR_20							CommodityName;					//Ʒ������
			TAPISTR_30							CommodityEngName;				//Ʒ��Ӣ������

			TAPISTR_10							RelateExchangeNo;
			TAPICommodityType					RelateCommodityType;
			TAPISTR_10							RelateCommodityNo;

			TAPISTR_10							RelateExchangeNo2;
			TAPICommodityType					RelateCommodityType2;
			TAPISTR_10							RelateCommodityNo2;

			TAPISTR_10							CurrencyGroupNo;
			TAPISTR_10							TradeCurrency;					//���ױ���
			TAPIREAL64							ContractSize;					//ÿ�ֳ���
			TAPIOpenCloseModeType				OpenCloseMode;					//��ƽ��ʽ
			TAPIREAL64							StrikePriceTimes;				//ִ�м۸���

			TAPIREAL64							CommodityTickSize;				//��С�䶯��λ
			TAPIINT32							CommodityDenominator;			//���۷�ĸ
			TAPICmbDirectType					CmbDirect;						//��Ϸ���
			TAPIDeliveryModeType				DeliveryMode;					//������Ȩ��ʽ
			TAPIINT32							DeliveryDays;					//������ƫ��
			TAPITIME							AddOneTime;						//T+1�ָ�ʱ��
			TAPIINT32							CommodityTimeZone;				//Ʒ��ʱ��
			TAPIYNFLAG							IsAddOne;						//�Ƿ���T+1ʱ�Ρ�


    };

    //! ���׺�Լ��Ϣ
    struct TapAPITradeContractInfo
    {
            TAPISTR_10              ExchangeNo;                             ///< ����������
            TAPICommodityType		CommodityType;                          ///< Ʒ������
            TAPISTR_10              CommodityNo;                            ///< Ʒ�ֱ��
            TAPISTR_10              ContractNo1;                            ///< ��Լ����1
            TAPISTR_10              StrikePrice1;                           ///< ִ�м�1
            TAPICallOrPutFlagType	CallOrPutFlag1;                         ///< ���ǿ�����ʾ1
            TAPISTR_10              ContractNo2;                            ///< ��Լ����2
            TAPISTR_10              StrikePrice2;                           ///< ִ�м�2
            TAPICallOrPutFlagType	CallOrPutFlag2;                         ///< ���ǿ�����ʾ2
            TAPIContractTypeType	ContractType;                           ///< ��Լ����
            TAPISTR_10				QuoteUnderlyingContract;				///< ������ʵ��Լ
            TAPISTR_70              ContractName;                           ///< ��Լ����
            TAPIDATE                ContractExpDate;                        ///< ��Լ������	
            TAPIDATE                LastTradeDate;                          ///< �������
            TAPIDATE                FirstNoticeDate;                        ///< �״�֪ͨ��
    };


    //! ���ױ�����Ϣ
    struct TapAPICurrencyInfo
    {
            TAPISTR_10					CurrencyNo;						///< ���ֱ��
            TAPISTR_10					CurrencyGroupNo;				///< ��������
            TAPIREAL64					TradeRate;						///< ���׻���
            TAPIREAL64					TradeRate2;						///< ���׻���2

            TAPIFutureAlgType			FutureAlg;						///< ���'1',���� '2'
            TAPIOptionAlgType			OptionAlg;						///< ��Ȩ�㷨,�ڻ���ʽ'1',��Ȩ��ʽ'2'
    };
    //=============================================================================
    /**
    *	 G_DATATYPE_T_TAPIMSGRECEIVERTYPE	��Ϣ����������
    *	
    */
    //=============================================================================
    //! ��Ϣ����������
    typedef TAPICHAR				TAPIMsgReceiverType;
    //! ���ʽ��˺ſͻ�
    const TAPIMsgReceiverType		TAPI_MSG_RECEIVER_ACCOUNTNO = '1';
    //! �ʽ��˺ŷ���
    const TAPIMsgReceiverType		TAPI_MSG_RECEIVER_ACCOUNTGROUPNO = '2';
    //! �������Ե��ʽ��˺�
    const TAPIMsgReceiverType		TAPI_MSG_RECEIVER_ATTRIBUTE = '3';
    //! ָ����¼�û�
    const TAPIMsgReceiverType		TAPI_MSG_RECEIVER_USERNO = '4';
    

    //=============================================================================
    /**
    *	 G_DATATYPE_T_TAPIMSGLEVELTYPE	��Ϣ����
    *	
    */
    //=============================================================================
    //! ��Ϣ����
    typedef TAPICHAR				TAPIMsgLevelType;
    //! ��ͨ
    const TAPIMsgLevelType			TAPI_MSG_LEVEL_NORMAL = '1';
    //! ��Ҫ
    const TAPIMsgLevelType			TAPI_MSG_LEVEL_IMPORTANT = '2';
    //! ����
    const TAPIMsgLevelType			TAPI_MSG_LEVEL_IMERGENCY = '3';
    


    //=============================================================================
    /**
    *	 G_DATATYPE_T_TAPIMSGTYPETYPE	��Ϣ����
    *	
    */
    //=============================================================================
    //! ��Ϣ����
    typedef TAPICHAR				TAPIMsgTypeType;
    //! ����
    const TAPIMsgTypeType			TAPI_Msg_TYPE_MANAGER = '1';
    //! ����
    const TAPIMsgTypeType			TAPI_Msg_TYPE_RISKCONTROL = '2';
    


    //=============================================================================
    /**
    *	 G_DATATYPE_T_TAPIBILLTYPETYPE	�˵�����
    *	
    */
    //=============================================================================
    //! �˵�����
    typedef TAPICHAR						TAPIBillTypeType;
    //! ���˵�
    const TAPIBillTypeType					TAPI_BILL_DATE = 'D';
    //! ���˵�
    const TAPIBillTypeType					TAPI_BILL_MONTH = 'M';
    

    //=============================================================================
    /**
    *	 G_DATATYPE_T_TAPIBILLFILETYPETYPE	�ʵ��ļ�����
    *	
    */
    //=============================================================================
    //! �ʵ��ļ�����
    typedef TAPICHAR						TAPIBillFileTypeType;
    //! txt��ʽ�ļ�
    const TAPIBillFileTypeType				TAPI_BILL_FILE_TXT = 'T';
    //! pdf��ʽ�ļ�
    const TAPIBillFileTypeType				TAPI_BILL_FILE_PDF = 'F';
    



    //! ���׻�����Ϣ��ѯ����ṹ
    struct TapAPITradeMessageReq
    {
            TAPISTR_20					AccountNo;
            TAPISTR_20					AccountAttributeNo;
            TAPIDATETIME				BenginSendDateTime;
            TAPIDATETIME				EndSendDateTime;
    };





    //! ������Ϣ����Ӧ��ṹ
    struct TapAPITradeMessage
    {
            TAPIUINT32				SerialID;						///< ����

            TAPISTR_20				AccountNo;						///< �ͻ��ʽ��˺�

            TAPIDATETIME			TMsgValidDateTime;				///< ��Ϣ��Чʱ��
            TAPISTR_50				TMsgTitle;						///< ��Ϣ����
            TAPISTR_500				TMsgContent;					///< ��Ϣ����
            TAPIMsgTypeType			TMsgType;						///< ��Ϣ����
            TAPIMsgLevelType		TMsgLevel;						///< ��Ϣ����

            TAPIYNFLAG				IsSendBySMS;					///< �Ƿ��Ͷ���
            TAPIYNFLAG				IsSendByEMail;					///< �Ƿ����ʼ�
            TAPISTR_20				Sender;							///< ������
            TAPIDATETIME			SendDateTime;					///< ����ʱ��
    };

    //! �ͻ��˵���ѯ����ṹ
    struct TapAPIBillQryReq
    {
            TAPISTR_20				UserNo;
            TAPIBillTypeType		BillType;
            TAPIDATE				BillDate;
            TAPIBillFileTypeType	BillFileType;
    };

    //! �ͻ��˵���ѯӦ��ṹ
    struct TapAPIBillQryRsp
    {
            TapAPIBillQryReq		Reqdata;
            TAPIINT32				BillLen;
            TAPICHAR				BillText[1];	///< �䳤�˵����ݣ�������BillLenָ��
    };

    //! ��ʷί�в�ѯ����ṹ
    struct TapAPIHisOrderQryReq
    {
            TAPISTR_20			AccountNo;							///< �ͻ��ʽ��˺�
            TAPISTR_20			AccountAttributeNo;					///< �ͻ����Ժ�
            TAPIDATE			BeginDate;							///< ��ʼʱ�� (����)
            TAPIDATE			EndDate;							///< ����ʱ�� (����)
    };

    struct TapAPIHisOrderQryRsp
    {
            TAPIDATE					Date;								///< ����
            TAPISTR_20					AccountNo;							///< �ͻ��ʽ��˺�

            TAPISTR_10					ExchangeNo;							///< ���������
            TAPICommodityType			CommodityType;						///< Ʒ������
            TAPISTR_10					CommodityNo;						///< Ʒ�ֱ�������
            TAPISTR_10					ContractNo;							///< ��Լ
            TAPISTR_10					StrikePrice;						///< ִ�м۸�
            TAPICallOrPutFlagType		CallOrPutFlag;						///< ���ſ���
            TAPISTR_10					ContractNo2;						///< ��Լ2
            TAPISTR_10					StrikePrice2;						///< ִ�м۸�2
            TAPICallOrPutFlagType		CallOrPutFlag2;						///< ���ſ���2

            TAPIOrderTypeType			OrderType;							///< ί������
            TAPIOrderSourceType			OrderSource;						///< ί����Դ
            TAPITimeInForceType			TimeInForce;						///< ί����Ч����
            TAPIDATETIME				ExpireTime;							///< ��Ч����(GTD�����ʹ��)
            TAPIYNFLAG					IsRiskOrder;						///< �Ƿ���ձ���
            TAPISideType				OrderSide;							///< ��������
            TAPIPositionEffectType		PositionEffect;						///< ��ƽ��־
            TAPIPositionEffectType		PositionEffect2;					///< ��ƽ��־2
            TAPISTR_50					InquiryNo;							///< ѯ�ۺ�
            TAPIHedgeFlagType			HedgeFlag;							///< Ͷ����ֵ
            TAPIREAL64					OrderPrice;							///< ί�м۸�
            TAPIREAL64					OrderPrice2;						///< ί�м۸�2��������Ӧ��ʹ��
            TAPIREAL64					StopPrice;							///< �����۸�
            TAPIUINT32					OrderQty;							///< ί������
            TAPIUINT32					OrderMinQty;						///< ��С�ɽ���
            TAPIUINT32					OrderCanceledQty;					///< ��������

            TAPIINT32					RefInt;								///< ���Ͳο�ֵ
            TAPIREAL64					RefDouble;							///<����ο��͡�
            TAPISTR_50					RefString;							///< �ַ����ο�ֵ

            TAPICHAR					ServerFlag;							///< ��������ʶ
            TAPISTR_20					OrderNo;                                            ///< ί�б���
            TAPIUINT32					OrderStreamID;						///< ί����ˮ��

            TAPISTR_10					UpperNo;							///< ���ֺ�
            TAPISTR_10					UpperChannelNo;						///< ����ͨ�����
            TAPISTR_20					OrderLocalNo;						///< ���غ�
            TAPIUINT32					UpperStreamID;						///< ��������

            TAPISTR_50					OrderSystemNo;						///< ϵͳ��
            TAPISTR_50					OrderExchangeSystemNo;				///< ������ϵͳ��
            TAPISTR_50					OrderParentSystemNo;				///< ����ϵͳ�� 

            TAPISTR_20					OrderInsertUserNo;					///< �µ���
            TAPIDATETIME				OrderInsertTime;					///< �µ�ʱ��
            TAPISTR_20					OrderCommandUserNo;					///< ָ���´���
            TAPISTR_20					OrderUpdateUserNo;					///< ί�и�����
            TAPIDATETIME				OrderUpdateTime;					///< ί�и���ʱ��

            TAPIOrderStateType			OrderState;							///< ί��״̬

            TAPIREAL64					OrderMatchPrice;					///< �ɽ���
            TAPIREAL64					OrderMatchPrice2;					///< �ɽ���2
            TAPIUINT32					OrderMatchQty;						///< �ɽ���
            TAPIUINT32					OrderMatchQty2;						///< �ɽ���2

            TAPIUINT32					ErrorCode;							///< ���һ�β���������Ϣ��
            TAPISTR_50					ErrorText;							///< ������Ϣ

            TAPIYNFLAG					IsBackInput;						///< �Ƿ�Ϊ¼��ί�е�
            TAPIYNFLAG					IsDeleted;							///< ί�гɽ�ɾ�����
            TAPIYNFLAG					IsAddOne;							///< �Ƿ�ΪT+1��
            TAPIYNFLAG					AddOneIsValid;						///< �Ƿ�T+1��Ч

            TAPIUINT32					MinClipSize;						///< ��ɽ����С�����
            TAPIUINT32					MaxClipSize;						///< ��ɽ����������
            TAPISTR_50					LicenseNo;							///< �����Ȩ��

            TAPITacticsTypeType			TacticsType;						///< ���Ե�����	
            TAPITriggerConditionType	TriggerCondition;					///< ��������
            TAPITriggerPriceTypeType	TriggerPriceType;					///< �����۸�����

    };
	  //! ��ʷ�ɽ���ѯ����ṹ
    struct TapAPIHisMatchQryReq
    {
            TAPISTR_20				AccountNo;							///< �ͻ��ʽ��˺�
            TAPISTR_20				AccountAttributeNo;					///< �ͻ����Ժ�
            TAPIDATE				BeginDate;							///< ��ʼ���ڣ�����
            TAPIDATE				EndDate;							///< �������ڣ�����
            TAPICHAR				CountType;							///< ͳ������
    };

    //! ��ʷ�ɽ���ѯӦ��ṹ
    //! key1=SerialID
    //! key2=ExchangeNo+MatchCmbNo+MatchNo+MatchSide
    struct TapAPIHisMatchQryRsp
    {

            TAPIDATE				SettleDate;							///< ��������
            TAPIDATE				TradeDate;							///<��������
            TAPISTR_20				AccountNo;							///< �ͻ��ʽ��˺�

            TAPISTR_10				ExchangeNo;							///< �г����߽���������
            TAPICommodityType		CommodityType;						///< Ʒ������
            TAPISTR_10				CommodityNo;						///< Ʒ�ֺ�
            TAPISTR_10				ContractNo;							///< ��Լ��
            TAPISTR_10				StrikePrice;						///< ִ�м�
            TAPICallOrPutFlagType	CallOrPutFlag;						///< ���ǿ�����־

            TAPIMatchSourceType		MatchSource;						///< �ɽ���Դ	
            TAPISideType			MatchSide;							///< ��������
            TAPIPositionEffectType	PositionEffect;						///< ��ƽ��־
            TAPIHedgeFlagType		HedgeFlag;							///< Ͷ����ֵ
            TAPIREAL64				MatchPrice;							///< �ɽ���
            TAPIUINT32				MatchQty;							///< �ɽ���

            TAPISTR_20				OrderNo;							///< ί�к�
            TAPISTR_20				MatchNo;							///< �ɽ����
            TAPIUINT32				MatchStreamID;						///< �ɽ���ˮ��

            TAPISTR_10				UpperNo;							///< ���ֺ�
            TAPISTR_20				MatchCmbNo;							///< ��Ϻ�
            TAPISTR_70				ExchangeMatchNo;					///< �ɽ����(�������ɽ���)
            TAPIUINT32				MatchUpperStreamID;					///< ������ˮ��

            TAPISTR_10				CommodityCurrencyGroup;
            TAPISTR_10				CommodityCurrency;					//Ʒ�ֱ���		

            TAPIREAL64				Turnover;							///< �ɽ����
            TAPIREAL64				PremiumIncome;						///< Ȩ��������
            TAPIREAL64				PremiumPay;							///< Ȩ����֧��

            TAPIREAL64				AccountFee;							///< �ͻ�������
            TAPISTR_10				AccountFeeCurrencyGroup;
            TAPISTR_10				AccountFeeCurrency;					///< �ͻ������ѱ���
            TAPIYNFLAG				IsManualFee;						///< �˹��ͻ������ѱ��
            TAPIREAL64				AccountOtherFee;					//�ͻ���������

            TAPIREAL64				UpperFee;							///< ����������
            TAPISTR_10				UpperFeeCurrencyGroup;
            TAPISTR_10				UpperFeeCurrency;					///< ���������ѱ���
            TAPIYNFLAG				IsUpperManualFee;					///< �˹����������ѱ��
            TAPIREAL64				UpperOtherFee;						//������������

            TAPIDATETIME			MatchDateTime;						///< �ɽ�ʱ��
            TAPIDATETIME			UpperMatchDateTime;					///< ���ֳɽ�ʱ��

            TAPIREAL64				CloseProfit;						///< ƽ��ӯ��
            TAPIREAL64				ClosePrice;							///< ָ��ƽ�ּ۸�

            TAPIUINT32				CloseQty;							///< ƽ����

            TAPISTR_10				SettleGroupNo;						///<�������
            TAPISTR_20				OperatorNo;							///< ����Ա
            TAPIDATETIME			OperateTime;						///< ����ʱ��


    };

    //! ��ʷί�����̲�ѯ����ṹ
    struct TapAPIHisOrderProcessQryReq
    {
            TAPIDATE				Date;
            TAPISTR_20				OrderNo;
    };

    //! ��ʷί�����̲�ѯӦ�����ݽṹ
    typedef TapAPIHisOrderQryRsp		TapAPIHisOrderProcessQryRsp;

 
    //=============================================================================
    /**
    *	 G_DATATYPE_SETTLEFLAG	��������
    *	
    */
    //=============================================================================
    //! ��������
    typedef TAPICHAR						TAPISettleFlagType;
    //! �Զ�����
    const TAPISettleFlagType					SettleFlag_AutoSettle = '0';
    //! �˹�����
    const TAPISettleFlagType					SettleFlagh_Manual= '2';
       
    
    //! ��ʷ�ֲֲ�ѯ����ṹ
    struct TapAPIHisPositionQryReq
    {
            TAPISTR_20				AccountNo;						///< �ͻ��ʽ��˺�
            //TAPISTR_20				AccountAttributeNo;				///< �ͻ����Ժ�
            TAPIDATE				Date;							///< ����
            //TAPICHAR				CountType;						///< ͳ������
            TAPISettleFlagType                  SettleFlag;                                             ///<��������
    };

    //! ��ʷ�ֲֲ�ѯ����Ӧ��ṹ
    //! key1=SerialID
    //! key2=��������+������+��ű��+�ֱֲ��+��������
    struct TapAPIHisPositionQryRsp
    {
            TAPIDATE				SettleDate;							///< ��������
            TAPIDATE				OpenDate;							///< ��������

            TAPISTR_20				AccountNo;							///< �ͻ��ʽ��˺�

            TAPISTR_10				ExchangeNo;							///< �г����߽���������
            TAPICommodityType		CommodityType;						///< Ʒ������
            TAPISTR_10				CommodityNo;						///< Ʒ�ֱ���
            TAPISTR_10				ContractNo;							///< ��Լ��
            TAPISTR_10				StrikePrice;						///< ִ�м�
            TAPICallOrPutFlagType	CallOrPutFlag;						///< ���ǿ�����־

            TAPISideType			MatchSide;							///< ��������
            TAPIHedgeFlagType		HedgeFlag;							///< Ͷ����ֵ
            TAPIREAL64				PositionPrice;						///< �ֲּ۸�
            TAPIUINT32				PositionQty;						///< �ֲ���

            TAPISTR_20				OrderNo;							///< 
            TAPISTR_70				PositionNo;							///< �ֱֲ��

            TAPISTR_10				UpperNo;							///< ���ֺ�	

            TAPISTR_10				CurrencyGroup;						///< Ʒ�ֱ�����
            TAPISTR_10				Currency;							///< Ʒ�ֱ���

            TAPIREAL64				PreSettlePrice;						///< ���ս���۸�
            TAPIREAL64				SettlePrice;						///< ����۸�
            TAPIREAL64				PositionDProfit;					///< �ֲ�ӯ��(����)
            TAPIREAL64				LMEPositionProfit;					///< LME�ֲ�ӯ��
            TAPIREAL64				OptionMarketValue;					///< ��Ȩ��ֵ

            TAPIREAL64				AccountInitialMargin;				///< �ͻ���ʼ��֤��
            TAPIREAL64				AccountMaintenanceMargin;			///< �ͻ�ά�ֱ�֤��
            TAPIREAL64				UpperInitialMargin;					///< ���ֳ�ʼ��֤��
            TAPIREAL64				UpperMaintenanceMargin;				///< ����ά�ֱ�֤��

            TAPISTR_10				SettleGroupNo;						///< �������
    };

    //! �����ѯ����ṹ
    struct TapAPIHisDeliveryQryReq
    {
            TAPISTR_20				AccountNo;							///< �ͻ��ʽ��˺�
            TAPISTR_20				AccountAttributeNo;					///< �ͻ����Ժ�
            TAPIDATE				BeginDate;							///< ��ʼ���ڣ����
            TAPIDATE				EndDate;							///< �������ڣ����
            TAPICHAR				CountType;							///< ͳ������
    };

    //! �����ѯӦ�����ݽṹ
    //! key1=SerialID
    struct TapAPIHisDeliveryQryRsp
    {
            TAPIDATE				DeliveryDate;						///< ��������
            TAPIDATE				OpenDate;							///< ��������
            TAPISTR_20				AccountNo;							///< �ͻ��ʽ��˺�

            TAPISTR_10				ExchangeNo;							///< �г��Ż���������
            TAPICommodityType       CommodityType;						///< Ʒ������
            TAPISTR_10				CommodityNo;						///< Ʒ�ֱ���
            TAPISTR_10				ContractNo;							///< ��Լ����
            TAPISTR_10				StrikePrice;						///< ִ�м�
            TAPICallOrPutFlagType	CallOrPutFlag;						///< ���ǿ�����־

            TAPIMatchSourceType		MatchSource;						///< �ɽ���Դ
            TAPISideType			OpenSide;							///< ���ַ���
            TAPIREAL64				OpenPrice;							///< ���ּ۸�
            TAPIREAL64				DeliveryPrice;						///< ����۸�
            TAPIUINT32				DeliveryQty;						///< ������
            TAPIUINT32				FrozenQty;							///< ������

            TAPISTR_20				OpenNo;								///< ���ֳɽ���
            TAPISTR_10				UpperNo;							///< ���ֱ��

            TAPISTR_10				CommodityCurrencyGroupy;			///< Ʒ�ֱ���
            TAPISTR_10				CommodityCurrency;					///< Ʒ�ֱ���
            TAPIREAL64				PreSettlePrice;						///< ���ս����
            TAPIREAL64				DeliveryProfit;						///< ����ӯ��

            TAPIREAL64				AccountFrozenInitialMargin;			///< �ͻ���ʼ���ᱣ֤��
            TAPIREAL64				AccountFrozenMaintenanceMargin;		///< �ͻ�ά�ֶ��ᱣ֤��
            TAPIREAL64				UpperFrozenInitialMargin;			///< ���ֳ�ʼ���ᱣ֤��
            TAPIREAL64				UpperFrozenMaintenanceMargin;		///< ����ά�ֶ��ᱣ֤��

            TAPISTR_10				AccountFeeCurrencyGroup;
            TAPISTR_10				AccountFeeCurrency;					///< �ͻ������ѱ���
            TAPIREAL64				AccountDeliveryFee;					///< �ͻ����������� 
            TAPISTR_10				UpperFeeCurrencyGroup;
            TAPISTR_10				UpperFeeCurrency;					///< ���������ѱ���
            TAPIREAL64				UpperDeliveryFee;					///< ���ֽ���������

            TAPIDeliveryModeType	DeliveryMode;						///< ������Ȩ��ʽ
            TAPISTR_20				OperatorNo;							///< ����Ա
            TAPIDATETIME			OperateTime;						///< ����ʱ��
            TAPISTR_20				SettleGourpNo;						///< �������
    };

    //! �ͻ��ʽ������ѯ����ṹ
    struct TapAPIAccountCashAdjustQryReq
    {
            TAPIUINT32				SerialID;
            TAPISTR_20				AccountNo;
            TAPISTR_20				AccountAttributeNo;				///< �ͻ�����
            TAPIDATE				BeginDate;						///< ����
            TAPIDATE				EndDate;						///< ����
    };

    //! �ͻ��ʽ������ѯӦ��ṹ
    struct TapAPIAccountCashAdjustQryRsp
    {
            TAPIDATE					Date;							///< ����
            TAPISTR_20					AccountNo;						///< �ͻ��ʽ��˺�

            TAPICashAdjustTypeType		CashAdjustType;					///< �ʽ��������
            TAPISTR_10					CurrencyGroupNo;					//�������
            TAPISTR_10					CurrencyNo;						///< ���ֺ�
            TAPIREAL64					CashAdjustValue;				///< �ʽ�������
            TAPISTR_100					CashAdjustRemark;				///< �ʽ������ע

            TAPIDATETIME				OperateTime;					///< ����ʱ��
            TAPISTR_20					OperatorNo;						///< ����Ա

            TAPISTR_10					AccountBank;					///< �ͻ�����
            TAPISTR_20					BankAccount;					///< �ͻ������˺�
            TAPIBankAccountLWFlagType	AccountLWFlag;					///< �ͻ�����ұ�ʶ
            TAPISTR_10					CompanyBank;					///< ��˾����
            TAPISTR_20					InternalBankAccount;			///< ��˾�����˻�
            TAPIBankAccountLWFlagType	CompanyLWFlag;					///< ��˾����ұ�ʶ
    };
	//! �ͻ��˻������Ѽ��������ѯ����ṹ
	struct TapAPIAccountFeeRentQryReq
	{
		TAPISTR_20						AccountNo;
	};
	//! �ͻ��˻������Ѽ��������ѯӦ��ṹ
	struct TapAPIAccountFeeRentQryRsp
	{
		TAPISTR_20						AccountNo;
		TAPISTR_10						ExchangeNo;
		TAPICommodityType				CommodityType;
		TAPISTR_10						CommodityNo;
		TAPIMatchSourceType				MatchSource;
		TAPICalculateModeType			CalculateMode;
		TAPISTR_10						CurrencyGroupNo;				
		TAPISTR_10						CurrencyNo;						
		TAPIREAL64						OpenCloseFee;
		TAPIREAL64						CloseTodayFee;
	};
	//! �ͻ��˻���֤����������ѯ�ṹ
	struct TapAPIAccountMarginRentQryReq
	{
		TAPISTR_20						AccountNo;
		TAPISTR_10						ExchangeNo;
		TAPICommodityType				CommodityType;
		TAPISTR_10						CommodityNo;
		//TAPISTR_10						ContractNo;//��ʱ�Ȳ����⿪�š�
	};

	//! �ͻ��˻���֤����������ѯӦ��
	struct  TapAPIAccountMarginRentQryRsp
	{
		TAPISTR_20						AccountNo;
		TAPISTR_10						ExchangeNo;
		TAPICommodityType				CommodityType;
		TAPISTR_10						CommodityNo;
		TAPISTR_10						ContractNo;
		TAPISTR_10						StrikePrice;
		TAPICallOrPutFlagType			CallOrPutFlag;
		TAPICalculateModeType			CalculateMode;
		TAPISTR_10						CurrencyGroupNo;
		TAPISTR_10						CurrencyNo;
		TAPIREAL64						InitialMargin;
		TAPIREAL64						MaintenanceMargin;
		TAPIREAL64						SellInitialMargin;
		TAPIREAL64						SellMaintenanceMargin;
		TAPIREAL64						LockMargin;
	};
	//! �۽���������ѯ��֪ͨ��
	struct TapAPIOrderQuoteMarketNotice
	{
		TAPISTR_10						ExchangeNo;				///< ���������
		TAPICommodityType				CommodityType;			///< Ʒ������
		TAPISTR_10						CommodityNo;			///< Ʒ�ֱ��
		TAPISTR_10						ContractNo;				///< ��Լ
		TAPISTR_10						StrikePrice;			///< ִ�м�
		TAPICallOrPutFlagType			CallOrPutFlag;			///< ���ǿ���
		TAPISideType					OrderSide;				///< ��������
		TAPIUINT32						OrderQty;				///< ί����
	};

	//! �������µ�����ṹ
	struct TapAPIOrderMarketInsertReq
	{
		TAPISTR_20				AccountNo;					///< �ͻ��ʽ��ʺ�
		TAPISTR_10				ExchangeNo;					///< ���������
		TAPICommodityType		CommodityType;				///< Ʒ������
		TAPISTR_10				CommodityNo;				///< Ʒ�ֱ�������
		TAPISTR_10				ContractNo;					///< ��Լ
		TAPISTR_10				StrikePrice;					///< ִ�м۸�
		TAPICallOrPutFlagType		CallOrPutFlag;				///< ���ſ���
		TAPIOrderTypeType		OrderType;					///< ί������
		TAPITimeInForceType		TimeInForce;					///< ��Ч����
		TAPIDATETIME			ExpireTime;						///< ��Ч��
		TAPIOrderSourceType		OrderSource;					///< ί����Դ	
		TAPIPositionEffectType	BuyPositionEffect;				///< ��ƽ��־
		TAPIPositionEffectType	SellPositionEffect;				///< ����ƽ��־

		TAPIYNFLAG				AddOneIsValid;					///< �Ƿ�T+1��Ч
		TAPIREAL64				OrderBuyPrice;				///< ��ί�м�
		TAPIREAL64				OrderSellPrice;				///< ��ί�м�	
		TAPIUINT32				OrderBuyQty;					///< ��ί����
		TAPIUINT32				OrderSellQty;					///< ��ί����
		TAPISTR_50				ClientBuyOrderNo;			///< ����ί�б��
		TAPISTR_50				ClientSellOrderNo;				///< ����ί�б��
		TAPIINT32				RefInt;						///< ���Ͳο�ֵ
		TAPIREAL64				RefDouble;					///< ����ο�ֵ
		TAPISTR_50				RefString;					///< �ַ����ο�ֵ
		TAPISTR_100				Remark;						///< ��ע
	};

	//! �ͻ�������Ӧ������ṹ
	struct TapAPIOrderMarketInsertRsp
	{
		TAPISTR_20							AccountNo;						///< �ͻ��ʽ��ʺ�

		TAPISTR_10							ExchangeNo;						///< ���������
		TAPICommodityType					CommodityType;					///< Ʒ������
		TAPISTR_10							CommodityNo;					///< Ʒ�ֱ�������
		TAPISTR_10							ContractNo;						///< ��Լ
		TAPISTR_10							StrikePrice;					///< ִ�м۸�
		TAPICallOrPutFlagType				CallOrPutFlag;					///< ���ſ���

		TAPIOrderTypeType					OrderType;						///< ί������
		TAPITimeInForceType					TimeInForce;					///< ί����Ч����
		TAPIDATETIME						ExpireTime;						///< ��Ч����(GTD�����ʹ��)
		TAPIOrderSourceType					OrderSource;					///< ί����Դ

		TAPIPositionEffectType				BuyPositionEffect;				///< ��ƽ��־
		TAPIPositionEffectType				SellPositionEffect;				///< ����ƽ��־

		TAPIREAL64							OrderBuyPrice;					///< ��ί�м�
		TAPIREAL64							OrderSellPrice;					///< ��ί�м�

		TAPIUINT32							OrderBuyQty;					///< ��ί����
		TAPIUINT32							OrderSellQty;					///< ��ί����

		TAPICHAR							ServerFlag;						///< ���׷����ʶ
		TAPISTR_20							OrderBuyNo;						///< ��ί�к�
		TAPISTR_20							OrderSellNo;					///< ��ί�к�

		TAPIYNFLAG							AddOneIsValid;					///< �Ƿ�T+1��Ч

		TAPISTR_20							OrderMarketUserNo;				///< �µ���
		TAPIDATETIME						OrderMarketTime;				///< �µ�ʱ��

		TAPIINT32							RefInt;							///< ���Ͳο�ֵ
		TAPIREAL64							RefDouble;						///< ����ο�ֵ
		TAPISTR_50							RefString;						///< �ַ����ο�ֵ

		TAPISTR_50							ClientBuyOrderNo;				///< �򱾵�ί�б��
		TAPISTR_50							ClientSellOrderNo;				///< ������ί�б��

		TAPIUINT32							ErrorCode;						///< ������Ϣ��
		TAPISTR_50							ErrorText;						///< ������Ϣ
			
		TAPISTR_40							ClientLocalIP;					///< �ն˱���IP��ַ���ͻ�����д��
		TAPIMACTYPE							ClientMac;						///< �ն˱���Mac��ַ���ͻ�����д��

		TAPISTR_40							ClientIP;						///< ǰ�ü�¼���ն�IP��ַ��ǰ����д��

		TAPISTR_100							Remark;							///< ��ע
	};
	//! �۽���������˫�߳�������
	struct TapAPIOrderMarketDeleteReq
	{
		TAPICHAR				ServerFlag;
		TAPISTR_20				OrderBuyNo;					///< ��ί�к�
		TAPISTR_20				OrderSellNo;					///< ��ί�к�
	};
	typedef TapAPIOrderMarketInsertRsp TapAPIOrderMarketDeleteRsp;


	//! ����ɾ������ṹ

	struct TapAPIOrderLocalRemoveReq
	{
		TAPICHAR				ServerFlag;
		TAPISTR_20				OrderNo;					
	};

	//! ����ɾ��Ӧ��ṹ
	struct TapAPIOrderLocalRemoveRsp
	{
		TapAPIOrderLocalRemoveReq req;
		TAPISTR_40							ClientLocalIP;					//�ն˱���IP��ַ���ͻ�����д��
		TAPIMACTYPE							ClientMac;						//�ն˱���Mac��ַ���ͻ�����д��

		TAPISTR_40							ClientIP;						//ǰ�ü�¼���ն�IP��ַ��ǰ����д��
	};

	//! ����¼������ṹ
	struct TapAPIOrderLocalInputReq
	{
		TAPISTR_20					AccountNo;						///< �ͻ��ʽ��ʺ�

		TAPISTR_10					ExchangeNo;						///< ���������
		TAPICommodityType			CommodityType;					///< Ʒ������
		TAPISTR_10					CommodityNo;					///< Ʒ�ֱ�������
		TAPISTR_10					ContractNo;						///< ��Լ1
		TAPISTR_10					StrikePrice;					///< ִ�м۸�1
		TAPICallOrPutFlagType		CallOrPutFlag;					///< ���ſ���1
		TAPISTR_10					ContractNo2;					///< ��Լ2
		TAPISTR_10					StrikePrice2;					///< ִ�м۸�2
		TAPICallOrPutFlagType		CallOrPutFlag2;					///< ���ſ���2

		TAPIOrderTypeType			OrderType;						///< ί������
		TAPIOrderSourceType			OrderSource;					///< ί����Դ
		TAPITimeInForceType			TimeInForce;					///< ί����Ч����
		TAPIDATETIME				ExpireTime;						///< ��Ч����(GTD�����ʹ��)

		TAPIYNFLAG					IsRiskOrder;					///< �Ƿ���ձ���
		TAPISideType				OrderSide;						///< ��������
		TAPIPositionEffectType		PositionEffect;					///< ��ƽ��־1
		TAPIPositionEffectType		PositionEffect2;				///< ��ƽ��־2
		TAPISTR_50					InquiryNo;						///< ѯ�ۺ�
		TAPIHedgeFlagType			HedgeFlag;						///< Ͷ����ֵ
		TAPIREAL64					OrderPrice;						///< ί�м۸�1
		TAPIREAL64					OrderPrice2;					///< ί�м۸�2��������Ӧ��ʹ��
		TAPIREAL64					StopPrice;						///< �����۸�
		TAPIUINT32					OrderQty;						///< ί������
		TAPIUINT32					OrderMinQty;					///< ��С�ɽ���
		TAPISTR_50					OrderSystemNo;					///< ϵͳ��
		TAPISTR_50					OrderExchangeSystemNo;			///< ������ϵͳ��

		TAPISTR_10					UpperNo;						///< ���ֺ�
		TAPIREAL64					OrderMatchPrice;				///< �ɽ���1
		TAPIREAL64					OrderMatchPrice2;				///< �ɽ���2
		TAPIUINT32					OrderMatchQty;					///< �ɽ���1
		TAPIUINT32					OrderMatchQty2;					///< �ɽ���2

		TAPIOrderStateType			OrderState;						///< ί��״̬

		TAPIYNFLAG					IsAddOne;						///< �Ƿ�ΪT+1��
	};
	typedef TapAPIOrderInfo TapAPIOrderLocalInputRsp;

	struct TapAPIOrderLocalModifyReq
	{
		TapAPIOrderLocalInputReq	req;
		TAPICHAR					ServerFlag;						///< ��������ʶ
		TAPISTR_20					OrderNo;						///< ί�б���
	};

	typedef TapAPIOrderInfo TapAPIOrderLocalModifyRsp;

	struct TapAPIOrderLocalTransferReq
	{
		TAPISTR_20					AccountNo;						///< �ͻ��ʽ��ʺ�
		TAPICHAR					ServerFlag;						///< ��������ʶ
		TAPISTR_20					OrderNo;						///< ί�б���
	};

	typedef TapAPIOrderInfo TapAPIOrderLocalTransferRsp;


	struct TapAPIFillLocalInputReq
	{
		TAPISTR_20					AccountNo;						///< �ͻ��ʽ��ʺ�

		TAPISTR_10					ExchangeNo;						///< ���������
		TAPICommodityType			CommodityType;					///< Ʒ������
		TAPISTR_10					CommodityNo;					///< Ʒ�ֱ�������
		TAPISTR_10					ContractNo;						///< ��Լ1
		TAPISTR_10					StrikePrice;					///< ִ�м۸�
		TAPICallOrPutFlagType		CallOrPutFlag;					///< ���ſ���

	
		TAPISideType				MatchSide;						///< ��������
		TAPIPositionEffectType      PositionEffect;					///< ��ƽ��־1
		TAPIHedgeFlagType			HedgeFlag;						///< Ͷ����ֵ
		TAPIREAL64					MatchPrice;						///< �ɽ���
		TAPIUINT32					MatchQty;						///< �ɽ���

		TAPISTR_50					OrderSystemNo;					///< ϵͳ��


		
		TAPISTR_70					UpperMatchNo;					///< ���ֳɽ���
		TAPIDATETIME				MatchDateTime;					///< �ɽ�ʱ��
		TAPIDATETIME				UpperMatchDateTime;				///< ���ֳɽ�ʱ��
		TAPISTR_10					UpperNo;						///< ���ֺ�

		TAPIYNFLAG					IsAddOne;						///< �Ƿ�ΪT+1��

		TAPISTR_10					FeeCurrencyGroup;				///< �ͻ������ѱ�����
		TAPISTR_10					FeeCurrency;					///< �ͻ������ѱ���
		TAPIREAL64					FeeValue;						///< ������
		TAPIYNFLAG					IsManualFee;					///< �˹��ͻ������ѱ��

		TAPIREAL64					ClosePositionPrice;				///< ָ���۸�ƽ��
	};
	typedef  TapAPIFillLocalInputReq TapAPIFillLocalInputRsp;


	//! �ͻ�¼���ɽ�ɾ������ṹ
	struct	TapAPIFillLocalRemoveReq
	{
		TAPICHAR					ServerFlag;						///< ��������ʶ
		TAPISTR_20					MatchNo;						///< ���سɽ���
	};
	//! �ͻ�¼���ɽ�ɾ��Ӧ��ṹ
	typedef TapAPIFillLocalRemoveReq	TapAPIFillLocalRemoveRsp;

	//! ϵͳ�����ղ�ѯӦ��
	struct TapAPITradingCalendarQryRsp
	{
		TAPIDATE CurrTradeDate;										///< ��ǰ������
		TAPIDATE LastSettlementDate;								///< �ϴν�����
		TAPIDATE PromptDate;										///< LME������
		TAPIDATE LastPromptDate;									///< ����LME������
	};

	//! �ͻ��ֻ���������ѯ����ṹ
	struct TapAPISpotLockQryReq
	{
		TAPISTR_20					AccountNo;						///<�ͻ��ʽ��ʺ�
	};
	//! �ͻ��ֻ���������ѯӦ��ṹ
	typedef TapAPISpotLockQryReq				TapAPISpotLockQryRsp;
	//! �ͻ��ֻ���������ѯ����Ӧ��ṹ
	struct TapAPISpotLockDataRsp
	{
		TAPISTR_20					AccountNo;						///<�ͻ��ʽ��˺�

		TAPISTR_10					ExchangeNo;						///< ���������
		TAPICommodityType			CommodityType;					///< Ʒ������
		TAPISTR_10					CommodityNo;					///< Ʒ�ֱ�������

		TAPIUINT32					LockQty;						///<������
		TAPIUINT32					FrozenQty;						///<������
		TAPIUINT32					CanUnLockQty;					///<�ɽ�����
	};
	//! �ͻ��ֻ����������֪ͨ
	typedef TapAPISpotLockDataRsp				TapAPISpotLockDataNotice;

	//=============================================================================
	/**
	*	\addtogroup G_DATATYPE_SETTLEFLAG	�ɼ���Ϣ�쳣��ʶ
	*/
	//=============================================================================
	typedef TAPICHAR						TapAPIAbnormalFalgType;
	//!����
	const	TapAPIAbnormalFalgType			TAPI_ABNORMAL_NORMAL				= '0';
	//!�ն���Ϣ�ɼ�Ϊ��
	const	TapAPIAbnormalFalgType			TAPI_ABNORMAL_GATHERINFO_NONE		= '1';
	//!�ն˲ɼ����ݼ�����Կ�汾�쳣
	const	TapAPIAbnormalFalgType			TAPI_ABNORMAL_AUTHKEYVERSION		= '2';
	//!�ն���Ϣ�����쳣
	const	TapAPIAbnormalFalgType			TAPI_ABNORMAL_GATHERINFO_WRONG		= '3';


	//! ����ʽ��ܲɼ���Ϣ_�м̷�ʽ�ϱ�
	struct TapAPISubmitUserLoginInfo
	{
		TAPISTR_20					UserNo;								///< �û���
		TAPISTR_500					GatherInfo;							///< �û��ն˲ɼ���Ϣ
		TAPISTR_40					ClientLoginIP;						///< �û�����IP
		TAPIUINT32					ClientLoginPort;					///< �û�����Port
		TAPIDATETIME				ClientLoginDateTime;				///< �û���¼ʱ��
		TAPISTR_30					ClientAppID;						///< �û�AppID
		TAPIUINT32					AuthKeyVersion;						///< �û��ն���Ϣ������Կ�汾��
		TapAPIAbnormalFalgType		AbnormalNo;							///< �û��ɼ���Ϣ�쳣��ʶ
	};

	//! ����ʽ��ܲɼ���ϢӦ��
	struct TapAPISubmitUserLoginRspInfo
	{
		TAPISTR_20					UserNo;								///< �û���
	};




	//=============================================================================
	/**
	*	\addtogroup G_DATATYPE_SETTLEFLAG	��ϲ��Դ���
	*/
	//=============================================================================
	typedef TAPICHAR					TapAPICombineStrategyType[10];
	//!�Ϲ�ţ�м۲����
	const TapAPICombineStrategyType		TAPI_STRATEGY_C_BULL						= "CNSJC";
	//!�Ϲ����м۲����
	const TapAPICombineStrategyType		TAPI_STRATEGY_P_BEAR						= "PXSJC";
	//!�Ϲ�ţ�м۲����
	const TapAPICombineStrategyType		TAPI_STRATEGY_P_BULL						= "PNSJC";
	//!�Ϲ����м۲����
	const TapAPICombineStrategyType		TAPI_STRATEGY_C_BEAR						= "CXSJC";
	//!��ʽ��ͷ����
	const TapAPICombineStrategyType		TAPI_STRATEGY_S_STRADDLE					= "KS";
	//!���ʽ��ͷ����
	const TapAPICombineStrategyType		TAPI_STRATEGY_S_STRANGLE					= "KKS";
	//!��ͨ��ת���Ҳ�
	const TapAPICombineStrategyType		TAPI_STRATEGY_ZBD							= "ZBD";
	//!���Ҳ�ת��ͨ��
	const TapAPICombineStrategyType		TAPI_STRATEGY_ZXJ							= "ZXJ";

	//=============================================================================
	/**
	*����ҵ��ί������
	*/
	//=============================================================================
	typedef TAPICHAR					TapAPISpecialOrderTypeType;
	//!��ϲ�������
	const TapAPISpecialOrderTypeType		TAPI_STRATEGY_COMBINE					= '1';
	//!��ϲ��Բ��
	const TapAPISpecialOrderTypeType		TAPI_STRATEGY_SPLIT						= '2';
	//!֤ȯ����
	const TapAPISpecialOrderTypeType		TAPI_SPOT_LOCK							= '3';
	//!֤ȯ����
	const TapAPISpecialOrderTypeType		TAPI_SPOT_UNLOCK						= '4';
	//!��Ȩ��Ȩ
	const TapAPISpecialOrderTypeType		TAPI_OPTION_EXERCISE					= '5';
	//!��Ȩ�����Ȩ
	const TapAPISpecialOrderTypeType		TAPI_OPTION_EXERCISE_COMBINE			= '6';


	//!�ͻ�����ҵ��ί������ṹ
	struct TapAPISpecialOrderInsertReq
	{
		TAPISTR_20						AccountNo;						///< �ͻ��ʽ��ʺ�
		TapAPISpecialOrderTypeType		SpecialOrderType;				///< ����ҵ������
		TAPIOrderSourceType				OrderSource;					///< ί����Դ
		TAPISTR_50						CombineNo;						///< ��ϱ���
		TAPIUINT32						OrderQty;						///< ί������
		TAPISTR_10						ExchangeNo;						///< ���������
		TAPICommodityType				CommodityType;					///< Ʒ������
		TAPISTR_10						CommodityNo;					///< Ʒ�ֱ���
		TAPISTR_10						ContractNo;						///< ��Լ1
		TAPISTR_10						StrikePrice;					///< ִ�м۸�1
		TAPICallOrPutFlagType			CallOrPutFlag;					///< ���ſ���1
		TAPISideType					OrderSide1;						///< ��������1
		TAPIHedgeFlagType				HedgeFlag1;						///< Ͷ������1
		TAPISTR_10						ContractNo2;					///< ��Լ2
		TAPISTR_10						StrikePrice2;					///< ִ�м۸�2
		TAPICallOrPutFlagType			CallOrPutFlag2;					///< ���ſ���2
	};

	//!��ѯ����ҵ��ί������ṹ
	struct TapAPISpecialOrderQryReq
	{
		TAPISTR_20						AccountNo;						///< �ͻ��ʽ��ʺ�	
		TAPISTR_20						OrderNo;						///< ����ҵ��ί�б��
	};
	//!�ͻ�����ҵ��ί����Ϣ
	struct TapAPISpecialOrderInfo
	{
		TAPIUINT32						SessionID;						///< �ỰID
		TAPIUINT32						ErrorCode; 						///< ������
		TAPISTR_50						ErrorText;						///< ������Ϣ
		TAPISTR_20						AccountNo;						///< �ͻ��ʽ��ʺ�
		TAPICHAR						ServerFlag;						///< ��������ʶ
		TAPISTR_20						OrderNo;						///< ����ҵ��ί�б��
		TAPISTR_50						ClientOrderNo;					///< �ͻ��˱���ί�б��
		TapAPISpecialOrderTypeType		SpecialOrderType;				///< ����ҵ������
		TAPIOrderSourceType				OrderSource;					///< ί����Դ��Ĭ�ϳ��򵥡�
		TapAPICombineStrategyType       CombineStrategy;				///< ��ϲ��Դ���
		TAPISTR_50						CombineNo;						///< ��ϱ���
		TAPIUINT32						OrderQty;						///< ί������
		TAPISTR_10						ExchangeNo;						///< ���������
		TAPICommodityType				CommodityType;					///< Ʒ������
		TAPISTR_10						CommodityNo;					///< Ʒ�ֱ���
		TAPISTR_10						ContractNo;						///< ��Լ1
		TAPISTR_10						StrikePrice;					///< ִ�м۸�1
		TAPICallOrPutFlagType			CallOrPutFlag;					///< ���ſ���1
		TAPISideType					OrderSide1;						///< ��������1
		TAPIUINT32						CombineQty1;					///< �������1
		TAPIHedgeFlagType				HedgeFlag1;						///< Ͷ������1
		TAPISTR_10						ContractNo2;					///< ��Լ2
		TAPISTR_10						StrikePrice2;					///< ִ�м۸�2
		TAPICallOrPutFlagType			CallOrPutFlag2;					///< ���ſ���2
		TAPISideType					OrderSide2;						///< ��������2
		TAPIUINT32						CombineQty2;					///< �������2
		TAPIHedgeFlagType				HedgeFlag2;						///< Ͷ������2

		TAPISTR_50		 				LicenseNo;						///< �����Ȩ��
		TAPISTR_40						ClientLocalIP;					///< �ն˱���IP
		TAPIMACTYPE						ClientMac;						///< �ն˱���Mac��ַ
		TAPISTR_40						ClientIP;						///< �ն������ַ.
		TAPIUINT32						OrderStreamID;					///< ί����ˮ��
		TAPISTR_10						UpperNo;						///< ���ֺ�
		TAPISTR_10						UpperChannelNo;					///< ����ͨ����
		TAPISTR_20						OrderLocalNo;					///< ���ر��غ�
		TAPISTR_50						OrderSystemNo;					///< ϵͳ��
		TAPISTR_50						OrderExchangeSystemNo;			///< ������ϵͳ��
		TAPISTR_20						OrderInsertUserNo;				///< �µ���
		TAPIDATETIME					OrderInsertTime;				///< �µ�ʱ��
		TAPIOrderStateType				OrderState;						///< ί��״̬
		
	};

	//!�ͻ���ϳֲֲ�ѯ����ṹ
	struct TapAPICombinePositionQryReq
	{
		TAPISTR_20						AccountNo;						///< �ͻ��ʽ��ʺ�
	};
	//!�ͻ���ϳֲ���Ϣ
	struct TapAPICombinePositionInfo
	{
		TAPISTR_20						AccountNo;						///< �ͻ��ʽ��ʺ�

		TAPIUINT32						PositionStreamID;				///< ��ϳֲ�����
		TAPICHAR						ServerFlag;						///< ��������ʶ
		TAPISTR_10						UpperNo;						///< ���ֺ�
		TapAPICombineStrategyType       CombineStrategy;				///< ��ϲ��Դ���
		TAPISTR_50						CombineNo;						///< ��ϱ���
		TAPIUINT32						PositionQty;					///< ί������

		TAPISTR_10						ExchangeNo;						///< ���������
		TAPICommodityType				CommodityType;					///< Ʒ������
		TAPISTR_10						CommodityNo;					///< Ʒ�ֱ���

		TAPISTR_10						ContractNo;						///< ��Լ1
		TAPISTR_10						StrikePrice;					///< ִ�м۸�1
		TAPICallOrPutFlagType			CallOrPutFlag;					///< ���ſ���1
		TAPISideType					OrderSide1;						///< ��������1
		TAPIUINT32						CombineQty1;					///< �������1
		TAPIHedgeFlagType				HedgeFlag1;						///< Ͷ������1
		TAPISTR_10						ContractNo2;					///< ��Լ2
		TAPISTR_10						StrikePrice2;					///< ִ�м۸�2
		TAPICallOrPutFlagType			CallOrPutFlag2;					///< ���ſ���2
		TAPISideType					OrderSide2;						///< ��������2
		TAPIUINT32						CombineQty2;					///< �������2
		TAPIHedgeFlagType				HedgeFlag2;						///< Ͷ������2

		TAPISTR_10						CommodityCurrencyGroup;			///< Ʒ�ֱ�����
		TAPISTR_10						CommodityCurrency;				///< Ʒ�ֱ���

		TAPIREAL64						AccountInitialMargin;			///< ��ʼ��ϱ�֤��
		TAPIREAL64						AccountMaintenanceMargin;		///< ά����ϱ�֤��
		TAPIREAL64						UpperInitialMargin;				///< ���ֳ�ʼ��ϱ�֤��
		TAPIREAL64						UpperMaintenanceMargin;			///< ����ά����ϱ�֤��
	};

#pragma pack(pop)
}
#endif //TAP_TRADE_API_DATA_TYPE_H