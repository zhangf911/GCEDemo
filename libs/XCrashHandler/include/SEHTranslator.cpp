#include "SEHTranslator.h"
#include "XCrashHandler.h"
#include "FileSystemUtil.h"
#include "Log.h"
#include <eh.h>
#include <windows.h>
#include "dbghelp.h"

void SEHTranslator::setup()
{
	//每个线程需要单独设置
	_set_se_translator(&trans_func);
}

void SEHTranslator::trans_func( unsigned int u, EXCEPTION_POINTERS* pExp )
{
	g_SEHTranslator->m_exptionCode = u;
	if (pExp)
	{
		g_SEHTranslator->m_contextRecord = *(pExp->ContextRecord);
		g_SEHTranslator->m_exceptionRecord = *(pExp->ExceptionRecord);
	}
	throw *g_SEHTranslator;
}

SEHTranslator::SEHTranslator()
{
	m_exptionCode = 0;
	memset(&m_exceptionRecord,0,sizeof(m_exceptionRecord));
	memset(&m_contextRecord,0,sizeof(m_contextRecord));
}

SEHTranslator* SEHTranslator::instance()
{
	static SEHTranslator s_instance;
	return &s_instance;
}

void SEHTranslator::log()
{
	EXCEPTION_POINTERS exception;
	exception.ContextRecord = &m_contextRecord;
	exception.ExceptionRecord = &m_exceptionRecord;
	
	//EXCEPTION_LOG("数据异常，生成minidump");

	RecordDump::Record(&exception);
}
