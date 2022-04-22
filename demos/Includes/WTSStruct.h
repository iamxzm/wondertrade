/*!
 * \file WTSStruct.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief Wt�����ṹ�嶨��
 */
#pragma once
#include <memory>
#include <stdint.h>
#include <string.h>
#include "WTSTypes.h"

#pragma warning(disable:4200)

NS_OTP_BEGIN

#pragma pack(push, 1)

struct WTSBarStruct
{
public:
	WTSBarStruct()
	{
		memset(this, 0, sizeof(WTSBarStruct));
	}

	uint32_t	date;
	uint32_t	time;
	double		open;		//��
	double		high;		//��
	double		low;		//��
	double		close;		//��
	double		settle;		//����
	double		money;		//�ɽ����

	uint32_t	vol;	//�ɽ���
	uint32_t	hold;	//�ܳ�
	int32_t		add;	//����
};

struct WTSTickStruct
{
	char		exchg[MAX_EXCHANGE_LENGTH];
	char		code[MAX_INSTRUMENT_LENGTH];

	double		price;				//���¼�
	double		open;				//���̼�
	double		high;				//��߼�
	double		low;				//��ͼ�
	double		settle_price;		//�����

	double		upper_limit;		//��ͣ��
	double		lower_limit;		//��ͣ��

	uint32_t	total_volume;		//�ܳɽ���
	uint32_t	volume;				//�ɽ���
	double		total_turnover;		//�ܳɽ���
	double		turn_over;			//�ɽ���
	uint32_t	open_interest;		//�ܳ�
	int32_t		diff_interest;		//����

	uint32_t	trading_date;		//������,��20140327
	uint32_t	action_date;		//��Ȼ����,��20140327
	uint32_t	action_time;		//����ʱ��,��ȷ������,��105932000

	double		pre_close;			//���ռ�
	double		pre_settle;			//�����
	int32_t		pre_interest;		//�����ܳ�

	double		bid_prices[10];		//ί��۸�
	double		ask_prices[10];		//ί���۸�
	uint32_t	bid_qty[10];		//ί����
	uint32_t	ask_qty[10];		//ί����
	WTSTickStruct()
	{
		memset(this, 0, sizeof(WTSTickStruct));
	}
};

struct WTSOrdQueStruct
{
	char		exchg[MAX_EXCHANGE_LENGTH];
	char		code[MAX_INSTRUMENT_LENGTH];

	uint32_t	trading_date;		//������,��20140327
	uint32_t	action_date;		//��Ȼ����,��20140327
	uint32_t	action_time;		//����ʱ��,��ȷ������,��105932000

	WTSBSDirectType	side;			//ί�з���
	double			price;			//ί�м۸�
	uint32_t		order_items;	//��������
	uint32_t		qsize;			//���г���
	uint32_t		volumes[50];	//ί����ϸ

	WTSOrdQueStruct()
	{
		memset(this, 0, sizeof(WTSOrdQueStruct));
	}
};


struct WTSOrdDtlStruct
{
	char		exchg[MAX_EXCHANGE_LENGTH];
	char		code[MAX_INSTRUMENT_LENGTH];

	uint32_t	trading_date;		//������,��20140327
	uint32_t	action_date;		//��Ȼ����,��20140327
	uint32_t	action_time;		//����ʱ��,��ȷ������,��105932000

	uint32_t		index;			//ί�б��(��1��ʼ,����1)
	WTSBSDirectType	side;			//ί�з���
	double			price;			//ί�м۸�
	uint32_t		volume;			//ί������
	WTSOrdDetailType	otype;		//ί������

	WTSOrdDtlStruct()
	{
		memset(this, 0, sizeof(WTSOrdDtlStruct));
	}
};

struct WTSTransStruct
{
	char		exchg[MAX_EXCHANGE_LENGTH];
	char		code[MAX_INSTRUMENT_LENGTH];

	uint32_t	trading_date;		//������,��20140327
	uint32_t	action_date;		//��Ȼ����,��20140327
	uint32_t	action_time;		//����ʱ��,��ȷ������,��105932000

	uint32_t		index;			//�ɽ����(��1��ʼ,����1)
	WTSTransType	ttype;			//�ɽ�����: 'C', 0
	WTSBSDirectType	side;			//BS��־

	double			price;			//�ɽ��۸�
	uint32_t		volume;			//�ɽ�����
	int32_t			askorder;		//�������
	int32_t			bidorder;		//�������

	WTSTransStruct()
	{
		memset(this, 0, sizeof(WTSTransStruct));
	}
};

#pragma pack(pop)

NS_OTP_END
