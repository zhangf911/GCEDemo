#ifndef GATESERVER_MSGHANDLER_H
#define GATESERVER_MSGHANDLER_H

#include "CommDef.h"
#include "NetPacket.h"

typedef std::function<UInt32(ReceivedPacket&)> MsgProcessHandler;
typedef map<UInt32, MsgProcessHandler> HandlerMap;
typedef HandlerMap::iterator HandlerMapIter;

class MsgHandler
{
public:
	MsgHandler();

	~MsgHandler();
public:
	void Register(UInt16 INtype, MsgProcessHandler& INhandler);

	UInt32 HandlerMsg(ReceivedPacket& INpacket);

private:
	// 消息处理
	HandlerMap m_handlerMap;
};

#endif