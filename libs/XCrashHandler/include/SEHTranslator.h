#ifndef _SEH_TRANSLATOR_H
#define _SEH_TRANSLATOR_H

#include <Windows.h>

class SEHTranslator
{
public:
	SEHTranslator();
	static SEHTranslator* instance();
	static void setup(); 	//初始化设置 每个线程需要单独设置
	static void trans_func( unsigned int u, EXCEPTION_POINTERS* pExp );
	
	void log();
public:
	unsigned int m_exptionCode;
	EXCEPTION_RECORD m_exceptionRecord;
	CONTEXT m_contextRecord;
};

#define g_SEHTranslator SEHTranslator::instance()

#endif