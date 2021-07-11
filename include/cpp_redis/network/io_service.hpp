#pragma once

#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>
#include "boost/function.hpp"

namespace cpp_redis {

namespace network {
//
//! boost io service wrapper
class io_service {
public:
    //! ctor & dtor
    io_service(void);
    ~io_service(void);

    //! copy ctor & assignment operator
    //io_service(const io_service&)= delete;
    //io_service& operator=(const io_service&)= delete;

public:
    //! methods
    void run(void);
    void post(const boost::function<void()>& fct);

    //! getters
    boost::asio::io_service& get(void);

private:
    boost::asio::io_service m_io_service;
    boost::asio::io_service::work m_work;
    boost::thread m_io_service_thread;
	void runInternal();
};

} //! network

} //! cpp_redis
