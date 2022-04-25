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

#ifdef _MSC_VER
#pragma warning(disable:4200)
#endif

NS_WTP_BEGIN

#pragma pack(push, 1)

struct WTSBarStructOld
{
public:
	WTSBarStructOld()
	{
		memset(this, 0, sizeof(WTSBarStructOld));
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

struct WTSTickStructOld
{
	char		exchg[10];
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
	WTSTickStructOld()
	{
		memset(this, 0, sizeof(WTSTickStructOld));
	}
};

#pragma pack(pop)


//By Wesley @ 2021.12.31
//�µĽṹ�壬ȫ���ĳ�8�ֽڶ���ķ�ʽ
#pragma pack(push, 8)

struct WTSBarStruct
{
public:
	WTSBarStruct()
	{
		memset(this, 0, sizeof(WTSBarStruct));
	}

	uint32_t	date;		//����
	uint32_t	reserve_;	//ռλ��
	uint64_t	time;		//ʱ��
	double		open;		//��
	double		high;		//��
	double		low;		//��
	double		close;		//��
	double		settle;		//����
	double		money;		//�ɽ����

	double		vol;	//�ɽ���
	double		hold;	//�ܳ�
	double		add;	//����

	//By Wesley @ 2021.12.30
	//ֱ�Ӹ����Ͻṹ��
	WTSBarStruct& operator = (const WTSBarStructOld& bar)
	{
		date = bar.date;
		time = bar.time;

		open = bar.open;
		high = bar.high;
		low = bar.low;
		close = bar.close;
		settle = bar.settle;
		money = bar.money;

		vol = bar.vol;
		hold = bar.hold;
		add = bar.add;

		return *this;
	}
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

	double		total_volume;		//�ܳɽ���
	double		volume;				//�ɽ���
	double		total_turnover;		//�ܳɽ���
	double		turn_over;			//�ɽ���
	double		open_interest;		//�ܳ�
	double		diff_interest;		//����

	uint32_t	trading_date;		//������,��20140327
	uint32_t	action_date;		//��Ȼ����,��20140327
	uint32_t	action_time;		//����ʱ��,��ȷ������,��105932000
	uint32_t	reserve_;			//ռλ��

	double		pre_close;			//���ռ�
	double		pre_settle;			//�����
	double		pre_interest;		//�����ܳ�

	double		bid_prices[10];		//ί��۸�
	double		ask_prices[10];		//ί���۸�
	double		bid_qty[10];		//ί����
	double		ask_qty[10];		//ί����
	WTSTickStruct()
	{
		memset(this, 0, sizeof(WTSTickStruct));
	}

	WTSTickStruct& operator = (const WTSTickStructOld& tick)
	{
		strncpy(exchg, tick.exchg, MAX_EXCHANGE_LENGTH);
		strncpy(code, tick.code, MAX_INSTRUMENT_LENGTH);

		price = tick.price;
		open = tick.open;
		high = tick.high;
		low = tick.low;
		settle_price = tick.settle_price;

		upper_limit = tick.upper_limit;
		lower_limit = tick.lower_limit;

		total_volume = tick.total_volume;
		total_turnover = tick.total_turnover;
		open_interest = tick.open_interest;
		volume = tick.volume;
		turn_over = tick.turn_over;
		diff_interest = tick.diff_interest;

		trading_date = tick.trading_date;
		action_date = tick.action_date;
		action_time = tick.action_time;

		pre_close = tick.pre_close;
		pre_interest = tick.pre_interest;
		pre_settle = tick.pre_settle;

		for(int i = 0; i < 10; i++)
		{
			bid_prices[i] = tick.bid_prices[i];
			bid_qty[i] = tick.bid_qty[i];
			ask_prices[i] = tick.ask_prices[i];
			ask_qty[i] = tick.ask_qty[i];
		}

		return *this;
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

	double		price;			//ί�м۸�
	uint32_t	order_items;	//��������
	uint32_t	qsize;			//���г���
	uint32_t	volumes[50];	//ί����ϸ

	WTSOrdQueStruct()
	{
		memset(this, 0, sizeof(WTSOrdQueStruct));
	}
};

struct WTSOrdDtlStruct
{
	char		exchg[MAX_EXCHANGE_LENGTH];
	char		code[MAX_INSTRUMENT_LENGTH];

	uint32_t		trading_date;		//������,��20140327
	uint32_t		action_date;		//��Ȼ����,��20140327
	uint32_t		action_time;		//����ʱ��,��ȷ������,��105932000

	uint64_t		index;			//ί�б��(��1��ʼ,����1)
	double			price;			//ί�м۸�
	uint64_t		volume;			//ί������
	WTSBSDirectType		side;		//ί�з���
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
	uint32_t	index;			//�ɽ����(��1��ʼ,����1)

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

NS_WTP_END
