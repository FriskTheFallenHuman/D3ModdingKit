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

/*
===================================================

  FIND BRUSH/ENTITY DIALOG

===================================================
*/

/*
=================
SelectBrush
=================
*/
void SelectBrush (int entitynum, int brushnum)
{
	idEditorEntity	*e;
	idEditorBrush		*b;
	int			i;

	if (entitynum == 0)
		e = world_entity;
	else
	{
		e = entities.next;
		while (--entitynum)
		{
			e=e->next;
			if (e == &entities)
			{
				Sys_Status ("No such entity.", 0);
				return;
			}
		}
	}

	b = e->brushes.onext;
	if (b == &e->brushes)
	{
		Sys_Status ("No such brush.", 0);
		return;
	}
	while (brushnum--)
	{
		b=b->onext;
		if (b == &e->brushes)
		{
			Sys_Status ("No such brush.", 0);
			return;
		}
	}

	Brush_RemoveFromList (b);
	Brush_AddToList (b, &selected_brushes);


	Sys_UpdateWindows (W_ALL);
	for (i=0 ; i<3 ; i++)
  {
	if (g_pParentWnd->GetXYWnd())
	  g_pParentWnd->GetXYWnd()->GetOrigin()[i] = (b->mins[i] + b->maxs[i])/2;

	if (g_pParentWnd->GetXZWnd())
	  g_pParentWnd->GetXZWnd()->GetOrigin()[i] = (b->mins[i] + b->maxs[i])/2;

	if (g_pParentWnd->GetYZWnd())
	  g_pParentWnd->GetYZWnd()->GetOrigin()[i] = (b->mins[i] + b->maxs[i])/2;
  }

	Sys_Status ("Selected.", 0);
}

/*
=================
GetSelectionIndex
=================
*/
void GetSelectionIndex (int *ent, int *brush)
{
	idEditorBrush		*b, *b2;
	idEditorEntity	*entity;

	*ent = *brush = 0;

	b = selected_brushes.next;
	if (b == &selected_brushes)
		return;

	// find entity
	if (b->owner != world_entity)
	{
		(*ent)++;
		for (entity = entities.next ; entity != &entities
			; entity=entity->next, (*ent)++)
		;
	}

	// find brush
	for (b2=b->owner->brushes.onext
		; b2 != b && b2 != &b->owner->brushes
		; b2=b2->onext, (*brush)++)
	;
}

/*
===================
FindBrushDlgProc
===================
*/
INT_PTR CALLBACK FindBrushDlgProc(
	HWND hwndDlg,	// handle to dialog box
	UINT uMsg,	// message
	WPARAM wParam,	// first message parameter
	LPARAM lParam	// second message parameter
   )
{
	char entstr[256];
	char brushstr[256];
	HWND	h;
	int		ent, brush;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		// set entity and brush number
		GetSelectionIndex (&ent, &brush);
		sprintf (entstr, "%i", ent);
		sprintf (brushstr, "%i", brush);
		SetWindowText(GetDlgItem(hwndDlg, IDC_FIND_ENTITY), entstr);
		SetWindowText(GetDlgItem(hwndDlg, IDC_FIND_BRUSH), brushstr);

		h = GetDlgItem(hwndDlg, IDC_FIND_ENTITY);
		SetFocus (h);
		return FALSE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDOK:
				GetWindowText(GetDlgItem(hwndDlg, IDC_FIND_ENTITY), entstr, 255);
				GetWindowText(GetDlgItem(hwndDlg, IDC_FIND_BRUSH), brushstr, 255);
				SelectBrush (atoi(entstr), atoi(brushstr));
				g_pParentWnd->OnViewCenter();
				EndDialog(hwndDlg, 1);
				return TRUE;

			case IDCANCEL:
				EndDialog(hwndDlg, 0);
				return TRUE;
		}
	}
	return FALSE;
}


/*
===================
DoFind
===================
*/
void DoFind(void)
{
	DialogBox(g_qeglobals.d_hInstance, (char *)IDD_FINDBRUSH, g_pParentWnd->GetSafeHwnd(), FindBrushDlgProc);
}

/*
===================================================

  ARBITRARY SIDES

===================================================
*/

bool g_bDoCone = false;
bool g_bDoSphere = false;

/*
===================
SidesDlgProc
===================
*/
INT_PTR CALLBACK SidesDlgProc(
	HWND hwndDlg,	// handle to dialog box
	UINT uMsg,	// message
	WPARAM wParam,	// first message parameter
	LPARAM lParam	// second message parameter
   )
{
	char str[256];
	HWND	h;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		h = GetDlgItem(hwndDlg, IDC_SIDES);
		SetFocus (h);
		return FALSE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {

		case IDOK:
			GetWindowText(GetDlgItem(hwndDlg, IDC_SIDES), str, 255);
	  if (g_bDoCone)
			  Brush_MakeSidedCone(atoi(str));
	  else if (g_bDoSphere)
			  Brush_MakeSidedSphere(atoi(str));
	  else
			  Brush_MakeSided (atoi(str));

			EndDialog(hwndDlg, 1);
		break;

		case IDCANCEL:
			EndDialog(hwndDlg, 0);
		break;
	}
	default:
		return FALSE;
	}
}

/*
===================
DoSides
===================
*/
void DoSides(bool bCone, bool bSphere, bool bTorus)
{
  g_bDoCone = bCone;
  g_bDoSphere = bSphere;
  //g_bDoTorus = bTorus;
	DialogBox(g_qeglobals.d_hInstance, (char *)IDD_SIDES, g_pParentWnd->GetSafeHwnd(), SidesDlgProc);
}


/*
===================================================

  ABOUT DIALOG

===================================================
*/

/*
===================
AboutDlgProc
===================
*/
INT_PTR CALLBACK AboutDlgProc( HWND hwndDlg,
							UINT uMsg,
							WPARAM wParam,
							LPARAM lParam )
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			char buffer[1024];
			idStr::snPrintf(buffer, 1024, "DOOM Radiant Build %d\nCopyright ©1999-2004 Id Software, Inc.\n", BUILD_NUMBER);
//			SetDlgItemText( hwndDlg, IDC_ABOUT_INFO, buffer);

			idStr::snPrintf( buffer, 1024, "Renderer:\t%s", qglGetString( GL_RENDERER ) );
			SetDlgItemText( hwndDlg, IDC_ABOUT_GLRENDERER, buffer );

			idStr::snPrintf( buffer, 1024, "Version:\t\t%s", qglGetString( GL_VERSION ) );
			SetDlgItemText( hwndDlg, IDC_ABOUT_GLVERSION, buffer );

			idStr::snPrintf( buffer, 1024, "Vendor:\t\t%s", qglGetString( GL_VENDOR ) );
			SetDlgItemText( hwndDlg, IDC_ABOUT_GLVENDOR, buffer);

			char extensions[4096];
			idStr::snPrintf( extensions, 4096, "%s", qglGetString( GL_EXTENSIONS ) );
			HWND hWndExtensions = GetDlgItem( hwndDlg, IDC_ABOUT_GLEXTENSIONS );

			char *start = extensions;
			char *end;
			do {
				end = strchr(start, ' ');
				if ( end ) {
					*end = 0;
				}
				SendMessage( hWndExtensions, LB_ADDSTRING, 0, (LPARAM)start );
				start = end + 1;
			} while ( end );
		}

		return TRUE;

	case WM_CLOSE:
		EndDialog( hwndDlg, 1 );
		return TRUE;

	case WM_COMMAND:
		if ( LOWORD( wParam ) == IDOK )
			EndDialog(hwndDlg, 1);
		return TRUE;
	}
	return FALSE;
}

/*
===================
DoAbout
===================
*/
void DoAbout(void)
{
	DialogBox( g_qeglobals.d_hInstance, ( char * ) IDD_ABOUT, g_pParentWnd->GetSafeHwnd(), AboutDlgProc );
}
