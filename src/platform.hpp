#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#ifdef linux
# define ON_LINUX 1
#elif _WIN32
# define ON_WINDOWS 1
#else
# error "unsupported platform"
#endif

#endif // PLATFORM_HPP
