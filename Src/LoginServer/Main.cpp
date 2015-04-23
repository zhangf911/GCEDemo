#include "CommDef.h"
#include "LoginServer.h"
#include <gce/actor/all.hpp>

int main()
{
	LOG_INSTANCE->Initial("LoginServer.log");
	LOG_INSTANCE->SetLogPriority(Library::ELogType_All & ~Library::ELogType_Console_Debug);

	gce::attributes attr;
	attr.id_ = gce::atom("login_server");

	gce::context ctx_(attr);

	gce::actor<gce::threaded> base_(gce::spawn(ctx_));

	gce::net_option opt;
	opt.heartbeat_period_ = boost::chrono::seconds(15);
	opt.heartbeat_count_ = 5;

	gce::bind(base_, "tcp://127.0.0.1:5001");

	LoginServer server;
	server.Start();

	server.Run();

	return 0;
}