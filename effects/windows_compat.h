#ifndef WINDOWS_COMPAT_H
#define WINDOWS_COMPAT_H

#ifdef _WIN32
#include <math.h>

// MinGW declares but doesn't define exp2f - replace it
#ifdef exp2f
#undef exp2f
#endif
#define exp2f(x) powf(2.0f, (x))

#endif // _WIN32

#endif // WINDOWS_COMPAT_H
