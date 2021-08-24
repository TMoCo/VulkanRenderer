///////////////////////////////////////////////////////
// Print to screen macro
///////////////////////////////////////////////////////

#ifndef print_H
#define print_H

#include <iostream>
// variadic print macro
#define print(format, ...) \
	if (format) \
		fprintf(stderr, format, __VA_ARGS__)
// More about variadic macros here: https://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html

#endif // !print_H
