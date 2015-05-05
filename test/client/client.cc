#include <boost/test/unit_test.hpp>
#define private public
#include "client.hh"
#include "server.hh"
#include "response.hh"

struct SetCookie : RequestHandler {
	void handleRequest(RequestPtr req, ResponsePtr res) {
		for(auto c : cookies)
			res->setCookie(c);	
	}
	std::vector<response_cookie_t> cookies;
};

struct EchoCookie : RequestHandler {
	void handleRequest(RequestPtr req, ResponsePtr res) {
		auto cptr = req->getHeader("Cookie");
		if(cptr)
			res->out() << *cptr;
	}
};

BOOST_AUTO_TEST_CASE(client_request_test)
{
/*
	Client c;
	c.request("GET", "http://www.example.com",
		[](ResponsePtr res) {
			BOOST_REQUIRE(res);
			BOOST_CHECK(res->status() == Response::ok);
		}
	);
	c.request("GET", "https://www.example.com",	
		[](ResponsePtr res) {
			BOOST_REQUIRE(res);
			BOOST_CHECK(res->status() == Response::ok);
		}
	);
	c.apply();
*/
}

BOOST_AUTO_TEST_CASE(client_cookie_simple_test)
{
	std::stringstream config("{\"http port\":\"8888\"}");
	Server s(config);
	auto set_cookie = new SetCookie();
	s.addHandler("/", set_cookie);
	Client c(s.service());
	c.enableCookie();
	set_cookie->cookies.push_back(response_cookie_t().setKey("key1").setVal("val1"));
	c.request("GET", "http://localhost:8888", [&](ResponsePtr res) {
		auto cookie_jar = c.cookie_jar_;
		BOOST_REQUIRE(cookie_jar.size());
		BOOST_CHECK(cookie_jar[0].key == "key1");
		BOOST_CHECK(cookie_jar[0].val == "val1");
		BOOST_CHECK(cookie_jar[0].expires == 0);
		BOOST_CHECK(cookie_jar[0].domain == "localhost");
		BOOST_CHECK(cookie_jar[0].path == "/");
		BOOST_CHECK(cookie_jar[0].secure == false);
		BOOST_CHECK(cookie_jar[0].httponly == false);
		s.stop();
	});
	s.run();
}

BOOST_AUTO_TEST_CASE(client_cookie_muti_test)
{
	std::stringstream config("{\"http port\":\"8888\"}");
	Server s(config);
	auto set_cookie = new SetCookie();
	auto echo_cookie = new EchoCookie();
	s.addHandler("/", set_cookie);
	s.addHandler("/echo", echo_cookie);
	set_cookie->cookies.push_back(response_cookie_t().setKey("key1").setVal("val1"));
	set_cookie->cookies.push_back(response_cookie_t().setKey("key2").setVal("val2").setMaxAge(10));
	set_cookie->cookies.push_back(response_cookie_t().setKey("key3").setVal("val3").setMaxAge(10));
	Client c(s.service());
	c.enableCookie();
	c.request("GET", "http://localhost:8888", [&](ResponsePtr res) {
		c.request("GET", "http://localhost:8888/echo", [&](ResponsePtr res) {
			std::stringstream ss;
			ss << res->out().rdbuf();
			BOOST_CHECK(ss.str() == "key2=val2; key3=val3");
			Log("DEBUG") << ss.str();
			s.stop();
		});
	});
	s.run();
}

BOOST_AUTO_TEST_CASE(client_cookie_expires_test)
{
	std::stringstream config("{\"http port\":\"8888\"}");
	Server s(config);
	auto set_cookie = new SetCookie();
	auto echo_cookie = new EchoCookie();
	s.addHandler("/", set_cookie);
	s.addHandler("/echo", echo_cookie);
	set_cookie->cookies.push_back(response_cookie_t().setKey("key1").setVal("val1").setMaxAge(1));
	set_cookie->cookies.push_back(response_cookie_t().setKey("key2").setVal("val2").setMaxAge(10));
	Client c(s.service());
	c.enableCookie();
	c.request("GET", "http://localhost:8888", [&](ResponsePtr res) {
		std::this_thread::sleep_for(std::chrono::seconds(2));
		c.request("GET", "http://localhost:8888/echo", [&](ResponsePtr res) {
			std::stringstream ss;
			ss << res->out().rdbuf();
			BOOST_CHECK(ss.str() == "key2=val2");
			Log("DEBUG") << ss.str();
			s.stop();
		});
	});
	s.run();
}

BOOST_AUTO_TEST_CASE(client_cookie_path_test)
{
	std::stringstream config("{\"http port\":\"8888\"}");
	Server s(config);
	auto set_cookie = new SetCookie();
	auto echo_cookie = new EchoCookie();
	s.addHandler("/", set_cookie);
	s.addHandler("/echo", echo_cookie);
	set_cookie->cookies.push_back(response_cookie_t().setKey("key1").setVal("val1").setPath("/echo").setMaxAge(10));
	set_cookie->cookies.push_back(response_cookie_t().setKey("key2").setVal("val2").setPath("/no_such_path").setMaxAge(10));
	set_cookie->cookies.push_back(response_cookie_t().setKey("key3").setVal("val3").setPath("/").setMaxAge(10));
	set_cookie->cookies.push_back(response_cookie_t().setKey("key4").setVal("val4").setPath("/echobad").setMaxAge(10));
	set_cookie->cookies.push_back(response_cookie_t().setKey("key5").setVal("val5").setPath("/echo/").setMaxAge(10));
	Client c(s.service());
	c.enableCookie();
	c.request("GET", "http://localhost:8888", [&](ResponsePtr res) {
		c.request("GET", "http://localhost:8888/echo", [&](ResponsePtr res) {
			std::stringstream ss;
			ss << res->out().rdbuf();
			BOOST_CHECK(ss.str() == "key1=val1; key3=val3; key5=val5");
			Log("DEBUG") << ss.str();
			s.stop();
		});
	});
	s.run();
}
