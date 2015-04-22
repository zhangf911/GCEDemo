﻿///
/// Copyright (c) 2009-2014 Nous Xiong (348944179 at qq dot com)
///
/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///
/// See https://github.com/nousxiong/gce for latest version.
///

#ifndef GCE_ACTOR_CONFIG_HPP
#define GCE_ACTOR_CONFIG_HPP

#include <gce/config.hpp>
#include <gce/actor/user.hpp>
#include <gce/integer.hpp>
#include <gce/actor/atom.hpp>

/// Do not define WIN32_LEAN_AND_MEAN before include boost/atomic.hpp
/// Or you will recv:
///   "error C3861: '_InterlockedExchange': identifier not found"
/// Under winxp(x86) vc9
#include <boost/atomic.hpp>

#include <gce/amsg/amsg.hpp>
#include <boost/system/error_code.hpp>
#include <boost/chrono.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/spawn.hpp>
#include <vector>
#include <string>
#include <utility>

#define GCE_MAX_MSG_SIZE (GCE_SOCKET_RECV_CACHE_SIZE - GCE_SOCKET_RECV_MAX_SIZE)

namespace gce
{
typedef boost::asio::io_service io_service_t;
typedef boost::asio::io_service::strand strand_t;
typedef boost::asio::system_timer timer_t;
typedef boost::asio::yield_context yield_t;

typedef boost::system::error_code errcode_t;
typedef boost::chrono::system_clock::duration duration_t;
typedef boost::chrono::seconds seconds_t;
static seconds_t zero(0);
static seconds_t infin(99999999);

typedef boost::uint32_t sid_t;
static boost::uint32_t const sid_nil = static_cast<sid_t>(-1);

typedef boost::uint64_t match_t;
static boost::uint64_t const match_nil = static_cast<boost::uint64_t>(-1);
typedef std::vector<match_t> match_list_t;

typedef match_t ctxid_t;
static match_t const ctxid_nil = static_cast<boost::uint64_t>(-1);

typedef boost::uint64_t timestamp_t;

enum link_type
{
  no_link = 0,
  linked,
  monitored
};

/// actor type tags
struct stackful {};
struct stackless {};
struct threaded {};
struct nonblocked {};

typedef match_t exit_code_t;
static exit_code_t const exit = atom("gce_actor_exit");
static exit_code_t const exit_normal = atom("gce_ex_normal");
static exit_code_t const exit_except = atom("gce_ex_except");
static exit_code_t const exit_remote = atom("gce_ex_remote");
static exit_code_t const exit_already = atom("gce_ex_already");
static exit_code_t const exit_neterr = atom("gce_ex_neterr");

namespace detail
{
typedef std::basic_string<byte_t, std::char_traits<byte_t>, std::allocator<byte_t> > bytes_t;
static match_t const msg_hb = atom("gce_msg_hb");
static match_t const msg_reg_skt = atom("gce_reg_skt");
static match_t const msg_dereg_skt = atom("gce_dereg_skt");
static match_t const msg_reg_svc = atom("gce_reg_svc");
static match_t const msg_dereg_svc = atom("gce_dereg_svc");
static match_t const msg_login = atom("gce_login");
static match_t const msg_login_ret = atom("gce_login_ret");
static match_t const msg_link = atom("gce_link");
static match_t const msg_send = atom("gce_send");
static match_t const msg_relay = atom("gce_relay");
static match_t const msg_request = atom("gce_request");
static match_t const msg_reply = atom("gce_reply");
static match_t const msg_stop = atom("gce_stop");
static match_t const msg_spawn = atom("gce_spawn");
static match_t const msg_spawn_ret = atom("gce_spawn_ret");
static match_t const msg_new_actor = atom("gce_new_actor");
static match_t const msg_new_conn = atom("gce_new_conn");
static match_t const msg_new_bind = atom("gce_new_bind");

enum socket_type
{
  socket_comm = 0,
  socket_router,
  socket_joint
};

enum spawn_type
{
  spw_nil = 0,
  spw_stacked,
  spw_evented
};

typedef boost::asio::coroutine coro_t;
} /// namespce detail

typedef std::pair<ctxid_t, detail::socket_type> ctxid_pair_t;
} /// namespace gce

#ifndef GCE_PACK
# define GCE_PACK AMSG
#endif

#ifndef GCE_REENTER
# define GCE_REENTER(t) BOOST_ASIO_CORO_REENTER(t.coro())
#endif

#ifndef GCE_YIELD
# define GCE_YIELD BOOST_ASIO_CORO_YIELD
#endif

#endif /// GCE_ACTOR_CONFIG_HPP
