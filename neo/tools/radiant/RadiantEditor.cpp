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

#include "tools/edit_gui_common.h"

#include "qe3.h"
#include "radiant.h"
#include "RadiantEditor.h"

HINSTANCE g_DoomInstance = NULL;
bool g_editorAlive = false;

/*
================
RadiantPrint
================
*/
void RadiantPrint( const char *text ) {
	if ( g_editorAlive && g_Inspectors ) {
		if ( g_Inspectors->consoleWnd.GetSafeHwnd() ) {
			g_Inspectors->consoleWnd.AddText( text );
		}
	}
}

/*
================
RadiantShutdown
================
*/
void RadiantShutdown( void ) {
	theApp.ExitInstance();
}

/*
=================
RadiantInit

This is also called when you 'quit' in doom
=================
*/
void RadiantInit( void ) {

	// make sure the renderer is initialized
	if ( !renderSystem->IsOpenGLRunning() ) {
		common->Printf( "no OpenGL running\n" );
		return;
	}

	g_editorAlive = true;

	// allocate a renderWorld and a soundWorld
	if ( g_qeglobals.rw == NULL ) {
		g_qeglobals.rw = renderSystem->AllocRenderWorld();
		g_qeglobals.rw->InitFromMap( NULL );
	}
	if ( g_qeglobals.sw == NULL ) {
		g_qeglobals.sw = soundSystem->AllocSoundWorld( g_qeglobals.rw );
	}

	if ( g_DoomInstance ) {
		if ( ::IsWindowVisible( win32.hWnd ) ) {
			::ShowWindow( win32.hWnd, SW_HIDE );
			g_pParentWnd->ShowWindow( SW_SHOW );
			g_pParentWnd->SetFocus();
		}
	} else {
		Sys_GrabMouseCursor( false );

		g_DoomInstance = win32.hInstance;

		InitAfx();

		CWinApp* pApp = AfxGetApp();
		CWinThread *pThread = AfxGetThread();

		// App global initializations (rare)
		pApp->InitApplication();

		// Perform specific initializations
		pThread->InitInstance();

		qglFinish();
		//qwglMakeCurrent(0, 0);
		qwglMakeCurrent(win32.hDC, win32.hGLRC);

		// hide the doom window by default
		::ShowWindow( win32.hWnd, SW_HIDE );
	}
}


extern void Map_VerifyCurrentMap(const char *map);

/*
================
RadiantSync
================
*/
void RadiantSync( const char *mapName, const idVec3 &viewOrg, const idAngles &viewAngles ) {
	if ( g_DoomInstance == NULL ) {
		RadiantInit();
	}

	if ( g_DoomInstance ) {
		idStr osPath;
		osPath = fileSystem->RelativePathToOSPath( mapName );
		Map_VerifyCurrentMap( osPath );
		idAngles flip = viewAngles;
		flip.pitch = -flip.pitch;
		g_pParentWnd->GetCamera()->SetView( viewOrg, flip );
		g_pParentWnd->SetFocus();
		Sys_UpdateWindows( W_ALL );
		g_pParentWnd->RoutineProcessing();
	}
}

/*
================
RadiantRun
================
*/
void RadiantRun( void ) {
	static bool exceptionErr = false;
	int show = ::IsWindowVisible( win32.hWnd );

	try {
		if ( !exceptionErr && !show ) {
			qglDepthMask( true );
			theApp.Run();

			if ( win32.hDC != NULL && win32.hGLRC != NULL )
				qwglMakeCurrent( win32.hDC, win32.hGLRC );
		}
	}
	catch( idException &ex ) {
		::MessageBox( NULL, ex.error, "Exception error", MB_OK );
		RadiantShutdown();
	}
}

/*
=============================================================

REGISTRY INFO

=============================================================
*/

/*
================
SaveRegistryInfo
================
*/
bool SaveRegistryInfo( const char *pszName, void *pvBuf, long lSize ) {
	SetCvarBinary( pszName, pvBuf, lSize );
	common->WriteFlaggedCVarsToFile( "editor.cfg", CVAR_TOOL, "sett" );
	return true;
}

/*
================
LoadRegistryInfo
================
*/
bool LoadRegistryInfo( const char *pszName, void *pvBuf, long *plSize ) {
	return GetCvarBinary( pszName, pvBuf, *plSize );
}

/*
================
SaveWindowState
================
*/
bool SaveWindowState( HWND hWnd, const char *pszName ) {
	RECT rc;
	GetWindowRect(hWnd, &rc);
	if ( hWnd != g_pParentWnd->GetSafeHwnd() ) {
		if ( ::GetParent(hWnd) != g_pParentWnd->GetSafeHwnd() ) {
		  ::SetParent( hWnd, g_pParentWnd->GetSafeHwnd() );
		}
		MapWindowPoints( NULL, g_pParentWnd->GetSafeHwnd(), (POINT *)&rc, 2 );
	}
	return SaveRegistryInfo( pszName, &rc, sizeof(rc) );
}

/*
================
LoadWindowState
================
*/
bool LoadWindowState( HWND hWnd, const char *pszName ) {
	RECT rc;
	LONG lSize = sizeof( rc );

	if ( LoadRegistryInfo( pszName, &rc, &lSize ) ) {
		if ( rc.left < 0 ) {
			rc.left = 0;
		}

		if ( rc.top < 0 ) {
			rc.top = 0;
		}

		if ( rc.right < rc.left + 16 ) {
			rc.right = rc.left + 16;
		}

		if ( rc.bottom < rc.top + 16 ) {
			rc.bottom = rc.top + 16;
		}

		MoveWindow( hWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE );
		return true;
	}

	return false;
}

/*
===============================================================

  STATUS WINDOW

===============================================================
*/

/*
================
Sys_UpdateStatusBar
================
*/
void Sys_UpdateStatusBar( void ) {
	extern int   g_numbrushes, g_numentities;

	char numbrushbuffer[100] = "";

	sprintf( numbrushbuffer, "Brushes: %d Entities: %d", g_numbrushes, g_numentities );
	Sys_Status( numbrushbuffer, 2 );
}

/*
================
Sys_Status
================
*/
void Sys_Status(const char *psz, int part ) {
	if ( part < 0 ) {
		common->Printf( "%s", psz );
		part = 0;
	}

	g_pParentWnd->SetStatusText( part, psz );
}