#include "WtDataWriter.h"

#include "../Includes/WTSSessionInfo.hpp"
#include "../Includes/WTSContractInfo.hpp"
#include "../Includes/WTSDataDef.hpp"
#include "../Includes/WTSVariant.hpp"
#include "../Share/BoostFile.hpp"
#include "../Share/StrUtil.hpp"
#include "../Share/IniHelper.hpp"
#include "../Share/DLLHelper.hpp"

#include "../Includes/IBaseDataMgr.h"

#include "../WTSTools/WTSCmpHelper.hpp"

#ifdef _WIN32
#pragma comment(lib, "libmysql.lib")
#endif


#include <set>

extern "C"
{
	EXPORT_FLAG IDataWriter* createWriter()
	{
		IDataWriter* ret = new WtDataWriter();
		return ret;
	}

	EXPORT_FLAG void deleteWriter(IDataWriter* &writer)
	{
		if (writer != NULL)
		{
			delete writer;
			writer = NULL;
		}
	}
};

static const uint32_t CACHE_SIZE_STEP = 200;
static const uint32_t TICK_SIZE_STEP = 2500;
static const uint32_t KLINE_SIZE_STEP = 200;

const char CMD_CLEAR_CACHE[] = "CMD_CLEAR_CACHE";
const char MARKER_FILE[] = "marker.ini";

#ifdef _WIN32
#include <wtypes.h>
HMODULE	g_dllModule = NULL;

BOOL APIENTRY DllMain(
	HANDLE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		g_dllModule = (HMODULE)hModule;
		break;
	}
	return TRUE;
}
#else
#include <dlfcn.h>

char PLATFORM_NAME[] = "UNIX";

std::string	g_moduleName;

__attribute__((constructor))
void on_load(void) {
	Dl_info dl_info;
	dladdr((void *)on_load, &dl_info);
	g_moduleName = dl_info.dli_fname;
}
#endif

std::string getBinDir()
{
	static std::string _bin_dir;
	if (_bin_dir.empty())
	{


#ifdef _WIN32
		char strPath[MAX_PATH];
		GetModuleFileName(g_dllModule, strPath, MAX_PATH);

		_bin_dir = StrUtil::standardisePath(strPath, false);
#else
		_bin_dir = g_moduleName;
#endif

		uint32_t nPos = _bin_dir.find_last_of('/');
		_bin_dir = _bin_dir.substr(0, nPos + 1);
	}

	return _bin_dir;
}



WtDataWriter::WtDataWriter()
	: _terminated(false)
	, _save_tick_log(false)
	, _log_group_size(1000)
	, _disable_day(false)
	, _disable_min1(false)
	, _disable_min5(false)
	, _disable_orddtl(false)
	, _disable_ordque(false)
	, _disable_trans(false)
	, _disable_tick(false)
{
}


WtDataWriter::~WtDataWriter()
{
}

bool WtDataWriter::isSessionProceeded(const char* sid)
{
	auto it = _proc_date.find(sid);
	if (it == _proc_date.end())
		return false;

	return (it->second >= TimeUtils::getCurDate());
}

void WtDataWriter::init_db()
{
	if (!_db_conf._active)
		return;

#ifdef _WIN32
	std::string module = getBinDir() + "libmysql.dll";
	DLLHelper::load_library(module.c_str());
#endif

	_db_conn.reset(new MysqlDb);
	my_bool autoreconnect = true;
	_db_conn->options(MYSQL_OPT_RECONNECT, &autoreconnect);
	_db_conn->options(MYSQL_SET_CHARSET_NAME, "utf8");

	if (_db_conn->connect(_db_conf._dbname, _db_conf._host, _db_conf._user, _db_conf._pass, _db_conf._port, CLIENT_MULTI_STATEMENTS | CLIENT_MULTI_RESULTS))
	{
		if(_sink)
			_sink->outputWriterLog(LL_INFO, "Mysql connected[%s:%d]", _db_conf._host, _db_conf._port);
	}
	else
	{
		if (_sink)
			_sink->outputWriterLog(LL_ERROR, "Mysql connecting failed[%s:%d]:%s", _db_conf._host, _db_conf._port, _db_conn->errstr());
		_db_conn.reset();
	}
}

bool WtDataWriter::init(WTSVariant* params, IDataWriterSink* sink)
{
	_sink = sink;
	_bd_mgr = sink->getBDMgr();
	_save_tick_log = params->getBoolean("savelog");

	_base_dir = StrUtil::standardisePath(params->getCString("path"));
	if (!BoostFile::exists(_base_dir.c_str()))
		BoostFile::create_directories(_base_dir.c_str());
	_cache_file = params->getCString("cache");
	if (_cache_file.empty())
		_cache_file = "cache.dmb";

	_async_proc = params->getBoolean("async");
	_log_group_size = params->getUInt32("groupsize");

	_disable_tick = params->getBoolean("disabletick");
	_disable_min1 = params->getBoolean("disablemin1");
	_disable_min5 = params->getBoolean("disablemin5");
	_disable_day = params->getBoolean("disableday");

	_disable_trans = params->getBoolean("disabletrans");
	_disable_ordque = params->getBoolean("disableordque");
	_disable_orddtl = params->getBoolean("disableorddtl");

	{
		std::string filename = _base_dir + MARKER_FILE;
		IniHelper iniHelper;
		iniHelper.load(filename.c_str());
		StringVector ayKeys, ayVals;
		iniHelper.readSecKeyValArray("markers", ayKeys, ayVals);
		for (uint32_t idx = 0; idx < ayKeys.size(); idx++)
		{
			_proc_date[ayKeys[idx].c_str()] = strtoul(ayVals[idx].c_str(), 0, 10);
		}
	}

	WTSVariant* dbConf = params->get("db");
	if(dbConf)
	{
		strcpy(_db_conf._host, dbConf->getCString("host"));
		strcpy(_db_conf._dbname, dbConf->getCString("dbname"));
		strcpy(_db_conf._user, dbConf->getCString("user"));
		strcpy(_db_conf._pass, dbConf->getCString("pass"));
		_db_conf._port = dbConf->getInt32("port");

		_db_conf._active = (strlen(_db_conf._host) > 0) && (strlen(_db_conf._dbname) > 0) && (_db_conf._port != 0);
		if (_db_conf._active)
			init_db();
	}

	loadCache();

	_proc_chk.reset(new StdThread(boost::bind(&WtDataWriter::check_loop, this)));
	return true;
}

void WtDataWriter::release()
{
	_terminated = true;
	if (_proc_thrd)
	{
		_proc_cond.notify_all();
		_proc_thrd->join();
	}

	for(auto& v : _rt_ticks_blocks)
	{
		delete v.second;
	}

	for (auto& v : _rt_trans_blocks)
	{
		delete v.second;
	}

	for (auto& v : _rt_orddtl_blocks)
	{
		delete v.second;
	}

	for (auto& v : _rt_ordque_blocks)
	{
		delete v.second;
	}

	for (auto& v : _rt_min1_blocks)
	{
		delete v.second;
	}

	for (auto& v : _rt_min5_blocks)
	{
		delete v.second;
	}
}

/*
void DataManager::preloadRtCaches(const char* exchg)
{
	if (!_preload_enable || _preloaded)
		return;

	_sink->outputWriterLog(LL_INFO, "��ʼԤ����ʵʱ���ݻ����ļ�...");
	TimeUtils::Ticker ticker;
	uint32_t cnt = 0;
	uint32_t codecnt = 0;
	WTSArray* ayCts = _bd_mgr->getContracts(exchg);
	if (ayCts != NULL && ayCts->size() > 0)
	{
		for (auto it = ayCts->begin(); it != ayCts->end(); it++)
		{
			WTSContractInfo* ct = (WTSContractInfo*)(*it);
			if (ct == NULL)
				continue;
			WTSCommodityInfo* commInfo = _bd_mgr->getCommodity(ct);
			if(commInfo == NULL)
				continue;

			bool isStk = (commInfo->getCategoty() == CC_Stock);
			codecnt++;
			
			releaseBlock(getTickBlock(ct->getCode(), TimeUtils::getCurDate(), true));
			releaseBlock(getKlineBlock(ct->getCode(), KP_Minute1, true));
			releaseBlock(getKlineBlock(ct->getCode(), KP_Minute5, true));
			cnt += 3;
			if (isStk && strcmp(commInfo->getProduct(), "STK") == 0)
			{
				releaseBlock(getOrdQueBlock(ct->getCode(), TimeUtils::getCurDate(), true));
				releaseBlock(getTransBlock(ct->getCode(), TimeUtils::getCurDate(), true));
				cnt += 2;
				if (strcmp(ct->getExchg(), "SZSE") == 0)
				{
					releaseBlock(getOrdDtlBlock(ct->getCode(), TimeUtils::getCurDate(), true));
					cnt++;
				}
			}
		}
	}

	if (ayCts != NULL)
		ayCts->release();
	_sink->outputWriterLog(LL_INFO, "Ԥ����%��Ʒ�ֵ�ʵʱ���ݻ����ļ�%u��,��ʱ%s΢��", codecnt, cnt, WTSLogger::fmtInt64(ticker.micro_seconds()));
	_preloaded = true;
}
*/

void WtDataWriter::loadCache()
{
	if (_tick_cache_file != NULL)
		return;

	bool bNew = false;
	std::string filename = _base_dir + _cache_file;
	if (!BoostFile::exists(filename.c_str()))
	{
		uint64_t uSize = sizeof(RTTickCache) + sizeof(TickCacheItem) * CACHE_SIZE_STEP;
		BoostFile bf;
		bf.create_new_file(filename.c_str());
		bf.truncate_file((uint32_t)uSize);
		bf.close_file();
		bNew = true;
	}

	_tick_cache_file.reset(new BoostMappingFile);
	_tick_cache_file->map(filename.c_str());
	_tick_cache_block = (RTTickCache*)_tick_cache_file->addr();

	_tick_cache_block->_size = min(_tick_cache_block->_size, _tick_cache_block->_capacity);

	if(bNew)
	{
		memset(_tick_cache_block, 0, _tick_cache_file->size());

		_tick_cache_block->_capacity = CACHE_SIZE_STEP;
		_tick_cache_block->_type = BT_RT_Cache;
		_tick_cache_block->_size = 0;
		_tick_cache_block->_version = 1;
		strcpy(_tick_cache_block->_blk_flag, BLK_FLAG);
	}
	else
	{
		for (uint32_t i = 0; i < _tick_cache_block->_size; i++)
		{
			const TickCacheItem& item = _tick_cache_block->_ticks[i];
			std::string key = StrUtil::printf("%s.%s", item._tick.exchg, item._tick.code);
			_tick_cache_idx[key] = i;
		}
	}
}

template<typename HeaderType, typename T>
void* WtDataWriter::resizeRTBlock(BoostMFPtr& mfPtr, uint32_t nCount)
{
	if (mfPtr == NULL)
		return NULL;

	//���øú���֮ǰ,Ӧ���Ѿ�������д����
	RTBlockHeader* tBlock = (RTBlockHeader*)mfPtr->addr();
	if (tBlock->_capacity >= nCount)
		return mfPtr->addr();

	const char* filename = mfPtr->filename();
	uint64_t uOldSize = sizeof(HeaderType) + sizeof(T)*tBlock->_capacity;
	uint64_t uNewSize = sizeof(HeaderType) + sizeof(T)*nCount;
	std::string data;
	data.resize(uNewSize - uOldSize, 0);
	try
	{
		BoostFile f;
		f.open_existing_file(filename);
		f.seek_to_end();
		f.write_file(data.c_str(), data.size());
		f.close_file();
	}
	catch(std::exception& ex)
	{
		_sink->outputWriterLog(LL_ERROR, "Exception occured while expanding RT cache file of %s[%u]: %s", filename, uNewSize, ex.what());
		return mfPtr->addr();
	}

	BoostMappingFile* pNewMf = new BoostMappingFile();
	if (!pNewMf->map(filename))
	{
		delete pNewMf;
		return NULL;
	}

	mfPtr.reset(pNewMf);

	tBlock = (RTBlockHeader*)mfPtr->addr();
	tBlock->_capacity = nCount;
	return mfPtr->addr();
}

bool WtDataWriter::writeTick(WTSTickData* curTick, bool bNeedSlice /* = true */)
{
	if (curTick == NULL)
		return false;

	curTick->retain();
	pushTask([this, curTick, bNeedSlice](){

		do
		{
			WTSContractInfo* ct = _bd_mgr->getContract(curTick->code(), curTick->exchg());
			WTSCommodityInfo* commInfo = _bd_mgr->getCommodity(ct);

			//�ٸ���״̬����
			if (!_sink->canSessionReceive(commInfo->getSession()))
				break;

			//�ȸ��»���
			if (!updateCache(ct, curTick, bNeedSlice))
				break;

			//д��tick����
			if(!_disable_tick)
				pipeToTicks(ct, curTick);

			//д��K�߻���
			pipeToKlines(ct, curTick);

			_sink->broadcastTick(curTick);

			static faster_hashmap<std::string, uint64_t> _tcnt_map;
			_tcnt_map[curTick->exchg()]++;
			if (_tcnt_map[curTick->exchg()] % _log_group_size == 0)
			{
				_sink->outputWriterLog(LL_INFO, "%s ticks received from exchange %s", StrUtil::fmtUInt64(_tcnt_map[curTick->exchg()]).c_str(), curTick->exchg());
			}
		} while (false);

		curTick->release();
	});
	return true;
}

bool WtDataWriter::writeOrderQueue(WTSOrdQueData* curOrdQue)
{
	if (curOrdQue == NULL || _disable_ordque)
		return false;

	curOrdQue->retain();
	pushTask([this, curOrdQue](){

		do
		{
			WTSContractInfo* ct = _bd_mgr->getContract(curOrdQue->code(), curOrdQue->exchg());
			WTSCommodityInfo* commInfo = _bd_mgr->getCommodity(ct);

			//�ٸ���״̬����
			if (!_sink->canSessionReceive(commInfo->getSession()))
				break;

			OrdQueBlockPair* pBlockPair = getOrdQueBlock(ct, curOrdQue->tradingdate());
			if (pBlockPair == NULL)
				break;

			StdUniqueLock lock(pBlockPair->_mutex);

			//�ȼ������������,����Ҫ��
			RTOrdQueBlock* blk = pBlockPair->_block;
			if (blk->_size >= blk->_capacity)
			{
				pBlockPair->_file->sync();
				pBlockPair->_block = (RTOrdQueBlock*)resizeRTBlock<RTDayBlockHeader, WTSOrdQueStruct>(pBlockPair->_file, blk->_capacity + TICK_SIZE_STEP);
				blk = pBlockPair->_block;
			}

			memcpy(&blk->_queues[blk->_size], &curOrdQue->getOrdQueStruct(), sizeof(WTSOrdQueStruct));
			blk->_size += 1;

			//TODO: Ҫ�㲥��
			//g_udpCaster.broadcast(curTrans);

			static faster_hashmap<std::string, uint64_t> _tcnt_map;
			_tcnt_map[curOrdQue->exchg()]++;
			if (_tcnt_map[curOrdQue->exchg()] % _log_group_size == 0)
			{
				//_sink->outputWriterLog(LL_INFO, "���յ�������%s��ί�ж�������%s��", curOrdQue->exchg(), StrUtil::fmtUInt64(_tcnt_map[curOrdQue->exchg()]).c_str());
				_sink->outputWriterLog(LL_INFO, "%s orderques received from exchange %s", StrUtil::fmtUInt64(_tcnt_map[curOrdQue->exchg()]).c_str(), curOrdQue->exchg());
			}
		} while (false);
		curOrdQue->release();
	});
	return true;
}

void WtDataWriter::pushTask(TaskInfo task)
{
	if(_async_proc)
	{
		StdUniqueLock lck(_task_mtx);
		_tasks.push(task);
		_task_cond.notify_all();
	}
	else
	{
		task();
		return;
	}

	if(_task_thrd == NULL)
	{
		_task_thrd.reset(new StdThread([this](){
			while (!_terminated)
			{
				if(_tasks.empty())
				{
					StdUniqueLock lck(_task_mtx);
					_task_cond.wait(_task_mtx);
					continue;
				}

				std::queue<TaskInfo> tempQueue;
				{
					StdUniqueLock lck(_task_mtx);
					tempQueue.swap(_tasks);
				}

				while(!tempQueue.empty())
				{
					TaskInfo& curTask = tempQueue.front();
					curTask();
					tempQueue.pop();
				}
			}
		}));
	}
}

bool WtDataWriter::writeOrderDetail(WTSOrdDtlData* curOrdDtl)
{
	if (curOrdDtl == NULL || _disable_orddtl)
		return false;

	curOrdDtl->retain();
	pushTask([this, curOrdDtl](){

		do
		{

			WTSContractInfo* ct = _bd_mgr->getContract(curOrdDtl->code(), curOrdDtl->exchg());
			WTSCommodityInfo* commInfo = _bd_mgr->getCommodity(ct);

			//�ٸ���״̬����
			if (!_sink->canSessionReceive(commInfo->getSession()))
				break;

			OrdDtlBlockPair* pBlockPair = getOrdDtlBlock(ct, curOrdDtl->tradingdate());
			if (pBlockPair == NULL)
				break;

			StdUniqueLock lock(pBlockPair->_mutex);

			//�ȼ������������,����Ҫ��
			RTOrdDtlBlock* blk = pBlockPair->_block;
			if (blk->_size >= blk->_capacity)
			{
				pBlockPair->_file->sync();
				pBlockPair->_block = (RTOrdDtlBlock*)resizeRTBlock<RTDayBlockHeader, WTSOrdDtlStruct>(pBlockPair->_file, blk->_capacity + TICK_SIZE_STEP);
				blk = pBlockPair->_block;
			}

			memcpy(&blk->_details[blk->_size], &curOrdDtl->getOrdDtlStruct(), sizeof(WTSOrdDtlStruct));
			blk->_size += 1;

			//TODO: Ҫ�㲥��
			//g_udpCaster.broadcast(curTrans);

			static faster_hashmap<std::string, uint64_t> _tcnt_map;
			_tcnt_map[curOrdDtl->exchg()]++;
			if (_tcnt_map[curOrdDtl->exchg()] % _log_group_size == 0)
			{
				//_sink->outputWriterLog(LL_INFO, "���յ�������%s�����ί������%s��", curOrdDtl->exchg(), StrUtil::fmtUInt64(_tcnt_map[curOrdDtl->exchg()]).c_str());
				_sink->outputWriterLog(LL_INFO, "%s orderdetails received from exchange %s", StrUtil::fmtUInt64(_tcnt_map[curOrdDtl->exchg()]).c_str(), curOrdDtl->exchg());
			}
		} while (false);

		curOrdDtl->release();
	});
	
	return true;
}

bool WtDataWriter::writeTransaction(WTSTransData* curTrans)
{
	if (curTrans == NULL || _disable_trans)
		return false;

	curTrans->retain();
	pushTask([this, curTrans](){

		do
		{

			WTSContractInfo* ct = _bd_mgr->getContract(curTrans->code(), curTrans->exchg());
			WTSCommodityInfo* commInfo = _bd_mgr->getCommodity(ct);

			//�ٸ���״̬����
			if (!_sink->canSessionReceive(commInfo->getSession()))
				break;

			TransBlockPair* pBlockPair = getTransBlock(ct, curTrans->tradingdate());
			if (pBlockPair == NULL)
				break;

			StdUniqueLock lock(pBlockPair->_mutex);

			//�ȼ������������,����Ҫ��
			RTTransBlock* blk = pBlockPair->_block;
			if (blk->_size >= blk->_capacity)
			{
				pBlockPair->_file->sync();
				pBlockPair->_block = (RTTransBlock*)resizeRTBlock<RTDayBlockHeader, WTSTransStruct>(pBlockPair->_file, blk->_capacity + TICK_SIZE_STEP);
				blk = pBlockPair->_block;
			}

			memcpy(&blk->_trans[blk->_size], &curTrans->getTransStruct(), sizeof(WTSTransStruct));
			blk->_size += 1;

			//TODO: Ҫ�㲥��
			//g_udpCaster.broadcast(curTrans);

			static faster_hashmap<std::string, uint64_t> _tcnt_map;
			_tcnt_map[curTrans->exchg()]++;
			if (_tcnt_map[curTrans->exchg()] % _log_group_size == 0)
			{
				//_sink->outputWriterLog(LL_INFO, "���յ�������%s����ʳɽ�����%s��", curTrans->exchg(), StrUtil::fmtUInt64(_tcnt_map[curTrans->exchg()]).c_str());
				_sink->outputWriterLog(LL_INFO, "%s transactions received from exchange %s", StrUtil::fmtUInt64(_tcnt_map[curTrans->exchg()]).c_str(), curTrans->exchg());
			}
		} while (false);

		curTrans->release();
	});
	return true;
}

void WtDataWriter::pipeToTicks(WTSContractInfo* ct, WTSTickData* curTick)
{
	TickBlockPair* pBlockPair = getTickBlock(ct, curTick->tradingdate());
	if (pBlockPair == NULL)
		return;

	StdUniqueLock lock(pBlockPair->_mutex);

	//�ȼ������������,����Ҫ��
	RTTickBlock* blk = pBlockPair->_block;
	if(blk->_size >= blk->_capacity)
	{
		pBlockPair->_file->sync();
		pBlockPair->_block = (RTTickBlock*)resizeRTBlock<RTDayBlockHeader, WTSTickStruct>(pBlockPair->_file, blk->_capacity + TICK_SIZE_STEP);
		blk = pBlockPair->_block;
	}

	memcpy(&blk->_ticks[blk->_size], &curTick->getTickStruct(), sizeof(WTSTickStruct));
	blk->_size += 1;

	if(_save_tick_log && pBlockPair->_fstream)
	{
		*(pBlockPair->_fstream) << curTick->code() << ","
			<< curTick->tradingdate() << ","
			<< curTick->actiondate() << ","
			<< curTick->actiontime() << ","
			<< TimeUtils::getLocalTime(false) << ","
			<< curTick->price() << ","
			<< curTick->totalvolume() << ","
			<< curTick->openinterest() << ","
			<< (uint64_t)curTick->totalturnover() << ","
			<< curTick->volume() << ","
			<< curTick->additional() << ","
			<< (uint64_t)curTick->turnover() << std::endl;
	}
}

WtDataWriter::OrdQueBlockPair* WtDataWriter::getOrdQueBlock(WTSContractInfo* ct, uint32_t curDate, bool bAutoCreate /* = true */)
{
	if (ct == NULL)
		return NULL;

	OrdQueBlockPair* pBlock = NULL;
	std::string key = StrUtil::printf("%s.%s", ct->getExchg(), ct->getCode());
	pBlock = _rt_ordque_blocks[key];
	if(pBlock == NULL)
	{
		pBlock = new OrdQueBlockPair();
		_rt_ordque_blocks[key] = pBlock;
	}

	if (pBlock->_block == NULL)
	{
		std::string path = StrUtil::printf("%srt/queue/%s/", _base_dir.c_str(), ct->getExchg());
		BoostFile::create_directories(path.c_str());
		path += ct->getCode();
		path += ".dmb";

		bool isNew = false;
		if (!BoostFile::exists(path.c_str()))
		{
			if (!bAutoCreate)
				return NULL;

			_sink->outputWriterLog(LL_INFO, "Data file %s not exists, initializing...", path.c_str());

			uint64_t uSize = sizeof(RTDayBlockHeader) + sizeof(WTSOrdQueStruct) * TICK_SIZE_STEP;

			BoostFile bf;
			bf.create_new_file(path.c_str());
			bf.truncate_file((uint32_t)uSize);
			bf.close_file();

			isNew = true;
		}

		//_sink->outputWriterLog(LL_INFO, "%s��ʼӳ��", path.c_str());
		pBlock->_file.reset(new BoostMappingFile);
		if (!pBlock->_file->map(path.c_str()))
		{
			_sink->outputWriterLog(LL_INFO, "Mapping file %s failed", path.c_str());
			pBlock->_file.reset();
			return NULL;
		}
		pBlock->_block = (RTOrdQueBlock*)pBlock->_file->addr();

		//_sink->outputWriterLog(LL_INFO, "%sӳ��ɹ�", path.c_str());

		if (!isNew &&  pBlock->_block->_date != curDate)
		{
			//_sink->outputWriterLog(LL_INFO, "orderqueue����ģ��%s����%u�����ڵ�ǰ����%u,���³�ʼ��", path.c_str(), pBlock->_block->_date, curDate);
			_sink->outputWriterLog(LL_INFO, "date[%u] of orderqueue cache block[%s] is different from current date[%u], reinitializing...", pBlock->_block->_date, path.c_str(), curDate);
			pBlock->_block->_size = 0;
			pBlock->_block->_date = curDate;

			memset(&pBlock->_block->_queues, 0, sizeof(WTSOrdQueStruct)*pBlock->_block->_capacity);
		}

		if (isNew)
		{
			pBlock->_block->_capacity = TICK_SIZE_STEP;
			pBlock->_block->_size = 0;
			pBlock->_block->_version = BLOCK_VERSION_RAW;
			pBlock->_block->_type = BT_RT_OrdQueue;
			pBlock->_block->_date = curDate;
			strcpy(pBlock->_block->_blk_flag, BLK_FLAG);
		}
		else
		{
			//��黺���ļ��Ƿ�������,Ҫ�Զ��ָ�
			do
			{
				uint64_t uSize = sizeof(RTDayBlockHeader) + sizeof(WTSOrdQueStruct) * pBlock->_block->_capacity;
				uint64_t oldSize = pBlock->_file->size();
				if (oldSize != uSize)
				{
					uint32_t oldCnt = (uint32_t)((oldSize - sizeof(RTDayBlockHeader)) / sizeof(WTSOrdQueStruct));
					//�ļ���С��ƥ��,һ������Ϊcapacity����,����ʵ��û����
					//������һ�����ݼ���
					pBlock->_block->_capacity = oldCnt;
					pBlock->_block->_size = oldCnt;

					//_sink->outputWriterLog(LL_WARN, "%s��Ʊ%u��ί�ж��л����ļ����޸�", ct->getCode(), curDate);
					_sink->outputWriterLog(LL_WARN, "Oderqueue cache file of %s on date %u repaired", ct->getCode(), curDate);
				}

			} while (false);

		}
	}

	pBlock->_lasttime = time(NULL);
	return pBlock;
}

WtDataWriter::OrdDtlBlockPair* WtDataWriter::getOrdDtlBlock(WTSContractInfo* ct, uint32_t curDate, bool bAutoCreate /* = true */)
{
	if (ct == NULL)
		return NULL;

	OrdDtlBlockPair* pBlock = NULL;
	std::string key = StrUtil::printf("%s.%s", ct->getExchg(), ct->getCode());
	pBlock = _rt_orddtl_blocks[key];
	if (pBlock == NULL)
	{
		pBlock = new OrdDtlBlockPair();
		_rt_orddtl_blocks[key] = pBlock;
	}

	if (pBlock->_block == NULL)
	{
		std::string path = StrUtil::printf("%srt/orders/%s/", _base_dir.c_str(), ct->getExchg());
		BoostFile::create_directories(path.c_str());
		path += ct->getCode();
		path += ".dmb";

		bool isNew = false;
		if (!BoostFile::exists(path.c_str()))
		{
			if (!bAutoCreate)
				return NULL;

			//_sink->outputWriterLog(LL_INFO, "�����ļ�%s������,���ڳ�ʼ��...", path.c_str());
			_sink->outputWriterLog(LL_INFO, "Data file %s not exists, initializing...", path.c_str());

			uint64_t uSize = sizeof(RTDayBlockHeader) + sizeof(WTSOrdDtlStruct) * TICK_SIZE_STEP;

			BoostFile bf;
			bf.create_new_file(path.c_str());
			bf.truncate_file((uint32_t)uSize);
			bf.close_file();

			isNew = true;
		}

		//_sink->outputWriterLog(LL_INFO, "%s��ʼӳ��", path.c_str());
		pBlock->_file.reset(new BoostMappingFile);
		if (!pBlock->_file->map(path.c_str()))
		{
			_sink->outputWriterLog(LL_INFO, "Mapping file %s failed", path.c_str());
			pBlock->_file.reset();
			return NULL;
		}
		pBlock->_block = (RTOrdDtlBlock*)pBlock->_file->addr();

		//_sink->outputWriterLog(LL_INFO, "%sӳ��ɹ�", path.c_str());

		if (!isNew &&  pBlock->_block->_date != curDate)
		{
			//_sink->outputWriterLog(LL_INFO, "orders����ģ��%s����%u�����ڵ�ǰ����%u,���³�ʼ��", path.c_str(), pBlock->_block->_date, curDate);
			_sink->outputWriterLog(LL_INFO, "date[%u] of orderdetail cache block[%s] is different from current date[%u], reinitializing...", pBlock->_block->_date, path.c_str(), curDate);
			pBlock->_block->_size = 0;
			pBlock->_block->_date = curDate;

			memset(&pBlock->_block->_details, 0, sizeof(WTSOrdDtlStruct)*pBlock->_block->_capacity);
		}

		if (isNew)
		{
			pBlock->_block->_capacity = TICK_SIZE_STEP;
			pBlock->_block->_size = 0;
			pBlock->_block->_version = BLOCK_VERSION_RAW;
			pBlock->_block->_type = BT_RT_OrdDetail;
			pBlock->_block->_date = curDate;
			strcpy(pBlock->_block->_blk_flag, BLK_FLAG);
		}
		else
		{
			//��黺���ļ��Ƿ�������,Ҫ�Զ��ָ�
			for (;;)
			{
				uint64_t uSize = sizeof(RTDayBlockHeader) + sizeof(WTSOrdDtlStruct) * pBlock->_block->_capacity;
				uint64_t oldSize = pBlock->_file->size();
				if (oldSize != uSize)
				{
					uint32_t oldCnt = (uint32_t)((oldSize - sizeof(RTDayBlockHeader)) / sizeof(WTSOrdDtlStruct));
					//�ļ���С��ƥ��,һ������Ϊcapacity����,����ʵ��û����
					//������һ�����ݼ���
					pBlock->_block->_capacity = oldCnt;
					pBlock->_block->_size = oldCnt;

					//_sink->outputWriterLog(LL_WARN, "%s��Ʊ%u����ʳɽ������ļ����޸�", ct->getCode(), curDate);
					_sink->outputWriterLog(LL_WARN, "Orderdetail cache file of %s on date %u repaired", ct->getCode(), curDate);
				}

				break;
			}

		}
	}

	pBlock->_lasttime = time(NULL);
	return pBlock;
}

WtDataWriter::TransBlockPair* WtDataWriter::getTransBlock(WTSContractInfo* ct, uint32_t curDate, bool bAutoCreate /* = true */)
{
	if (ct == NULL)
		return NULL;

	TransBlockPair* pBlock = NULL;
	std::string key = StrUtil::printf("%s.%s", ct->getExchg(), ct->getCode());
	pBlock = _rt_trans_blocks[key];
	if (pBlock == NULL)
	{
		pBlock = new TransBlockPair();
		_rt_trans_blocks[key] = pBlock;
	}

	if (pBlock->_block == NULL)
	{
		std::string path = StrUtil::printf("%srt/trans/%s/", _base_dir.c_str(), ct->getExchg());
		BoostFile::create_directories(path.c_str());
		path += ct->getCode();
		path += ".dmb";

		bool isNew = false;
		if (!BoostFile::exists(path.c_str()))
		{
			if (!bAutoCreate)
				return NULL;

			_sink->outputWriterLog(LL_INFO, "Data file %s not exists, initializing...", path.c_str());

			uint64_t uSize = sizeof(RTDayBlockHeader) + sizeof(WTSTransStruct) * TICK_SIZE_STEP;

			BoostFile bf;
			bf.create_new_file(path.c_str());
			bf.truncate_file((uint32_t)uSize);
			bf.close_file();

			isNew = true;
		}

		//_sink->outputWriterLog(LL_INFO, "%s��ʼӳ��", path.c_str());
		pBlock->_file.reset(new BoostMappingFile);
		if (!pBlock->_file->map(path.c_str()))
		{
			_sink->outputWriterLog(LL_INFO, "Mapping file %s failed", path.c_str());
			pBlock->_file.reset();
			return NULL;
		}
		pBlock->_block = (RTTransBlock*)pBlock->_file->addr();

		//_sink->outputWriterLog(LL_INFO, "%sӳ��ɹ�", path.c_str());

		if (!isNew &&  pBlock->_block->_date != curDate)
		{
			//_sink->outputWriterLog(LL_INFO, "trans����ģ��%s����%u�����ڵ�ǰ����%u,���³�ʼ��", path.c_str(), pBlock->_block->_date, curDate);
			_sink->outputWriterLog(LL_INFO, "date[%u] of transaction cache block[%s] is different from current date[%u], reinitializing...", pBlock->_block->_date, path.c_str(), curDate);
			pBlock->_block->_size = 0;
			pBlock->_block->_date = curDate;

			memset(&pBlock->_block->_trans, 0, sizeof(WTSTransStruct)*pBlock->_block->_capacity);
		}

		if (isNew)
		{
			pBlock->_block->_capacity = TICK_SIZE_STEP;
			pBlock->_block->_size = 0;
			pBlock->_block->_version = BLOCK_VERSION_RAW;
			pBlock->_block->_type = BT_RT_Trnsctn;
			pBlock->_block->_date = curDate;
			strcpy(pBlock->_block->_blk_flag, BLK_FLAG);
		}
		else
		{
			//��黺���ļ��Ƿ�������,Ҫ�Զ��ָ�
			for (;;)
			{
				uint64_t uSize = sizeof(RTDayBlockHeader) + sizeof(WTSTransStruct) * pBlock->_block->_capacity;
				uint64_t oldSize = pBlock->_file->size();
				if (oldSize != uSize)
				{
					uint32_t oldCnt = (uint32_t)((oldSize - sizeof(RTDayBlockHeader)) / sizeof(WTSTransStruct));
					//�ļ���С��ƥ��,һ������Ϊcapacity����,����ʵ��û����
					//������һ�����ݼ���
					pBlock->_block->_capacity = oldCnt;
					pBlock->_block->_size = oldCnt;

					//_sink->outputWriterLog(LL_WARN, "%s��Ʊ%u����ʳɽ������ļ����޸�", ct->getCode(), curDate);
					_sink->outputWriterLog(LL_WARN, "Transaction cache file of %s on date %u repaired", ct->getCode(), curDate);
				}

				break;
			}

		}
	}

	pBlock->_lasttime = time(NULL);
	return pBlock;
}

WtDataWriter::TickBlockPair* WtDataWriter::getTickBlock(WTSContractInfo* ct, uint32_t curDate, bool bAutoCreate /* = true */)
{
	if (ct == NULL)
		return NULL;

	TickBlockPair* pBlock = NULL;
	std::string key = StrUtil::printf("%s.%s", ct->getExchg(), ct->getCode());
	pBlock = _rt_ticks_blocks[key];
	if (pBlock == NULL)
	{
		pBlock = new TickBlockPair();
		_rt_ticks_blocks[key] = pBlock;
	}

	if(pBlock->_block == NULL)
	{
		std::string path = StrUtil::printf("%srt/ticks/%s/", _base_dir.c_str(), ct->getExchg());
		BoostFile::create_directories(path.c_str());

		if(_save_tick_log)
		{
			std::stringstream fname;
			fname << path << ct->getCode() << "." << curDate << ".csv";
			pBlock->_fstream.reset(new std::ofstream());
			pBlock->_fstream->open(fname.str().c_str(), std::ios_base::app);
		}

		path += ct->getCode();
		path += ".dmb";

		bool isNew = false;
		if (!BoostFile::exists(path.c_str()))
		{
			if (!bAutoCreate)
				return NULL;

			//_sink->outputWriterLog(LL_INFO, "�����ļ�%s������,���ڳ�ʼ��...", path.c_str());
			_sink->outputWriterLog(LL_INFO, "Data file %s not exists, initializing...", path.c_str());

			uint64_t uSize = sizeof(RTTickBlock) + sizeof(WTSTickStruct) * TICK_SIZE_STEP;
			BoostFile bf;
			bf.create_new_file(path.c_str());
			bf.truncate_file((uint32_t)uSize);
			bf.close_file();

			isNew = true;
		}

		//_sink->outputWriterLog(LL_INFO, "%s��ʼӳ��", path.c_str());
		pBlock->_file.reset(new BoostMappingFile);
		if(!pBlock->_file->map(path.c_str()))
		{
			//_sink->outputWriterLog(LL_INFO, "�ļ�%sӳ��ʧ��", path.c_str());
			_sink->outputWriterLog(LL_INFO, "Mapping file %s failed", path.c_str());
			pBlock->_file.reset();
			return NULL;
		}
		pBlock->_block = (RTTickBlock*)pBlock->_file->addr();

		//_sink->outputWriterLog(LL_INFO, "%sӳ��ɹ�", path.c_str());

		if (!isNew &&  pBlock->_block->_date != curDate)
		{
			//_sink->outputWriterLog(LL_INFO, "tick����ģ��%s����%u�����ڵ�ǰ����%u,���³�ʼ��", path.c_str(), pBlock->_block->_date, curDate);
			_sink->outputWriterLog(LL_INFO, "date[%u] of tick cache block[%s] is different from current date[%u], reinitializing...", pBlock->_block->_date, path.c_str(), curDate);
			pBlock->_block->_size = 0;
			pBlock->_block->_date = curDate;

			memset(&pBlock->_block->_ticks, 0, sizeof(WTSTickStruct)*pBlock->_block->_capacity);
		}

		if(isNew)
		{
			pBlock->_block->_capacity = TICK_SIZE_STEP;
			pBlock->_block->_size = 0;
			pBlock->_block->_version = BLOCK_VERSION_RAW;
			pBlock->_block->_type = BT_RT_Ticks;
			pBlock->_block->_date = curDate;
			strcpy(pBlock->_block->_blk_flag, BLK_FLAG);
		}
		else
		{
			//��黺���ļ��Ƿ�������,Ҫ�Զ��ָ�
			for (;;)
			{
				uint64_t uSize = sizeof(RTTickBlock) + sizeof(WTSTickStruct) * pBlock->_block->_capacity;
				uint64_t oldSize = pBlock->_file->size();
				if (oldSize != uSize)
				{
					uint32_t oldCnt = (uint32_t)((oldSize - sizeof(RTTickBlock)) / sizeof(WTSTickStruct));
					//�ļ���С��ƥ��,һ������Ϊcapacity����,����ʵ��û����
					//������һ�����ݼ���
					pBlock->_block->_capacity = oldCnt;
					pBlock->_block->_size = oldCnt;

					//_sink->outputWriterLog(LL_WARN, "%s����Ϊ%u��tick�����ļ����޸�", ct->getCode(), curDate);
					_sink->outputWriterLog(LL_WARN, "Tick cache file of %s on date %u repaired", ct->getCode(), curDate);
				}
				
				break;
			}
			
		}
	}

	pBlock->_lasttime = time(NULL);
	return pBlock;
}

void WtDataWriter::pipeToKlines(WTSContractInfo* ct, WTSTickData* curTick)
{
	uint32_t uDate = curTick->actiondate();
	WTSSessionInfo* sInfo = _bd_mgr->getSessionByCode(curTick->code(), curTick->exchg());
	uint32_t curTime = curTick->actiontime() / 100000;

	uint32_t minutes = sInfo->timeToMinutes(curTime, false);
	if (minutes == INVALID_UINT32)
		return;

	//������Ϊ0,Ҫר�Ŵ���,����091500000,���tickҪ����0915��
	//�����С�ڽ���,Ҫ����С�ڽ�����һ����,��Ϊ�������г�������ʱ��ļ۸����,��113000500
	//����ͬʱ����,������or	
	if (sInfo->isLastOfSection(curTime))
	{
		minutes--;
	}

	//����1������
	if (!_disable_min1)
	{
		KBlockPair* pBlockPair = getKlineBlock(ct, KP_Minute1);
		if (pBlockPair && pBlockPair->_block)
		{
			StdUniqueLock lock(pBlockPair->_mutex);
			RTKlineBlock* blk = pBlockPair->_block;
			if (blk->_size == blk->_capacity)
			{
				pBlockPair->_file->sync();
				pBlockPair->_block = (RTKlineBlock*)resizeRTBlock<RTKlineBlock, WTSBarStruct>(pBlockPair->_file, blk->_capacity + KLINE_SIZE_STEP);
				blk = pBlockPair->_block;
			}

			WTSBarStruct* lastBar = NULL;
			if (blk->_size > 0)
			{
				lastBar = &blk->_bars[blk->_size - 1];
			}

			//ƴ��1������
			uint32_t barMins = minutes + 1;
			uint32_t barTime = sInfo->minuteToTime(barMins);
			uint32_t barDate = uDate;
			if (barTime == 0)
			{
				barDate = TimeUtils::getNextDate(barDate);
			}
			barTime = TimeUtils::timeToMinBar(barDate, barTime);

			bool bNew = false;
			if (lastBar == NULL || barTime > lastBar->time)
			{
				bNew = true;
			}

			WTSBarStruct* newBar = NULL;
			if (bNew)
			{
				newBar = &blk->_bars[blk->_size];
				blk->_size += 1;

				newBar->date = curTick->tradingdate();
				newBar->time = barTime;
				newBar->open = curTick->price();
				newBar->high = curTick->price();
				newBar->low = curTick->price();
				newBar->close = curTick->price();

				newBar->vol = curTick->volume();
				newBar->money = curTick->turnover();
				newBar->hold = curTick->openinterest();
				newBar->add = curTick->additional();
			}
			else
			{
				newBar = &blk->_bars[blk->_size - 1];

				newBar->close = curTick->price();
				newBar->high = max(curTick->price(), newBar->high);
				newBar->low = min(curTick->price(), newBar->low);

				newBar->vol += curTick->volume();
				newBar->money += curTick->turnover();
				newBar->hold = curTick->openinterest();
				newBar->add += curTick->additional();
			}
		}
	}

	//����5������
	if (!_disable_min5)
	{
		KBlockPair* pBlockPair = getKlineBlock(ct, KP_Minute5);
		if (pBlockPair && pBlockPair->_block)
		{
			StdUniqueLock lock(pBlockPair->_mutex);
			RTKlineBlock* blk = pBlockPair->_block;
			if (blk->_size == blk->_capacity)
			{
				pBlockPair->_file->sync();
				pBlockPair->_block = (RTKlineBlock*)resizeRTBlock<RTKlineBlock, WTSBarStruct>(pBlockPair->_file, blk->_capacity + KLINE_SIZE_STEP);
				blk = pBlockPair->_block;
			}

			WTSBarStruct* lastBar = NULL;
			if (blk->_size > 0)
			{
				lastBar = &blk->_bars[blk->_size - 1];
			}

			uint32_t barMins = (minutes / 5) * 5 + 5;
			uint32_t barTime = sInfo->minuteToTime(barMins);
			uint32_t barDate = uDate;
			if (barTime == 0)
			{
				barDate = TimeUtils::getNextDate(barDate);
			}
			barTime = TimeUtils::timeToMinBar(barDate, barTime);

			bool bNew = false;
			if (lastBar == NULL || barTime > lastBar->time)
			{
				bNew = true;
			}

			WTSBarStruct* newBar = NULL;
			if (bNew)
			{
				newBar = &blk->_bars[blk->_size];
				blk->_size += 1;

				newBar->date = curTick->tradingdate();
				newBar->time = barTime;
				newBar->open = curTick->price();
				newBar->high = curTick->price();
				newBar->low = curTick->price();
				newBar->close = curTick->price();

				newBar->vol = curTick->volume();
				newBar->money = curTick->turnover();
				newBar->hold = curTick->openinterest();
				newBar->add = curTick->additional();
			}
			else
			{
				newBar = &blk->_bars[blk->_size - 1];

				newBar->close = curTick->price();
				newBar->high = max(curTick->price(), newBar->high);
				newBar->low = min(curTick->price(), newBar->low);

				newBar->vol += curTick->volume();
				newBar->money += curTick->turnover();
				newBar->hold = curTick->openinterest();
				newBar->add += curTick->additional();
			}
		}
	}
}

template<typename T>
void WtDataWriter::releaseBlock(T* block)
{
	if (block == NULL || block->_file == NULL)
		return;

	StdUniqueLock lock(block->_mutex);
	block->_block = NULL;
	block->_file.reset();
	block->_lasttime = 0;
}

WtDataWriter::KBlockPair* WtDataWriter::getKlineBlock(WTSContractInfo* ct, WTSKlinePeriod period, bool bAutoCreate /* = true */)
{
	if (ct == NULL)
		return NULL;

	KBlockPair* pBlock = NULL;
	std::string key = StrUtil::printf("%s.%s", ct->getExchg(), ct->getCode());

	KBlockFilesMap* cache_map = NULL;
	std::string subdir = "";
	BlockType bType;
	switch(period)
	{
	case KP_Minute1: 
		cache_map = &_rt_min1_blocks; 
		subdir = "min1";
		bType = BT_RT_Minute1;
		break;
	case KP_Minute5: 
		cache_map = &_rt_min5_blocks;
		subdir = "min5";
		bType = BT_RT_Minute5;
		break;
	default: break;
	}

	if (cache_map == NULL)
		return NULL;

	pBlock = (*cache_map)[key];
	if (pBlock == NULL)
	{
		pBlock = new KBlockPair();
		(*cache_map)[key] = pBlock;
	}

	if (pBlock->_block == NULL)
	{
		std::string path = StrUtil::printf("%srt/%s/%s/", _base_dir.c_str(), subdir.c_str(), ct->getExchg());
		BoostFile::create_directories(path.c_str());

		path += ct->getCode();
		path += ".dmb";

		bool isNew = false;
		if (!BoostFile::exists(path.c_str()))
		{
			if (!bAutoCreate)
				return NULL;

			//_sink->outputWriterLog(LL_INFO, "�����ļ�%s������,���ڳ�ʼ��...", path.c_str());
			_sink->outputWriterLog(LL_INFO, "Data file %s not exists, initializing...", path.c_str());

			uint64_t uSize = sizeof(RTKlineBlock) + sizeof(WTSBarStruct) * KLINE_SIZE_STEP;
			BoostFile bf;
			bf.create_new_file(path.c_str());
			bf.truncate_file((uint32_t)uSize);
			bf.close_file();

			isNew = true;
		}

		pBlock->_file.reset(new BoostMappingFile);
		if(pBlock->_file->map(path.c_str()))
		{
			pBlock->_block = (RTKlineBlock*)pBlock->_file->addr();
		}
		else
		{
			_sink->outputWriterLog(LL_ERROR, "Mapping file %s failed", path.c_str());
			pBlock->_file.reset();
			return NULL;
		}

		//if(pBlock->_block->_date != uDate)
		//{
		//	pBlock->_block->_size = 0;
		//	pBlock->_block->_date = uDate;

		//	memset(&pBlock->_block->_bars, 0, sizeof(WTSBarStruct)*pBlock->_block->_capacity);
		//}

		if (isNew)
		{
			//memset(pBlock->_block, 0, pBlock->_file->size());
			pBlock->_block->_capacity = KLINE_SIZE_STEP;
			pBlock->_block->_size = 0;
			pBlock->_block->_version = BLOCK_VERSION_RAW;
			pBlock->_block->_type = bType;
			pBlock->_block->_date = TimeUtils::getCurDate();
			strcpy(pBlock->_block->_blk_flag, BLK_FLAG);
		}
	}

	pBlock->_lasttime = time(NULL);
	return pBlock;
}

WTSTickData* WtDataWriter::getCurTick(const char* code, const char* exchg/* = ""*/)
{
	if (strlen(code) == 0)
		return NULL;

	WTSContractInfo* ct = _bd_mgr->getContract(code, exchg);
	if (ct == NULL)
		return NULL;

	std::string key = StrUtil::printf("%s.%s", ct->getExchg(), ct->getCode());
	StdUniqueLock lock(_mtx_tick_cache);
	auto it = _tick_cache_idx.find(key);
	if (it == _tick_cache_idx.end())
		return NULL;

	uint32_t idx = it->second;
	TickCacheItem& item = _tick_cache_block->_ticks[idx];
	return WTSTickData::create(item._tick);
}

bool WtDataWriter::updateCache(WTSContractInfo* ct, WTSTickData* curTick, bool bNeedSlice /* = true */)
{
	if (curTick == NULL || _tick_cache_block == NULL)
	{
		_sink->outputWriterLog(LL_ERROR, "Tick cache data not initialized");
		return false;
	}

	StdUniqueLock lock(_mtx_tick_cache);
	std::string key = StrUtil::printf("%s.%s", curTick->exchg(), curTick->code());
	uint32_t idx = 0;
	if (_tick_cache_idx.find(key) == _tick_cache_idx.end())
	{
		idx = _tick_cache_block->_size;
		_tick_cache_idx[key] = _tick_cache_block->_size;
		_tick_cache_block->_size += 1;
		if(_tick_cache_block->_size >= _tick_cache_block->_capacity)
		{
			_tick_cache_block = (RTTickCache*)resizeRTBlock<RTTickCache, TickCacheItem>(_tick_cache_file, _tick_cache_block->_capacity + CACHE_SIZE_STEP);
			_sink->outputWriterLog(LL_INFO, "Tick Cache resized to %u items", _tick_cache_block->_capacity);
		}
	}
	else
	{
		idx = _tick_cache_idx[key];
	}


	TickCacheItem& item = _tick_cache_block->_ticks[idx];
	if (curTick->tradingdate() < item._date)
	{
		//_sink->outputWriterLog(LL_INFO, "%s��������%uС�ڻ��潻����%u", curTick->tradingdate(), item._date);
		_sink->outputWriterLog(LL_INFO, "Tradingday[%u] of %s is less than cached tradingday[%u]", curTick->tradingdate(), curTick->code(), item._date);
		return false;
	}

	WTSTickStruct& newTick = curTick->getTickStruct();

	if (curTick->tradingdate() > item._date)
	{
		//�����ݽ����մ���������,����Ϊ����һ�������
		item._date = curTick->tradingdate();
		memcpy(&item._tick, &newTick, sizeof(WTSTickStruct));
		if (bNeedSlice)
		{
			item._tick.volume = item._tick.total_volume;
			item._tick.turn_over = item._tick.total_turnover;
			item._tick.diff_interest = item._tick.open_interest - item._tick.pre_interest;

			newTick.volume = newTick.total_volume;
			newTick.turn_over = newTick.total_turnover;
			newTick.diff_interest = newTick.open_interest - newTick.pre_interest;
		}

		//_sink->outputWriterLog(LL_INFO, "�½�����%u�ĵ�һ������,%s.%s,%u,%f,%d,%d",
		//	newTick.trading_date, curTick->exchg(), curTick->code(), curTick->volume(),
		//	curTick->turnover(), curTick->openinterest(), curTick->additional());
		_sink->outputWriterLog(LL_INFO, "First tick of new tradingday %u,%s.%s,%f,%u,%f,%u,%d", 
			newTick.trading_date, curTick->exchg(), curTick->code(), curTick->price(), curTick->volume(),
			curTick->turnover(), curTick->openinterest(), curTick->additional());
	}
	else
	{
		//�����������������ڴ����������������
		//���߻������ʱ����ڵ������������ʱ��,���ݾͲ���Ҫ����
		WTSSessionInfo* sInfo = _bd_mgr->getSessionByCode(curTick->code(), curTick->exchg());
		uint32_t tdate = sInfo->getOffsetDate(curTick->actiondate(), curTick->actiontime() / 100000);
		if (tdate > curTick->tradingdate())
		{
			//_sink->outputWriterLog(LL_ERROR, "%s.%s����tick����(ʱ��%u.%u)�쳣,����", curTick->exchg(), curTick->code(), curTick->actiondate(), curTick->actiontime());
			_sink->outputWriterLog(LL_ERROR, "Last tick of %s.%s with time %u.%u has an exception, abandoned", curTick->exchg(), curTick->code(), curTick->actiondate(), curTick->actiontime());
			return false;
		}
		else if (curTick->totalvolume() < item._tick.total_volume)
		{
			_sink->outputWriterLog(LL_ERROR, "Last tick of %s.%s with time %u.%u, volume %u is less than cached volume %u, abandoned", 
				curTick->exchg(), curTick->code(), curTick->actiondate(), curTick->actiontime(), curTick->totalvolume(), item._tick.total_volume);
			return false;
		}

		//ʱ�����ͬ,���ǳɽ������ڵ���ԭ����,�������һ����֣����,����Ĵ���ʽ����ʱ���+500����
		if(newTick.action_date == item._tick.action_date && newTick.action_time == item._tick.action_time && newTick.total_volume >= item._tick.total_volume)
		{
			newTick.action_time += 500;
		}

		//�����Ҫ���費��ҪԤ������
		if(!bNeedSlice)
		{
			memcpy(&item._tick, &newTick, sizeof(WTSTickStruct));
		}
		else
		{
			newTick.volume = newTick.total_volume - item._tick.total_volume;
			newTick.turn_over = newTick.total_turnover - item._tick.total_turnover;
			newTick.diff_interest = newTick.open_interest - item._tick.open_interest;

			memcpy(&item._tick, &newTick, sizeof(WTSTickStruct));
		}
	}

	return true;
}

void WtDataWriter::transHisData(const char* sid)
{
	StdUniqueLock lock(_proc_mtx);
	if (strcmp(sid, CMD_CLEAR_CACHE) != 0)
	{
		CodeSet* pCommSet = _sink->getSessionComms(sid);
		if (pCommSet == NULL)
			return;

		for (auto it = pCommSet->begin(); it != pCommSet->end(); it++)
		{
			const std::string& key = *it;

			const StringVector& ay = StrUtil::split(key, ".");
			const char* exchg = ay[0].c_str();
			const char* pid = ay[1].c_str();

			WTSCommodityInfo* pCommInfo = _bd_mgr->getCommodity(exchg, pid);
			if (pCommInfo == NULL)
				continue;

			const CodeSet& codes = pCommInfo->getCodes();
			for (auto code : codes)
			{
				WTSContractInfo* ct = _bd_mgr->getContract(code.c_str(), exchg);
				if(ct)
					_proc_que.push(ct->getFullCode());
			}
		}

		_proc_que.push(StrUtil::printf("MARK.%s", sid));
	}
	else
	{
		_proc_que.push(sid);
	}

	if (_proc_thrd == NULL)
	{
		_proc_thrd.reset(new StdThread(boost::bind(&WtDataWriter::proc_loop, this)));
	}
	else
	{
		_proc_cond.notify_all();
	}
}

void WtDataWriter::check_loop()
{
	uint32_t expire_secs = 600;
	while(!_terminated)
	{
		std::this_thread::sleep_for(std::chrono::seconds(10));
		uint64_t now = time(NULL);
		for (auto it = _rt_ticks_blocks.begin(); it != _rt_ticks_blocks.end(); it++)
		{
			const std::string& key = it->first;
			TickBlockPair* tBlk = (TickBlockPair*)it->second;
			if (tBlk->_lasttime != 0 && (now - tBlk->_lasttime > expire_secs))
			{
				_sink->outputWriterLog(LL_INFO, "tick cache %s mapping expired, automatically closed", key.c_str());
				releaseBlock<TickBlockPair>(tBlk);
			}
		}

		for (auto it = _rt_trans_blocks.begin(); it != _rt_trans_blocks.end(); it++)
		{
			const std::string& key = it->first;
			TransBlockPair* tBlk = (TransBlockPair*)it->second;
			if (tBlk->_lasttime != 0 && (now - tBlk->_lasttime > expire_secs))
			{
				//_sink->outputWriterLog(LL_INFO, "trans���� %s ӳ�䳬ʱ,�Զ��ر�", key.c_str());
				_sink->outputWriterLog(LL_INFO, "trans cache %s mapping expired, automatically closed", key.c_str());
				releaseBlock<TransBlockPair>(tBlk);
			}
		}

		for (auto it = _rt_orddtl_blocks.begin(); it != _rt_orddtl_blocks.end(); it++)
		{
			const std::string& key = it->first;
			OrdDtlBlockPair* tBlk = (OrdDtlBlockPair*)it->second;
			if (tBlk->_lasttime != 0 && (now - tBlk->_lasttime > expire_secs))
			{
				//_sink->outputWriterLog(LL_INFO, "order���� %s ӳ�䳬ʱ,�Զ��ر�", key.c_str());
				_sink->outputWriterLog(LL_INFO, "order cache %s mapping expired, automatically closed", key.c_str());
				releaseBlock<OrdDtlBlockPair>(tBlk);
			}
		}

		for (auto& v : _rt_ordque_blocks)
		{
			const std::string& key = v.first;
			OrdQueBlockPair* tBlk = (OrdQueBlockPair*)v.second;
			if (tBlk->_lasttime != 0 && (now - tBlk->_lasttime > expire_secs))
			{
				//_sink->outputWriterLog(LL_INFO, "queue���� %s ӳ�䳬ʱ,�Զ��ر�", key.c_str());
				_sink->outputWriterLog(LL_INFO, "queue cache %s mapping expired, automatically closed", key.c_str());
				releaseBlock<OrdQueBlockPair>(tBlk);
			}
		}

		for (auto it = _rt_min1_blocks.begin(); it != _rt_min1_blocks.end(); it++)
		{
			const std::string& key = it->first;
			KBlockPair* kBlk = (KBlockPair*)it->second;
			if (kBlk->_lasttime != 0 && (now - kBlk->_lasttime > expire_secs))
			{
				//_sink->outputWriterLog(LL_INFO, "1���ӻ��� %s ӳ�䳬ʱ,�Զ��ر�", key.c_str());
				_sink->outputWriterLog(LL_INFO, "min1 cache %s mapping expired, automatically closed", key.c_str());
				releaseBlock<KBlockPair>(kBlk);
			}
		}

		for (auto it = _rt_min5_blocks.begin(); it != _rt_min5_blocks.end(); it++)
		{
			const std::string& key = it->first;
			KBlockPair* kBlk = (KBlockPair*)it->second;
			if (kBlk->_lasttime != 0 && (now - kBlk->_lasttime > expire_secs))
			{
				//_sink->outputWriterLog(LL_INFO, "5���ӻ��� %s ӳ�䳬ʱ,�Զ��ر�", key.c_str());
				_sink->outputWriterLog(LL_INFO, "min5 cache %s mapping expired, automatically closed", key.c_str());
				releaseBlock<KBlockPair>(kBlk);
			}
		}
	}
}

uint32_t WtDataWriter::dump_hisdata_to_db(WTSContractInfo* ct)
{
	if (ct == NULL)
		return 0;

	if (!_db_conf._active || _db_conn == NULL)
		return 0;

	std::string key = StrUtil::printf("%s.%s", ct->getExchg(), ct->getCode());

	uint32_t count = 0;

	MysqlDb& db = *_db_conn;
	MysqlTransaction trans(db);
	//�ӻ����ж�ȡ����tick,���µ���ʷ����
	auto it = _tick_cache_idx.find(key);
	if (it != _tick_cache_idx.end())
	{
		uint32_t idx = it->second;

		const TickCacheItem& tci = _tick_cache_block->_ticks[idx];
		const WTSTickStruct& ts = tci._tick;

		char sql[512] = { 0 };
		sprintf(sql, "REPLACE INTO tb_kline_day(exchange,code,date,open,high,low,close,settle,volume,turnover,interest,diff_interest) "
			"VALUES('%s','%s',%u,%f,%f,%f,%f,%f,%u,%f,%u,%d);", ct->getExchg(), ct->getCode(), ts.trading_date, ts.open, ts.high, ts.low, ts.price, 
			ts.settle_price, ts.total_volume, ts.total_turnover, ts.open_interest, ts.diff_interest);

		MysqlQuery query(db);
		if(!query.exec(sql))
		{
			_sink->outputWriterLog(LL_ERROR, "ClosingTask of day bar failed: %s", query.errormsg());
		}
		else
		{
			count++;
		}
	}

	//ת��ʵʱ1������
	KBlockPair* kBlkPair = getKlineBlock(ct, KP_Minute1, false);
	if (kBlkPair != NULL && kBlkPair->_block->_size > 0)
	{
		uint32_t size = kBlkPair->_block->_size;
		_sink->outputWriterLog(LL_INFO, "Transfering min1 bars of %s...", ct->getFullCode());
		StdUniqueLock lock(kBlkPair->_mutex);

		std::string sql = "REPLACE INTO tb_kline_min1(exchange,code,date,time,open,high,low,close,volume,turnover,interest,diff_interest) VALUES";
		for(uint32_t i = 0; i < size; i++)
		{
			const WTSBarStruct& bs = kBlkPair->_block->_bars[i];
			sql += StrUtil::printf("('%s','%s',%u,%u,%f,%f,%f,%f,%u,%f,%u,%d),", ct->getExchg(), ct->getCode(), bs.date, bs.time, bs.open,
				bs.high, bs.low, bs.close, bs.vol, bs.money, bs.hold, bs.add);
		}
		sql = sql.substr(0, sql.size() - 1);
		sql += ";";

		MysqlQuery query(db);
		if (!query.exec(sql))
		{
			_sink->outputWriterLog(LL_ERROR, "ClosingTask of min1 bar failed: %s", query.errormsg());
		}
		else
		{
			count += size;
			//��󽫻������
			kBlkPair->_block->_size = 0;
		}
	}

	if (kBlkPair)
		releaseBlock(kBlkPair);

	//���Ĳ�,ת��ʵʱ5������
	kBlkPair = getKlineBlock(ct, KP_Minute5, false);
	if (kBlkPair != NULL && kBlkPair->_block->_size > 0)
	{
		uint32_t size = kBlkPair->_block->_size;
		//_sink->outputWriterLog(LL_INFO, "��ʼת��%s��5��������", ct->getFullCode());
		_sink->outputWriterLog(LL_INFO, "Transfering min5 bars of %s...", ct->getFullCode());
		StdUniqueLock lock(kBlkPair->_mutex);

		std::string sql = "REPLACE INTO tb_kline_min5(exchange,code,date,time,open,high,low,close,volume,turnover,interest,diff_interest) VALUES";
		for (uint32_t i = 0; i < size; i++)
		{
			const WTSBarStruct& bs = kBlkPair->_block->_bars[i];
			sql += StrUtil::printf("('%s','%s',%u,%u,%f,%f,%f,%f,%u,%f,%u,%d),", ct->getExchg(), ct->getCode(), bs.date, bs.time, bs.open,
				bs.high, bs.low, bs.close, bs.vol, bs.money, bs.hold, bs.add);
		}
		sql = sql.substr(0, sql.size() - 1);
		sql += ";";

		MysqlQuery query(db);
		if (!query.exec(sql))
		{
			//_sink->outputWriterLog(LL_ERROR, "min5����������ҵʧ��: %s", query.errormsg());
			_sink->outputWriterLog(LL_ERROR, "ClosingTask of min5 bar failed: %s", query.errormsg());
		}
		else
		{
			count += size;
			//��󽫻������
			kBlkPair->_block->_size = 0;
		}
	}

	if (kBlkPair)
		releaseBlock(kBlkPair);

	trans.commit();

	return count;
}

uint32_t WtDataWriter::dump_hisdata_to_file(WTSContractInfo* ct)
{
	if (ct == NULL)
		return 0;

	std::string key = StrUtil::printf("%s.%s", ct->getExchg(), ct->getCode());

	uint32_t count = 0;

	//�ӻ����ж�ȡ����tick,���µ���ʷ����
	if (!_disable_day)
	{
		auto it = _tick_cache_idx.find(key);
		if (it != _tick_cache_idx.end())
		{
			uint32_t idx = it->second;

			const TickCacheItem& tci = _tick_cache_block->_ticks[idx];
			const WTSTickStruct& ts = tci._tick;

			WTSBarStruct bs;
			bs.date = ts.trading_date;
			bs.time = 0;
			bs.open = ts.open;
			bs.close = ts.price;
			bs.high = ts.high;
			bs.low = ts.low;
			bs.settle = ts.settle_price;
			bs.vol = ts.total_volume;
			bs.hold = ts.open_interest;
			bs.money = ts.total_turnover;
			bs.add = ts.open_interest - ts.pre_interest;

			std::stringstream ss;
			ss << _base_dir << "his/day/" << ct->getExchg() << "/";
			std::string path = ss.str();
			BoostFile::create_directories(ss.str().c_str());
			std::string filename = StrUtil::printf("%s%s.dsb", path.c_str(), ct->getCode());

			bool bNew = false;
			if (!BoostFile::exists(filename.c_str()))
				bNew = true;

			BoostFile f;
			if (f.create_or_open_file(filename.c_str()))
			{
				bool bNeedWrite = true;
				if (bNew)
				{
					BlockHeader header;
					strcpy(header._blk_flag, BLK_FLAG);
					header._type = BT_HIS_Day;
					header._version = BLOCK_VERSION_RAW;

					f.write_file(&header, sizeof(header));

					f.write_file(&bs, sizeof(WTSBarStruct));
					count++;
				}
				else
				{
					//���߱���Ҫ���һ��
					std::string content;
					BoostFile::read_file_contents(filename.c_str(), content);
					uint64_t flength = content.size();

					HisKlineBlock* kBlock = (HisKlineBlock*)content.data();
					if (strcmp(kBlock->_blk_flag, BLK_FLAG) != 0)
					{
						//_sink->outputWriterLog(LL_ERROR, "��ʷ�����ļ�%sͷ���쳣,����������ؽ�", filename.c_str());
						_sink->outputWriterLog(LL_ERROR, "File checking of history data file %s failed, clear and rebuild...", filename.c_str());
						f.truncate_file(0);
						BlockHeader header;
						strcpy(header._blk_flag, BLK_FLAG);
						header._type = BT_HIS_Day;
						header._version = BLOCK_VERSION_RAW;

						f.write_file(&header, sizeof(header));

						f.write_file(&bs, sizeof(WTSBarStruct));
						count++;
					}
					else
					{
						std::vector<WTSBarStruct>	bars;
						if (kBlock->_version == BLOCK_VERSION_RAW)	//����ϵ��ļ��Ƿ�ѹ���汾,��ֱ�Ӱ�K�߶���������ȥ
						{
							uint32_t barcnt = (uint32_t)(flength - BLOCK_HEADER_SIZE) / sizeof(WTSBarStruct);
							bars.resize(barcnt);
							memcpy(bars.data(), kBlock->_bars, (uint32_t)(flength - BLOCK_HEADER_SIZE));
						}
						else if (kBlock->_version == BLOCK_VERSION_CMP)	//ѹ���汾
						{
							//���ھ���Ҫ�����ݽ�ѹ�Ժ����������ȥ
							HisKlineBlockV2* kBlockV2 = (HisKlineBlockV2*)content.data();
							std::string rawData = WTSCmpHelper::uncompress_data(kBlockV2->_data, (uint32_t)kBlockV2->_size);
							uint32_t barcnt = rawData.size() / sizeof(WTSBarStruct);
							bars.resize(barcnt);
							memcpy(bars.data(), rawData.data(), rawData.size());
						}

						//��ʼ�Ƚ�K��ʱ���ǩ,��ҪΪ�˷�ֹ�����ظ�д
						if (!bars.empty())
						{
							WTSBarStruct& oldBS = bars.at(bars.size() - 1);	//��ȡ�����һ��K��

							if (oldBS.date == bs.date && memcmp(&oldBS, &bs, sizeof(WTSBarStruct)) != 0)
							{
								//������ͬ�����ݲ�ͬ,�������µ��滻���һ��
								oldBS = bs;
							}
							else if (oldBS.date < bs.date)	//�ϵ�K������С���µ�,��ֱ��׷�ӵ�����
							{
								bars.emplace_back(bs);
							}
						}

						//����ϵ��ļ��Ѿ���ѹ���汾,�����������ݴ�С����100��,�����ѹ��
						bool bNeedCompress = false;
						if (kBlock->_version == BLOCK_VERSION_CMP || bars.size() > 100)
						{
							bNeedCompress = true;
						}

						if (bNeedCompress)
						{
							std::string cmpData = WTSCmpHelper::compress_data(bars.data(), bars.size() * sizeof(WTSBarStruct));
							BlockHeaderV2 header;
							strcpy(header._blk_flag, BLK_FLAG);
							header._type = BT_HIS_Day;
							header._version = BLOCK_VERSION_CMP;
							header._size = cmpData.size();

							f.truncate_file(0);
							f.seek_to_begin();
							f.write_file(&header, sizeof(header));

							f.write_file(cmpData.data(), cmpData.size());
							count++;
						}
						else
						{
							BlockHeader header;
							strcpy(header._blk_flag, BLK_FLAG);
							header._type = BT_HIS_Day;
							header._version = BLOCK_VERSION_RAW;

							f.truncate_file(0);
							f.seek_to_begin();
							f.write_file(&header, sizeof(header));
							f.write_file(bars.data(), bars.size() * sizeof(WTSBarStruct));
							count++;
						}
					}
				}

				f.close_file();
			}
			else
			{
				//_sink->outputWriterLog(LL_ERROR, "��ʷ�����ļ�%s��ʧ��,day����������ҵʧ��", filename.c_str());
				_sink->outputWriterLog(LL_ERROR, "ClosingTask of day bar failed: openning history data file %s failed", filename.c_str());
			}
		}
	}

	//ת��ʵʱ1������
	if (!_disable_min1)
	{
		KBlockPair* kBlkPair = getKlineBlock(ct, KP_Minute1, false);
		if (kBlkPair != NULL && kBlkPair->_block->_size > 0)
		{
			uint32_t size = kBlkPair->_block->_size;
			//_sink->outputWriterLog(LL_INFO, "��ʼת��%s��1��������", ct->getFullCode());
			_sink->outputWriterLog(LL_INFO, "Transfering min1 bars of %s...", ct->getFullCode());
			StdUniqueLock lock(kBlkPair->_mutex);

			std::stringstream ss;
			ss << _base_dir << "his/min1/" << ct->getExchg() << "/";
			BoostFile::create_directories(ss.str().c_str());
			std::string path = ss.str();
			BoostFile::create_directories(ss.str().c_str());
			std::string filename = StrUtil::printf("%s%s.dsb", path.c_str(), ct->getCode());

			bool bNew = false;
			if (!BoostFile::exists(filename.c_str()))
				bNew = true;

			_sink->outputWriterLog(LL_INFO, "Openning data storage faile: %s", filename.c_str());

			BoostFile f;
			if (f.create_or_open_file(filename.c_str()))
			{
				std::string newData;
				if (!bNew)
				{
					std::string content;
					BoostFile::read_file_contents(filename.c_str(), content);
					HisKlineBlock* kBlock = (HisKlineBlock*)content.data();
					if (kBlock->_version == BLOCK_VERSION_RAW)
					{
						uint32_t barcnt = (content.size() - BLOCK_HEADER_SIZE) / sizeof(WTSBarStruct);
						newData.resize(sizeof(WTSBarStruct)*barcnt);
						memcpy((void*)newData.data(), kBlock->_bars, sizeof(WTSBarStruct)*barcnt);
					}
					else //if (kBlock->_version == BLOCK_VERSION_CMP)
					{
						HisKlineBlockV2* kBlockV2 = (HisKlineBlockV2*)content.data();
						newData = WTSCmpHelper::uncompress_data(kBlockV2->_data, (uint32_t)kBlockV2->_size);
					}
				}
				newData.append((const char*)kBlkPair->_block->_bars, sizeof(WTSBarStruct)*size);

				std::string cmpData = WTSCmpHelper::compress_data(newData.data(), newData.size());

				f.truncate_file(0);
				f.seek_to_begin(0);

				BlockHeaderV2 header;
				strcpy(header._blk_flag, BLK_FLAG);
				header._type = BT_HIS_Minute1;
				header._version = BLOCK_VERSION_CMP;
				header._size = cmpData.size();
				f.write_file(&header, sizeof(header));
				f.write_file(cmpData);
				count += size;

				//��󽫻������
				//memset(kBlkPair->_block->_bars, 0, sizeof(WTSBarStruct)*kBlkPair->_block->_size);
				kBlkPair->_block->_size = 0;
			}
			else
			{
				//_sink->outputWriterLog(LL_ERROR, "��ʷ�����ļ�%s��ʧ��,min1����������ҵʧ��", filename.c_str());
				_sink->outputWriterLog(LL_ERROR, "ClosingTask of min1 bar failed: openning history data file %s failed", filename.c_str());
			}
		}

		if (kBlkPair)
			releaseBlock(kBlkPair);
	}

	//���Ĳ�,ת��ʵʱ5������
	if (!_disable_min5)
	{
		KBlockPair* kBlkPair = getKlineBlock(ct, KP_Minute5, false);
		if (kBlkPair != NULL && kBlkPair->_block->_size > 0)
		{
			uint32_t size = kBlkPair->_block->_size;
			_sink->outputWriterLog(LL_INFO, "Transfering min5 bar of %s...", ct->getFullCode());
			StdUniqueLock lock(kBlkPair->_mutex);

			std::stringstream ss;
			ss << _base_dir << "his/min5/" << ct->getExchg() << "/";
			BoostFile::create_directories(ss.str().c_str());
			std::string path = ss.str();
			BoostFile::create_directories(ss.str().c_str());
			std::string filename = StrUtil::printf("%s%s.dsb", path.c_str(), ct->getCode());

			bool bNew = false;
			if (!BoostFile::exists(filename.c_str()))
				bNew = true;

			_sink->outputWriterLog(LL_INFO, "Openning data storage file: %s", filename.c_str());

			BoostFile f;
			if (f.create_or_open_file(filename.c_str()))
			{
				std::string newData;
				if (!bNew)
				{
					std::string content;
					BoostFile::read_file_contents(filename.c_str(), content);
					HisKlineBlock* kBlock = (HisKlineBlock*)content.data();
					if (kBlock->_version == BLOCK_VERSION_RAW)
					{
						uint32_t barcnt = (content.size() - BLOCK_HEADER_SIZE) / sizeof(WTSBarStruct);
						newData.resize(sizeof(WTSBarStruct)*barcnt);
						memcpy((void*)newData.data(), kBlock->_bars, sizeof(WTSBarStruct)*barcnt);
					}
					else //if (kBlock->_version == BLOCK_VERSION_CMP)
					{
						HisKlineBlockV2* kBlockV2 = (HisKlineBlockV2*)content.data();
						newData = WTSCmpHelper::uncompress_data(kBlockV2->_data, (uint32_t)kBlockV2->_size);
					}
				}
				newData.append((const char*)kBlkPair->_block->_bars, sizeof(WTSBarStruct)*size);

				std::string cmpData = WTSCmpHelper::compress_data(newData.data(), newData.size());

				f.truncate_file(0);
				f.seek_to_begin(0);

				BlockHeaderV2 header;
				strcpy(header._blk_flag, BLK_FLAG);
				header._type = BT_HIS_Minute5;
				header._version = BLOCK_VERSION_CMP;
				header._size = cmpData.size();
				f.write_file(&header, sizeof(header));
				f.write_file(cmpData);
				count += size;

				//��󽫻������
				kBlkPair->_block->_size = 0;
			}
			else
			{
				//_sink->outputWriterLog(LL_ERROR, "��ʷ�����ļ�%s��ʧ��,min5����������ҵʧ��", filename.c_str());
				_sink->outputWriterLog(LL_ERROR, "ClosingTask of min5 bar failed: openning history data file %s failed", filename.c_str());
			}
		}

		if (kBlkPair)
			releaseBlock(kBlkPair);
	}

	return count;
}

void WtDataWriter::proc_loop()
{
	while (!_terminated)
	{
		if(_proc_que.empty())
		{
			StdUniqueLock lock(_proc_mtx);
			_proc_cond.wait(_proc_mtx);
			continue;
		}

		std::string fullcode;
		try
		{
			StdUniqueLock lock(_proc_mtx);
			fullcode = _proc_que.front().c_str();
			_proc_que.pop();
		}
		catch(std::exception& e)
		{
			_sink->outputWriterLog(LL_ERROR, e.what());
			continue;
		}

		if (fullcode.compare(CMD_CLEAR_CACHE) == 0)
		{
			//������
			StdUniqueLock lock(_mtx_tick_cache);

			std::set<std::string> setCodes;
			std::stringstream ss_snapshot;
			ss_snapshot << "date,exchg,code,open,high,low,close,settle,volume,turnover,openinterest,upperlimit,lowerlimit,preclose,presettle,preinterest" << std::endl << std::fixed;
			for (auto it = _tick_cache_idx.begin(); it != _tick_cache_idx.end(); it++)
			{
				const std::string& key = it->first;
				const StringVector& ay = StrUtil::split(key, ".");
				WTSContractInfo* ct = _bd_mgr->getContract(ay[1].c_str(), ay[0].c_str());
				if (ct != NULL)
				{
					setCodes.insert(key);

					uint32_t idx = it->second;

					const TickCacheItem& tci = _tick_cache_block->_ticks[idx];
					const WTSTickStruct& ts = tci._tick;
					ss_snapshot << ts.trading_date << ","
						<< ts.exchg << ","
						<< ts.code << ","
						<< ts.open << ","
						<< ts.high << ","
						<< ts.low << ","
						<< ts.price << ","
						<< ts.settle_price << ","
						<< ts.total_volume << ","
						<< ts.total_turnover << ","
						<< ts.open_interest << ","
						<< ts.upper_limit << ","
						<< ts.lower_limit << ","
						<< ts.pre_close << ","
						<< ts.pre_settle << ","
						<< ts.pre_interest << std::endl;
				}
				else
				{
					_sink->outputWriterLog(LL_WARN, "%s[%s] expired, cache will be cleared", ay[1].c_str(), ay[0].c_str());

					//ɾ���Ѿ����ڴ����ʵʱtick�ļ�
					std::string path = StrUtil::printf("%srt/ticks/%s/%s.dmb", _base_dir.c_str(), ay[0].c_str(), ay[1].c_str());
					BoostFile::delete_file(path.c_str());
				}
			}

			//���������������ͬ,˵���д��������,���ų���
			if(setCodes.size() != _tick_cache_idx.size())
			{
				uint32_t diff = _tick_cache_idx.size() - setCodes.size();

				uint32_t scale = setCodes.size() / CACHE_SIZE_STEP;
				if (setCodes.size() % CACHE_SIZE_STEP != 0)
					scale++;

				uint32_t size = sizeof(RTTickCache) + sizeof(TickCacheItem)*scale*CACHE_SIZE_STEP;
				std::string buffer;
				buffer.resize(size, 0);

				RTTickCache* newCache = (RTTickCache*)buffer.data();
				newCache->_capacity = scale*CACHE_SIZE_STEP;
				newCache->_type = BT_RT_Cache;
				newCache->_size = setCodes.size();
				newCache->_version = BLOCK_VERSION_RAW;
				strcpy(newCache->_blk_flag, BLK_FLAG);

				faster_hashmap<std::string, uint32_t> newIdxMap;

				uint32_t newIdx = 0;
				for (const std::string& key : setCodes)
				{
					uint32_t oldIdx = _tick_cache_idx[key];
					newIdxMap[key] = newIdx;

					memcpy(&newCache->_ticks[newIdx], &_tick_cache_block->_ticks[oldIdx], sizeof(TickCacheItem));

					newIdx++;
				}

				//�����滻
				_tick_cache_idx = newIdxMap;
				_tick_cache_file->close();
				_tick_cache_block = NULL;

				std::string filename = _base_dir + _cache_file;
				BoostFile f;
				if (f.create_new_file(filename.c_str()))
				{
					f.write_file(buffer.data(), buffer.size());
					f.close_file();
				}

				_tick_cache_file->map(filename.c_str());
				_tick_cache_block = (RTTickCache*)_tick_cache_file->addr();
				
				_sink->outputWriterLog(LL_INFO, "%u expired cache cleared totally", diff);
			}

			//�����յ����߿�����ص�һ�������ļ�
			{

				std::stringstream ss;
				ss << _base_dir << "his/snapshot/";
				BoostFile::create_directories(ss.str().c_str());
				ss << TimeUtils::getCurDate() << ".csv";
				std::string path = ss.str();

				const std::string& content = ss_snapshot.str();
				BoostFile f;
				f.create_new_file(path.c_str());
				f.write_file(content.data());
				f.close_file();
			}

			int try_count = 0;
			do
			{
				if(try_count >= 5)
				{
					//_sink->outputWriterLog(LL_ERROR, "����ʵʱ���ݻ����ļ����Զ��û����ɣ������ò���");
					_sink->outputWriterLog(LL_ERROR, "Too many trys to clear rt cache files��skip");
					break;
				}

				try_count++;
				try
				{
					std::string path = StrUtil::printf("%srt/min1/", _base_dir.c_str());
					boost::filesystem::remove_all(boost::filesystem::path(path));
					path = StrUtil::printf("%srt/min5/", _base_dir.c_str());
					boost::filesystem::remove_all(boost::filesystem::path(path));
					path = StrUtil::printf("%srt/ticks/", _base_dir.c_str());
					boost::filesystem::remove_all(boost::filesystem::path(path));
					path = StrUtil::printf("%srt/orders/", _base_dir.c_str());
					boost::filesystem::remove_all(boost::filesystem::path(path));
					path = StrUtil::printf("%srt/queue/", _base_dir.c_str());
					boost::filesystem::remove_all(boost::filesystem::path(path));
					path = StrUtil::printf("%srt/trans/", _base_dir.c_str());
					boost::filesystem::remove_all(boost::filesystem::path(path));
					break;
				}
				catch (...)
				{
					//_sink->outputWriterLog(LL_ERROR, "����ʵʱ���ݻ����ļ������쳣��300s������");
					_sink->outputWriterLog(LL_ERROR, "Error occured while clearing rt cache files��retry in 300s");
					std::this_thread::sleep_for(std::chrono::seconds(300));
					continue;
				}
			} while (true);

			continue;
		}
		else if (StrUtil::startsWith(fullcode, "MARK.", false))
		{
			//���ָ����MARK.��ͷ,˵���Ǳ��ָ��,Ҫдһ�����
			std::string filename = _base_dir + MARKER_FILE;
			std::string sid = fullcode.substr(5);
			uint32_t curDate = TimeUtils::getCurDate();
			//IniFile::WriteConfigInt("markers", sid.c_str(), curDate, filename.c_str());
			IniHelper iniHelper;
			iniHelper.load(filename.c_str());
			iniHelper.writeInt("markers", sid.c_str(), curDate);
			iniHelper.save();
			_sink->outputWriterLog(LL_INFO, "ClosingTask mark of Trading session [%s] updated: %u", sid.c_str(), curDate);
		}

		auto pos = fullcode.find(".");
		std::string exchg = fullcode.substr(0, pos);
		std::string code = fullcode.substr(pos + 1);
		WTSContractInfo* ct = _bd_mgr->getContract(code.c_str(), exchg.c_str());
		if(ct == NULL)
			continue;

		uint32_t count = 0;

		uint32_t uDate = _sink->getTradingDate(ct->getFullCode());
		//ת��ʵʱtick����
		if(!_disable_tick)
		{
			TickBlockPair *tBlkPair = getTickBlock(ct, uDate, false);
			if (tBlkPair != NULL)
			{
				if(tBlkPair->_fstream)
					tBlkPair->_fstream.reset();

				if (tBlkPair->_block->_size > 0)
				{
					_sink->outputWriterLog(LL_INFO, "Transfering tick data of %s...", fullcode.c_str());
					StdUniqueLock lock(tBlkPair->_mutex);

					std::stringstream ss;
					ss << _base_dir << "his/ticks/" << ct->getExchg() << "/" << tBlkPair->_block->_date << "/";
					std::string path = ss.str();
					_sink->outputWriterLog(LL_INFO, path.c_str());
					BoostFile::create_directories(ss.str().c_str());
					std::string filename = StrUtil::printf("%s%s.dsb", path.c_str(), code.c_str());

					bool bNew = false;
					if (!BoostFile::exists(filename.c_str()))
						bNew = true;

					_sink->outputWriterLog(LL_INFO, "Openning data storage file: %s", filename.c_str());
					BoostFile f;
					if (f.create_new_file(filename.c_str()))
					{
						//��ѹ������
						std::string cmp_data = WTSCmpHelper::compress_data(tBlkPair->_block->_ticks, sizeof(WTSTickStruct)*tBlkPair->_block->_size);

						BlockHeaderV2 header;
						strcpy(header._blk_flag, BLK_FLAG);
						header._type = BT_HIS_Ticks;
						header._version = BLOCK_VERSION_CMP;
						header._size = cmp_data.size();
						f.write_file(&header, sizeof(header));

						f.write_file(cmp_data.c_str(), cmp_data.size());
						f.close_file();

						count += tBlkPair->_block->_size;

						//��󽫻������
						//memset(tBlkPair->_block->_ticks, 0, sizeof(WTSTickStruct)*tBlkPair->_block->_size);
						tBlkPair->_block->_size = 0;
					}
					else
					{
						//_sink->outputWriterLog(LL_ERROR, "��ʷ�����ļ�%s��ʧ��,tick����������ҵʧ��", filename.c_str());
						_sink->outputWriterLog(LL_ERROR, "ClosingTask of tick failed: openning history data file %s failed", filename.c_str());
					}
				}
			}

			if (tBlkPair)
				releaseBlock<TickBlockPair>(tBlkPair);
		}

		//ת��ʵʱtrans����
		if(!_disable_trans)
		{
			TransBlockPair *tBlkPair = getTransBlock(ct, uDate, false);
			if (tBlkPair != NULL && tBlkPair->_block->_size > 0)
			{
				//_sink->outputWriterLog(LL_INFO, "��ʼת��%s��trans����", fullcode.c_str());
				_sink->outputWriterLog(LL_INFO, "Transfering transaction data of %s...", fullcode.c_str());
				StdUniqueLock lock(tBlkPair->_mutex);

				std::stringstream ss;
				ss << _base_dir << "his/trans/" << ct->getExchg() << "/" << tBlkPair->_block->_date << "/";
				std::string path = ss.str();
				_sink->outputWriterLog(LL_INFO, path.c_str());
				BoostFile::create_directories(ss.str().c_str());
				std::string filename = StrUtil::printf("%s%s.dsb", path.c_str(), code.c_str());

				bool bNew = false;
				if (!BoostFile::exists(filename.c_str()))
					bNew = true;

				_sink->outputWriterLog(LL_INFO, "Openning data storage file: %s", filename.c_str());
				BoostFile f;
				if (f.create_new_file(filename.c_str()))
				{
					//��ѹ������
					std::string cmp_data = WTSCmpHelper::compress_data(tBlkPair->_block->_trans, sizeof(WTSTransStruct)*tBlkPair->_block->_size);

					BlockHeaderV2 header;
					strcpy(header._blk_flag, BLK_FLAG);
					header._type = BT_HIS_Trnsctn;
					header._version = BLOCK_VERSION_CMP;
					header._size = cmp_data.size();
					f.write_file(&header, sizeof(header));

					f.write_file(cmp_data.c_str(), cmp_data.size());
					f.close_file();

					count += tBlkPair->_block->_size;

					//��󽫻������
					//memset(tBlkPair->_block->_ticks, 0, sizeof(WTSTickStruct)*tBlkPair->_block->_size);
					tBlkPair->_block->_size = 0;
				}
				else
				{
					//_sink->outputWriterLog(LL_ERROR, "��ʷ�����ļ�%s��ʧ��,trans����������ҵʧ��", filename.c_str());
					_sink->outputWriterLog(LL_ERROR, "ClosingTask of transaction failed: openning history data file %s failed", filename.c_str());
				}
			}

			if (tBlkPair)
				releaseBlock<TransBlockPair>(tBlkPair);
		}

		//ת��ʵʱorder����
		if(!_disable_orddtl)
		{
			OrdDtlBlockPair *tBlkPair = getOrdDtlBlock(ct, uDate, false);
			if (tBlkPair != NULL && tBlkPair->_block->_size > 0)
			{
				//_sink->outputWriterLog(LL_INFO, "��ʼת��%s��order����", fullcode.c_str());
				_sink->outputWriterLog(LL_INFO, "Transfering order detail data of %s...", fullcode.c_str());
				StdUniqueLock lock(tBlkPair->_mutex);

				std::stringstream ss;
				ss << _base_dir << "his/orders/" << ct->getExchg() << "/" << tBlkPair->_block->_date << "/";
				std::string path = ss.str();
				_sink->outputWriterLog(LL_INFO, path.c_str());
				BoostFile::create_directories(ss.str().c_str());
				std::string filename = StrUtil::printf("%s%s.dsb", path.c_str(), code.c_str());

				bool bNew = false;
				if (!BoostFile::exists(filename.c_str()))
					bNew = true;

				_sink->outputWriterLog(LL_INFO, "Openning data storage file: %s", filename.c_str());
				BoostFile f;
				if (f.create_new_file(filename.c_str()))
				{
					//��ѹ������
					std::string cmp_data = WTSCmpHelper::compress_data(tBlkPair->_block->_details, sizeof(WTSOrdDtlStruct)*tBlkPair->_block->_size);

					BlockHeaderV2 header;
					strcpy(header._blk_flag, BLK_FLAG);
					header._type = BT_HIS_OrdDetail;
					header._version = BLOCK_VERSION_CMP;
					header._size = cmp_data.size();
					f.write_file(&header, sizeof(header));

					f.write_file(cmp_data.c_str(), cmp_data.size());
					f.close_file();

					count += tBlkPair->_block->_size;

					//��󽫻������
					//memset(tBlkPair->_block->_ticks, 0, sizeof(WTSTickStruct)*tBlkPair->_block->_size);
					tBlkPair->_block->_size = 0;
				}
				else
				{
					//_sink->outputWriterLog(LL_ERROR, "��ʷ�����ļ�%s��ʧ��,orders����������ҵʧ��", filename.c_str());
					_sink->outputWriterLog(LL_ERROR, "ClosingTask of order detail failed: openning history data file %s failed", filename.c_str());
				}
			}

			if (tBlkPair)
				releaseBlock<OrdDtlBlockPair>(tBlkPair);
		}

		//ת��ʵʱqueue����
		if(!_disable_ordque)
		{
			OrdQueBlockPair *tBlkPair = getOrdQueBlock(ct, uDate, false);
			if (tBlkPair != NULL && tBlkPair->_block->_size > 0)
			{
				//_sink->outputWriterLog(LL_INFO, "��ʼת��%s��queue����", fullcode.c_str());
				_sink->outputWriterLog(LL_INFO, "Transfering order queue data of %s...", fullcode.c_str());
				StdUniqueLock lock(tBlkPair->_mutex);

				std::stringstream ss;
				ss << _base_dir << "his/queue/" << ct->getExchg() << "/" << tBlkPair->_block->_date << "/";
				std::string path = ss.str();
				_sink->outputWriterLog(LL_INFO, path.c_str());
				BoostFile::create_directories(ss.str().c_str());
				std::string filename = StrUtil::printf("%s%s.dsb", path.c_str(), code.c_str());

				bool bNew = false;
				if (!BoostFile::exists(filename.c_str()))
					bNew = true;

				_sink->outputWriterLog(LL_INFO, "Openning data storage file: %s", filename.c_str());
				BoostFile f;
				if (f.create_new_file(filename.c_str()))
				{
					//��ѹ������
					std::string cmp_data = WTSCmpHelper::compress_data(tBlkPair->_block->_queues, sizeof(WTSOrdQueStruct)*tBlkPair->_block->_size);

					BlockHeaderV2 header;
					strcpy(header._blk_flag, BLK_FLAG);
					header._type = BT_HIS_OrdQueue;
					header._version = BLOCK_VERSION_CMP;
					header._size = cmp_data.size();
					f.write_file(&header, sizeof(header));

					f.write_file(cmp_data.c_str(), cmp_data.size());
					f.close_file();

					count += tBlkPair->_block->_size;

					//��󽫻������
					//memset(tBlkPair->_block->_ticks, 0, sizeof(WTSTickStruct)*tBlkPair->_block->_size);
					tBlkPair->_block->_size = 0;
				}
				else
				{
					//_sink->outputWriterLog(LL_ERROR, "��ʷ�����ļ�%s��ʧ��,queue����������ҵʧ��", filename.c_str());
					_sink->outputWriterLog(LL_ERROR, "ClosingTask of order queue failed: openning history data file %s failed", filename.c_str());
				}


			}

			if (tBlkPair)
				releaseBlock<OrdQueBlockPair>(tBlkPair);
		}

		//ת����ʷK��
		if(_db_conn)
			count += dump_hisdata_to_db(ct);
		else
			count += dump_hisdata_to_file(ct);

		//_sink->outputWriterLog(LL_INFO, "%s[%s]���������, ������ҵ����������%u��", ct->getCode(), ct->getExchg(), count);
		_sink->outputWriterLog(LL_INFO, "ClosingTask of %s[%s] done, %u datas processed totally", ct->getCode(), ct->getExchg(), count);
	}
}