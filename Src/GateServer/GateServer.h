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
	// ������¼������
	bool Start();

	// ֹͣ��¼������
	void Stop();

	// ��¼������������
	void Run();

public:
	// ����������Ϣ����Ϸ������
	void SendPacketToGameServer(UInt32 INserverID, SendPacket& INsendPacket);

	// ����������Ϣ���ͻ���
	void SendPacketToClient(UInt32 INclientID, SendPacket& INsendPacket);

	// ��ȡ��Ϸ��������Ϣ
	void GetGameServerInfo(ServerInfo& OUTserverInfo);

	// �����Ϸ������
	void AddGameServerInfo(const ServerInfo& INserverInfo);

private:
	// ����Ϸ��������������
	void ReConnectGameServer();

	// ��Ϣ����
	void DespatchPacket(ReceivedPacket& INpacket);
private:
	// ������������
	gce_server& m_netServer;

	// ��Ϸ���������ӿͻ���
	map<UInt32, NetClientPtr> m_netClient;

	// ����״̬
	bool m_runStatus;
	// ��Ϸ�������б�
	ServerConfigList m_gameServerList;
	// 
	MsgHandler m_msgHandler;
};




#endif