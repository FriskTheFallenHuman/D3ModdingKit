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
#ifndef DEBUGGERSERVER_H_
#define DEBUGGERSERVER_H_

#ifndef DEBUGGERMESSAGES_H_
#include "DebuggerMessages.h"
#endif

#ifndef DEBUGGERBREAKPOINT_H_
#include "DebuggerBreakpoint.h"
#endif

#include "../framework/Game.h"
class idInterpreter;
class idProgram;

#if SDL_VERSION_ATLEAST(3, 0, 0)
  // backwards-compat with SDL <= 2
  #define SDL_mutex SDL_Mutex
  #define SDL_cond SDL_Condition
#endif

class function_t;
typedef struct prstack_s prstack_t;

class rvDebuggerServer
{
public:

	rvDebuggerServer ( );
	~rvDebuggerServer ( );

	bool		Initialize				( void );
	void		Shutdown				( void );

	bool		ProcessMessages			( void );

	bool		IsConnected				( void );

	void		CheckBreakpoints		( idInterpreter *interpreter, idProgram *program, int instructionPointer );

	void		Print					( const char *text );

	void		OSPathToRelativePath	( const char *osPath, idStr &qpath );

	bool		GameSuspended			( void );
private:

	void		ClearBreakpoints		( void );

	void		Break					( idInterpreter *interpreter, idProgram *program, int instructionPointer );
	void		Resume					( void );

	void		SendMessage				( EDebuggerMessage dbmsg );
	void		SendPacket				( void* data, int datasize );

	// Message handlers
	void		HandleAddBreakpoint		( idBitMsg *msg );
	void		HandleRemoveBreakpoint	( idBitMsg *msg );
	void		HandleResume			( idBitMsg *msg );
	void		HandleInspectVariable	( idBitMsg *msg );
	void		HandleInspectCallstack	( idBitMsg *msg );
	void		HandleInspectThreads	( idBitMsg *msg );
	void		HandleInspectScripts	( idBitMsg *msg );
	void		HandleExecCommand		( idBitMsg *msg );
	////

	bool							mConnected;
	netadr_t						mClientAdr;
	idPort							mPort;
	idList<rvDebuggerBreakpoint*>	mBreakpoints;
	SDL_mutex*						mCriticalSection;


	SDL_cond*						mGameThreadBreakCond;
	SDL_mutex*						mGameThreadBreakLock;
	bool							mBreak;

	bool							mBreakNext;
	bool							mBreakStepOver;
	bool							mBreakStepInto;
	int								mBreakStepOverDepth;
	const function_t*				mBreakStepOverFunc1;
	const function_t*				mBreakStepOverFunc2;
	idProgram*						mBreakProgram;
	int								mBreakInstructionPointer;
	idInterpreter*					mBreakInterpreter;

	idStr							mLastStatementFile;
	int								mLastStatementLine;
	uintptr_t						mGameDLLHandle;
	idStrList						mScriptFileList;

};

/*
================
rvDebuggerServer::IsConnected
================
*/
ID_INLINE bool rvDebuggerServer::IsConnected ( void )
{
	return mConnected;
}

/*
================
rvDebuggerServer::SendPacket
================
*/
ID_INLINE void rvDebuggerServer::SendPacket ( void *data, int size )
{
	mPort.SendPacket ( mClientAdr, data, size );
}

/*
================
rvDebuggerServer::GameSuspended
================
*/
ID_INLINE bool rvDebuggerServer::GameSuspended( void )
{
	return mBreak;
}

#endif // DEBUGGERSERVER_H_
