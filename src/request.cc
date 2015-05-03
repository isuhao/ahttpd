#include "request.hh"
#include "connection.hh"
#include "base64.hh"
#include <regex>

void
Request::flush()
{

	if(connection() == nullptr)
		return;
	if(!chunked()) {        /**< 说明是第一次手动调用flush */
		/** 由于是第一次调用，说明第一行并未发送 */
		setChunked();
		if(query_ == "") {
			Log("NOTE") << method_ << " " << path_ << " " << version_;
			connection()->async_write(method_ + " " + path_ + " " + version_ + "\r\n");
		} else {
			Log("NOTE") << method_ << " " << path_ << "?" << query_ << " " << version_;
			connection()->async_write(method_ + " " + path_ + "?" + query_ + " " + version_ + "\r\n");
		}
	}
	flushPackage();
}

std::string
Request::basicAuthInfo()
{
	auto auth = getHeader("Authorization"); 
	if(auth) {
		static const std::regex basic_auth_reg("[Bb]asic ([[:print:]]*)");
		std::smatch results;
		if(std::regex_search(*auth, results, basic_auth_reg)) {
			return Base64::decode(results.str(1));
		} else {
			return {};
		}
	}
	return {};
}

void
Request::basicAuth(const std::string& auth)
{
	setHeader("Authorization", "Basic " + Base64::encode(auth));
}

Request::~Request()
{

	if(connection() == nullptr)
		return;
	try {
		if(!chunked()) {
			if(query_ == "") {
				Log("NOTE") << method_ << " " << path_ << " " << version_;
				connection()->async_write(method_ + " " + path_ + " " + version_ + "\r\n");
			} else {
				Log("NOTE") << method_ << " " << path_ << "?" << query_ << " " << version_;
				connection()->async_write(method_ + " " + path_ + "?" + query_ + " " + version_ + "\r\n");
			}
		} else {
			flushPackage();
			connection()->async_write("0\r\n\r\n");
		}
		flushPackage();
	} catch(std::exception& e) {
		fprintf(stderr, "%s\n", e.what());
	}
}
		
