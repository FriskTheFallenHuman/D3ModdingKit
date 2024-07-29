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

#include "precompiled.h"
#pragma hdrstop

#include "qe3.h"
#include "radiant.h"
#include "MainFrm.h"
#include "lightdlg.h"

#include <process.h>    // for _beginthreadex and _endthreadex
#include <ddeml.h>  // for MSGF_DDEMGR

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

idCVar radiant_entityMode( "radiant_entityMode", "0", CVAR_TOOL | CVAR_ARCHIVE, "" );

/////////////////////////////////////////////////////////////////////////////
// CRadiantApp

BEGIN_MESSAGE_MAP(CRadiantApp, CWinApp)
	//{{AFX_MSG_MAP(CRadiantApp)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRadiantApp construction

/*
================
CRadiantApp::CRadiantApp
================
*/
CRadiantApp::CRadiantApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CRadiantApp object

CRadiantApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CRadiantApp initialization

/*
================
CRadiantApp::InitInstance
================
*/
BOOL CRadiantApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);

	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		return FALSE;
	}

	AfxEnableControlContainer();

	AfxInitRichEdit2();

	// Change the registry key under which our settings are stored.
	SetRegistryKey( EDITOR_REGISTRY_KEY );

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)


	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

//	CMultiDocTemplate* pDocTemplate;
//	pDocTemplate = new CMultiDocTemplate(
//		IDR_RADIANTYPE,
//		RUNTIME_CLASS(CRadiantDoc),
//		RUNTIME_CLASS(CMainFrame), // custom MDI child frame
//		RUNTIME_CLASS(CRadiantView));
//	AddDocTemplate(pDocTemplate);

	// create main MDI Frame window

	g_PrefsDlg.LoadPrefs();

	qglEnableClientState( GL_VERTEX_ARRAY );

	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME)) {
		return FALSE;
	}

	if (pMainFrame->m_hAccelTable) {
		::DestroyAcceleratorTable(pMainFrame->m_hAccelTable);
	}

	pMainFrame->LoadAccelTable(MAKEINTRESOURCE(IDR_MINIACCEL));

	m_pMainWnd = pMainFrame;

	// The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CRadiantApp commands

/*
================
CRadiantApp::ExitInstance
================
*/
int CRadiantApp::ExitInstance()
{
	common->Shutdown();
	g_pParentWnd = NULL;
	int ret = CWinApp::ExitInstance();
	ExitProcess(0);
	return ret;
}

/*
================
CRadiantApp::OnIdle
================
*/
BOOL CRadiantApp::OnIdle(LONG lCount) {
	if (g_pParentWnd) {
		g_pParentWnd->RoutineProcessing();
	}
	return FALSE;
	//return CWinApp::OnIdle(lCount);
}

/*
================
CRadiantApp::OnHelp
================
*/
void CRadiantApp::OnHelp()
{
	ShellExecute(m_pMainWnd->GetSafeHwnd(), "open", "https://iddevnet.dhewm3.org/doom3/index.html", NULL, NULL, SW_SHOW);
}

/*
================
CRadiantApp::Run
================
*/
int CRadiantApp::Run( void )
{
	BOOL bIdle = TRUE;
	LONG lIdleCount = 0;

	lastFrameTime = std::chrono::steady_clock::now();

#if _MSC_VER >= 1300
	MSG *msg = AfxGetCurrentMessage();			// TODO Robert fix me!!
#else
	MSG *msg = &m_msgCur;
#endif

	// phase1: check to see if we can do idle work
	while (bIdle &&	!::PeekMessage(msg, NULL, NULL, NULL, PM_NOREMOVE)) {
		// call OnIdle while in bIdle state
		if (!OnIdle(lIdleCount++)) {
			bIdle = FALSE; // assume "no idle" state
		}
	}

	// phase2: pump messages while available
	do {
		// pump message, but quit on WM_QUIT
		if (!PumpMessage()) {
			return ExitInstance();
		}

		// reset "no idle" state after pumping "normal" message
		if (IsIdleMessage(msg)) {
			bIdle = TRUE;
			lIdleCount = 0;
		}

	} while (::PeekMessage(msg, NULL, NULL, NULL, PM_NOREMOVE));

	return 0;
}