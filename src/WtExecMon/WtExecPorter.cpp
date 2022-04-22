#include "WtExecPorter.h"
#include "WtExecRunner.h"

#include "../WtCore/WtHelper.h"
#include "../WTSTools/WTSLogger.h"

#include "../Share/StrUtil.hpp"
#include "../Includes/WTSVersion.h"


void inst_hlp() {}

#ifdef _WIN32
#include "../Common/mdump.h"
#ifdef _WIN64
char PLATFORM_NAME[] = "X64";
#else
char PLATFORM_NAME[] = "WIN32";
#endif

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

WtExecRunner& getRunner()
{
	static WtExecRunner runner;
	return runner;
}

const char* getModuleName()
{
	static char MODULE_NAME[250] = { 0 };
	if (strlen(MODULE_NAME) == 0)
	{
#ifdef _WIN32
		GetModuleFileName(g_dllModule, MODULE_NAME, 250);
		boost::filesystem::path p(MODULE_NAME);
		strcpy(MODULE_NAME, p.filename().string().c_str());
#else
		boost::filesystem::path p(getInstPath());
		strcpy(MODULE_NAME, p.filename().string().c_str());
#endif
	}

	return MODULE_NAME;
}

const char* getBinDir()
{
	static std::string _bin_dir;
	if (_bin_dir.empty())
	{


#ifdef _WIN32
		char strPath[MAX_PATH];
		GetModuleFileName(g_dllModule, strPath, MAX_PATH);

		_bin_dir = StrUtil::standardisePath(strPath, false);
#else
		_bin_dir = getInstPath();
#endif

		uint32_t nPos = _bin_dir.find_last_of('/');
		_bin_dir = _bin_dir.substr(0, nPos + 1);
	}

	return _bin_dir.c_str();
}

void init_exec(WtString logCfg, bool isFile /*= true*/)
{
	static bool inited = false;

	if (inited)
		return;
#ifdef _WIN32
	CMiniDumper::Enable(getModuleName(), true, WtHelper::getCWD().c_str());
#endif

	getRunner().init(logCfg);

	inited = true;
}

void config_exec(WtString cfgfile, bool isFile /*= true*/)
{
	if (strlen(cfgfile) == 0)
		getRunner().config("cfgexec.json");
	else
		getRunner().config(cfgfile);
}

void run_exec()
{
	getRunner().run();
}

void release_exec()
{
	getRunner().release();
}

WtString get_version()
{
	static std::string _ver;
	if (_ver.empty())
	{
		_ver = PLATFORM_NAME;
		_ver += " ";
		_ver += WT_VERSION;
		_ver += " Build@";
		_ver += __DATE__;
		_ver += " ";
		_ver += __TIME__;
	}
	return _ver.c_str();
}

void write_log(unsigned int level, WtString message, WtString catName)
{
	if (strlen(catName) > 0)
	{
		WTSLogger::log2(catName, (WTSLogLevel)level, message);
	}
	else
	{
		WTSLogger::log((WTSLogLevel)level, message);
	}
}

void set_position(WtString stdCode, double targetPos)
{
	getRunner().setPosition(stdCode, targetPos);
}