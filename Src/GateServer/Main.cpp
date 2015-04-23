#include "GateServer.h"
#include "gce_server.h"
#include <gce/actor/all.hpp>

int main()
{
	LOG_INSTANCE->Initial("GateServer.log");
	LOG_INSTANCE->SetLogPriority(Library::ELogType_All & ~Library::ELogType_Console_Debug);

	gce::attributes attr;
	attr.id_ = gce::atom("gate_server");

	gce::context ctx_(attr);

	gce::actor<gce::threaded> base = gce::spawn(ctx_);

	gce::net_option opt;
	opt.reconn_period_ = boost::chrono::seconds(10);
	opt.reconn_try_ = 3;
	opt.init_reconn_period_ = boost::chrono::seconds(10);
	opt.init_reconn_try_ = 3;

	gce::connect(base, gce::atom("game_server"), "tcp://127.0.0.1:7001", false, opt);

	gce_server svr(ctx_, base, "127.0.0.1", 6001);

	GateServer gateServer(svr);

	gateServer.Start();

	gateServer.Run();

	system("pause");

	return 0;
}