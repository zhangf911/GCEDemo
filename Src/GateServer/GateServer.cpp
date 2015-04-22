#include "GateServer.h"
#include "NetPacket.h"
#include "Singleton.h"
#include "CommDef.h"
#include <chrono>
#include <thread>

#define INSTANCE_INITIAL(Instance) do {if(!Instance->Initial()){return false;}} while (0);

GateServer::GateServer(gce_server& INnetServer)
	: m_netServer(INnetServer), m_runStatus(false)
{

}

GateServer::~GateServer()
{
	Stop();
}

bool GateServer::Start()
{
	m_netServer.SetPacketHandler(std::bind(&GateServer::DespatchPacket, this, std::placeholders::_1));

	m_runStatus = true;

	return true;
}

void GateServer::Run()
{
	NOTE_LOG("登录服务器逻辑线程, 线程编号 : %d",
		std::this_thread::get_id());

	while (m_runStatus)
	{

		ReConnectGameServer();

		std::chrono::milliseconds dura(40);
		std::this_thread::sleep_for(dura);
	}
}

void GateServer::Stop()
{
	//m_netServer.Stop();
}

void GateServer::SendPacketToGameServer(UInt32 INserverID, SendPacket& INsendPacket)
{
	INsendPacket.EnCodePacket();

	DEBUG_LOG("Send Packet , Type : %d Size : %d",
		INsendPacket.GetPacketType(), INsendPacket.GetPacketSize());

	auto it = m_netClient.find(INserverID);
	if (it != m_netClient.end())
	{
		it->second->Send(INsendPacket);
	}
}

void GateServer::SendPacketToClient(UInt32 INclientID, SendPacket& INsendPacket)
{
	INsendPacket.EnCodePacket();

	DEBUG_LOG("Send Packet , Type : %d Size : %d",
		INsendPacket.GetPacketType(), INsendPacket.GetPacketSize());

	m_netServer.Send(INclientID, INsendPacket);
}

void GateServer::GetGameServerInfo(ServerInfo& OUTserverInfo)
{
	if (!m_gameServerList.empty())
	{
		OUTserverInfo = m_gameServerList.front();
	}
}

void GateServer::AddGameServerInfo(const ServerInfo& INserverInfo)
{
	m_gameServerList.push_back(INserverInfo);
}

// 与游戏服务器断线重连
void GateServer::ReConnectGameServer()
{
	for(auto& clientPair : m_netClient)
	{
		if (!clientPair.second->IsSocketOpen())
		{
			cout << "reconnect to gameserver...";
			clientPair.second->ReConnect();
		}
	}
}

void GateServer::DespatchPacket(ReceivedPacket& INpacket)
{
	// 收取网络包
	NOTE_LOG("DespatchPacket....");
	m_msgHandler.HandlerMsg(INpacket);

	//gce::send()
}