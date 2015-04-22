#include "CommDef.h"
#include "GameServer.h"
#include <gce/actor/all.hpp>

int main()
{
	LOG_INSTANCE->Initial("GameServer.log");
	LOG_INSTANCE->SetLogPriority(Library::ELogType_All & ~Library::ELogType_Console_Debug);

	gce::attributes attr;
	attr.id_ = gce::atom("game_server");

	gce::context ctx_(attr);

	gce::actor<gce::threaded> base_(gce::spawn(ctx_));

	gce::net_option opt;
	opt.heartbeat_period_ = boost::chrono::seconds(15);
	opt.heartbeat_count_ = 5;

	gce::bind(base_, "tcp://127.0.0.1:12345");

	GameServer gameServer;
	gameServer.Start();

	gameServer.Run();

	return 0;
}