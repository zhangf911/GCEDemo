#ifndef GATESERVER_GATESERVER_H
#define GATESERVER_GATESERVER_H

#include "Type.h"
#include "NetServer.h"
#include "NetClient.h"
#include "gce_server.h"
#include "MsgHandler.h"
#include <thread>

class SendPacket;
class ReceivedPacket;

struct ServerInfo
{
	string ip;
	UInt16 port;

	ServerInfo()
	{
		ip = "";
		port = 0;
	}
};

typedef list<ServerInfo> ServerConfigList;

class GateServer
{
public:
	typedef std::shared_ptr<NetClient> NetClientPtr;

public:
	GateServer(gce_server& INnetServer);

	~GateServer();
public:
	// 启动登录服务器
	bool Start();

	// 停止登录服务器
	void Stop();

	// 登录服务器事务处理
	void Run();

public:
	// 发送网络消息到游戏服务器
	void SendPacketToGameServer(UInt32 INserverID, SendPacket& INsendPacket);

	// 发送网络消息给客户端
	void SendPacketToClient(UInt32 INclientID, SendPacket& INsendPacket);

	// 获取游戏服务器信息
	void GetGameServerInfo(ServerInfo& OUTserverInfo);

	// 添加游戏服务器
	void AddGameServerInfo(const ServerInfo& INserverInfo);

private:
	// 与游戏服务器断线重连
	void ReConnectGameServer();

	// 消息处理
	void DespatchPacket(ReceivedPacket& INpacket);
private:
	// 网络服务管理器
	gce_server& m_netServer;

	// 游戏服务器连接客户端
	map<UInt32, NetClientPtr> m_netClient;

	// 运行状态
	bool m_runStatus;
	// 游戏服务器列表
	ServerConfigList m_gameServerList;
	// 
	MsgHandler m_msgHandler;
};




#endif