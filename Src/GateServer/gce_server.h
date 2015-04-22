#ifndef GATESERVER_GCE_SERVER_H
#define GATESERVER_GCE_SERVER_H

#include <gce/actor/all.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <mutex>
#include <memory>

#include "CommDef.h"

#include "NetPacket.h"

class gce_connection;

class gce_server
{
	typedef boost::asio::ip::tcp::socket socket_t;
	typedef boost::shared_ptr<socket_t> socket_ptr;
	typedef deque<SendPacket> SendPacketDeque;
	typedef deque<ReceivedPacket> ReceivedPacketDeque;
public:
	gce_server(
		gce::context& ctx,
		gce::actor<gce::threaded>& base,
		const std::string& host,
		boost::uint16_t port
		);

	~gce_server();

private:
	void Run(gce::actor<gce::stackful>& self);

	void AddNewSession(gce::actor<gce::stackful>& self, boost::shared_ptr<socket_t> sock);\

	void DespatchPacket(gce::actor<gce::stackful>& self);
public:
	void AddReceivedPacket(ReceivedPacket& INpacket);

	UInt32 GetReceivedPacketDeque(ReceivedPacketDeque& INpacketDeque);

	void Send(UInt32 INclientID, SendPacket& INsendPacket);

	void SetPacketHandler(std::function<void(ReceivedPacket&)> INpacketHandler){ m_packetHandler = INpacketHandler; };

	void DeleteSession(size_t INconnectionID);
private:
	gce::context& m_ctx;
	boost::asio::ip::tcp::acceptor m_acpr;
	std::string m_host;
	boost::uint16_t m_port;
	std::size_t m_connectionID;
	// 连接表
	std::map<size_t, std::shared_ptr<gce_connection>> m_connectionMap;
	// 已收消息包互斥锁
	std::mutex m_receiveDequeMutex;
	// 网络接收包
	ReceivedPacketDeque m_receivedPacketDeque;
	// 一次获取接受包的最大条数，0表示全部
	UInt32 m_receiveDequeCount;
	// 网络接受包的最大条数， 0表示不限制
	UInt32 m_receivePacketDequeMax;
	// 消息处理回调
	std::function<void(ReceivedPacket&)> m_packetHandler;
};

#endif