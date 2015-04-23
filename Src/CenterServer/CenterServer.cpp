#include "CenterServer.h"

#include <thread>

CenterServer::CenterServer()
{

}

CenterServer::~CenterServer()
{

}

void CenterServer::Start()
{

}

void CenterServer::Run()
{
	while (true)
	{

		std::chrono::milliseconds dura(40);
		std::this_thread::sleep_for(dura);
	}
}

void CenterServer::Stop()
{

}