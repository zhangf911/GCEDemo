#include "gce_server.h"
#include "gce_connection.h"

gce_server::gce_server(
	gce::context& ctx,
	gce::actor<gce::threaded>& base,
	const std::string& host,
	boost::uint16_t port
	)
	: m_ctx(ctx)
	, m_acpr(m_ctx.get_io_service())
	, m_host(host)
	, m_port(port)
{
	m_connectionID = 0;
	m_receiveDequeCount = 0;
	m_receivePacketDequeMax = 0;

	gce::spawn(base, boost::bind(&gce_server::Run, this, _1));

	gce::spawn(base, boost::bind(&gce_server::DespatchPacket, this, _1));
}

gce_server::~gce_server()
{
	gce::errcode_t ignored_ec;
	m_acpr.close(ignored_ec);
}

void gce_server::Run(gce::actor<gce::stackful>& self)
{
	try
	{
		gce::yield_t yield = self.get_yield();
		gce::io_service_t& ios = m_ctx.get_io_service();

		boost::asio::ip::address addr;
		std::cout << "Host : " << m_host << " Port : " << m_port << std::endl;
		addr.from_string(m_host);
		boost::asio::ip::tcp::endpoint ep(addr, m_port);
		m_acpr.open(ep.protocol());

		m_acpr.set_option(boost::asio::socket_base::reuse_address(true));
		m_acpr.bind(ep);

		m_acpr.set_option(boost::asio::socket_base::receive_buffer_size(640000));
		m_acpr.set_option(boost::asio::socket_base::send_buffer_size(640000));

		m_acpr.listen(1024);

		m_acpr.set_option(boost::asio::ip::tcp::no_delay(true));
		m_acpr.set_option(boost::asio::socket_base::keep_alive(true));
		m_acpr.set_option(boost::asio::socket_base::enable_connection_aborted(true));

		while (true)
		{
			gce::errcode_t ec;
			boost::shared_ptr<socket_t> sock(new socket_t(ios));
			m_acpr.async_accept(*sock, yield[ec]);

			if (!ec)
			{
				std::cout << "new connection!\n";
				gce::spawn(self, boost::bind(&gce_server::AddNewSession, this, _1, sock));
			}
			else
			{
				break;
			}
		}
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}

void gce_server::AddNewSession(gce::actor<gce::stackful>& self, boost::shared_ptr<socket_t> sock)
{
	try
	{
		gce::yield_t yield = self.get_yield();

		++m_connectionID;
		// 显示远程IP 
		boost::system::error_code ignored_ec;
		std::cout << "New Connection From IP : " << sock->remote_endpoint(ignored_ec).address()
			<< " ConnectionID : " << m_connectionID << std::endl;

		std::shared_ptr<gce_connection> connPtr(new gce_connection(*this, sock, m_connectionID));

		m_connectionMap[m_connectionID] = connPtr;

		gce::spawn(self, boost::bind(&gce_connection::OnReceive, connPtr, _1));

		gce::spawn(self, boost::bind(&gce_connection::OnSend, connPtr, _1));
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}

void gce_server::DeleteSession(size_t INconnectionID)
{
	auto it = m_connectionMap.find(INconnectionID);
	if(it != m_connectionMap.end())
	{
		m_connectionMap.erase(it);
	}
}

void gce_server::Send(UInt32 INclientID, SendPacket& INsendPacket)
{
	auto it = m_connectionMap.find(INclientID);
	if(it != m_connectionMap.end())
	{
		it->second->Send(INsendPacket);
	}
}

void gce_server::AddReceivedPacket(ReceivedPacket& INpacket)
{
	std::lock_guard<std::mutex> guard(m_receiveDequeMutex);

	if (m_receivePacketDequeMax > 0 && m_receivedPacketDeque.size() >= m_receivePacketDequeMax)
	{
		cout << "ReceivePacket size :" << m_receivedPacketDeque.size() << endl;
		//return;  // 暂时只是输出提示，不做逻辑处理 
	}

	m_receivedPacketDeque.push_back(std::move(INpacket));
}

UInt32 gce_server::GetReceivedPacketDeque(ReceivedPacketDeque& INpacketDeque)
{
	std::lock_guard<std::mutex> guard(m_receiveDequeMutex);

	if (m_receiveDequeCount == 0 || m_receivedPacketDeque.size() <= m_receiveDequeCount)
	{
		INpacketDeque = std::move(m_receivedPacketDeque);
		m_receivedPacketDeque.clear();

		return 0;
	}
	else
	{
		for (auto it = m_receivedPacketDeque.begin(); it != m_receivedPacketDeque.begin() + m_receiveDequeCount; ++it)
		{
			INpacketDeque.push_back(std::move(*it));
		}
		m_receivedPacketDeque.erase(m_receivedPacketDeque.begin(), m_receivedPacketDeque.begin() + m_receiveDequeCount);

		return m_receivedPacketDeque.size();
	}

	return 0;
}


void gce_server::DespatchPacket(gce::actor<gce::stackful>& self)
{
	while (true)
	{
		UInt32 iResiduePacket = 0;

		gce_server::ReceivedPacketDeque packetDeque;
		iResiduePacket = GetReceivedPacketDeque(packetDeque);

		int packetSize = packetDeque.size();

		while (!packetDeque.empty())
		{
			ReceivedPacket packet = std::move(packetDeque.front());

			UInt32 errorCode = 0;

			UInt32  iPacketType = packet.GetPacketType();
			NOTE_LOG("Receive Packet, Type : %d Size : %d",
				iPacketType, packet.GetPacketDef().packetLength);

			m_packetHandler(packet);

			packetDeque.pop_front();
		}

		if (packetSize > 100)
		{
			NOTE_LOG("GetReceivedPacketDeque size=%d, end...", packetSize);
		}
	}
}