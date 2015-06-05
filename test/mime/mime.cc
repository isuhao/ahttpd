#include <boost/test/unit_test.hpp>
#include "MimeType.hh"

using namespace ahttpd;

BOOST_AUTO_TEST_CASE(mime_test)
{
	BOOST_CHECK_EQUAL(guessMimeType("no_dot"), "text/plain");
	BOOST_CHECK_EQUAL(guessMimeType("something.htm"), "text/html");
	BOOST_CHECK_EQUAL(guessMimeType("something.txt"), "text/plain");
	BOOST_CHECK_EQUAL(guessMimeType("something.323"), "text/h323");
}
