/*
 ***********************************************************************
 *
 *        @version  1.0
 *        @date     03/11/2014 05:36:46 PM
 *        @author   Fridolin Pokorny <fridex.devel@gmail.com>
 *
 ***********************************************************************
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <iostream>
#include <cstdio>
#include <cassert>
#include <unistd.h>

#define UNUSED(V)				((void) V)
#define UNREACHABLE()		assert(0)

/**
 * @brief  Print error
 *
 * @return   std::cerr
 */
inline std::ostream & err() {
#ifdef __linux__
	if (isatty(fileno(stderr)))
		std::cerr << "\033[1;31mERROR:\e[0m ";
	else
		std::cerr << "ERROR: ";
	return std::cerr;
#else
	return std::cerr << "ERROR: ";
#endif
}

/**
 * @brief  Print warning
 *
 * @return   std::cerr
 */
inline std::ostream & warn() {
#ifdef __linux__
	if (isatty(fileno(stderr)))
		std::cerr << "\e[0;33mWARNING:\e[0m ";
	else
		std::cerr << "WARNING: ";
	return std::cerr;
#else
	return std::cerr << "WARNING: ";
#endif
}

/**
 * @brief  Print Info
 *
 * @return   std::cerr
 */
inline std::ostream & info() {
#ifdef __linux__
	if (isatty(fileno(stderr)))
		std::cerr << "\e[0;32mINFO:\e[0m ";
	else
		std::cerr << "INFO: ";
	return std::cerr;
#else
	return std::cerr << "INFO: ";
#endif
}

/**
 * @brief  Print debug
 *
 * @return   std::cerr
 */
#ifndef NDEBUG
#	if defined(__linux__)
#		define dbg() ((isatty(fileno(stderr))) ? \
							std::cerr << "\e[1;35mDBG:\e[0m " : \
							std::cerr << "DBG: ")
#	else
#		define dbg() std::cerr << "DBG: "
#	endif
#else
#	define dbg()		while(0) std::cerr
#endif // NDEBUG

#endif // COMMON_H_

