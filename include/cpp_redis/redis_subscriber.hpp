#pragma once

//#include <functional>
#include <map>
#include "boost/thread/mutex.hpp"
#include <string>
#include "boost/function.hpp"
#include <cpp_redis/network/redis_connection.hpp>

namespace cpp_redis {

class redis_subscriber {
public:
  //! ctor & dtor
  redis_subscriber();
  ~redis_subscriber(void);

  //! copy ctor & assignment operator
  //redis_subscriber(const redis_subscriber&){}; //= delete;
  //redis_subscriber& operator=(const redis_subscriber&){}; //= delete;

public:
  //! handle connection
  typedef boost::function<void(redis_subscriber&)> sub_disconnection_callback_t;
  void connect(const std::string& host = "127.0.0.1", std::size_t port = 6379,
    const sub_disconnection_callback_t& sub_disconnection_callback = NULL);
  void disconnect(void);
  bool is_connected(void);
  redis_subscriber& auth(const std::string& password);
  //! subscribe - unsubscribe
  typedef boost::function<void(const std::string&, const std::string&)> subscribe_callback_t;
  typedef boost::function<void(int64_t)> acknowledgement_callback_t;
  redis_subscriber& subscribe(const std::string& channel, const subscribe_callback_t& callback, const acknowledgement_callback_t& acknowledgement_callback = NULL);
  redis_subscriber& psubscribe(const std::string& pattern, const subscribe_callback_t& callback, const acknowledgement_callback_t& acknowledgement_callback = NULL);
  redis_subscriber& unsubscribe(const std::string& channel);
  redis_subscriber& punsubscribe(const std::string& pattern);

  //! commit pipelined transaction
  redis_subscriber& commit(void);

private:
  struct callback_holder {
    subscribe_callback_t subscribe_callback;
    acknowledgement_callback_t acknowledgement_callback;
  };

private:
  void connection_receive_handler(network::redis_connection&, reply& reply);
  void connection_disconnection_handler(network::redis_connection&);

  void handle_acknowledgement_reply(const std::vector<reply>& reply);
  void handle_subscribe_reply(const std::vector<reply>& reply);
  void handle_psubscribe_reply(const std::vector<reply>& reply);

  void call_acknowledgement_callback(const std::string& channel, const std::map<std::string, callback_holder>& channels, boost::mutex& channels_mtx, int64_t nb_chans);

public:
	void *m_pCaller;
private:
  //! redis connection
  network::redis_connection m_redisConn;

  //! (p)subscribed channels and their associated channels
  std::map<std::string, callback_holder> m_subscribed_channels;
  std::map<std::string, callback_holder> m_psubscribed_channels;

  //! disconnection handler
  sub_disconnection_callback_t m_disconnection_callback;

  //! thread safety
  boost::mutex m_psubscribed_channels_mutex;
  boost::mutex m_subscribed_channels_mutex;
};

} //! cpp_redis
