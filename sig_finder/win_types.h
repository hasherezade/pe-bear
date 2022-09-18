#ifndef USE_WINNT

#ifndef __WIN_TYPES__
#define __WIN_TYPES__

#if _MSC_VER
    #define snprintf _snprintf
    #define snscanf _snscanf
#endif

#ifdef _MSC_VER
    typedef unsigned __int8 uint8_t;
    typedef unsigned __int16 uint16_t;
    typedef unsigned __int32 uint32_t;
    typedef unsigned __int64 uint64_t;
    typedef __int8 int8_t;
    typedef __int16 int16_t;
    typedef __int32 int32_t;
    typedef __int64 int64_t;
#else
# include <inttypes.h>
#endif

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

#endif /* __WIN_TYPES__*/
#endif /* #ifndef USE_WINNT */
