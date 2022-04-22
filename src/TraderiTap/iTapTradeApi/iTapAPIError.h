#ifndef ITAP_API_ERROR_H
#define ITAP_API_ERROR_H

//=============================================================================
/**
 *	\addtogroup G_ERR_INNER_API		ITapAPI�ڲ����صĴ����붨�塣
 *	@{
 */
//=============================================================================
//! �ɹ�
namespace ITapTrade
{
	//=============================================================================
	/**
	*	\addtogroup G_ERR_LOGIN		��¼���̴���
	*	@{
	*/
	//=============================================================================
    //! ��¼����ִ�д���
	const int			TAPIERROR_LOGIN										= 10001;
    //! ��¼�û�������
	const int			TAPIERROR_LOGIN_USER								= 10002;
    //! ��Ҫ���ж�̬��֤
	const int			TAPIERROR_LOGIN_DDA									= 10003;
    //! ��¼�û�δ��Ȩ
	const int			TAPIERROR_LOGIN_LICENSE 							= 10004;
    //! ��¼ģ�鲻��ȷ
	const int			TAPIERROR_LOGIN_MODULE								= 10005;
    //! ��Ҫǿ���޸�����
	const int			TAPIERROR_LOGIN_FORCE 								= 10006;
    //! ��¼״̬��ֹ��¼
	const int			TAPIERROR_LOGIN_STATE 								= 10007;
    //! ��¼���벻��ȷ
	const int			TAPIERROR_LOGIN_PASS 								= 10008;
    //! û�и�ģ���¼Ȩ��
	const int			TAPIERROR_LOGIN_RIGHT 								= 10009;
    //! ��¼��������
	const int			TAPIERROR_LOGIN_COUNT 								= 10010;
    //! ��¼�û����ڷ�������ʶ�¿ɵ�¼�û��б���
	const int			TAPIERROR_LOGIN_NOTIN_SERVERFLAGUSRES 				= 10011;
	//! ��¼�û��ѱ�����
	const int			TAPIERROR_LOGIN_FREEZE 								= 10012;
	//! ��������û�����
	const int			TAPIERROR_LOGIN_TOFREEZE 							= 10013;
	//! �ͻ�״̬�������¼
	const int			TAPIERROR_LOGIN_ACCOUNTSTATE 						= 10014;
	//! ��Ҫ���ж�����֤
	const int			TAPIERROR_LOGIN_SECCERTIFI 							= 10015;

	//δ�󶨶�����֤��Ϣ
	const int			TAPIERROR_LOGIN_NOSECONDSET 						= 10016;
	//�������εļ������¼
	const int			TAPIERROR_LOGIN_NOTURSTHOST 						= 10017;
	//�Ǳ��������Ŀͻ�
	const int			TAPIERROR_LOGIN_NOTINTRADECENTER 					= 10019;
	//�汾�ͺ�̨�汾����һ��
	const int			TAPIERROR_LOGIN_INCONSISTENT 						= 10020;
	//�ͻ�������������ǰ�õ�ַδ����
	const int			TAPIERROR_LOGIN_NOCENTERFRONTADDRESS 				= 10021;
	//��������˺����͵�¼
	const int			TAPIERROR_LOGIN_PROHIBITACCOUNTTYPE 				= 10022;

	//!��Ҫ��Ϣ�ɼ�-ֱ��
	const int			TAPIERROR_LOGIN_GATHERINFO_DIRECT 					= 10023;
	//!��Ҫ��Ϣ�ɼ�-�м�
	const int			TAPIERROR_LOGIN_GATHERINFO_RELAY 					= 10024;

	//!	������֤ʧ��
	const int          TAPIERROR_SECONDCERTIFICATION_FAIL 					= 14001;
	//!	������֤��ʱ
	const int          TAPIERROR_SECONDCERTIFICATION_TIMEOVER 				= 14002;
	//!������֤����������ޣ����µ�¼
	const int		 TAPIERROR_SECONDCERTIFICATION_RELOGIN 					= 14003;
	//!������֤����������ޣ��û�����
	const int		 TAPIERROR_SECONDCERTIFICATION_FREEZE 					= 14004;
	//=============================================================================

    //! ���ݿ�����ʧ��
	const int			TAPIERROR_CONN_DATABASE 							= 11000;
    //! ���ݿ����ʧ��
	const int			TAPIERROR_OPER_DATABASE 							= 11001;
    //! ������һ�Զ�
	const int			TAPIERROR_NEED_ONETOONE 							= 11002;
    //! ɾ��ʧ��-���ڹ�����Ϣ��
	const int			TAPIERROR_EXIST_RELATEINFO 							= 11003;
	//! ɾ������ʧ��-�������������ڲ���Ա������
	const int			TAPIERROR_EXIST_RELATEINFOOFGROUP 					= 11004;

	//���״̬�������޸�
	const int		 TAPIERROR_CHECK_FAILED 								= 11006;
	//����������ظ�3.0�ⲿƷ�ֱ��
	const int		 TAPIERROR_EXIST_OUTSIDECOMMODITYNO 					= 11007;
	//�ͻ����㵥������
	const int		 TAPIERROR_NOTEXIST_BILL 								= 11008;
	//���������Ӵ������˺�
	const int		 TAPIERROR_LOGIN_PROHIBITADDACCOUNTTYPE 				= 11009;
	//�˺����Ͳ�����Ϊ��
	const int		 TAPIERROR_ACCOUNTINFO_NOTEXPTY 						= 11010;
	//���˺Ų�����Ϊ��
	const int		 TAPIERROR_ACCOUNTINFO_SuperiorNOTEMPTY 				= 11011;

    //! ��¼�û������޸�ʧ��-ԭʼ�������
	const int			TAPIERROR_USERPASSWORD_MOD_SOURCE 					= 12001;
    //! ��¼�û������޸�ʧ��-������ǰn��������ͬ
	const int			TAPIERROR_USERPASSWORD_MOD_SAME 					= 12002;
    //! ��¼�û������޸�ʧ��-�����벻�������븴�Ӷ�Ҫ��
	const int          TAPIERROR_USERPASSWORD_MOD_COMPLEXITY 				= 12003;

    //! һ��������ֻ������һ������
	const int			TAPIERROR_CURRENCY_ONLY_ONEBASE 					= 13001;
    //! ����ֻ������Ԫ��۱�
	const int			TAPIERROR_CURRENCY_ONLY_USDHKD 						= 13002;

	//! �û��ɼ���Ϣ��Կδ�ҵ�
	const int			TAPIERROR_GATHERINFO_NO_AUTHKEY						= 17001;
	//! APPID��֤ʧ��
	const int			TAPIERROR_GATHERINFO_AUTH_FAILED					= 17002;
	//=============================================================================
	/**
	*	\addtogroup G_ERR_TRADE_SERVICE		���׷��������붨��
	*	@{
	*/
	//=============================================================================

    //! �ʽ��˺Ų�����
	const int			TAPIERROR_ORDERINSERT_ACCOUNT 						= 60001;
    //! �ʽ��˺�״̬����ȷ
	const int			TAPIERROR_ORDERINSERT_ACCOUNT_STATE 				= 60002;
	//! �ʽ��˺Ž������Ĳ�һ��
	const int			TAPIERROR_ORDERINSERT_TRADECENT_ERROR 				= 60003;
	//���˺Ų������µ�
	const int		 TAPIERROR_ORDERINT_MAINACCOUNT_ERROR 					= 60004;
	//���˺���Ϣ����
	const int		 TAPIERROR_ORDERINT_MAINACCINFO_ERROR 					= 60005;
	//�˺Ž�ֹ��Ȩ�м��µ�
	const int		 TAPIERROR_ORDERINT_NO_OPTMARKET_ERROR 					= 60006;
	//Ŀǰ��֧�ֵ�ָ��
	const int		 TAPIERROR_ORDERINT_UN_SUPPORT_ERROR 					= 60007;
    //! �µ���Ч�ĺ�Լ
	const int			TAPIERROR_ORDERINSERT_CONTRACT 						= 60011;
    //! LMEδ׼������
	const int			TAPIERROR_ORDERINSERT_LME_NOTREADY 					= 60012;
	//!��֧�ֵ��µ�����
	const int			TAPIERROR_ORDERINSERT_ERROR_ORDER_TYPE 				= 60013;
	//!�����������
	const int			TAPIERROR_ORDERINSERT_READY_TYPE_ERROR 				= 60014;
	//!���Ϸ���ί������
	const int			TAPIERROR_ORDERINSERT_ORDER_TYPE_ERROR 				= 60015;
	//�µ���Լ��ĺ�Լ������
	const int		 TAPIERROR_ORDERINSERT_SUBCONTRACT 						= 60016;
	//�ֻ��µ�������������
	const int		 TAPIERROR_ORDERINSERT_SPOTBUYQTY 						= 60017;
	//���Ϲ���Ȩ��������
	const int		 TAPIERROR_ORDERINSERT_RESERVE_PUT 						= 60018;
	//���뿪�ֲ�������
	const int		 TAPIERROR_ORDERINSERT_RESERVE_B_OPEN 					= 60019;
	//����ƽ�ֲ�������
	const int		 TAPIERROR_ORDERINSERT_RESERVE_S_COVER 					= 60020;
    //! �ͻ�Ȩ�޽�ֹ����
	const int			TAPIERROR_ORDER_NOTRADE_ACCOUNT 					= 60021;
    //! �ͻ�Ʒ�ַ����ֹ����
	const int			TAPIERROR_ORDER_NOTRADE_COM_GROUP 					= 60022;
    //! �ͻ���Լ�����ֹ����
	const int			TAPIERROR_ORDER_NOTRADE_ACC_CONTRACT 				= 60023;
    //! ϵͳȨ�޽�ֹ����
	const int			TAPIERROR_ORDER_NOTRADE_SYSTEM 						= 60024;
    //! �ͻ�Ȩ��ֻ��ƽ��
	const int			TAPIERROR_ORDER_CLOSE_ACCOUNT 						= 60025;
    //! �ͻ���Լ����ֻ��ƽ��
	const int			TAPIERROR_ORDER_CLOSE_ACC_CONTRACT 					= 60026;
    //! ϵͳȨ��ֻ��ƽ��
	const int			TAPIERROR_ORDER_CLOSE_SYSTEM 						= 60027;
	//! ֻ��ƽ����ǰ��������ֻ��ƽ��
	const int			TAPIERROR_ORDER_CLOSE_DAYS 							= 60028;
	//! �ͻ�Ʒ�ַ��Ȩ�޽�ֹ����
	const int			TAPIERROR_ORDER_NOTRADE_RISK 						= 60029;
	//! �ͻ�Ʒ�ַ��Ȩ��ֻ��ƽ��
	const int			TAPIERROR_ORDER_CLOSE_RISK 							= 60030;

    //! �ֲ��������������
	const int			TAPIERROR_ORDERINSERT_POSITIONMAX 					= 60031;
    //! �µ��������������
	const int			TAPIERROR_ORDERINSERT_ONCEMAX 						= 60032;
    //! �µ���Լ�޽���·��
	const int			TAPIERROR_ORDERINSERT_TRADEROUTE 					= 60033;
	//! ί�м۸񳬳�ƫ�뷶Χ
	const int			TAPIERROR_ORDER_IN_MOD_PRICE_ERROR 					= 60034;
	//! ����GiveUp���ֲ���
	const int			TAPIERROR_ORDER_IN_GIVEUP_POS_MAX 					= 60035;
	//�ֲ�������ETF�ֲܳ�����
	const int			TAIERROR_ORDER_ETF_POSITIONMAX 						= 60036;
	//�ֲ�������ETFȨ��������
	const int			TAIERROR_ORDER_ETF_BUYPOSITIONMAX 					= 60037;
	//�ֲ�������ETF�������뿪������
	const int			TAIERROR_ORDER_ETF_BUYONEDAYPOSITIONMAX 			= 60038;
	//���������ֻ�����
	const int			TAIERROR_ORDERINSERT_NOTENOUGHSPOT 					= 60039;
	//�ֻ���֧�ֱ���
	const int			TAIERROR_ORDERINSERT_RESERVE_SPOT 					= 60040;


    //! δ��¼����
	const int          TAPIERROR_UPPERCHANNEL_NOT_LOGIN 					= 60041;
	//! δ�ҵ�������Ϣ
	const int          TAPIERROR_UPPERCHANNEL_NOT_FOUND 					= 60042;
	//��Ʒ�ֲ�֧�ֽ���������
	const int          TAPIERROR_COMMODITY_LOCK 							= 60043;
	//�ֻ�δ���ö�Ӧ��ȨƷ��
	const int          TAPIERROR_SPOT_ROOT_COMMODITY 						= 60044;
	//�ֻ���Ӧ��Ȩ�޽���·��
	const int			TAIERROR_SPOT_ROOTCOM_TRADEROUTE 					= 60045;

    //! �µ��ʽ���
	const int			TAPIERROR_ORDERINSERT_NOTENOUGHFUND 				= 60051;
    //! �����Ѳ�������
	const int			TAPIERROR_ORDERINSERT_FEE 							= 60052;
    //! ��֤���������
	const int			TAPIERROR_ORDERINSERT_MARGIN 						= 60053;
    //! �ܻ����ʽ���
	const int			TAPIERROR_ORDERINSERT_BASENOFUND 					= 60054;

	//! ������֤����
	const int			TAPIERROR_ORDERINSERT_MARGINAMOUNT 					= 60055;
	//! �ܻ��ҳ������ֱ�������
	const int			TAPIERROR_ORDERINSERT_OPENRATIO 					= 60056;
	//! ���������鳬�����ֱ�������
	const int			TAPIERROR_ORDERINSERT_GROUP_OPENRATIO 				= 60057;
	//! �������в�������
	const int			TAPIERROR_ORDERINSERT_RISKARRAY 					= 60058;
	//�ܻ��ҳ����޹��������
	const int			TAIERROR_ORDERINSERT_BUYLIMITE 						= 60059;
	//���������鳬���޹��������
	const int			TAIERROR_ORDERINSERT_GROUP_BUYLIMITE 				= 60060;
    //! �����޴�ϵͳ��
	const int          TAPIERROR_ORDERDELETE_NOT_SYSNO 						= 60061;
    //! ��״̬��������
	const int          TAPIERROR_ORDERDELETE_NOT_STATE 						= 60062;
	//! ¼����������
	const int          TAPIERROR_ORDERDELETE_NO_INPUT 						= 60063;
	//������������/����ָ��
	const int          TAPIERROR_ORDERDELETE_NO_TRADE 						= 60064;

    //! ��״̬������ĵ�
	const int			TAPIERROR_ORDERMODIFY_NOT_STATE 					= 60071;
    //! �˹���������ĵ�
	const int			TAPIERROR_ORDERMODIFY_BACK_INPUT 					= 60072;
	//! ���ձ���������ĵ�
	const int			TAPIERROR_ORDERMODIFY_RISK_ORDER 					= 60073;
	//! �ɽ������ڸĵ���
	const int			TAPIERROR_ORDERMODIFY_ERROR_QTY 					= 60074;
	//! Ԥ�񵥲�����ĵ�
	const int			TAPIERROR_ORDERMODIFY_ERROR_READY 					= 60075;

    //! ��ɾ����������ת��
	const int			TAPIERROR_ORDERINPUT_CANNOTMOVE 					= 60081;

    //! ¼���ظ�
	const int			TAPIERROR_ORDERINPUT_REPEAT 						= 60091;

	//! ��Լ����۸��޸�ʧ��
	const int			TAPIERROR_CONTRACT_QUOTE 							= 60101;

	//! �µ��������ֵ��������
	const int			TAPIERROR_UPPER_ONCEMAX 							= 60111;
	//! �µ������������ֲ���
	const int			TAPIERROR_UPPER_POSITIONMAX 						= 60112;

	//! ��ƽ��ʽ����
	const int			TAPIERROR_ORDERINSERT_CLOSEMODE 					= 60121;
	//! ί��ƽ�ֲֲֳ���
	const int			TAPIERROR_CLOSE_ORDER 								= 60122;
	//! �ɽ�ƽ��ʧ��
	const int			TAPIERROR_CLOSE_MATCH 								= 60123;

	//! δ�ҵ�����ί��
	const int			TAPIERROR_MOD_DEL_NO_ORDER 							= 60131;
	//! �����ضϿ�����
	const int			TAPIERROR_MOD_DEL_GATEWAY_DISCON 					= 60132;

	//! ¼���ɽ��ظ�
	const int			TAPIERROR_MATCHINPUT_REPEAT 						= 60141;
	//! ¼���ɽ�δ�ҵ���Ӧί��
	const int			TAPIERROR_MATCHINPUT_NO_ORDER 						= 60142;
	//! ¼���ɽ���Լ������
	const int			TAPIERROR_MATCHINPUT_NO_CONTRACT 					= 60143;
	//! ¼���ɽ���������
	const int			TAPIERROR_MATCHINPUT_PARM_ERROR 					= 60144;
	//! ¼���ɽ�ί��״̬����
	const int			TAPIERROR_MATCHINPUT_OSTATE_ERROR 					= 60145;

	//! �ɽ�ɾ��δ�ҵ��ɽ�
	const int			TAPIERROR_MATCHREMOVE_NO_MATCH 						= 60151;
	//! ��״̬�ɽ�����ɾ
	const int			TAPIERROR_MATCHREMOVE_STATE_ERROR 					= 60152;

	//! ������¼���״̬����
	const int			TAPIERROR_ORDERINPUT_STATE_ERROR 					= 60161;
	//! ������޸Ķ�������
	const int			TAPIERROR_ORDERINPUT_MOD_ERROR 						= 60162;
	//! ��������ɾ�����ڶ�Ӧ�ɽ�
	const int			TAPIERROR_ORDERREMOVE_ERROR 						= 60163;
	//! ���Ϸ���ί��״̬
	const int			TAPIERROR_ORDERINPUT_MOD_STATE_ERROR 				= 60164;
	//! ��״̬��������ת��
	const int			TAPIERROR_ORDEREXCHANGE_STATE_ERROR 				= 60165;
	//! ����������ɾ��
	const int			TAPIERROR_ORDERREMOVE_NOT_ERROR 					= 60166;

	//! ������˫�߳���δ�ҵ�ί��
	const int			TAPIERROR_ORDERMARKET_DELETE_NOTFOUND 				= 60171;
	//! ������˫�߳����ͻ���һ��
	const int			TAPIERROR_ORDERMARKET_DEL_ACCOUNT_NE 				= 60172;
	//! ������˫�߳���Ʒ�ֲ�һ��
	const int			TAPIERROR_ORDERMARKET_DEL_COMMODITY_NE 				= 60173;
	//! ������˫�߳�����Լ��һ��
	const int			TAPIERROR_ORDERMARKET_DEL_CONTRACT_NE 				= 60174;
	//! ������˫�߳�������������ͬ
	const int			TAPIERROR_ORDERMARKET_DEL_SIDE_EQ 					= 60175;
	//! ������˫�߳��������������
	const int			TAPIERROR_ORDERMARKET_DEL_SIDE_ERROR 				= 60176;
	//! �����̵��߼��δͨ��
	const int			TAPIERROR_ORDERMARKET_OTHER_SIDE_ERROR 				= 60177;

	//! �񵥼���ʧ�ܣ�����δ�ҵ�
	const int			TAPIERROR_ORDERACTIVATE_NOTFOUND_ERROR 				= 60181;
	//! �񵥼���ʧ�ܣ�����Ч״̬
	const int			TAPIERROR_ORDERACTIVATE_STATE_ERROR 				= 60182;
	
	//����Ա�޿������µ�Ȩ��
	const int			TAPIERROR_TRANSIT_ORDERINSERT_RIGHT 				= 60191;
	//δ������ת����
	const int			TAPIERROR_TRANSIT_ORDERINSERT_DISCON 				= 60192;
	//�µ�δ����Ŀ�꽻������
	const int			TAPIERROR_TRANSIT_ORDERINSERT_DISCON_DEST 			= 60193;
	//����δ����Ŀ�꽻������
	const int			TAPIERROR_TRANSIT_ORDERDELETE_DISCON_DEST 			= 60194;
	//�ĵ�δ����Ŀ�꽻������
	const int			TAPIERROR_TRANSIT_ORDERMODIFY_DISCON_DEST 			= 60195;
	//�������ת��������
	const int			TAPIERROR_TRANSIT_ORDER_OPERATOR 					= 60196;

	//�ͻ�Ȩ�޽�ֹ����
	const int			TAPIERROR_ORDER_DISALLOWBUY_ACCOUNT 				= 60201;
	//�ͻ�Ȩ�޽�ֹ����
	const int			TAPIERROR_ORDER_DISALLOWSELL_ACCOUNT 				= 60202;
	//ϵͳȨ�޽�ֹ����
	const int			TAPIERROR_ORDER_DISALLOWBUY_SYSTEM 					= 60203;
	//ϵͳȨ�޽�ֹ����
	const int			TAPIERROR_ORDER_DISALLOWSELL_SYSTEM 				= 60204;
	//�ͻ�Ȩ�޽�ֹ��������Ȩ -������ϵͳ
	const int			TAPIERROR_ORDER_DIS_SELLOPTION_ACCOUNT				= 60205;
	//ϵͳȨ�޽�ֹ��������Ȩ -������ϵͳ
	const int			TAPIERROR_ORDER_DIS_SELLOPTION_SYSTEM				= 60206;

	//���������޶�
	const int			TAPIERROR_ORDERINSERT_LOANAMOUNT					= 60211;
	//��Ʒ�ֲ�֧����ϲ���
	const int			TAPIERROR_COMBINE_COMMODITY							= 60220;
	//����걨��Լ��Ȩ���Ͳ�����Ҫ��
	const int			TAPIERROR_COMBINE_CALLORPUT							= 60221;
	//��ֵ���ϳֲֲ�����
	const int			TAPIERROR_COMBINE_COMPOSITION						= 60222;
	//��ֵ���ϳֲ���������
	const int			TAPIERROR_COMBINE_COMPOSITION_QTY					= 60223;
	//����걨��Լ���ұ�ʶ������Ҫ��
	const int			TAPIERROR_COMBINE_HEDGEFLAG							= 60224;
	//����걨��Լ�������򲻷���Ҫ��
	const int			TAPIERROR_COMBINE_ORDERSIDE							= 60225;
	//����걨��Լ��С������Ҫ��
	const int			TAPIERROR_COMBINE_CONTRACTSIZE						= 60226;
	//����걨��Լ�����ղ�����Ҫ��
	const int			TAPIERROR_COMBINE_CONTRACTDAYS						= 60227;
	//����걨��Լ��Ȩ�۲�����Ҫ��
	const int			TAPIERROR_COMBINE_STRIKEPRICE						= 60228;
	//��ͬ��Լ���������
	const int			TAPIERROR_COMBINE_CONTRACT_SAME						= 60229;

	//=============================================================================
	/**
	*	\addtogroup G_ERR_GATE_WAY		���ش�����붨��
	*	@{
	*/
	//=============================================================================

    //! ����δ������δ��������
	const int			TAPIERROR_GW_NOT_READY 								= 80001;
    //! Ʒ�ִ���
	const int			TAPIERROR_GW_INVALID_COMMODITY 						= 80002;
    //! ��Լ����
	const int			TAPIERROR_GW_INVALID_CONTRACT 						= 80003;
    //! �����ֶ�����
	const int			TAPIERROR_GW_INVALID_FIELD 							= 80004;
    //! �۸񲻺Ϸ�
	const int		    TAPIERROR_GW_INVALID_PRICE 							= 80005;
    //! �������Ϸ�
	const int			TAPIERROR_GW_INVALID_VOLUME 						= 80006;
    //! �������Ͳ��Ϸ�
	const int			TAPIERROR_GW_INVALID_TYPE 							= 80007;
    //! ί��ģʽ���Ϸ�
	const int			TAPIERROR_GW_INVALID_MODE 							= 80008;
    //! ί�в����ڣ��ĵ���������
	const int			TAPIERROR_GW_ORDER_NOT_EXIST 						= 80009;
    //! ���ͱ���ʧ��
	const int			TAPIERROR_GW_SEND_FAIL 								= 80010;
    //! �����־ܾ�
	const int			TAPIERROR_GW_REJ_BYUPPER 							= 80011;

	//=============================================================================
	/**
	*	\addtogroup G_ERR_FRONT_SERVICE		ǰ�÷��ش���
	*	@{
	*/
	//=============================================================================

    //! ǰ�ò������ģ���¼
	const int			TAPIERROR_TRADEFRONT_MODULETYPEERR 					= 90001;
    //! һ������̫������
	const int			TAPIERROR_TRADEFRONT_TOOMANYDATA 					= 90002;
    //! ǰ��û����Ҫ����
	const int			TAPIERROR_TRADEFRONT_NODATA 						= 90003;
	//! ����ѯ�Ĳ���Ա��Ϣ������
	const int			TAPIERROT_TRADEFRONT_NOUSER 						= 90004;

    //! ǰ���뽻�׶Ͽ�
	const int			TAPIERROR_TRADEFRONT_DISCONNECT_TRADE 				= 90011;
    //! ǰ�������Ͽ�
	const int			TAPIERROR_TRADEFRONT_DISCONNECT_MANAGE 				= 90012;

    //! �����ʽ��˺Ų�����
	const int			TAPIERROR_TRADEFRONT_ACCOUNT 						= 90021;
	//! �ò���Ա��������
	const int			TAPIERROR_TRADEFRONT_ORDER 							= 90022;
	//! ��ѯƵ�ʹ���
	const int			TAPIERROR_TRADEFRONT_FREQUENCY 						= 90023;
	//! ����Ȩ�������¼
	const int			TAPIERROR_TRADEFRONT_RUFUSE 						= 90024;
	//! �Գɽ���֤��ͨ��
	const int			TAPIERROR_TRADEFRONT_SELFMATCH 						= 90025;
    
    const int TAPIERROR_SUCCEED                                            = 0;
    //! ���ӷ���ʧ��
    const int TAPIERROR_ConnectFail                                        = -1;
    //! ��·��֤ʧ��
    const int TAPIERROR_LinkAuthFail                                       = -2;
    //! ������ַ������
    const int TAPIERROR_HostUnavailable                                    = -3;
    //! �������ݴ���
    const int TAPIERROR_SendDataError                                      = -4;
    //! ���Ա�Ų��Ϸ�
    const int TAPIERROR_TestIDError                                        = -5;
    //! û׼���ò�������
    const int TAPIERROR_NotReadyTestNetwork                                = -6;
    //! ��ǰ������Ի�û����
    const int TAPIERROR_CurTestNotOver                                     = -7;
    //! û�ÿ��õĽ���ǰ��
    const int TAPIERROR_NOFrontAvailable                                   = -8;
    //! ����·��������
    const int TAPIERROR_DataPathAvaiable                                   = -9;
    //! �ظ���¼
    const int TAPIERROR_RepeatLogin                                        = -10;
    //! �ڲ�����	
    const int TAPIERROR_InnerError                                         = -11;
    //! ��һ������û�н���	
    const int TAPIERROR_LastReqNotFinish                                   = -12;
    //! ��������Ƿ�	
    const int TAPIERROR_InputValueError                                    = -13;
    //! ��Ȩ�벻�Ϸ�	
    const int TAPIERROR_AuthCode_Invalid                                   = -14;
    //! ��Ȩ�볬��	
    const int TAPIERROR_AuthCode_Expired                                   = -15;
    //! ��Ȩ�����Ͳ�ƥ��	
    const int TAPIERROR_AuthCode_TypeNotMatch                              = -16;
    //! API��û��׼����
    const int TAPIERROR_API_NotReady                                       = -17;
    //! UDP�˿ڼ���ʧ��
    const int TAPIERROR_UDP_LISTEN_FAILED                                  = -18;
    //! UDP���ڼ���
    const int TAPIERROR_UDP_LISTENING                                      = -19;
    //! �ӿ�δʵ��
    const int TAPIERROR_NotImplemented                                     = -20;
    //! ÿ�ε�¼ֻ�������һ��
	const int TAPIERROR_CallOneTimeOnly										= -21;
	//! �����µ�Ƶ�ʡ�
	const int TAPIERROR_ORDER_FREQUENCY										= -22;
	//! ��ѯƵ��̫�졣
	const int TAPIERROR_RENTQRY_TOOFAST										= -23;
	//! �����ϵ���������
	const int TAPIERROR_CALL_NOCONDITION									= -24;
	//! �ĵ�����ʱû���ҵ���Ӧ������
	const int TAPIERROR_ORDER_NOTFOUND										= -25;

	//! ��־·��Ϊ�ա�
	const int TAPIERROR_LOGPATH_EMPTY										= -26;
	//! ����־�ļ�ʧ��
	const int TAPIERROR_LOGPATH_FAILOPEN									= -27;
	//! û�н���Ա��¼Ȩ��
	const int TAPIERROR_RIGHT_TRADER 										= -28;
	//! û�ж���¼����߳ɽ�¼��Ȩ��
	const int TAPIERROR_RIGHT_ORDERINPUT									= -29;
	//! û�ж����޸ĺͶ���ɾ��Ȩ�ޣ��ɽ�ɾ��Ȩ��
	const int TAPIERROR_RIGHT_LOCALOPERATION 								= -30;
	//! û�ж���ת��Ȩ��
	const int TAPIERROR_RIGHT_ORDERTRANSFER 								= -31;
	//! �ɽ�¼��ʱϵͳ��Ϊ��
	const int TAPIERROR_FILLINPUT_SYSTEMNO 									= -32;
	//! �ɽ�ɾ��ʱ�ɽ���Ϊ�ա�
	const int TAPIERROR_FILLREMOVE_MATCHNO 									= -33;

	//! �ɽ�ɾ��ʱû���ҵ���Ӧ�ĳɽ�
	const int TAPIERROR_FILLREQMOVE_NOFUND 									= -34;
	//! �����޸�ʱ�ͻ��˺ű䶯��
	const int TAPIERROR_LOCALMODIFY_ACCOUNT 								= -35;
	//! ����ת��ʱ�ͻ��˺�û�б䶯
	const int TAPIERROR_LOCALTRANSFER_ACCOUNT 								= -36;
	//! �޸ĵĵ绰����λ�����Ի��߰��������ַ���
	const int TAPIERROR_INPUTERROR_PHONE 									= -37;

	//!	δ�󶨵Ķ�����֤��Ϣ
	const int TAPIERROR_ERROR_CONTACT										= -38;
	//! ������֤��Ч���ڲ��������������֤��
	const int TAPIERROR_ERROR_REJESTVERTIFICATE 							= -39;
	//! ���Ͷ�����֤�����֤����֮ǰ����Ҫ�ȷ������������֤��
	const int TAPIERROR_ERROR_NOTREQUESTSECONDCODE 							= -44;

	//! û�����ÿͻ������Ȩ�ޡ�
	const int TAPIERROR_RIGHT_SETPASSWORD 									= -40;
	//! ���ձ������ͻ��޷����������
	const int TAPIERROR_RISK_OPERERROR										= -41;
	//! �ĵ��ǿͻ��˺���д�붩���ͻ��˺Ų�һ��
	const int TAPIERROR_ORDER_MODACCOUNT									= -42;
	//! �ڴ�����ʧ��
	const int TAPIERROR_MEMORY_ALLOCFAILED									= -43;

	//! �û���Ȩ��Ϣû�и�Ʒ���µ�Ȩ��
	const int TAPIERROR_ERROR_LICENSECOMMODITY								= -45;
	//! ���м�ģʽ��ֹ���øýӿ�
	const int TAPIERROR_GATHERINFO_NORELAY									= -46;
	//! �û��ɼ���Ϣ��ȫ����Ӱ���¼��
	const int TAPIERROR_GATHERINFO_PARTY									= -47;
	//! ������ϵͳ���ɵ���
	const int TAPIERROR_ERROR_SYSTEMTYPE									= -48;
	//! ����ʽ��Ϣ�ɼ������ʧ��
	const int TAPIERROR_GATHERINFO_DATALOAD									= -49;

    /** @}*/


    //=============================================================================
    /**
     *	\addtogroup G_ERR_INPUT_CHECK		�������������
     *	@{
     */
    //=============================================================================
    //! ��������ΪNULL
    const int TAPIERROR_INPUTERROR_NULL                                    = -10000;
    //! ��������:TAPIYNFLAG
    const int TAPIERROR_INPUTERROR_TAPIYNFLAG                              = -10001;
    //! ��������:TAPILOGLEVEL
    const int TAPIERROR_INPUTERROR_TAPILOGLEVEL                            = -10002;
    //! ��������:TAPICommodityType
    const int TAPIERROR_INPUTERROR_TAPICommodityType                       = -10003;
    //! ��������:TAPICallOrPutFlagType
    const int TAPIERROR_INPUTERROR_TAPICallOrPutFlagType                   = -10004;
    //! ��������:TAPIBucketDateFlag
    const int TAPIERROR_INPUTERROR_TAPIBucketDateFlag                      = -11001;
    //! ��������:TAPIHisQuoteType
    const int TAPIERROR_INPUTERROR_TAPIHisQuoteType                        = -11002;
    //! ��������:TAPIAccountType
    const int TAPIERROR_INPUTERROR_TAPIAccountType                         = -12001;
    //! ��������:TAPIUserTypeType
    const int TAPIERROR_INPUTERROR_TAPIUserTypeType                        = -12002;
    //! ��������:TAPIAccountState
    const int TAPIERROR_INPUTERROR_TAPIAccountState                        = -12003;
    //! ��������:TAPIAccountFamilyType
    const int TAPIERROR_INPUTERROR_TAPIAccountFamilyType                   = -12004;
    //! ��������:TAPIOrderTypeType
    const int TAPIERROR_INPUTERROR_TAPIOrderTypeType                       = -12005;
    //! ��������:TAPIOrderSourceType
    const int TAPIERROR_INPUTERROR_TAPIOrderSourceType                     = -12006;
    //! ��������:TAPITimeInForceType
    const int TAPIERROR_INPUTERROR_TAPITimeInForceType                     = -12007;
    //! ��������:TAPISideType
    const int TAPIERROR_INPUTERROR_TAPISideType                            = -12008;
    //! ��������:TAPIPositionEffectType
    const int TAPIERROR_INPUTERROR_TAPIPositionEffectType                  = -12009;
    //! ��������:TAPIHedgeFlagType
    const int TAPIERROR_INPUTERROR_TAPIHedgeFlagType                       = -12010;
    //! ��������:TAPIOrderStateType
    const int TAPIERROR_INPUTERROR_TAPIOrderStateType                      = -12011;
    //! ��������:TAPICalculateModeType
    const int TAPIERROR_INPUTERROR_TAPICalculateModeType                   = -12012;
    //! ��������:TAPIMatchSourceType
    const int TAPIERROR_INPUTERROR_TAPIMatchSourceType                     = -12013;
    //! ��������:TAPIOpenCloseModeType
    const int TAPIERROR_INPUTERROR_TAPIOpenCloseModeType                   = -12014;
    //! ��������:TAPIFutureAlgType
    const int TAPIERROR_INPUTERROR_TAPIFutureAlgType                       = -12015;
    //! ��������:TAPIOptionAlgType
    const int TAPIERROR_INPUTERROR_TAPIOptionAlgType                       = -12016;
    //! ��������:TAPIBankAccountLWFlagType
    const int TAPIERROR_INPUTERROR_TAPIBankAccountLWFlagType               = -12017;
    //! ��������:TAPIBankAccountStateType
    const int TAPIERROR_INPUTERROR_TAPIBankAccountStateType                = -12018;
    //! ��������:TAPIBankAccountSwapStateType
    const int TAPIERROR_INPUTERROR_TAPIBankAccountSwapStateType            = -12019;
    //! ��������:TAPIBankAccountTransferStateType
    const int TAPIERROR_INPUTERROR_TAPIBankAccountTransferStateType        = -12020;
    //! ��������:TAPIMarginCalculateModeType
    const int TAPIERROR_INPUTERROR_TAPIMarginCalculateModeType             = -12021;
    //! ��������:TAPIOptionMarginCalculateModeType
    const int TAPIERROR_INPUTERROR_TAPIOptionMarginCalculateModeType       = -12022;
    //! ��������:TAPICmbDirectType
    const int TAPIERROR_INPUTERROR_TAPICmbDirectType                       = -12023;
    //! ��������:TAPIDeliveryModeType
    const int TAPIERROR_INPUTERROR_TAPIDeliveryModeType                    = -12024;
    //! ��������:TAPIContractTypeType
    const int TAPIERROR_INPUTERROR_TAPIContractTypeType                    = -12025;
    //! ��������:TAPIPartyTypeType
    const int TAPIERROR_INPUTERROR_TAPIPartyTypeType                       = -12026;
    //! ��������:TAPIPartyCertificateTypeType
    const int TAPIERROR_INPUTERROR_TAPIPartyCertificateTypeType            = -12027;
    //! ��������:TAPIMsgReceiverType
    const int TAPIERROR_INPUTERROR_TAPIMsgReceiverType                     = -12028;
    //! ��������:TAPIMsgTypeType
    const int TAPIERROR_INPUTERROR_TAPIMsgTypeType                         = -12029;
    //! ��������:TAPIMsgLevelType
    const int TAPIERROR_INPUTERROR_TAPIMsgLevelType                        = -12030;
    //! ��������:TAPITransferDirectType
    const int TAPIERROR_INPUTERROR_TAPITransferDirectType                  = -12031;
    //! ��������:TAPITransferStateType
    const int TAPIERROR_INPUTERROR_TAPITransferStateType                   = -12032;
    //! ��������:TAPITransferTypeType
    const int TAPIERROR_INPUTERROR_TAPITransferTypeType                    = -12033;
    //! ��������:TAPITransferDeviceIDType
    const int TAPIERROR_INPUTERROR_TAPITransferDeviceIDType                = -12034;
    //! ��������:TAPITacticsTypeType
    const int TAPIERROR_INPUTERROR_TAPITacticsTypeType                     = -12035;
    //! ��������:TAPIORDERACT
    const int TAPIERROR_INPUTERROR_TAPIORDERACT                            = -12036;
    //! ��������:TAPIBillTypeType
    const int TAPIERROR_INPUTERROR_TAPIBillTypeType                        = -12037;
    //! ��������:TAPIBillFileTypeType
    const int TAPIERROR_INPUTERROR_TAPIBillFileTypeType                    = -12038;
    //! ��������:TAPIOFFFlagType
    const int TAPIERROR_INPUTERROR_TAPIOFFFlagType                         = -12039;
    //! ��������:TAPICashAdjustTypeType
    const int TAPIERROR_INPUTERROR_TAPICashAdjustTypeType                  = -12040;
    //! ��������:TAPITriggerConditionType
    const int TAPIERROR_INPUTERROR_TAPITriggerConditionType                = -12041;
    //! ��������:TAPITriggerPriceTypeType
    const int TAPIERROR_INPUTERROR_TAPITriggerPriceTypeType                = -12042;
    //! ��������:TAPITradingStateType 
    const int TAPIERROR_INPUTERROR_TAPITradingStateType                    = -12043;
    //! ��������:TAPIMarketLevelType 
    const int TAPIERROR_INPUTERROR_TAPIMarketLevelType                     = -12044;
    //! ��������:TAPIOrderQryTypeType 
    const int TAPIERROR_INPUTERROR_TAPIOrderQryTypeType                    = -12045;
	//! ��������: ClientID��ClientID���������ַ���
	const int TAPIERROR_INPUTERROR_TAPIClientID                            = -12046;
    //! ��ʷ�����ѯ�������Ϸ�
    const int TAPIERROR_INPUTERROR_QryHisQuoteParam                        = -13001;
	//! �۸�������а���NAN����INF���Ϸ�����ֵ
	const int TAPIERROR_INPUTERROR_TAPIIncludeNAN							= -13002;
	//! �������ĵ�����
	const  int TAPIERROR_INPUTERROR_TAPIExpireTime							= -12047;
	//! �������������
	const int TAPIERROR_INPUTERROR_TAPIPasswordType							= -12048;
    //! ����Ľ�����������
    const int TAPIERROR_INPUTERROR_TAPISettleFlagType                       = -12049;
	//! ��������:TAPILoginTypeType
	const int TAPIERROR_INPUTERROR_TAPILoginTypeType						= -12050;
	//! ��������:TapAPISpecialOrderTypeType			
	const int TAPIERROR_INPUTERROR_TapAPISpecialOrderTypeType				= -12051;
	//! ��������:TapAPISpecialOrderTypeType
	const int TAPIERROR_INPUTERROR_TapAPICombineStrategyType				= -12052;

    /** @}*/

    //=============================================================================
    /**
     *	\addtogroup G_ERR_DISCONNECT_REASON	����Ͽ�������붨��
     *	@{
     */
    //=============================================================================
    //! �����Ͽ�
    const int TAPIERROR_DISCONNECT_CLOSE_INIT                              = 1;
    //! �����Ͽ�
    const int TAPIERROR_DISCONNECT_CLOSE_PASS                              = 2;
    //! ������
    const int TAPIERROR_DISCONNECT_READ_ERROR                              = 3;
    //! д����
    const int TAPIERROR_DISCONNECT_WRITE_ERROR                             = 4;
    //! ��������
    const int TAPIERROR_DISCONNECT_BUF_FULL                                = 5;
    //! �첽��������
    const int TAPIERROR_DISCONNECT_IOCP_ERROR                              = 6;
    //! �������ݴ���
    const int TAPIERROR_DISCONNECT_PARSE_ERROR                             = 7;
    //! ���ӳ�ʱ
    const int TAPIERROR_DISCONNECT_CONNECT_TIMEOUT                         = 8;
    //! ��ʼ��ʧ��
    const int TAPIERROR_DISCONNECT_INIT_ERROR                              = 9;
    //! �Ѿ�����
    const int TAPIERROR_DISCONNECT_HAS_CONNECTED                           = 10;
    //! �����߳��ѽ���
    const int TAPIERROR_DISCONNECT_HAS_EXIT                                = 11;
    //! �������ڽ��У����Ժ�����
    const int TAPIERROR_DISCONNECT_TRY_LATER                               = 12;

    /** @}*/
}
#endif //! TAP_API_ERROR_H
