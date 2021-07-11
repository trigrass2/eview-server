#pragma once

#include <memory>
#include <boost/thread/mutex.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>

namespace cpp_redis {

//! logger_iface
//! should be inherited by any class intended to be used for logging
class logger_iface {
public:
  //! ctor & dtor
  logger_iface(void)          {}; //= default;
  virtual ~logger_iface(void) {}; //= default;

  //! copy ctor & assignment operator
  //logger_iface(const logger_iface&) {}; //= default;
  //logger_iface& operator=(const logger_iface&) {return *this;}; //= default;

public:
  virtual void debug(const std::string& msg, const std::string& file, std::size_t line) = 0;
  virtual void info(const std::string& msg, const std::string& file, std::size_t line)  = 0;
  virtual void warn(const std::string& msg, const std::string& file, std::size_t line)  = 0;
  virtual void error(const std::string& msg, const std::string& file, std::size_t line) = 0;
};

//! default logger class provided by the library
class logger : public logger_iface {
public:
  //! log level
  enum log_level {
	  log_level_error = 0,
	  log_level_warn  = 1,
	  log_level_info  = 2,
	  log_level_debug = 3
  };

public:
  //! ctor & dtor
  logger(log_level level = log_level_info);
  ~logger(void)          {}; //= default;

  //! copy ctor & assignment operator
  //logger(const logger&) {}; //= default;
  //logger& operator=(const logger&) {return *this;}; //= default;

public:
  void debug(const std::string& msg, const std::string& file, std::size_t line);
  void info(const std::string& msg, const std::string& file, std::size_t line);
  void warn(const std::string& msg, const std::string& file, std::size_t line);
  void error(const std::string& msg, const std::string& file, std::size_t line);

private:
  log_level m_level;
  boost::mutex m_mutex;
};

//! variable containing the current logger
//! by default, not set (no logs)
extern logger_iface* active_logger;

//! convenience functions used internaly to call the logger
void debug(const std::string& msg, const std::string& file, std::size_t line);
void info(const std::string& msg, const std::string& file, std::size_t line);
void warn(const std::string& msg, const std::string& file, std::size_t line);
void error(const std::string& msg, const std::string& file, std::size_t line);

//! convenience macro to log with file and line information
#ifdef __CPP_REDIS_LOGGING_ENABLED
#define __CPP_REDIS_LOG(level, msg) cpp_redis::level(msg, __FILE__, __LINE__);
#else
#define __CPP_REDIS_LOG(level, msg)
#endif /* __CPP_REDIS_LOGGING_ENABLED */

} //! cpp_redis
