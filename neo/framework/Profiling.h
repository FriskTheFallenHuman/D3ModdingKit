/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __PROFILING_H__
#define __PROFILING_H__

// Note: the macros in this file are deliberately designed in a way that can
// work with both Tracy and Remotery (though the latter is currently not implemented)

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>

#define D3_PROFILING_ENABLED 1

#define D3P_FRAMEMARK FrameMark;

// at least tracy supports defining additional frame marks (though D3P_FRAMEMARK must always be set!)
// useful to visualize renderframes (starting/ending at GLimp_SwapBuffers())
//  vs engine frames (idCommonLocal::Frame())
#define D3P_NAMED_FRAMEMARK(NAMESTR) FrameMarkNamed(NAMESTR)

#define D3P_ScopedCPUSample(NAME) \
	ZoneNamedN(_D3_TRACY_ ## NAME, #NAME, 1)

#define D3P_BeginCPUSample(NAME) \
	TracyCZoneN(_D3_TRACY_ ## NAME, #NAME, 1)

// NAMESTR is a dynamic string to be used as name, ID must be a static name (not string, like in D3P_BeginCPUSample())
#define D3P_BeginCPUSampleDynamic(ID, NAMESTR) \
	TracyCZoneCtx _D3_TRACY_ ## ID; TracyCZone(_D3_TRACY_ ## ID, 1); \
	TracyCZoneName(_D3_TRACY_ ## ID, namestr, strlen(namestr))

// ID_NAME is the ID from D3_PROF_BeginCPUSampleDynamic or the NAME from D3P_BeginCPUSample
#define D3P_EndCPUSample(ID_NAME) \
	TracyCZoneEnd(_D3_TRACY_ ## ID_NAME)

// scoped sample for whole function with its backtrace, using the function name as name
#define D3P_CPUSampleFnBacktrace() \
	ZoneNamedNS(_D3_TRACY_FunctionTraceBT_, __PRETTY_FUNCTION__, 10, 1)

// scoped sample for whole function, using the function name as name (no backtrace => cheaper)
#define D3P_CPUSampleFn() \
	ZoneNamedN(_D3_TRACY_FunctionTrace_, __PRETTY_FUNCTION__, 1)

// TODO: opengl profiling?

#else // tracy disabled, use empty macros

// Note: this might seem redundant (Tracys headers themselves define empty macros #ifndef TRACY_ENABLE)
// but keep in mind that CMake only fetches the Tracy source (incl. those headers) if it's enabled via our TRACY CMake option

#define D3P_FRAMEMARK
#define D3P_NAMED_FRAMEMARK(NAMESTR)
#define D3P_ScopedCPUSample(NAME)
#define D3P_BeginCPUSample(NAME)
#define D3P_BeginCPUSampleDynamic(ID, NAMESTR)
#define D3P_EndCPUSample(ID_NAME)

#define D3P_CPUSampleFnBacktrace()
#define D3P_CPUSampleFn()

#endif // TRACY_ENABLE

#endif /* __PROFILING_H__ */
