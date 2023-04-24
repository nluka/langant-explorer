#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#ifdef __linux__
# define ON_LINUX 1
#elif _WIN32
# define ON_WINDOWS 1
#else
# error "unsupported platform"
#endif

#endif // PLATFORM_HPP
