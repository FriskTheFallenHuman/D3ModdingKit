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
#include "Radiant.h"
#include "mediapreviewdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMediaPreviewDlg dialog

IMPLEMENT_DYNAMIC( CMediaPreviewDlg, CDialogEx )

CMediaPreviewDlg::CMediaPreviewDlg( CWnd *pParent )
    : CDialogEx( CMediaPreviewDlg::IDD, pParent ),
      mode( MATERIALS ),
      media( "" ) {
}

void CMediaPreviewDlg::SetMedia( const char *_media ) {
    media = _media;
    Refresh();
}

void CMediaPreviewDlg::Refresh() {
    if ( mode == GUIS ) {
        const idMaterial* mat = declManager->FindMaterial( "guisurfs/guipreview" );
        mat->SetGui( media );
        drawMaterial.setMedia( "guisurfs/guipreview" );
    } else {
        drawMaterial.setMedia( media );
    }
    drawMaterial.setScale( 20.0f );
    wndPreview.setDrawable( &drawMaterial );
    wndPreview.Invalidate();
    wndPreview.RedrawWindow();
    RedrawWindow();
}

CMediaPreviewDlg::~CMediaPreviewDlg() {
}

void CMediaPreviewDlg::DoDataExchange( CDataExchange *pDX ) {
    CDialogEx::DoDataExchange( pDX );
    DDX_Control( pDX, IDC_PREVIEW, wndPreview );
}

BEGIN_MESSAGE_MAP( CMediaPreviewDlg, CDialogEx)
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

// CMediaPreviewDlg message handlers

BOOL CMediaPreviewDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();

    wndPreview.setDrawable( &testDrawable );
    CRect rct;
    LONG lSize = sizeof( rct );
    if ( LoadRegistryInfo( "Radiant::EditPreviewWindow", &rct, &lSize ) ) {
        SetWindowPos( nullptr, rct.left, rct.top, rct.Width(), rct.Height(), SWP_SHOWWINDOW );
    }

    GetClientRect( rct );
    int h = ( mode == GUIS ) ? ( rct.Width() - 8 ) / 1.333333f : rct.Height() - 8;
    wndPreview.SetWindowPos( nullptr, 4, 4, rct.Width() - 8, h, SWP_SHOWWINDOW );

    return TRUE;
}

void CMediaPreviewDlg::OnSize( UINT nType, int cx, int cy ) {

    float scaling_factor = Win_GetWindowScalingFactor( GetSafeHwnd() );
    int s8 = int( 8 * scaling_factor );
    int s4 = int( 4 * scaling_factor );

    CDialogEx::OnSize( nType, cx, cy );
    if ( wndPreview.GetSafeHwnd() == nullptr ) {
        return;
    }
    CRect rect;
    GetClientRect( rect );
    int h = rect.Height() - s8;
    wndPreview.SetWindowPos( nullptr, s4, s4, rect.Width() - s8, h, SWP_SHOWWINDOW );
}

void CMediaPreviewDlg::OnDestroy() {
    if ( GetSafeHwnd() ) {
        CRect rct;
        GetWindowRect( rct );
        SaveRegistryInfo( "radiant_editpreviewwindow", &rct, sizeof( rct ) );
    }

    CDialogEx::OnDestroy();
}

void CMediaPreviewDlg::OnLButtonDown( UINT nFlags, CPoint point ) {
    if ( mode == GUIS ) {
        idUserInterface* gui = uiManager->FindGui( media );
        if ( gui ) {
            sysEvent_t ev;
            memset( &ev, 0, sizeof( ev ) );
            ev.evType = SE_KEY;
            ev.evValue = K_MOUSE1;
            ev.evValue2 = 1;
            gui->HandleEvent( &ev, 0 );
        }
    }
    CDialogEx::OnLButtonDown( nFlags, point );
}

void CMediaPreviewDlg::OnLButtonUp( UINT nFlags, CPoint point ) {
    if ( mode == GUIS ) {
        idUserInterface *gui = uiManager->FindGui( media );
        if ( gui ) {
            sysEvent_t ev;
            memset( &ev, 0, sizeof( ev ) );
            ev.evType = SE_KEY;
            ev.evValue = K_MOUSE1;
            ev.evValue2 = 0;
            gui->HandleEvent( &ev, 0 );
        }
    }
    CDialogEx::OnLButtonUp( nFlags, point );
}

void CMediaPreviewDlg::OnMouseMove( UINT nFlags, CPoint point ) {
    if ( mode == GUIS ) {
        idUserInterface *gui = uiManager->FindGui( media );
        if ( gui ) {
            CRect rct;
            wndPreview.GetClientRect( rct );
            sysEvent_t ev;
            memset( &ev, 0, sizeof( ev ) );
            ev.evType = SE_MOUSE;
            ev.evValue = ( point.x / rct.Width() ) * 640.0f;
            ev.evValue2 = ( point.y / rct.Height() ) * 480.0f;
            gui->HandleEvent( &ev, 0 );
        }
    }
    CDialogEx::OnMouseMove( nFlags, point );
}
