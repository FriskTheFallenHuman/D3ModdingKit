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
#ifndef SYS_INCLUDES_H
#define SYS_INCLUDES_H

// Include the various platform specific header files (windows.h, etc)

/*
================================================================================================

	Windows

================================================================================================
*/

#ifdef _WIN32
#if defined(ID_ALLOW_TOOLS) && !defined(_D3SDK) && !defined(GAME_DLL)
// (hopefully) suppress "warning C4996: 'MBCS_Support_Deprecated_In_MFC':
//   MBCS support in MFC is deprecated and may be removed in a future version of MFC."
#define NO_WARN_MBCS_MFC_DEPRECATION

#include <afxwin.h>

#include "tools/comafx/framework.h"
#include "tools/comafx/pch.h"
#endif /* !ID_ALLOW_TOOLS && !_D3SDK && !GAME_DLL */

#include <winsock2.h>
#include <mmsystem.h>
#include <mmreg.h>

#include <intrin.h>			// needed for intrinsics like _mm_setzero_si28
#include <malloc.h>			// no malloc.h on mac or unix
#endif /* _WIN32 */

/*
================================================================================================

	Common Include Files

================================================================================================
*/

#if !defined( _DEBUG ) && !defined( NDEBUG )
	// don't generate asserts
	#define NDEBUG
#endif

#if !defined(_MSC_VER)
	// MSVC does not provide this C99 header
#include <inttypes.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <cstddef>
#include <typeinfo>
#include <errno.h>
#include <math.h>
//#define FLT_EPSILON 1.19209290E-07F
#include <cfloat>
#include <limits>
#include <chrono>
#include <thread>
#include <algorithm>

//-----------------------------------------------------

// Hacked stuff we may want to consider implementing later
class idScopedGlobalHeap {
};

#endif // SYS_INCLUDES_H
