#include "MsgHandler.h"

MsgHandler::MsgHandler()
{

}

MsgHandler::~MsgHandler()
{

}

void MsgHandler::Register(UInt16 INtype, MsgProcessHandler& INhandler)
{
	m_handlerMap[INtype] = INhandler;
}

UInt32 MsgHandler::HandlerMsg(ReceivedPacket& INpacket)
{
	UInt32 errorCode = 0;

	return errorCode;
}