#include <string>
#include <map>
//v6.3.15
#include "./ThostTraderApi/ThostFtdcTraderApi.h"
#include "TraderSpi.h"

#include "../Share/IniHelper.hpp"
#include "../Share/StrUtil.hpp"
#include "../Share/DLLHelper.hpp"
#include "../Share/StdUtils.hpp"
#include <boost/filesystem.hpp>

std::string g_bin_dir;

void inst_hlp() {}

#ifdef _WIN32
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

const std::string& getInstPath()
{
	static std::string moduleName;
	if (moduleName.empty())
	{
		Dl_info dl_info;
		dladdr((void *)inst_hlp, &dl_info);
		moduleName = dl_info.dli_fname;
		//printf("1:%s\n", moduleName.c_str());
	}

	return moduleName;
}
#endif

const char* getBaseFolder()
{
	if (g_bin_dir.empty())
	{
#ifdef _WIN32
		char strPath[MAX_PATH];
		GetModuleFileName(g_dllModule, strPath, MAX_PATH);

		g_bin_dir = StrUtil::standardisePath(strPath, false);
#else
		g_bin_dir = getInstPath();
#endif
		boost::filesystem::path p(g_bin_dir);
		g_bin_dir = p.branch_path().string() + "/";
	}

	return g_bin_dir.c_str();
}


// UserApi����
CThostFtdcTraderApi* pUserApi;

// ���ò���
std::string	FRONT_ADDR;	// ǰ�õ�ַ
std::string	BROKER_ID;	// ���͹�˾����
std::string	INVESTOR_ID;// Ͷ���ߴ���
std::string	PASSWORD;	// �û�����
std::string SAVEPATH;	//����λ��
std::string APPID;
std::string AUTHCODE;
uint32_t	CLASSMASK;	//��Ȩ

std::string COMM_FILE;		//�����Ʒ���ļ���
std::string CONT_FILE;		//����ĺ�Լ�ļ���

std::string MODULE_NAME;	//�ⲿģ����

typedef std::map<std::string, std::string>	SymbolMap;
SymbolMap	MAP_NAME;
SymbolMap	MAP_SESSION;

typedef CThostFtdcTraderApi* (*CTPCreator)(const char *);
CTPCreator		g_ctpCreator = NULL;

// ������
int iRequestID = 0;

#ifdef _WIN32
#	define EXPORT_FLAG __declspec(dllexport)
#else
#	define EXPORT_FLAG __attribute__((__visibility__("default")))
#endif

#ifdef __cplusplus
extern "C"
{
#endif
	EXPORT_FLAG int run(const char* cfgfile);
#ifdef __cplusplus
}
#endif

int run(const char* cfgfile)
{
	std::string cfg = cfgfile;
	IniHelper ini;
	ini.load(cfg.c_str());

	FRONT_ADDR = ini.readString("ctp", "front", "");
	BROKER_ID	= ini.readString("ctp", "broker", "");
	INVESTOR_ID = ini.readString("ctp", "user", "");
	PASSWORD	= ini.readString("ctp", "pass", "");
	APPID = ini.readString("ctp", "appid", "");
	AUTHCODE = ini.readString("ctp", "authcode", "");

	SAVEPATH	= ini.readString("config", "path", "");
	CLASSMASK = ini.readUInt("config", "mask", 1 | 2 | 4); //1-�ڻ�,2-��Ȩ,4-��Ʊ

	COMM_FILE = ini.readString("config", "commfile", "commodities.json");
	CONT_FILE = ini.readString("config", "contfile", "contracts.json");

#ifdef _WIN32
	MODULE_NAME = ini.readString("config", "module", "thosttraderapi_se.dll");
#else
	MODULE_NAME = ini.readString("config", "module", "thosttraderapi_se.so");
#endif
	if(!boost::filesystem::exists(MODULE_NAME.c_str()))
	{
		MODULE_NAME = getBaseFolder();
#ifdef _WIN32
		MODULE_NAME += "traders/thosttraderapi_se.dll";
#else
		MODULE_NAME += "traders/thosttraderapi_se.so";
#endif
	}

	if(FRONT_ADDR.empty() || BROKER_ID.empty() || INVESTOR_ID.empty() || PASSWORD.empty() || SAVEPATH.empty())
	{
		return 0;
	}

	SAVEPATH = StrUtil::standardisePath(SAVEPATH);

	std::string map_files = ini.readString("config", "mapfiles", "");
	if(!map_files.empty())
	{
		StringVector ayFiles = StrUtil::split(map_files, ",");
		for(const std::string& fName:ayFiles)
		{
			printf("��ʼ��ȡӳ���ļ�%s...", fName.c_str());
			IniHelper iniMap;
			if(!StdFile::exists(fName.c_str()))
				continue;

			iniMap.load(fName.c_str());
			FieldArray ayKeys, ayVals;
			int cout = iniMap.readSecKeyValArray("Name", ayKeys, ayVals);
			for (int i = 0; i < cout; i++)
			{
				MAP_NAME[ayKeys[i]] = ayVals[i];
				printf("Ʒ������ӳ��: %s - %s\r\n", ayKeys[i].c_str(), ayVals[i].c_str());
			}

			ayKeys.clear();
			ayVals.clear();
			cout = iniMap.readSecKeyValArray("Session", ayKeys, ayVals);
			for (int i = 0; i < cout; i++)
			{
				MAP_SESSION[ayKeys[i]] = ayVals[i];
				printf("����ʱ��ӳ��: %s - %s\r\n", ayKeys[i].c_str(), ayVals[i].c_str());
			}
		}
		
	}

	// ��ʼ��UserApi
	DllHandle dllInst = DLLHelper::load_library(MODULE_NAME.c_str());
#ifdef _WIN32
#	ifdef _WIN64
	g_ctpCreator = (CTPCreator)DLLHelper::get_symbol(dllInst, "?CreateFtdcTraderApi@CThostFtdcTraderApi@@SAPEAV1@PEBD@Z");
#	else
	g_ctpCreator = (CTPCreator)DLLHelper::get_symbol(dllInst, "?CreateFtdcTraderApi@CThostFtdcTraderApi@@SAPAV1@PBD@Z");
#	endif
#else
	g_ctpCreator = (CTPCreator)DLLHelper::get_symbol(dllInst, "_ZN19CThostFtdcTraderApi19CreateFtdcTraderApiEPKc");
#endif
	pUserApi = g_ctpCreator("");			// ����UserApi
	CTraderSpi* pUserSpi = new CTraderSpi();
	pUserApi->RegisterSpi((CThostFtdcTraderSpi*)pUserSpi);			// ע���¼���
	pUserApi->SubscribePublicTopic(THOST_TERT_QUICK);					// ע�ṫ����
	pUserApi->SubscribePrivateTopic(THOST_TERT_QUICK);					// ע��˽����
	pUserApi->RegisterFront((char*)FRONT_ADDR.c_str());				// connect
	pUserApi->Init();

	pUserApi->Join();
	return 0;
}