#include <tests.h>
#include <util/cache.hpp>

void util_cache() {

	cache<int, int> c;

	c.set_max_size(100);
	for (int i = 0; i < 100; i++) {

		BOOST_CHECK_EQUAL(c.size(), i);
		c.get(i, [i]{ return 2*i; });
	}

	BOOST_CHECK_EQUAL(c.size(), 100);
	c.get(100, []{ return 2*100; });
	BOOST_CHECK_EQUAL(c.size(), 100);

	// should not be overwritten
	for (int i = 1; i <= 100; i++)
		BOOST_CHECK_EQUAL(c.get(i, [i]{ return 3*i; }), 2*i);

	// should be overwritten
	BOOST_CHECK_EQUAL(c.get(0, []{ return -1; }), -1);

	std::cout << "cache test finished" << std::endl;
}
