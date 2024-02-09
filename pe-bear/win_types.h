#pragma once

#ifdef _MSC_VER
	#include <stdint.h>
#else
	#include <inttypes.h>
#endif

#if _WIN32
	#include <windows.h>
#else
	#ifndef __WIN_TYPES
	#define __WIN_TYPES__

	#ifndef BYTE
	typedef unsigned char BYTE;
	#endif

	#ifndef BOOLEAN
	   typedef BYTE BOOLEAN;
	#endif

	#ifndef BOOL
	   typedef BYTE BOOL; 
	#endif

	#ifndef WORD
	   typedef uint16_t WORD; 
	#endif

	#ifndef DWORD
	   typedef uint32_t DWORD; 
	#endif

	#ifndef ULONGLONG
	   typedef uint64_t ULONGLONG;
	#endif

	#ifndef CHAR
		typedef char CHAR;
	#endif

	#ifndef WCHAR
		typedef wchar_t WCHAR;
	#endif

	#ifndef VOID
		#define VOID void
		typedef char CHAR;
		typedef uint16_t SHORT;
		typedef uint32_t LONG;

		#if !defined(MIDL_PASS)
		typedef int INT;
		#endif
	#endif //VOID

	#endif // __WIN_TYPES__

#endif // #if _MSC_VER
