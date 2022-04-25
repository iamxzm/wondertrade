#pragma once
#include "../Includes/WTSStruct.h"

USING_NS_WTP;

#pragma pack(push, 1)

const char BLK_FLAG[] = "&^%$#@!\0";

const int FLAG_SIZE = 8;

typedef enum tagBlockType
{
	BT_RT_Minute1		= 1,	//ʵʱ1������
	BT_RT_Minute5		= 2,	//ʵʱ5������
	BT_RT_Ticks			= 3,	//ʵʱtick����
	BT_RT_Cache			= 4,	//ʵʱ����
	BT_RT_Trnsctn		= 5,	//ʵʱ��ʳɽ�
	BT_RT_OrdDetail		= 6,	//ʵʱ���ί��
	BT_RT_OrdQueue		= 7,	//ʵʱί�ж���

	BT_HIS_Minute1		= 21,	//��ʷ1������
	BT_HIS_Minute5		= 22,	//��ʷ5������
	BT_HIS_Day			= 23,	//��ʷ����
	BT_HIS_Ticks		= 24,	//��ʷtick
	BT_HIS_Trnsctn		= 25,	//��ʷ��ʳɽ�
	BT_HIS_OrdDetail	= 26,	//��ʷ���ί��
	BT_HIS_OrdQueue		= 27	//��ʷί�ж���
} BlockType;

#define BLOCK_VERSION_RAW		0x01	//�Ͻṹ��δѹ��
#define BLOCK_VERSION_CMP		0x02	//�Ͻṹ��ѹ��
#define BLOCK_VERSION_RAW_V2	0x03	//�½ṹ��δѹ��
#define BLOCK_VERSION_CMP_V2	0x04	//�½ṹ��ѹ��

typedef struct _BlockHeader
{
	char		_blk_flag[FLAG_SIZE];
	uint16_t	_type;
	uint16_t	_version;

	inline bool is_old_version() const {
		return (_version == BLOCK_VERSION_CMP || _version == BLOCK_VERSION_RAW);
	}

	inline bool is_compressed() const {
		return (_version == BLOCK_VERSION_CMP || _version == BLOCK_VERSION_CMP_V2);
	}
} BlockHeader;

typedef struct _BlockHeaderV2
{
	char		_blk_flag[FLAG_SIZE];
	uint16_t	_type;
	uint16_t	_version;

	uint64_t	_size;		//ѹ��������ݴ�С

	inline bool is_old_version() const {
		return (_version == BLOCK_VERSION_CMP || _version == BLOCK_VERSION_RAW);
	}

	inline bool is_compressed() const {
		return (_version == BLOCK_VERSION_CMP || _version == BLOCK_VERSION_CMP_V2);
	}
} BlockHeaderV2;

#define BLOCK_HEADER_SIZE	sizeof(BlockHeader)
#define BLOCK_HEADERV2_SIZE sizeof(BlockHeaderV2)

typedef struct _RTBlockHeader : BlockHeader
{
	uint32_t _size;
	uint32_t _capacity;
} RTBlockHeader;

//ÿ��ʵʱ���ݿ�ͷ��
typedef struct _RTDayBlockHeader : RTBlockHeader
{
	uint32_t		_date;
} RTDayBlockHeader;

//ʵʱK�����ݿ�
typedef struct _RTKlineBlock : _RTDayBlockHeader
{
	WTSBarStruct	_bars[0];
} RTKlineBlock;

//tick�������ݿ�
//By Wesley @ 2021.12.30
//ʵʱtick���棬ֱ�����°汾��tick�ṹ
//�л�����һ��Ҫ���̺���У�����
typedef struct _RTTickBlock : RTDayBlockHeader
{
	WTSTickStruct	_ticks[0];
} RTTickBlock;

//��ʳɽ����ݿ�
typedef struct _RTTransBlock : RTDayBlockHeader
{
	WTSTransStruct	_trans[0];
} RTTransBlock;

//���ί�����ݿ�
typedef struct _RTOrdDtlBlock : RTDayBlockHeader
{
	WTSOrdDtlStruct	_details[0];
} RTOrdDtlBlock;

//ί�ж������ݿ�
typedef struct _RTOrdQueBlock : RTDayBlockHeader
{
	WTSOrdQueStruct	_queues[0];
} RTOrdQueBlock;

typedef struct _TickCacheItem
{
	uint32_t		_date;
	WTSTickStruct	_tick;
} TickCacheItem;

//ʵʱtick����
typedef struct _RTTickCache : RTBlockHeader
{
	TickCacheItem	_ticks[0];
} RTTickCache;


//��ʷTick����
typedef struct _HisTickBlock : BlockHeader
{
	WTSTickStruct	_ticks[0];
} HisTickBlock;

//��ʷTick����V2
typedef struct _HisTickBlockV2 : BlockHeaderV2
{
	char			_data[0];
} HisTickBlockV2;

typedef struct _HisTransBlock : BlockHeader
{
	WTSTransStruct	_items[0];
} HisTransBlock;

typedef struct _HisTransBlockV2 : BlockHeaderV2
{
	char			_data[0];
} HisTransBlockV2;

typedef struct _HisOrdDtlBlock : BlockHeader
{
	WTSOrdDtlStruct	_items[0];
} HisOrdDtlBlock;

typedef struct _HisOrdDtlBlockV2 : BlockHeaderV2
{
	char			_data[0];
} HisOrdDtlBlockV2;

typedef struct _HisOrdQueBlock : BlockHeader
{
	WTSOrdQueStruct	_items[0];
} HisOrdQueBlock;

typedef struct _HisOrdQueBlockV2 : BlockHeaderV2
{
	char			_data[0];
} HisOrdQueBlockV2;

//��ʷK������
typedef struct _HisKlineBlock : BlockHeader
{
	WTSBarStruct	_bars[0];
} HisKlineBlock;

//��ʷK������V2
typedef struct _HisKlineBlockV2 : BlockHeaderV2
{
	char			_data[0];
} HisKlineBlockV2;

//��ʷK������
typedef struct _HisKlineBlockOld : BlockHeader
{
	WTSBarStructOld	_bars[0];
} HisKlineBlockOld;

#pragma pack(pop)
