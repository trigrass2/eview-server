#pragma once

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/atomic/atomic.hpp>
#include <stdexcept>
#include <boost/asio.hpp>
#include "boost/function.hpp"
#include "cpp_redis/network/io_service.hpp"
#include <list>
#include <string>

using boost::asio::ip::tcp;

namespace cpp_redis {

namespace network {

//! tcp_client
//! async tcp client based on boost asio
class tcp_client {
public:
    //! ctor & dtor
    tcp_client(void);
    ~tcp_client(void);

    //! assignment operator & copy ctor
    //tcp_client(const tcp_client&) = delete;
    //tcp_client& operator=(const tcp_client&) = delete;

    //! returns whether the client is connected or not
    bool is_connected(void);

    //! handle connection & disconnection
	  //! handle connection & disconnection
	typedef boost::function<void(tcp_client&)> tcp_disconnection_handler_t;
	typedef boost::function<bool(tcp_client&, const char *szRecvBuf, int nRecvLen)> tcp_receive_handler_t;
    void connect(const std::string& host, unsigned int port, const tcp_disconnection_handler_t& disconnection_handler, const tcp_receive_handler_t& receive_handler);
	void connect(); // 用于重连时使用
	void tryConnect(); // 用于重连时使用
    void disconnect(void);

    //! send data
    void send(const std::string& buffer);
    //void send(const std::vector<char>& buffer);

    //! set receive callback
    //! called each time some data has been received
    //! Returns true if everything is ok, false otherwise (trigger disconnection)
    typedef boost::function<bool(tcp_client&, const char *szRecvBuf, int nRecvLen)> tcp_receive_callback_t;
    void set_receive_handler(const tcp_receive_callback_t& handler);

    //! set disconnection callback
    //! called each time a disconnection has been detected
    typedef boost::function<void(tcp_client&)> tcp_disconnection_callback_t;
	void set_disconnection_handler(const tcp_disconnection_callback_t& handler);

private:
    //! make boost asio async read and write operations
	void handler_read_response(const boost::system::error_code & error, size_t bytes_transferred);
    void async_read(void);
    void async_write(void);

    //! close socket and call disconnect callback
    //! called in case of error
    void process_disconnection(void);
	void handle_connect(const boost::system::error_code &error);
	void handle_disconnect(boost::condition_variable *close_socket_cond_var, boost::atomic<bool> *is_notified);
	void onAsyncWrite(boost::system::error_code error, std::size_t length);

private:
    //! io service
    static io_service m_io_service;
    tcp::socket m_socket;

    //! is connected
    boost::atomic<bool> m_is_connected;
    //! async connect, 判断是否连接完成了
	boost::atomic<bool> m_is_connectOver;
	// 连接是否完成的信号量
    boost::condition_variable m_conn_cond_var;


    //! buffers
	char *m_szReadBuffer;
	std::list<std::string> m_list_write_buffer;

    //! handlers
    tcp_receive_callback_t m_receive_callback;
    tcp_disconnection_callback_t m_disconnection_callback;

    //! thread safety
	boost::mutex m_write_buffer_mutex;
	boost::mutex m_receive_handler_mutex;
	boost::mutex m_disconnection_handler_mutex;
	boost::mutex m_conn_mutex; // 用于连接的互斥信号量，避免多个地方同时调用及做为条件变量m_conn_cond_var之用
private:
	std::string m_host;
	std::size_t m_port;
	void startTryConnectTimer();
	void onTryConnectTimer(const boost::system::error_code& e, boost::asio::deadline_timer* t);
};

} //! network

} //! cpp_redis
