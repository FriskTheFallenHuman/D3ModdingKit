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

#ifndef __USERINTERFACE_LOCAL_H__
#define __USERINTERFACE_LOCAL_H__

class idWindow;

class idUserInterfaceLocal : public idUserInterface {
	friend class idUserInterfaceManagerLocal;
public:
								idUserInterfaceLocal();
	virtual						~idUserInterfaceLocal();

	virtual const char *		Name() const;
	virtual const char *		Comment() const;
	virtual bool				IsInteractive() const;
	virtual bool				InitFromFile( const char *qpath, bool rebuild = true, bool cache = true );
	virtual const char *		HandleEvent( const sysEvent_t *event, int time, bool *updateVisuals );
	virtual void				HandleNamedEvent( const char* namedEvent );
	virtual void				Redraw( int time );
	virtual void				DrawCursor();
	virtual const idDict &		State() const;
	virtual void				DeleteStateVar( const char *varName );
	virtual void				SetStateString( const char *varName, const char *value );
	virtual void				SetStateBool( const char *varName, const bool value );
	virtual void				SetStateInt( const char *varName, const int value );
	virtual void				SetStateFloat( const char *varName, const float value );

	// Gets a gui state variable
	virtual const char*			GetStateString( const char *varName, const char* defaultString = "" ) const;
	virtual bool				GetStateBool( const char *varName, const char* defaultString = "0" ) const;
	virtual int					GetStateInt( const char *varName, const char* defaultString = "0" ) const;
	virtual float				GetStateFloat( const char *varName, const char* defaultString = "0" ) const;

	virtual void				StateChanged( int time, bool redraw );
	virtual const char *		Activate( bool activate, int time );
	virtual void				Trigger( int time );
	virtual void				ReadFromDemoFile( class idDemoFile *f );
	virtual void				WriteToDemoFile( class idDemoFile *f );
	virtual bool				WriteToSaveGame( idFile *savefile ) const;
	virtual bool				ReadFromSaveGame( idFile *savefile );
	virtual void				SetKeyBindingNames( void );
	virtual bool				IsUniqued() const { return uniqued; };
	virtual void				SetUniqued( bool b ) { uniqued = b; };
	virtual void				SetCursor( float x, float y );

	virtual float				CursorX() { return cursorX; }
	virtual float				CursorY() { return cursorY; }

	size_t						Size();

	idDict *					GetStateDict() { return &state; }

	const char *				GetSourceFile( void ) const { return source; }
	ID_TIME_T					GetTimeStamp( void ) const { return timeStamp; }

	idWindow *					GetDesktop() const { return desktop; }
	void						SetBindHandler( idWindow *win ) { bindHandler = win; }
	bool						Active() const { return active; }
	int							GetTime() const { return time; }
	void						SetTime( int _time ) { time = _time; }

	void						ClearRefs() { refs = 0; }
	void						AddRef() { refs++; }
	int							GetRefs() { return refs; }

	void						RecurseSetKeyBindingNames( idWindow *window );
	idStr						&GetPendingCmd() { return pendingCmd; };
	idStr						&GetReturnCmd() { return returnCmd; };

	virtual idRectangle			GetScreenRect( void ) { return desktop->drawRect; }
private:
	bool						active;
	bool						loading;
	bool						interactive;
	bool						uniqued;

	idDict						state;
	idWindow *					desktop;
	idWindow *					bindHandler;

	idStr						source;
	idStr						activateStr;
	idStr						pendingCmd;
	idStr						returnCmd;
	ID_TIME_T						timeStamp;

	float						cursorX;
	float						cursorY;

	int							time;

	int							refs;

	// DG: used so we can notify GUI scripts about changes in side padding.
	//  Relevant if they use cstAnchor, so they can know how big padding on the left/right
	//  or above/below a centered windowDef with full 640x480 size is.
	//  Then they can adjust the sizes of windowDefs anchored to left/right or upper/lower borders
	//  to fill up the gap
	// if force=true, the variables are set no matter if the size has changed or not
	// returns true if the windowSize has changed and thus the variables were updated, otherwise false
	bool						MaybeSetCstWinRegs(bool force=false);
	int							lastGlWidth;
	int							lastGlHeight;
};

class idUserInterfaceManagerLocal : public idUserInterfaceManager {
	friend class idUserInterfaceLocal;

public:
	virtual void				Init();
	virtual void				Shutdown();
	virtual void				Touch( const char *name );
	virtual void				WritePrecacheCommands( idFile *f );
	virtual void				SetSize( float width, float height );
	virtual void				BeginLevelLoad();
	virtual void				EndLevelLoad();
	virtual void				Reload( bool all );
	virtual void				ListGuis() const;
	virtual bool				CheckGui( const char *qpath ) const;
	virtual idUserInterface *	Alloc( void ) const;
	virtual void				DeAlloc( idUserInterface *gui );
	virtual idUserInterface *	FindGui( const char *qpath, bool autoLoad = false, bool needInteractive = false, bool forceUnique = false );
	virtual idUserInterface *	FindDemoGui( const char *qpath );
	virtual	idListGUI *			AllocListGUI( void ) const;
	virtual void				FreeListGUI( idListGUI *listgui );

private:
	idRectangle					screenRect;
	idDeviceContext				dc;

	idList<idUserInterfaceLocal*> guis;
	idList<idUserInterfaceLocal*> demoGuis;

};

#endif /* !__USERINTERFACE_LOCAL_H__ */
