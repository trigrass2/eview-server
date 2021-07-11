#pragma once

#include <boost/thread/mutex.hpp>
#include <string>
#include <vector>
#include <functional>
#include "boost/function.hpp"
#include "cpp_redis/network/tcp_client.hpp"
#include "cpp_redis/builders/reply_builder.hpp"

namespace cpp_redis {

namespace network {

class redis_connection {
public:
    //! ctor & dtor
    redis_connection(void);
    ~redis_connection(void);

    //! copy ctor & assignment operator
    //redis_connection(const redis_connection&) = delete;
    //redis_connection& operator=(const redis_connection&) = delete;

public:
    //! handle connection
	typedef boost::function<void(redis_connection&)> redis_disconnection_callback_t;
	typedef boost::function<void(redis_connection&, reply&)> redis_reply_callback_t;
    void connect(const std::string& host, unsigned int port = 6379,
    const redis_disconnection_callback_t& redis_disconnection_handler = NULL,
    const redis_reply_callback_t& redis_reply_callback               = NULL);
	void connect();

    void disconnect(void);
    bool is_connected(void);

    //! disconnection handler
    void set_disconnection_handler(const redis_disconnection_callback_t& handler);

    //! send cmd
    redis_connection & send(const std::vector<std::string>& redis_cmd);

	//! commit pipelined transaction
	redis_connection& commit(void);

    //! receive handler
    typedef boost::function<void(redis_connection&, reply&)> reply_callback_t;
    void set_reply_callback(const reply_callback_t& handler);

private:
    //! receive & disconnection handlers
	bool tcp_client_receive_handler(network::tcp_client&, const char *, int);
    void tcp_client_disconnection_handler(network::tcp_client&);

    void build_command(const std::vector<std::string>& redis_cmd,std::string &strCmd);

public:
    //! tcp client for redis connection
    network::tcp_client m_tcpClient;

    //! reply callback
    redis_reply_callback_t m_reply_callback;

    //! user defined disconnection handler
    redis_disconnection_callback_t m_disconnection_callback;

    //! reply builder
    builders::reply_builder m_builder;

    //! thread safety
    boost::mutex m_disconnection_handler_mutex;
	boost::mutex m_reply_callback_mutex;

	//! internal buffer used for pipelining
	std::string m_buffer;

	//! protect internal buffer against race conditions
	boost::mutex m_buffer_mutex;

private:
	cpp_redis::network::tcp_client::tcp_disconnection_handler_t m_disconnection_handler;
	cpp_redis::network::tcp_client::tcp_receive_handler_t m_receive_handler;
	std::string m_host;
	std::size_t m_port;
};

} //! network

} //! cpp_redis
