#ifndef GATESERVER_COMMDEF_H
#define GATESERVER_COMMDEF_H

#include <boost/optional.hpp>

#include "Singleton.h"
#include "Log.h"
#include "DBEngine.h"
#include "DataTime.h"

#define LOG_INSTANCE Singleton<Library::Log>::Instance()
#define DEBUG_LOG Singleton<Library::Log>::Instance()->WriteDebug
#define ERROR_LOG Singleton<Library::Log>::Instance()->WriteError
#define NOTE_LOG Singleton<Library::Log>::Instance()->WriteNote
#define EXCEPTION_LOG Singleton<Library::Log>::Instance()->WriteException
#define ASSERT_LOG Singleton<Library::Log>::Instance()->WriteAssert
#define NON_LOG Singleton<Library::Log>::Instance()->WriteNon

inline void	log_assert(const char* type, const char* str, const char* file, int line) { ASSERT_LOG("%s(%s) in %s, %d", type, str, file, line); }
#define SJ_ASSERT(x)   if( (!(x)) || (!"(#x)") ) { log_assert("SJ_ASSERT", #x, __FILE__, __LINE__); assert(!"(#x)"); } 

//#define TXJ_DEBUG

#ifndef TXJ_DEBUG
#define		SJ_TRY
#define     SJ_DEPACKETCATCH(x,y)
#define		SJ_CATCH(x)
#define		SJ_CATCH2(x,y)
#define		SJ_CATCH3(x,y,z)
#else
#define		SJ_TRY		            try{
#define     SJ_DEPACKETCATCH(x,y)   }catch(DePacketErrorCode e) { EXCEPTION_LOG(("Process Msg Exception , ConnectionID=%d, PacketType=%d, DePacketErrorCode=%d"),x,y,e); 
#define		SJ_CATCH(x)	            }catch(...){ EXCEPTION_LOG((x)); }
#define		SJ_CATCH2(x,y)	        }catch(...){ EXCEPTION_LOG((x),(y)); }
#define		SJ_CATCH3(x,y,z)	    }catch(...){ EXCEPTION_LOG((x),(y),(z)); }
#endif

#define CONFIG_LOAD_NOTE NOTE_LOG

#define OPEN_BATTLE_LOG

#ifdef OPEN_BATTLE_LOG
#define BATTLE_DEBUG_LOG NON_LOG

#define BATTLE_NOTE_LOG NOTE_LOG
#else
#define BATTLE_DEBUG_LOG NON_LOG

#define BATTLE_NOTE_LOG NON_LOG
#endif

#define DESTROY_INSTANCE_LOG(Instance) do{	NOTE_LOG("%s, %d, 管理器 %s 执行析构", __FUNCTION__, __LINE__, #Instance); } while (0);

#define OUT_PROCESS_MEM_SIZE(x)  		{ UInt64 i64MemSize = Library::Common::GetProcessMemorySize(); NOTE_LOG("进程内存: %s  (%I64d) sign='%s'", Library::Common::MemSizeToString(i64MemSize).c_str(), i64MemSize, x); }

#define OUT_INSTANCE_MEM_SIZE(x,y)      { NOTE_LOG("模块内存: %s  (%d) sign='%s'", Library::Common::MemSizeToString(y).c_str(), y, #x); }

#define LOAD_PLAYER_DATA_NOTE NON_LOG

#define OPEN_DATA_SYNC_LOG

#ifdef OPEN_DATA_SYNC_LOG
#define DATA_SYNC_LOG DEBUG_LOG
#else
#define DATA_SYNC_LOG NON_LOG
#endif

// 天梯日志标记
#define TIANTILOGSING    "*TIANTI*: "


#endif