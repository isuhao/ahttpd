#include <boost/test/unit_test.hpp>
#include "ThreadPool.hh"
#include "log.hh"

using namespace ahttpd;

BOOST_AUTO_TEST_CASE(thread_pool_test)
{
	ThreadPool pool(1, []{ 
		Log("NOTE") << "task start(thread pool)";
	}, []{
		Log("NOTE") << "task end(thread pool)";
	});
	bool flag = false;
	pool.enqueue([&]{ 
		Log("NOTE") << "do task(thread pool)";
		flag = true;	
		std::this_thread::sleep_for(std::chrono::seconds(2));
	});
	BOOST_CHECK(pool.size());
	Log("NOTE") << "pool.size() = " << pool.size();
	sleep(3);
	BOOST_CHECK(flag);
}
