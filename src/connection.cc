#include "connection.hh"

#define ASYNC_APPLY(op, func, handler, ...)								\
enqueue##op([=, ptr = shared_from_this()] {							\
	auto handle = [this, handler, ptr](const boost::system::error_code& e, size_t n) {	\
		handler(e, n);									\
		dequeue##op();									\
	};											\
	func(__VA_ARGS__, handle);								\
})
	
/*
	if(sock.type == socket_type::ordinary) {						\
		func(*sock.ordinary_socket, __VA_ARGS__, handle);				\
	} else if(sock.type == socket_type::ssl) {						\
		func(*sock.ssl_socket, __VA_ARGS__, handle);					\
	} else {										\
		Log("PANIC") << "UNKNOWN SOCKET TYPE";						\
		assert(false && "unknown socket type");						\
	}											\
})
*/
void 
Connection::asyncRead(std::function<size_t(const boost::system::error_code &, size_t)> completion,
	std::function<void(const boost::system::error_code &, size_t)> handler) 
{
	ASYNC_APPLY(Read, async_read, handler, completion);
}

void 
Connection::asyncReadUntil(const std::string& delim, 
	std::function<void(const boost::system::error_code &, size_t)> handler)
{
	ASYNC_APPLY(Read, async_read_until, handler, delim);
}
	
void 
Connection::asyncWrite(const std::string& msg, 
	std::function<void(const boost::system::error_code&, size_t)> handler)
{
	ASYNC_APPLY(Write, async_write, handler, msg);
}
