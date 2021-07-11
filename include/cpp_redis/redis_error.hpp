#pragma once

#include <stdexcept>

namespace cpp_redis {

class redis_error : public std::runtime_error {
public:
	redis_error(const char *msg) :std::runtime_error(msg){	}
	redis_error(const std::string& _Message) :std::runtime_error(_Message){	}
  //using std::runtime_error::runtime_error;
  //using std::runtime_error::what;
};

} //! cpp_redis
