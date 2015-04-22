#include "gce_connection.h"
#include "gce_server.h"

gce_connection::gce_connection(gce_server& INserver, socket_ptr INsocket, UInt32 INconnectionID)
	: m_netServer(INserver), m_socket(INsocket), m_connectionID(INconnectionID),
	m_receivePacket(EPacket_Small_MaxBodyLength), m_receiveBigPacket(EPacket_Big_MaxBodyLength),
	m_receivePacketCount(0), m_runing(true)
{

}

gce_connection::~gce_connection()
{

}

void gce_connection::Stop()
{
	m_runing = false;
	m_netServer.DeleteSession(m_connectionID);
}

void gce_connection::OnReceive(gce::actor<gce::stackful>& self)
{
	gce::yield_t yield = self.get_yield();

	while(m_runing)
	{
		boost::asio::async_read(*m_socket,
			boost::asio::buffer(m_receivePacket.GetPacketDef().packetData.data(), EPacketHeaderLength), yield);

		m_receivePacket.DecodeHeader();

		if (m_receivePacket.GetPacketDef().packetLength > EPacket_Small_MaxBodyLength)
		{
			m_receiveBigPacket.GetPacketDef().packetLength = m_receivePacket.GetPacketDef().packetLength;

			boost::asio::async_read(*m_socket,
				boost::asio::buffer(m_receiveBigPacket.GetPacketDef().packetData.data(), m_receiveBigPacket.GetPacketDef().packetLength), yield);
		}
		else
		{
			boost::asio::async_read(*m_socket,
				boost::asio::buffer(m_receivePacket.GetPacketDef().packetData.data(), m_receivePacket.GetPacketDef().packetLength), yield);
		}

		bool reCount = false;
		if (m_receivePacketCount >= 100000000)
		{
			reCount = true;
			m_receivePacketCount = 1;
		}
		else
		{
			++m_receivePacketCount;
		}

		// 投递网络消息包
		if (m_receivePacket.GetPacketDef().packetLength > EPacket_Small_MaxBodyLength)
		{
			m_receiveBigPacket.SetConnectionID(m_connectionID);
			if (!reCount && m_receivePacketCount == 1)
			{
				m_receiveBigPacket.SetPacketType(EPacketTypeSpecial_Connect);
			}
			m_netServer.AddReceivedPacket(m_receiveBigPacket);
			m_receiveBigPacket.Clear();
		}
		else
		{
			m_receivePacket.SetConnectionID(m_connectionID);
			if (!reCount && m_receivePacketCount == 1)
			{
				m_receivePacket.SetPacketType(EPacketTypeSpecial_Connect);
			}
			m_netServer.AddReceivedPacket(m_receivePacket);
			m_receivePacket.Clear();
		}
	}
};

// 发送消息包
void gce_connection::Send(SendPacket& INsendPacket)
{
	if (m_socket && m_socket->is_open())
	{
		std::lock_guard<std::mutex> guard(m_sendDequeMutex);
		m_sendPacketDeque.push_back(std::move(INsendPacket));
	}
}

// 发送
void gce_connection::OnSend(gce::actor<gce::stackful>& self)
{
	gce::yield_t yield = self.get_yield();
	while (m_runing)
	{
		std::lock_guard<std::mutex> guard(m_sendDequeMutex);
		if (!m_sendPacketDeque.empty())
		{
			boost::asio::async_write(*m_socket,
				boost::asio::buffer(m_sendPacketDeque.front().GetEnPacket().GetString(),
				m_sendPacketDeque.front().GetEnPacket().GetLength()), yield);

			m_sendPacketDeque.pop_front();
		}
	}
};