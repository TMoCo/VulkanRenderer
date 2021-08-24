//
// customised assert macro, taken from Eugen Madgalits @
// https://stackoverflow.com/questions/3692954/add-custom-messages-in-assert/3692961
//

#ifndef ASSERT_H
#define ASSERT_H

#include <iostream>

#include <common/Print.h>

#ifndef NDEBUG
	#define m_assert(exp, msg) \
		__m_assert(#exp, exp, __FILE__, __LINE__, msg)
#else
	#define m_assert(exp, msg);
#endif

inline void __m_assert(const char* exp_str, bool exp, const char* file, int line, const char* msg) {
	if (!exp) {
		print("Assertion failed:\t%s\nExpected:\t\t%s\nSource:\t\t\t%s -> line %i\n", msg, exp_str, file, line);
		abort();
	}
}

#endif // !ASSERT_H