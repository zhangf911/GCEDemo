#include "LoginServer.h"

#include <thread>

LoginServer::LoginServer()
{

}

LoginServer::~LoginServer()
{

}

void LoginServer::Start()
{

}

void LoginServer::Run()
{
	while (true)
	{

		std::chrono::milliseconds dura(40);
		std::this_thread::sleep_for(dura);
	}
}

void LoginServer::Stop()
{

}