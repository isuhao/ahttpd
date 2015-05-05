#include "client.hh"
#include "parser.hh"
#include "response.hh"
#include "request.hh"
#include "SslConnection.hh"
#include "TcpConnection.hh"
#include <regex>
#include <boost/asio/ssl.hpp>
#include <cstdio>

namespace {
bool
is_ip_address(const std::string& server)
{
	const static std::regex ip_reg("[[:number:]]{1,3}\\."
		"[[:number:]]{1,3}\\.[[:number:]]{1,3}\\.[[:number:]]{1,3}");
	std::smatch ignore_result;
	return std::regex_match(server, ignore_result, ip_reg);
}

bool
is_domain_security_ok(std::string domain, const std::string& server)
{
	if(domain[0] == '.')
		domain = domain.substr(1, domain.size());

	if(domain.size() > server.size()) 
		return false;

	if(strcasecmp(domain.c_str(), server.substr(server.size() - domain.size(), server.size()).c_str()))
		return false;

	if(domain.size() == server.size())
		return true;

	if(is_ip_address(server))
		return false;

	return true;
}
bool
is_path_security_ok(std::string path, std::string real_path)
{
	if(path == "/")
		return true;

	if(path[path.size() - 1] == '/')
		path = path.substr(0, path.size() - 1);

	if(real_path != "/" && real_path[real_path.size() - 1] == '/')
		real_path = real_path.substr(0, real_path.size() - 1);

	if(path.size() > real_path.size())
		return false;
	
	if(path != real_path.substr(0, path.size()))
		return false;
	
	if(path.size() == real_path.size())
		return true;

	if(real_path[path.size()] == '/')
		return true;

	return false;
}
}

Client::Client(boost::asio::io_service& service)
 	:service_(service) 
{
	ssl_context_ = new boost::asio::ssl::context(boost::asio::ssl::context::sslv23);
	ssl_context_->set_default_verify_paths();
}
	
Client::Client()
 	: Client(*(new boost::asio::io_service()))
{
	service_holder_.reset(&service_);
}

Client::~Client() { delete ssl_context_; }

void
Client::apply()
{
	service_.run();
}

void
Client::request(const std::string& method, const std::string& url,
	std::function<void(ResponsePtr)> res_handler,
	std::function<void(RequestPtr)> req_handler)
{
	static const std::regex url_reg("((http|https)(://))?((((?!@)[[:print:]])*)@)?"
		"(((?![/\\?])[[:print:]])*)([[:print:]]+)?");	/**< http://user:pass@server:port/path?query */
	std::smatch results;
	if(std::regex_search(url, results, url_reg)) {
		std::string scheme = "http";
		if(results[2].matched) 
			scheme = results.str(2);	

		std::string auth{};
		if(results[5].matched)
			auth = results.str(5);

		std::string host = results.str(7);

		std::string path = "/";
		if(results[9].matched)
			path = results.str(9);
		if(path[0] != '/')
			path = "/" + path;
		std::string port = scheme;
		static const std::regex host_port_reg("(((?!:)[[:print:]])*)(:([0-9]+))?");
		if(std::regex_search(host, results, host_port_reg)) {
			host = results.str(1);
			if(results[4].matched)
				port = results.str(4);
		}
		ConnectionPtr connection;
		if(scheme == "http") {
			connection = std::make_shared<TcpConnection>(service_);
		} else if(scheme == "https") {
			connection = std::make_shared<SslConnection>(service_, *ssl_context_);
		}

		connection->async_connect(host, port, [=](ConnectionPtr conn) {
			if(conn) {
				auto req = std::make_shared<Request>(conn);
				auto res = std::make_shared<Response>(conn);
				req->method() = method;
				auto pos = path.find("?");
				if(pos == path.npos) {
					req->path() = path;
				} else {
					req->path() = path.substr(0, pos); 
					req->query() = path.substr(pos + 1, path.size());
				}
				req->version() = "HTTP/1.1";
				req->addHeader("Host", host);
				if(enable_cookie_) {
					std::unique_lock<std::mutex> lck(cookie_mutex_);
					add_cookie_to_request(req, scheme, host);
				}
				req->addHeader("Connection", "close");
				if(auth != "")
					req->basicAuth(auth);

				req_handler(req);
				parseResponse(res, [=](ResponsePtr response) {
					res->discardConnection();
					if(response) {
						if(enable_cookie_) {
							std::unique_lock<std::mutex> lck(cookie_mutex_);
							add_cookie_to_cookie_jar(response, host);
						}
						res_handler(response);
					} else {
						res_handler(nullptr);
					}
				});
			} else {
				req_handler(nullptr);
				res_handler(nullptr);
			}
		});
	} else {
		res_handler(nullptr);
	}
}

void
Client::add_cookie_to_request(RequestPtr req, const std::string& scheme, const std::string& host)
{
	for(auto iter = cookie_jar_.begin(); iter != cookie_jar_.end();) {
		if(iter->expires < time(nullptr)) {
			cookie_jar_.erase(iter);
		} else {
			++iter;
		}
	}
	for(auto& c : cookie_jar_) {
		if(!is_path_security_ok(c.path, req->path()))
			continue;
		if(!is_domain_security_ok(c.domain, host))
			continue;
		if(c.secure && scheme != "https")
			continue;
		req->setCookie({c.key, c.val});
	}
}

void
Client::add_cookie_to_cookie_jar(ResponsePtr res, const std::string& host)
{
	for(auto c : res->cookieJar()) {
		bool is_new = true;
		if(c.domain == "")
			c.domain = host;
		if(c.path == "")
			c.path = "/";
		for(auto& oc : cookie_jar_) {
			if(oc.key == c.key) {
				oc = c;
				is_new = false;
				break;
			}
		}
		if(is_new)
			cookie_jar_.push_back(c);
	}
}
