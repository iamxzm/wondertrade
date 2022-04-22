/*!
 * \file WtDtPorter.h
 * \project	WonderTrader
 *
 * \author Wesley
 * \date 2020/03/30
 * 
 * \brief 
 */
#pragma once
#include "PorterDefs.h"


#ifdef __cplusplus
extern "C"
{
#endif

	EXPORT_FLAG void		initialize(WtString cfgFile, WtString logCfg);
	EXPORT_FLAG void		start(bool bAsync = false);

	EXPORT_FLAG	WtString	get_version();
	EXPORT_FLAG	void		write_log(unsigned int level, const char* message, const char* catName);


#pragma region "��չParser�ӿ�"
	EXPORT_FLAG	bool		create_ext_parser(const char* id);

	EXPORT_FLAG void		register_parser_callbacks(FuncParserEvtCallback cbEvt, FuncParserSubCallback cbSub);

	EXPORT_FLAG	void		parser_push_quote(const char* id, WTSTickStruct* curTick, WtUInt32 uProcFlag);
#pragma endregion "��չParser�ӿ�"

#pragma region "��չDumper�ӿ�"
	EXPORT_FLAG	bool		create_ext_dumper(const char* id);

	EXPORT_FLAG void		register_extended_dumper(FuncDumpBars barDumper, FuncDumpTicks tickDumper);

	EXPORT_FLAG void		register_extended_hftdata_dumper(FuncDumpOrdQue ordQueDumper, FuncDumpOrdDtl ordDtlDumper, FuncDumpTrans transDumper);
#pragma endregion "��չDumper�ӿ�"

#ifdef __cplusplus
}
#endif