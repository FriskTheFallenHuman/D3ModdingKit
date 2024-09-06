/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").  

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#ifndef SYS_DEFINES_H
#define SYS_DEFINES_H

// NOTE: By default Win32 uses a 1MB stack. Doom3 1.3.1 uses 4MB (probably set after compiling with EDITBIN /STACK
// dhewm3 now uses a 8MB stack, set with a linker flag in CMakeLists.txt (/STACK:8388608 for MSVC, -Wl,--stack,8388608 for mingw)
// Linux has a 8MB stack by default, and so does macOS, at least for the main thread
// anyway, a 2MB limit alloca should be safe even when using it multiple times in the same function
#define ID_MAX_ALLOCA_SIZE 2097152 // 2MB

/*
================================================================================================

	Platform Specific ID_ Defines

	The ID_ defines are the only platform defines we should be using.

================================================================================================
*/

#undef ID_PC
#undef ID_PC_WIN
#undef ID_PC_LINUX
#undef ID_PC_OSX
#undef ID_PC_WIN64
#undef ID_PC_LINUX64
#undef ID_PC_OSX64
#undef ID_WIN32
#undef ID_LINUX32
#undef ID_OSX32

#if defined(_WIN32)
	// _WIN32 always defined
	// _WIN64 also defined for x64 target
	#if !defined( _WIN64 )
		#define ID_WIN_X86_ASM
		#define ID_WIN_X86_MMX_ASM
		#define ID_WIN_X86_MMX_INTRIN
		#define ID_WIN_X86_SSE_ASM
		#define ID_WIN_X86_SSE_INTRIN
		#define ID_WIN_X86_SSE2_ASM
		#define ID_WIN_X86_SSE2_INTRIN
	#else
		#define ID_PC_WIN64
		#define ID_WIN_X86_MMX_INTRIN
		#define ID_WIN_X86_SSE_INTRIN
		#define ID_WIN_X86_SSE2_INTRIN
		#define ID_WIN_X86_SSE3_INTRIN
	#endif

	#define ID_PC
	#define ID_PC_WIN
	#define ID_WIN32
#elif defined(__unix__)
	// __x86_64__ always defined
	// __i386__ also defined for x64 target
	#if !defined( __i386__ )
		#define ID_WIN_X86_ASM
		#define ID_WIN_X86_MMX_ASM
		#define ID_WIN_X86_MMX_INTRIN
		#define ID_WIN_X86_SSE_ASM
		#define ID_WIN_X86_SSE_INTRIN
		#define ID_WIN_X86_SSE2_ASM
		#define ID_WIN_X86_SSE2_INTRIN
	#else
		#define ID_PC_LINUX64
		#define ID_WIN_X86_MMX_INTRIN
		#define ID_WIN_X86_SSE_INTRIN
		#define ID_WIN_X86_SSE2_INTRIN
		#define ID_WIN_X86_SSE3_INTRIN
	#endif

	#define ID_PC
	#define ID_PC_LINUX
	#define ID_LINUX32
#elif defined(MACOS_X) || defined(__APPLE__)
	// __x86_64__ always defined
	// __i386__ also defined for x64 target
	#if !defined( __i386__ )
		#define ID_WIN_X86_ASM
		#define ID_WIN_X86_MMX_ASM
		#define ID_WIN_X86_MMX_INTRIN
		#define ID_WIN_X86_SSE_ASM
		#define ID_WIN_X86_SSE_INTRIN
		#define ID_WIN_X86_SSE2_ASM
		#define ID_WIN_X86_SSE2_INTRIN
	#else
		#define ID_PC_OSX64
		#define ID_WIN_X86_MMX_INTRIN
		#define ID_WIN_X86_SSE_INTRIN
		#define ID_WIN_X86_SSE2_INTRIN
		#define ID_WIN_X86_SSE3_INTRIN
	#endif

	#define ID_PC
	#define ID_PC_OSX
	#define ID_OSX32
#else
#error Unknown Platform
#endif

/*
===============================================================================

	CPU Arch detection.

===============================================================================
*/

// Setting D3_ARCH for VisualC++ from CMake doesn't work when using VS integrated CMake
// so set it in code instead
#ifdef ID_PC_WIN
#ifdef _MSC_VER
	#ifdef D3_ARCH
	  #undef D3_ARCH
	#endif // D3_ARCH

	#ifdef _M_X64
	  // this matches AMD64 and ARM64EC (but not regular ARM64), but they're supposed to be binary-compatible somehow, so whatever
	  #define D3_ARCH "x86_64"
	#elif defined( _M_ARM64 )
	  #define D3_ARCH "arm64"
	#elif defined( _M_ARM )
	  #define D3_ARCH "arm"
	#elif defined( _M_IX86 )
	  #define D3_ARCH "x86"
	#else
	  // if you're not targeting one of the aforementioned architectures,
	  // check https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros
	  // to find out how to detect yours and add it here - and please send a patch :)
	  #error "Unknown CPU architecture!"
	  // (for a quick and dirty solution, comment out the previous line, but keep in mind
	  //  that savegames may not be compatible with other builds of dhewm3)
	  #define D3_ARCH "UNKNOWN"
	#endif // _M_X64 etc
#endif // _MSC_VER
#endif // ID_PC_WIN

/*
================================================================================================

	PC Windows

================================================================================================
*/

#ifdef ID_PC_WIN
#ifdef _MSC_VER
	#define ALIGN16( x )			__declspec(align(16)) x
	#define ALIGNTYPE16				__declspec(align(16))
	#define ALIGNTYPE128			__declspec(align(128))
#else
	#define ALIGN16( x )			x __attribute__ ((aligned (16)))
	#define ALIGNTYPE16
	#define ALIGNTYPE128
#endif
#define FORMAT_PRINTF( x )

#define PATHSEPERATOR_STR			"\\"
#define PATHSEPERATOR_CHAR			'\\'
#define NEWLINE						"\r\n"

#ifdef _MSC_VER
#ifdef GAME_DLL
	#define ID_GAME_API				__declspec(dllexport)
#else
	#define ID_GAME_API
#endif

#define PACKED

#define ID_INLINE					__forceinline
// DG: alternative to forced inlining of ID_INLINE for functions that do alloca()
//     and are called in a loop so inlining them might cause stack overflow
#define ID_MAYBE_INLINE				__inline
#define ID_STATIC_TEMPLATE			static
#else
#ifdef GAME_DLL
	#define ID_GAME_API				__attribute__((visibility ("default")))
#else
	#define ID_GAME_API
#endif
#define PACKED						__attribute__((packed))

#define ID_INLINE					inline
#define ID_STATIC_TEMPLATE
#define strtok_s	strtok_r
#endif

// we should never rely on this define in our code. this is here so dodgy external libraries don't get confused
#ifndef WIN32
	#define WIN32
#endif
#endif

/*
================================================================================================

	PC Linux

================================================================================================
*/

#ifdef ID_PC_LINUX
#define ALIGN16( x )				x __attribute__ ((aligned (16)))
#define ALIGNTYPE16					__attribute__ ((aligned (16)))
#define ALIGNTYPE128				__attribute__ ((aligned (128)))
#define FORMAT_PRINTF( x )

#define PATHSEPERATOR_STR			"/"
#define PATHSEPERATOR_CHAR			'/'
#define NEWLINE						"\n"

#ifdef	__GNUC__
  // NOTE: Do *not* use __builtin_alloca_with_align(), unlike regular alloca it frees at end of block instead of end of function !
  #define _alloca16( x )			(({assert( (x)<ID_MAX_ALLOCA_SIZE );}),((void *)((((uintptr_t)__builtin_alloca( (x)+15 )) + 15) & ~15)))
  #define _alloca( x )				( ({assert((x)<ID_MAX_ALLOCA_SIZE);}), __builtin_alloca( (x) ) )
#else
  #define _alloca( x )				(({assert( (x)<ID_MAX_ALLOCA_SIZE );}), alloca( (x) ))
  #define _alloca16( x )			(({assert( (x)<ID_MAX_ALLOCA_SIZE );}),((void *)((((uintptr_t)alloca( (x)+15 )) + 15) & ~15)))
#endif

#ifdef GAME_DLL
	#define ID_GAME_API				__attribute__((visibility ("default")))
#else
	#define ID_GAME_API
#endif

#define PACKED						__attribute__((packed))

#define __cdecl
#define ASSERT						assert

#define ID_INLINE					inline
#define ID_STATIC_TEMPLATE

#define strtok_s	strtok_r
#endif

/*
================================================================================================

	PC MacOs

================================================================================================
*/

#ifdef ID_PC_OSX
#define ALIGN16( x )				x __attribute__ ((aligned (16)))
#define ALIGNTYPE16					__attribute__ ((aligned (16)))
#define ALIGNTYPE128				__attribute__ ((aligned (128)))
#define FORMAT_PRINTF( x )

#ifdef GAME_DLL
	#define ID_GAME_API				__attribute__((visibility ("default")))
#else
	#define ID_GAME_API
#endif

#define PACKED						__attribute__((packed))

#define PATHSEPERATOR_STR			"/"
#define PATHSEPERATOR_CHAR			'/'
#define NEWLINE						"\n"

#define _alloca						alloca
#define _alloca16( x )				((void *)((((uintptr_t)alloca( (x)+15 )) + 15) & ~15))

#define __cdecl
#define ASSERT						assert

#define ID_INLINE					inline
#define ID_STATIC_TEMPLATE

#define strtok_s	strtok_r
#endif

// Apple legacy
#ifdef __APPLE__
	#include <Availability.h>
	#ifdef __MAC_OS_X_VERSION_MIN_REQUIRED
		#if __MAC_OS_X_VERSION_MIN_REQUIRED == 1040
			#define OSX_TIGER
		#elif __MAC_OS_X_VERSION_MIN_REQUIRED < 1060
			#define OSX_LEOPARD
		#endif
	#endif
#endif

/*
================================================================================================

Defines and macros usable in all code

================================================================================================
*/

#ifdef __MINGW32__
  #undef _alloca // in mingw _alloca is a #define
  // NOTE: Do *not* use __builtin_alloca_with_align(), unlike regular alloca it frees at end of block instead of end of function !
  #define _alloca16( x )			( (void *) ( (assert((x)<ID_MAX_ALLOCA_SIZE)), ((((uintptr_t)__builtin_alloca( (x)+15 )) + 15) & ~15) ) )
  #define _alloca( x )				( (assert((x)<ID_MAX_ALLOCA_SIZE)), __builtin_alloca( (x) ) )
#else
  #define _alloca16( x )			( (void *) ( (assert((x)<ID_MAX_ALLOCA_SIZE)), ((((uintptr_t)_alloca( (x)+15 )) + 15) & ~15) ) )
  #define _alloca( x )				( (void *) ( (assert((x)<ID_MAX_ALLOCA_SIZE)), _alloca( (x) ) ) )
#endif

#define likely( x )	( x )
#define unlikely( x )	( x )

// A macro to disallow the copy constructor and operator= functions
// NOTE: The macro contains "private:" so all members defined after it will be private until
// public: or protected: is specified.
#define DISALLOW_COPY_AND_ASSIGN(TypeName)	\
private:									\
  TypeName(const TypeName&);				\
  void operator=(const TypeName&);

#ifndef ID_MAYBE_INLINE
	// for MSVC it's __inline, otherwise just inline should work
	#define ID_MAYBE_INLINE inline
#endif // ID_MAYBE_INLINE

/*
================================================================================================
Setup for /analyze code analysis, which we currently only have on the 360, but
we may get later for win32 if we buy the higher end vc++ licenses.

Even with VS2010 ultmate, /analyze only works for x86, not x64

Also note the __analysis_assume macro in sys_assert.h relates to code analysis.

This header should be included even by job code that doesn't reference the
bulk of the codebase, so it is the best place for analyze pragmas.
================================================================================================
*/

#if defined( ID_WIN32 )
// disable some /analyze warnings here
#pragma warning( disable: 6255 )	// warning C6255: _alloca indicates failure by raising a stack overflow exception. Consider using _malloca instead. (Note: _malloca requires _freea.)
#pragma warning( disable: 6262 )	// warning C6262: Function uses '36924' bytes of stack: exceeds /analyze:stacksize'32768'. Consider moving some data to heap
#pragma warning( disable: 6326 )	// warning C6326: Potential comparison of a constant with another constant

#pragma warning( disable: 6031 )	//  warning C6031: Return value ignored
// this warning fires whenever you have two calls to new in a function, but we assume new never fails, so it is not relevant for us
#pragma warning( disable: 6211 )	// warning C6211: Leaking memory 'staticModel' due to an exception. Consider using a local catch block to clean up memory

// we want to fix all these at some point...
#pragma warning( disable: 6246 )	// warning C6246: Local declaration of 'es' hides declaration of the same name in outer scope. For additional information, see previous declaration at line '969' of 'w:\tech5\rage\game\ai\fsm\fsm_combat.cpp': Lines: 969
#pragma warning( disable: 6244 )	// warning C6244: Local declaration of 'viewList' hides previous declaration at line '67' of 'w:\tech5\engine\renderer\rendertools.cpp'

// win32 needs this, but 360 doesn't
#pragma warning( disable: 6540 )	// warning C6540: The use of attribute annotations on this function will invalidate all of its existing __declspec annotations [D:\tech5\engine\engine-10.vcxproj]

#pragma warning( disable: 4467 )	// .. Include\CodeAnalysis\SourceAnnotations.h(68): warning C4467: usage of ATL attributes is deprecated

#if !defined(VERIFY_FORMAT_STRING)
	// checking format strings catches a LOT of errors
	#include <CodeAnalysis\SourceAnnotations.h>
	#define	VERIFY_FORMAT_STRING	[SA_FormatString(Style="printf")]
	// DG: alternative for GCC with attribute (NOOP for MSVC)
	#define ID_STATIC_ATTRIBUTE_PRINTF(STRIDX, FIRSTARGIDX)
#endif
#else
	#define	VERIFY_FORMAT_STRING
	// STRIDX: index of format string in function arguments (first arg == 1)
	// FIRSTARGIDX: index of first argument for the format string
	#define ID_STATIC_ATTRIBUTE_PRINTF(STRIDX, FIRSTARGIDX) __attribute__ ((format (printf, STRIDX, FIRSTARGIDX)))
#endif

// This needs to be handled so shift by 1
#define ID_INSTANCE_ATTRIBUTE_PRINTF(STRIDX, FIRSTARGIDX) ID_STATIC_ATTRIBUTE_PRINTF((STRIDX+1),(FIRSTARGIDX+1))

// We need to inform the compiler that Error() and FatalError() will
// never return, so any conditions that leeds to them being called are
// guaranteed to be false in the following code
#if defined(_MSC_VER)
	#define NO_RETURN __declspec(noreturn)
#elif defined(__GNUC__)
	#define NO_RETURN __attribute__((noreturn))
#else
	#define NO_RETURN
#endif

// I don't want to disable "warning C6031: Return value ignored" from /analyze
// but there are several cases with sprintf where we pre-initialized the variables
// being scanned into, so we truly don't care if they weren't all scanned.
// Rather than littering #pragma statements around these cases, we can assign the
// return value to this, which means we have considered the issue and decided that
// it doesn't require action.
// The volatile qualifier is to prevent:PVS-Studio warnings like:
// False	2	4214	V519	The 'ignoredReturnValue' object is assigned values twice successively. Perhaps this is a mistake. Check lines: 545, 547.	Rage	collisionmodelmanager_debug.cpp	547	False
extern volatile int ignoredReturnValue;

#define MAX_TYPE( x )			( ( ( ( 1 << ( ( sizeof( x ) - 1 ) * 8 - 1 ) ) - 1 ) << 8 ) | 255 )
#define MIN_TYPE( x )			( - MAX_TYPE( x ) - 1 )
#define MAX_UNSIGNED_TYPE( x )	( ( ( ( 1U << ( ( sizeof( x ) - 1 ) * 8 ) ) - 1 ) << 8 ) | 255U )
#define MIN_UNSIGNED_TYPE( x )	0

#endif
