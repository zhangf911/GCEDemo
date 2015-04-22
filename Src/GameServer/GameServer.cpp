#include "GameServer.h"

#include <thread>

GameServer::GameServer()
{

}

GameServer::~GameServer()
{

}

void GameServer::Start()
{

}

void GameServer::Run()
{
	while (true)
	{

		std::chrono::milliseconds dura(40);
		std::this_thread::sleep_for(dura);
	}
}

void GameServer::Stop()
{

}