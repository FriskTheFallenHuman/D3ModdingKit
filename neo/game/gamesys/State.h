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

#ifndef __SYS_STATE_H__
#define __SYS_STATE_H__

typedef enum {
	SRESULT_OK,				// Call was made successfully
	SRESULT_ERROR,			// An unrecoverable error occurred
	SRESULT_DONE,			// Done with current state, move to next
	SRESULT_DONE_WAIT,		// Done with current state, wait a frame then move to next
	SRESULT_WAIT,			// Wait a frame and re-run current state
	SRESULT_IDLE,			// State thread is currently idle (ie. no states)
	SRESULT_SETSTAGE,		// Sets the current stage of the current state and reruns the state
	// NOTE: this has to be the last result becuase the stage is added to
	//		 the result.
	SRESULT_SETDELAY = SRESULT_SETSTAGE + 20
} stateResult_t;

#define MAX_STATE_CALLS		50

#define SRESULT_STAGE( x )	( (stateResult_t)( (int)SRESULT_SETSTAGE + (int)(x) ) )
#define SRESULT_DELAY( x )	( (stateResult_t)( (int)SRESULT_SETDELAY + (int)(x) ) )

struct stateParms_t {
	int		blendFrames;
	int		time;
	int		stage;

	void	Save( idSaveGame *saveFile ) const;
	void	Restore( idRestoreGame *saveFile );
};

typedef stateResult_t(idClass::* stateCallback_t)( const stateParms_t& parms );

template< class Type >
struct idStateFunc {
	const char* name;
	stateCallback_t		function;
};

/*
================
CLASS_STATES_PROTOTYPE

This macro must be included in the definition of any subclass of idClass that
wishes to have its own custom states.  Its prototypes variables used in the process
of managing states.
================
*/
#define CLASS_STATES_PROTOTYPE( nameofclass )						\
protected:														\
	static	idStateFunc<nameofclass>		stateCallbacks[]

/*
================
CLASS_STATES_DECLARATION

This macro must be included in the code to properly initialize variables
used in state processing for a idClass dervied class
================
*/
#define CLASS_STATES_DECLARATION( nameofclass )				\
idStateFunc<nameofclass> nameofclass::stateCallbacks[] = {

/*
================
STATE

This macro declares a single state.  It must be surrounded by the CLASS_STATES_DECLARATION
and END_CLASS_STATES macros.
================
*/
#define STATE( statename, function )			{ statename, (stateCallback_t)( &function ) },

/*
================
END_CLASS_STATES

Terminates a state block
================
*/
#define END_CLASS_STATES					{ NULL, NULL } };

struct stateCall_t {
	const idStateFunc<idClass> *state;
	idLinkList<stateCall_t>		node;
	int							flags;
	int							delay;
	stateParms_t				parms;

	void						Save( idSaveGame *saveFile ) const;
	void						Restore( idRestoreGame *saveFile, const idClass *owner );
};

class idClass;

const int SFLAG_ONCLEAR = BIT(0);			// Executes, even if the state queue is cleared
const int SFLAG_ONCLEARONLY = BIT(1);			// Executes only if the state queue is cleared

class idStateThread {
public:

					idStateThread( void );
	virtual			~idStateThread( void );

	void			SetName( const char *name );
	void			SetOwner( idClass *owner );

	bool			Interrupt(void);

	stateResult_t	InterruptState( const char* state, int blendFrames = 0, int delay = 0, int flags = 0 );
	stateResult_t	PostState( const char* state, int blendFrames = 0, int delay = 0, int flags = 0 );
	stateResult_t	SetState( const char* state, int blendFrames = 0, int delay = 0, int flags = 0 );
	stateCall_t	*	GetState( void ) const;
	bool			CurrentStateIs( const char *name ) const;

	stateResult_t	Execute( void );

	void			Clear( bool ignoreStateCalls = false );

	bool			IsIdle( void ) const;
	bool			IsExecuting( void ) const;

	void			Save( idSaveGame *saveFile ) const;
	void			Restore( idRestoreGame *saveFile, idClass *owner );

protected:

	struct flags {
		bool		stateCleared		: 1;		// State list was cleared
		bool		stateInterrupted	: 1;		// State list was interrupted
		bool		executing			: 1;		// Execute is currently processing states
	} fl;

	idStr						name;
	idClass *					owner;
	idLinkList<stateCall_t>		states;
	idLinkList<stateCall_t>		interrupted;
	stateCall_t *				insertAfter;
	stateResult_t				lastResult;
};

ID_INLINE void idStateThread::SetName( const char *_name ) {
	name = _name;
}

ID_INLINE stateCall_t *idStateThread::GetState( void ) const {
	return states.Next();
}

ID_INLINE bool idStateThread::IsIdle( void ) const {
	return !states.Next() && !interrupted.Next();
}

ID_INLINE bool idStateThread::IsExecuting( void ) const {
	return fl.executing;
}

#endif /* !__SYS_STATE_H__ */
