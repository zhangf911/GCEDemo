#ifndef GATESERVER_GCE_CONNECTION_H
#define GATESERVER_GCE_CONNECTION_H


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

class gce_server;

class gce_connection
{
	typedef boost::asio::ip::tcp::socket socket_t;
	typedef boost::shared_ptr<socket_t> socket_ptr;
	typedef deque<SendPacket> SendPacketDeque;
	typedef deque<ReceivedPacket> ReceivedPacketDeque;
public:
	gce_connection(gce_server& INserver, socket_ptr INsocket, UInt32 INconnectionID);

	~gce_connection();
public:
	// 发送消息包
	void Send(SendPacket& INsendPacket);

	// 关闭网络连接
	void Stop();

public:
	// 开启网络接收
	void OnReceive(gce::actor<gce::stackful>& self);

	// 发送
	void OnSend(gce::actor<gce::stackful>& self);
private:
	// 套接字
	socket_ptr m_socket;
	// 连接编号
	size_t m_connectionID;
	// 接收包
	ReceivedPacket m_receivePacket;
	// 接收大包
	ReceivedPacket m_receiveBigPacket;
	// 发送包队列
	SendPacketDeque m_sendPacketDeque;
	// 发送队列互斥锁
	std::mutex m_sendDequeMutex;
	// 接受包计数
	UInt32 m_receivePacketCount;
	// 
	gce_server& m_netServer;
	// 
	bool m_runing;
};

#endif