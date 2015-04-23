#ifndef GAMESERVER_GAMESERVER_H
#define GAMESERVER_GAMESERVER_H

#include "CommDef.h"

class LoginServer
{
public:
	LoginServer();

	~LoginServer();

public:
	void Start();

	void Run();

	void Stop();

private:
	
};

#endif