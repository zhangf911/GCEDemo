#ifndef GAMESERVER_GAMESERVER_H
#define GAMESERVER_GAMESERVER_H

#include "CommDef.h"

class GameServer
{
public:
	GameServer();

	~GameServer();

public:
	void Start();

	void Run();

	void Stop();

private:
	
};

#endif