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

#include "DeviceContext.h"
#include "../renderer/tr_local.h"

idVec4 idDeviceContext::colorPurple;
idVec4 idDeviceContext::colorOrange;
idVec4 idDeviceContext::colorYellow;
idVec4 idDeviceContext::colorGreen;
idVec4 idDeviceContext::colorBlue;
idVec4 idDeviceContext::colorRed;
idVec4 idDeviceContext::colorBlack;
idVec4 idDeviceContext::colorWhite;
idVec4 idDeviceContext::colorNone;


idCVar gui_smallFontLimit( "gui_smallFontLimit", "0", CVAR_GUI | CVAR_ARCHIVE, "" );
idCVar gui_mediumFontLimit( "gui_mediumFontLimit", "0", CVAR_GUI | CVAR_ARCHIVE, "" );


idList<fontInfoEx_t> idDeviceContext::fonts;

int idDeviceContext::FindFont( const char *name ) {
	int c = fonts.Num();
	for (int i = 0; i < c; i++) {
		if (idStr::Icmp(name, fonts[i].name) == 0) {
			return i;
		}
	}

	// If the font was not found, try to register it
	idStr fileName = name;
	fileName.Replace("fonts", va("fonts/%s", fontLang.c_str()) );

	fontInfoEx_t fontInfo = {}; // DG: initialize this
	int index = fonts.Append( fontInfo );
	if ( renderSystem->RegisterFont( fileName, fonts[index] ) ){
		idStr::Copynz( fonts[index].name, name, sizeof( fonts[index].name ) );
		return index;
	} else {
		common->Printf( "Could not register font %s [%s]\n", name, fileName.c_str() );
		return -1;
	}
}

void idDeviceContext::SetupFonts() {
	fonts.SetGranularity( 1 );

	fontLang = cvarSystem->GetCVarString( "sys_lang" );

	// western european languages can use the english font
	if ( fontLang == "french" || fontLang == "german" || fontLang == "spanish" || fontLang == "italian" ) {
		fontLang = "english";
	}

	// Default font has to be added first
	FindFont( "fonts" );
}

void idDeviceContext::SetFont( int num ) {
	if ( num >= 0 && num < fonts.Num() ) {
		activeFont = &fonts[num];
	} else {
		activeFont = &fonts[0];
	}
}


void idDeviceContext::Init() {
	xScale = 0.0;
	SetSize(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
	whiteImage = declManager->FindMaterial("guis/assets/white.tga");
	whiteImage->SetSort( SS_GUI );
	mbcs = false;
	SetupFonts();
	activeFont = &fonts[0];
	colorPurple = idVec4(1, 0, 1, 1);
	colorOrange = idVec4(1, 1, 0, 1);
	colorYellow = idVec4(0, 1, 1, 1);
	colorGreen = idVec4(0, 1, 0, 1);
	colorBlue = idVec4(0, 0, 1, 1);
	colorRed = idVec4(1, 0, 0, 1);
	colorWhite = idVec4(1, 1, 1, 1);
	colorBlack = idVec4(0, 0, 0, 1);
	colorNone = idVec4(0, 0, 0, 0);
	cursorImages[CURSOR_ARROW] = declManager->FindMaterial("ui/assets/guicursor_arrow.tga");
	cursorImages[CURSOR_HAND] = declManager->FindMaterial("ui/assets/guicursor_hand.tga");
	scrollBarImages[SCROLLBAR_HBACK] = declManager->FindMaterial("ui/assets/scrollbarh.tga");
	scrollBarImages[SCROLLBAR_VBACK] = declManager->FindMaterial("ui/assets/scrollbarv.tga");
	scrollBarImages[SCROLLBAR_THUMB] = declManager->FindMaterial("ui/assets/scrollbar_thumb.tga");
	scrollBarImages[SCROLLBAR_RIGHT] = declManager->FindMaterial("ui/assets/scrollbar_right.tga");
	scrollBarImages[SCROLLBAR_LEFT] = declManager->FindMaterial("ui/assets/scrollbar_left.tga");
	scrollBarImages[SCROLLBAR_UP] = declManager->FindMaterial("ui/assets/scrollbar_up.tga");
	scrollBarImages[SCROLLBAR_DOWN] = declManager->FindMaterial("ui/assets/scrollbar_down.tga");
	cursorImages[CURSOR_ARROW]->SetSort( SS_GUI );
	cursorImages[CURSOR_HAND]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_HBACK]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_VBACK]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_THUMB]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_RIGHT]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_LEFT]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_UP]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_DOWN]->SetSort( SS_GUI );
	cursor = CURSOR_ARROW;
	enableClipping = true;
	overStrikeMode = true;
	mat.Identity();
	origin.Zero();
	initialized = true;

	// DG: this is used for the "make sure menus are rendered as 4:3" hack
	fixScaleForMenu.Set(1, 1);
	fixOffsetForMenu.Set(0, 0);
}

void idDeviceContext::Shutdown() {
	fontName.Clear();
	clipRects.Clear();
	fonts.Clear();
	Clear();
}

void idDeviceContext::Clear() {
	initialized = false;
	useFont = NULL;
	activeFont = NULL;
	mbcs = false;
}

idDeviceContext::idDeviceContext() {
	Clear();
}

void idDeviceContext::SetTransformInfo(const idVec3 &org, const idMat3 &m) {
	origin = org;
	mat = m;
}

//
//  added method
void idDeviceContext::GetTransformInfo(idVec3& org, idMat3& m )
{
	m = mat;
	org = origin;
}
//

void idDeviceContext::PopClipRect() {
	if (clipRects.Num()) {
		clipRects.RemoveIndex(clipRects.Num()-1);
	}
}

void idDeviceContext::PushClipRect(idRectangle r) {
	clipRects.Append(r);
}

void idDeviceContext::PushClipRect(float x, float y, float w, float h) {
	clipRects.Append(idRectangle(x, y, w, h));
}

bool idDeviceContext::ClippedCoords(float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2) {

	if ( enableClipping == false || clipRects.Num() == 0 ) {
		return false;
	}

	int c = clipRects.Num();
	while( --c > 0 ) {
		idRectangle *clipRect = &clipRects[c];

		float ox = *x;
		float oy = *y;
		float ow = *w;
		float oh = *h;

		if ( ow <= 0.0f || oh <= 0.0f ) {
			break;
		}

		if (*x < clipRect->x) {
			*w -= clipRect->x - *x;
			*x = clipRect->x;
		} else if (*x > clipRect->x + clipRect->w) {
			*x = *w = *y = *h = 0;
		}
		if (*y < clipRect->y) {
			*h -= clipRect->y - *y;
			*y = clipRect->y;
		} else if (*y > clipRect->y + clipRect->h) {
			*x = *w = *y = *h = 0;
		}
		if (*w > clipRect->w) {
			*w = clipRect->w - *x + clipRect->x;
		} else if (*x + *w > clipRect->x + clipRect->w) {
			*w = clipRect->Right() - *x;
		}
		if (*h > clipRect->h) {
			*h = clipRect->h - *y + clipRect->y;
		} else if (*y + *h > clipRect->y + clipRect->h) {
			*h = clipRect->Bottom() - *y;
		}

		if ( s1 && s2 && t1 && t2 && ow > 0.0f ) {
			float ns1, ns2, nt1, nt2;
			// upper left
			float u = ( *x - ox ) / ow;
			ns1 = *s1 * ( 1.0f - u ) + *s2 * ( u );

			// upper right
			u = ( *x + *w - ox ) / ow;
			ns2 = *s1 * ( 1.0f - u ) + *s2 * ( u );

			// lower left
			u = ( *y - oy ) / oh;
			nt1 = *t1 * ( 1.0f - u ) + *t2 * ( u );

			// lower right
			u = ( *y + *h - oy ) / oh;
			nt2 = *t1 * ( 1.0f - u ) + *t2 * ( u );

			// set values
			*s1 = ns1;
			*s2 = ns2;
			*t1 = nt1;
			*t2 = nt2;
		}
	}

	return (*w == 0 || *h == 0) ? true : false;
}

// DG: this is used for the "make sure menus are rendered as 4:3" hack
void idDeviceContext::SetMenuScaleFix(bool enable) {
	if(enable) {
		float w = renderSystem->GetScreenWidth();
		float h = renderSystem->GetScreenHeight();
		float aspectRatio = w/h;
		static const float virtualAspectRatio = float(VIRTUAL_WIDTH)/float(VIRTUAL_HEIGHT); // 4:3
		if(aspectRatio > 1.4f) {
			// widescreen (4:3 is 1.333 3:2 is 1.5, 16:10 is 1.6, 16:9 is 1.7778)
			// => we need to scale and offset X
			// All the coordinates here assume 640x480 (VIRTUAL_WIDTH x VIRTUAL_HEIGHT)
			// screensize, so to fit a 4:3 menu into 640x480 stretched to a widescreen,
			// we need do decrease the width to something smaller than 640 and center
			// the result with an offset
			float scaleX = virtualAspectRatio/aspectRatio;
			float offsetX = (1.0f-scaleX)*(VIRTUAL_WIDTH*0.5f); // (640 - scale*640)/2
			fixScaleForMenu.Set(scaleX, 1);
			fixOffsetForMenu.Set(offsetX, 0);
		} else if(aspectRatio < 1.24f) {
			// portrait-mode, "thinner" than 5:4 (which is 1.25)
			// => we need to scale and offset Y
			// it's analogue to the other case, but inverted and with height and Y
			float scaleY = aspectRatio/virtualAspectRatio;
			float offsetY = (1.0f - scaleY)*(VIRTUAL_HEIGHT*0.5f); // (480 - scale*480)/2
			fixScaleForMenu.Set(1, scaleY);
			fixOffsetForMenu.Set(0, offsetY);
		}
	} else {
		fixScaleForMenu.Set(1, 1);
		fixOffsetForMenu.Set(0, 0);
	}
}

// DG: Note: not sure if AdjustCoords() works entirely as it should, but it seems
//     good enough for the idRenderWindow with the mars globe in the main menu
void idDeviceContext::AdjustCoords(float *x, float *y, float *w, float *h) {
	// TODO: not sure about cst_*Offset
	if (x) {
		*x *= xScale;
		*x += cst_xOffset; // DG: for CstDoom3 anchored windows

		*x *= fixScaleForMenu.x; // DG: for "render menus as 4:3" hack
		*x += fixOffsetForMenu.x;
	}
	if (y) {
		*y *= yScale;
		*y += cst_yOffset; // DG: for CstDoom3 anchored windows

		*y *= fixScaleForMenu.y; // DG: for "render menus as 4:3" hack
		*y += fixOffsetForMenu.y;
	}
	if (w) {
		*w *= xScale;

		*w *= fixScaleForMenu.x; // DG: for "render menus as 4:3" hack
	}
	if (h) {
		*h *= yScale;

		*h *= fixScaleForMenu.y; // DG: for "render menus as 4:3" hack
	}
}

// fva/DG: added adjustCoords argument for CstDoom3 anchored GUIs and our old
//         scale-menus-to-4:3-fix, it basically replaces calling AdjustCoords(&x, &y, &w, &h)
//         before calling this
void idDeviceContext::DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *shader, bool adjustCoords) {
	idDrawVert verts[4];
	glIndex_t indexes[6];
	indexes[0] = 3;
	indexes[1] = 0;
	indexes[2] = 2;
	indexes[3] = 2;
	indexes[4] = 0;
	indexes[5] = 1;
	verts[0].xyz[0] = x;
	verts[0].xyz[1] = y;
	verts[0].xyz[2] = 0;
	verts[0].st[0] = s1;
	verts[0].st[1] = t1;
	verts[0].normal[0] = 0;
	verts[0].normal[1] = 0;
	verts[0].normal[2] = 1;
	verts[0].tangents[0][0] = 1;
	verts[0].tangents[0][1] = 0;
	verts[0].tangents[0][2] = 0;
	verts[0].tangents[1][0] = 0;
	verts[0].tangents[1][1] = 1;
	verts[0].tangents[1][2] = 0;
	verts[1].xyz[0] = x + w;
	verts[1].xyz[1] = y;
	verts[1].xyz[2] = 0;
	verts[1].st[0] = s2;
	verts[1].st[1] = t1;
	verts[1].normal[0] = 0;
	verts[1].normal[1] = 0;
	verts[1].normal[2] = 1;
	verts[1].tangents[0][0] = 1;
	verts[1].tangents[0][1] = 0;
	verts[1].tangents[0][2] = 0;
	verts[1].tangents[1][0] = 0;
	verts[1].tangents[1][1] = 1;
	verts[1].tangents[1][2] = 0;
	verts[2].xyz[0] = x + w;
	verts[2].xyz[1] = y + h;
	verts[2].xyz[2] = 0;
	verts[2].st[0] = s2;
	verts[2].st[1] = t2;
	verts[2].normal[0] = 0;
	verts[2].normal[1] = 0;
	verts[2].normal[2] = 1;
	verts[2].tangents[0][0] = 1;
	verts[2].tangents[0][1] = 0;
	verts[2].tangents[0][2] = 0;
	verts[2].tangents[1][0] = 0;
	verts[2].tangents[1][1] = 1;
	verts[2].tangents[1][2] = 0;
	verts[3].xyz[0] = x;
	verts[3].xyz[1] = y + h;
	verts[3].xyz[2] = 0;
	verts[3].st[0] = s1;
	verts[3].st[1] = t2;
	verts[3].normal[0] = 0;
	verts[3].normal[1] = 0;
	verts[3].normal[2] = 1;
	verts[3].tangents[0][0] = 1;
	verts[3].tangents[0][1] = 0;
	verts[3].tangents[0][2] = 0;
	verts[3].tangents[1][0] = 0;
	verts[3].tangents[1][1] = 1;
	verts[3].tangents[1][2] = 0;

	bool ident = !mat.IsIdentity();
	if ( ident ) {
		verts[0].xyz -= origin;
		verts[0].xyz *= mat;
		verts[0].xyz += origin;
		verts[1].xyz -= origin;
		verts[1].xyz *= mat;
		verts[1].xyz += origin;
		verts[2].xyz -= origin;
		verts[2].xyz *= mat;
		verts[2].xyz += origin;
		verts[3].xyz -= origin;
		verts[3].xyz *= mat;
		verts[3].xyz += origin;
	}

	//#modified-fva; BEGIN
	if (adjustCoords) {
		for( int i=0; i<4; ++i) {
			// Note: if cstAdjustCoords == false; cst_*Offset is 0, so that doesn't require special handling
			float x = verts[i].xyz[0] * xScale + cst_xOffset;
			float y = verts[i].xyz[1] * yScale + cst_yOffset;
			verts[i].xyz[0] = x * fixScaleForMenu.x + fixOffsetForMenu.x;
			verts[i].xyz[1] = y * fixScaleForMenu.y + fixOffsetForMenu.y;
		}
	}
	//#modified-fva; END

	renderSystem->DrawStretchPic( &verts[0], &indexes[0], 4, 6, shader, ident );

}


void idDeviceContext::DrawMaterial(float x, float y, float w, float h, const idMaterial *mat, const idVec4 &color, float scalex, float scaley) {

	renderSystem->SetColor(color);

	float	s0, s1, t0, t1;
//
//  handle negative scales as well
	if ( scalex < 0 )
	{
		w *= -1;
		scalex *= -1;
	}
	if ( scaley < 0 )
	{
		h *= -1;
		scaley *= -1;
	}
//
	if( w < 0 ) {	// flip about vertical
		w  = -w;
		s0 = 1 * scalex;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1 * scalex;
	}

	if( h < 0 ) {	// flip about horizontal
		h  = -h;
		t0 = 1 * scaley;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1 * scaley;
	}

	if ( ClippedCoords( &x, &y, &w, &h, &s0, &t0, &s1, &t1 ) ) {
		return;
	}

	//#modified-fva; BEGIN
	/*
	AdjustCoords(&x, &y, &w, &h);

	DrawStretchPic( x, y, w, h, s0, t0, s1, t1, mat);
	*/
	DrawStretchPic(x, y, w, h, s0, t0, s1, t1, mat, true);
	//#modified-fva; END
}

void idDeviceContext::DrawMaterialRotated(float x, float y, float w, float h, const idMaterial *mat, const idVec4 &color, float scalex, float scaley, float angle) {

	renderSystem->SetColor(color);

	float	s0, s1, t0, t1;
	//
	//  handle negative scales as well
	if ( scalex < 0 )
	{
		w *= -1;
		scalex *= -1;
	}
	if ( scaley < 0 )
	{
		h *= -1;
		scaley *= -1;
	}
	//
	if( w < 0 ) {	// flip about vertical
		w  = -w;
		s0 = 1 * scalex;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1 * scalex;
	}

	if( h < 0 ) {	// flip about horizontal
		h  = -h;
		t0 = 1 * scaley;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1 * scaley;
	}

	if ( angle == 0.0f && ClippedCoords( &x, &y, &w, &h, &s0, &t0, &s1, &t1 ) ) {
		return;
	}

	//#modified-fva; BEGIN
	/*
	AdjustCoords(&x, &y, &w, &h);

	DrawStretchPicRotated( x, y, w, h, s0, t0, s1, t1, mat, angle);
	*/
	DrawStretchPicRotated(x, y, w, h, s0, t0, s1, t1, mat, angle, true);
	//#modified-fva; END
}

// fva/DG: added adjustCoords argument for CstDoom3 anchored GUIs and our old
//         scale-menus-to-4:3-fix, it basically replaces calling AdjustCoords(&x, &y, &w, &h)
//         before calling this
void idDeviceContext::DrawStretchPicRotated(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *shader, float angle, bool adjustCoords) {

	idDrawVert verts[4];
	glIndex_t indexes[6];
	indexes[0] = 3;
	indexes[1] = 0;
	indexes[2] = 2;
	indexes[3] = 2;
	indexes[4] = 0;
	indexes[5] = 1;
	verts[0].xyz[0] = x;
	verts[0].xyz[1] = y;
	verts[0].xyz[2] = 0;
	verts[0].st[0] = s1;
	verts[0].st[1] = t1;
	verts[0].normal[0] = 0;
	verts[0].normal[1] = 0;
	verts[0].normal[2] = 1;
	verts[0].tangents[0][0] = 1;
	verts[0].tangents[0][1] = 0;
	verts[0].tangents[0][2] = 0;
	verts[0].tangents[1][0] = 0;
	verts[0].tangents[1][1] = 1;
	verts[0].tangents[1][2] = 0;
	verts[1].xyz[0] = x + w;
	verts[1].xyz[1] = y;
	verts[1].xyz[2] = 0;
	verts[1].st[0] = s2;
	verts[1].st[1] = t1;
	verts[1].normal[0] = 0;
	verts[1].normal[1] = 0;
	verts[1].normal[2] = 1;
	verts[1].tangents[0][0] = 1;
	verts[1].tangents[0][1] = 0;
	verts[1].tangents[0][2] = 0;
	verts[1].tangents[1][0] = 0;
	verts[1].tangents[1][1] = 1;
	verts[1].tangents[1][2] = 0;
	verts[2].xyz[0] = x + w;
	verts[2].xyz[1] = y + h;
	verts[2].xyz[2] = 0;
	verts[2].st[0] = s2;
	verts[2].st[1] = t2;
	verts[2].normal[0] = 0;
	verts[2].normal[1] = 0;
	verts[2].normal[2] = 1;
	verts[2].tangents[0][0] = 1;
	verts[2].tangents[0][1] = 0;
	verts[2].tangents[0][2] = 0;
	verts[2].tangents[1][0] = 0;
	verts[2].tangents[1][1] = 1;
	verts[2].tangents[1][2] = 0;
	verts[3].xyz[0] = x;
	verts[3].xyz[1] = y + h;
	verts[3].xyz[2] = 0;
	verts[3].st[0] = s1;
	verts[3].st[1] = t2;
	verts[3].normal[0] = 0;
	verts[3].normal[1] = 0;
	verts[3].normal[2] = 1;
	verts[3].tangents[0][0] = 1;
	verts[3].tangents[0][1] = 0;
	verts[3].tangents[0][2] = 0;
	verts[3].tangents[1][0] = 0;
	verts[3].tangents[1][1] = 1;
	verts[3].tangents[1][2] = 0;

	bool ident = !mat.IsIdentity();
	if ( ident ) {
		verts[0].xyz -= origin;
		verts[0].xyz *= mat;
		verts[0].xyz += origin;
		verts[1].xyz -= origin;
		verts[1].xyz *= mat;
		verts[1].xyz += origin;
		verts[2].xyz -= origin;
		verts[2].xyz *= mat;
		verts[2].xyz += origin;
		verts[3].xyz -= origin;
		verts[3].xyz *= mat;
		verts[3].xyz += origin;
	}

	//Generate a translation so we can translate to the center of the image rotate and draw
	idVec3 origTrans;
	origTrans.x = x+(w/2);
	origTrans.y = y+(h/2);
	origTrans.z = 0;


	//Rotate the verts about the z axis before drawing them
	idMat4 rotz;
	rotz.Identity();
	float sinAng = idMath::Sin(angle);
	float cosAng = idMath::Cos(angle);
	rotz[0][0] = cosAng;
	rotz[0][1] = sinAng;
	rotz[1][0] = -sinAng;
	rotz[1][1] = cosAng;
	for(int i = 0; i < 4; i++) {
		//Translate to origin
		verts[i].xyz -= origTrans;

		//Rotate
		verts[i].xyz = rotz * verts[i].xyz;

		//Translate back
		verts[i].xyz += origTrans;
	}

	//#modified-fva; BEGIN
	if (adjustCoords) {
		for( int i=0; i<4; ++i) {
			// Note: if cstAdjustCoords == false; cst_*Offset is 0, so that doesn't require special handling
			float x = verts[i].xyz[0] * xScale + cst_xOffset;
			float y = verts[i].xyz[1] * yScale + cst_yOffset;
			verts[i].xyz[0] = x * fixScaleForMenu.x + fixOffsetForMenu.x;
			verts[i].xyz[1] = y * fixScaleForMenu.y + fixOffsetForMenu.y;
		}
	}
	//#modified-fva; END

	renderSystem->DrawStretchPic( &verts[0], &indexes[0], 4, 6, shader, (angle == 0.0) ? false : true );
}

void idDeviceContext::DrawFilledRect( float x, float y, float w, float h, const idVec4 &color) {

	if ( color.w == 0.0f ) {
		return;
	}

	renderSystem->SetColor(color);

	if (ClippedCoords(&x, &y, &w, &h, NULL, NULL, NULL, NULL)) {
		return;
	}

	//#modified-fva; BEGIN
	/*
	AdjustCoords(&x, &y, &w, &h);
	DrawStretchPic( x, y, w, h, 0, 0, 0, 0, whiteImage);
	*/
	DrawStretchPic(x, y, w, h, 0, 0, 0, 0, whiteImage, true);
	//#modified-fva; END
}


void idDeviceContext::DrawRect( float x, float y, float w, float h, float size, const idVec4 &color) {

	if ( color.w == 0.0f ) {
		return;
	}

	renderSystem->SetColor(color);

	if (ClippedCoords(&x, &y, &w, &h, NULL, NULL, NULL, NULL)) {
		return;
	}

	//#modified-fva; BEGIN
	/*
	AdjustCoords(&x, &y, &w, &h);
	DrawStretchPic( x, y, size, h, 0, 0, 0, 0, whiteImage );
	DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, whiteImage );
	DrawStretchPic( x, y, w, size, 0, 0, 0, 0, whiteImage );
	DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, whiteImage );
	*/
	DrawStretchPic(x, y + size, size, h - 2.0f * size, 0, 0, 0, 0, whiteImage, true);
	DrawStretchPic(x + w - size, y + size, size, h - 2.0f * size, 0, 0, 0, 0, whiteImage, true);
	DrawStretchPic(x, y, w, size, 0, 0, 0, 0, whiteImage, true);
	DrawStretchPic(x, y + h - size, w, size, 0, 0, 0, 0, whiteImage, true);
	//#modified-fva; END
}

void idDeviceContext::DrawMaterialRect( float x, float y, float w, float h, float size, const idMaterial *mat, const idVec4 &color) {

	if ( color.w == 0.0f ) {
		return;
	}

	renderSystem->SetColor(color);
	DrawMaterial( x, y, size, h, mat, color );
	DrawMaterial( x + w - size, y, size, h, mat, color );
	DrawMaterial( x, y, w, size, mat, color );
	DrawMaterial( x, y + h - size, w, size, mat, color );
}


void idDeviceContext::SetCursor(int n) {
	cursor = (n < CURSOR_ARROW || n >= CURSOR_COUNT) ? CURSOR_ARROW : n;
}

void idDeviceContext::DrawCursor(float *x, float *y, float size) {
	if (*x < 0) {
		*x = 0;
	}

	if (*x >= vidWidth) {
		*x = vidWidth;
	}

	if (*y < 0) {
		*y = 0;
	}

	if (*y >= vidHeight) {
		*y = vidHeight;
	}

	renderSystem->SetColor(colorWhite);

	// DG: originally, this just called AdjustCoords() and then DrawStretchPic().
	//     It had to be adjusted to scale menus and other fullscreen GUIs to 4:3 aspect ratio
	//     and for the CstDoom3 anchored GUIs, so all that is now done here

	// the following block used to be Adjust(Cursor)Coords()
	// (no point in keeping that function when it's only used here)

	// if cstAdjustCoords is used, x and y shouldn't be scaled, otherwise the cursor moves to a window border
	if ( !cstAdjustCoords ) {
		*x *= xScale;
		*y *= yScale;
	}

	// the *actual* sizes and position used (but not set to *x and *y) need to apply the menu fixes
	float sizeW = size * fixScaleForMenu.x * xScale;
	float sizeH = size * fixScaleForMenu.y * yScale;
	float fixedX = *x * fixScaleForMenu.x + fixOffsetForMenu.x;
	float fixedY = *y * fixScaleForMenu.y + fixOffsetForMenu.y;

	DrawStretchPic(fixedX, fixedY, sizeW, sizeH, 0, 0, 1, 1, cursorImages[cursor]);
}
/*
 =======================================================================================================================
 =======================================================================================================================
 */

void idDeviceContext::PaintChar(float x,float y,float width,float height,float scale,float	s,float	t,float	s2,float t2,const idMaterial *hShader) {
	float	w, h;
	w = width * scale;
	h = height * scale;

	if (ClippedCoords(&x, &y, &w, &h, &s, &t, &s2, &t2)) {
		return;
	}

	//#modified-fva; BEGIN
	/*
	AdjustCoords(&x, &y, &w, &h);
	DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader);
	*/
	DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader, true);
	//#modified-fva; END
}


void idDeviceContext::SetFontByScale(float scale) {
	if (scale <= gui_smallFontLimit.GetFloat()) {
		useFont = &activeFont->fontInfoSmall;
		activeFont->maxHeight = activeFont->maxHeightSmall;
		activeFont->maxWidth = activeFont->maxWidthSmall;
	} else if (scale <= gui_mediumFontLimit.GetFloat()) {
		useFont = &activeFont->fontInfoMedium;
		activeFont->maxHeight = activeFont->maxHeightMedium;
		activeFont->maxWidth = activeFont->maxWidthMedium;
	} else {
		useFont = &activeFont->fontInfoLarge;
		activeFont->maxHeight = activeFont->maxHeightLarge;
		activeFont->maxWidth = activeFont->maxWidthLarge;
	}
}

int idDeviceContext::DrawText(float x, float y, float scale, idVec4 color, const char *text, float adjust, int limit, int style, int cursor) {
	int			len, count;
	idVec4		newColor;
	const glyphInfo_t *glyph;
	float		useScale;
	SetFontByScale(scale);
	useScale = scale * useFont->glyphScale;
	count = 0;
	if ( text && color.w != 0.0f ) {
		const unsigned char	*s = (const unsigned char*)text;
		renderSystem->SetColor(color);
		memcpy(&newColor[0], &color[0], sizeof(idVec4));
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}

		while (s && *s && count < len) {
			if ( *s < GLYPH_START || *s > GLYPH_END ) {
				s++;
				continue;
			}
			glyph = &useFont->glyphs[*s];

			//
			// int yadj = Assets.textFont.glyphs[text[i]].bottom +
			// Assets.textFont.glyphs[text[i]].top; float yadj = scale *
			// (Assets.textFont.glyphs[text[i]].imageHeight -
			// Assets.textFont.glyphs[text[i]].height);
			//
			if ( idStr::IsColor((const char*)s) ) {
				if ( *(s+1) == C_COLOR_DEFAULT ) {
					newColor = color;
				} else {
					newColor = idStr::ColorForIndex( *(s+1) );
					newColor[3] = color[3];
				}
				if (cursor == count || cursor == count+1) {
					float partialSkip = ((glyph->xSkip * useScale) + adjust) / 5.0f;
					if ( cursor == count ) {
						partialSkip *= 2.0f;
					} else {
						renderSystem->SetColor(newColor);
					}
					DrawEditCursor(x - partialSkip, y, scale);
				}
				renderSystem->SetColor(newColor);
				s += 2;
				count += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				PaintChar(x,y - yadj,glyph->imageWidth,glyph->imageHeight,useScale,glyph->s,glyph->t,glyph->s2,glyph->t2,glyph->glyph);

				if (cursor == count) {
					DrawEditCursor(x, y, scale);
				}
				x += (glyph->xSkip * useScale) + adjust;
				s++;
				count++;
			}
		}
		if (cursor == len) {
			DrawEditCursor(x, y, scale);
		}
	}
	return count;
}

void idDeviceContext::SetSize(float width, float height) {
	vidWidth = VIRTUAL_WIDTH;
	vidHeight = VIRTUAL_HEIGHT;
	xScale = yScale = 1.0f; // DG: I think this was also changed by fva
	//#modified-fva; BEGIN
	cst_xOffset = cst_yOffset = 0.0f;
	cstAdjustCoords = false;
	if ((width != vidWidth || height != vidHeight) && width > 0.0f && height > 0.0f) {
		cstAdjustCoords = true;
	//#modified-fva; END
		xScale = vidWidth * ( 1.0f / width );
		yScale = vidHeight * ( 1.0f / height );
	}
}

//#modified-fva; BEGIN
// ===============
static bool CstGetVidScale(float &_xScale, float &_yScale) {
	int glWidth, glHeight;
	renderSystem->GetGLSettings(glWidth, glHeight);
	if (glWidth <= 0 || glHeight <= 0) {
		return false;
	}

	float glAspectRatio = (float)glWidth / (float)glHeight;

	const float vidWidth = VIRTUAL_WIDTH;
	const float vidHeight = VIRTUAL_HEIGHT;
	const float vidAspectRatio = (float)VIRTUAL_WIDTH / (float)VIRTUAL_HEIGHT;

	float modWidth = vidWidth;
	float modHeight = vidHeight;
	if (glAspectRatio >= vidAspectRatio) {
		modWidth = modHeight * glAspectRatio;
	} else {
		modHeight = modWidth / glAspectRatio;
	}

	_xScale = vidWidth / modWidth;
	_yScale = vidHeight / modHeight;
	return true;
}


static void CstAdjustParmsForAnchor(int anchor, float &_xScale, float &_yScale, float &_xOffset, float &_yOffset) {
	const float vidWidth = VIRTUAL_WIDTH;
	const float vidHeight = VIRTUAL_HEIGHT;

	switch (anchor) {
	case idDeviceContext::CST_ANCHOR_TOP_LEFT: {
		_xOffset = 0.0f;
		_yOffset = 0.0f;
		break;
	}
	case idDeviceContext::CST_ANCHOR_TOP_CENTER: {
		_xOffset = (vidWidth * 0.5f) * (1.0f - _xScale);
		_yOffset = 0.0f;
		break;
	}
	case idDeviceContext::CST_ANCHOR_TOP_RIGHT: {
		_xOffset = vidWidth * (1.0f - _xScale);
		_yOffset = 0.0f;
		break;
	}
	case idDeviceContext::CST_ANCHOR_CENTER_LEFT: {
		_xOffset = 0.0f;
		_yOffset = (vidHeight * 0.5f) * (1.0f - _yScale);
		break;
	}
	case idDeviceContext::CST_ANCHOR_CENTER_CENTER: {
		_xOffset = (vidWidth * 0.5f) * (1.0f - _xScale);
		_yOffset = (vidHeight * 0.5f) * (1.0f - _yScale);
		break;
	}
	case idDeviceContext::CST_ANCHOR_CENTER_RIGHT: {
		_xOffset = vidWidth * (1.0f - _xScale);
		_yOffset = (vidHeight * 0.5f) * (1.0f - _yScale);
		break;
	}
	case idDeviceContext::CST_ANCHOR_BOTTOM_LEFT: {
		_xOffset = 0.0f;
		_yOffset = vidHeight * (1.0f - _yScale);
		break;
	}
	case idDeviceContext::CST_ANCHOR_BOTTOM_CENTER: {
		_xOffset = (vidWidth * 0.5f) * (1.0f - _xScale);
		_yOffset = vidHeight * (1.0f - _yScale);
		break;
	}
	case idDeviceContext::CST_ANCHOR_BOTTOM_RIGHT: {
		_xOffset = vidWidth * (1.0f - _xScale);
		_yOffset = vidHeight * (1.0f - _yScale);
		break;
	}
	case idDeviceContext::CST_ANCHOR_TOP: {
		_xScale = 1.0f; // no horizontal scaling
		_xOffset = 0.0f;
		_yOffset = 0.0f;
		break;
	}
	case idDeviceContext::CST_ANCHOR_VCENTER: {
		_xScale = 1.0f; // no horizontal scaling
		_xOffset = 0.0f;
		_yOffset = (vidHeight * 0.5f) * (1.0f - _yScale);
		break;
	}
	case idDeviceContext::CST_ANCHOR_BOTTOM: {
		_xScale = 1.0f; // no horizontal scaling
		_xOffset = 0.0f;
		_yOffset = vidHeight * (1.0f - _yScale);
		break;
	}
	case idDeviceContext::CST_ANCHOR_LEFT: {
		_yScale = 1.0f; // no vertical scaling
		_xOffset = 0.0f;
		_yOffset = 0.0f;
		break;
	}
	case idDeviceContext::CST_ANCHOR_HCENTER: {
		_yScale = 1.0f; // no vertical scaling
		_xOffset = (vidWidth * 0.5f) * (1.0f - _xScale);
		_yOffset = 0.0f;
		break;
	}
	case idDeviceContext::CST_ANCHOR_RIGHT: {
		_yScale = 1.0f; // no vertical scaling
		_xOffset = vidWidth * (1.0f - _xScale);
		_yOffset = 0.0f;
		break;
	}
	default: {
		_xOffset = 0.0f;
		_yOffset = 0.0f;
		break;
	}
	}
}

// static
bool idDeviceContext::CstGetParams(int anchor, int anchorTo, float factor, idVec2& out_Scale, idVec2& out_Offset)
{
	float xScale = 1.0f;
	float yScale = 1.0f;
	out_Offset.Set(0, 0);

	if (!CstGetVidScale(xScale, yScale)) {
		out_Scale.Set(1, 1);
		return false;
	}

	if (anchorTo == idDeviceContext::CST_ANCHOR_NONE) {
		CstAdjustParmsForAnchor(anchor, xScale, yScale, out_Offset.x, out_Offset.y);
	} else {
		float from_xScale = xScale;
		float from_yScale = yScale;
		float from_xOffset = 0.0f;
		float from_yOffset = 0.0f;
		CstAdjustParmsForAnchor(anchor, from_xScale, from_yScale, from_xOffset, from_yOffset);

		float to_xScale = xScale;
		float to_yScale = yScale;
		float to_xOffset = 0.0f;
		float to_yOffset = 0.0f;
		CstAdjustParmsForAnchor(anchorTo, to_xScale, to_yScale, to_xOffset, to_yOffset);

		factor = idMath::ClampFloat(0.0f, 1.0f, factor);

		xScale = from_xScale * (1.0f - factor) + to_xScale * factor;
		yScale = from_yScale * (1.0f - factor) + to_yScale * factor;

		out_Offset.x = from_xOffset * (1.0f - factor) + to_xOffset * factor;
		out_Offset.y = from_yOffset * (1.0f - factor) + to_yOffset * factor;
	}
	out_Scale.Set(xScale, yScale);
	return true;
}

void idDeviceContext::CstSetSize(int anchor, int anchorTo, float factor) {
	vidWidth = VIRTUAL_WIDTH;
	vidHeight = VIRTUAL_HEIGHT;

	idVec2 scale;
	idVec2 offset;
	cstAdjustCoords = CstGetParams(anchor, anchorTo, factor, scale, offset);
	xScale = scale.x;
	yScale = scale.y;
	cst_xOffset = offset.x;
	cst_yOffset = offset.y;
}
//#modified-fva; END

int idDeviceContext::CharWidth( const char c, float scale ) {
	glyphInfo_t *glyph;
	float		useScale;
	SetFontByScale(scale);
	fontInfo_t	*font = useFont;
	useScale = scale * font->glyphScale;
	glyph = &font->glyphs[(const unsigned char)c];
	return idMath::FtoiFast( glyph->xSkip * useScale );
}

int idDeviceContext::TextWidth( const char *text, float scale, int limit ) {
	int i, width;

	SetFontByScale( scale );
	const glyphInfo_t *glyphs = useFont->glyphs;

	if ( text == NULL ) {
		return 0;
	}

	width = 0;
	if ( limit > 0 ) {
		for ( i = 0; text[i] != '\0' && i < limit; i++ ) {
			if ( idStr::IsColor( text + i ) ) {
				i++;
			} else {
				width += glyphs[((const unsigned char *)text)[i]].xSkip;
			}
		}
	} else {
		for ( i = 0; text[i] != '\0'; i++ ) {
			if ( idStr::IsColor( text + i ) ) {
				i++;
			} else {
				width += glyphs[((const unsigned char *)text)[i]].xSkip;
			}
		}
	}
	return idMath::FtoiFast( scale * useFont->glyphScale * width );
}

int idDeviceContext::TextHeight(const char *text, float scale, int limit) {
	int			len, count;
	float		max;
	glyphInfo_t *glyph;
	float		useScale;
	const char	*s = text;
	SetFontByScale(scale);
	fontInfo_t	*font = useFont;

	useScale = scale * font->glyphScale;
	max = 0;
	if (text) {
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}

		count = 0;
		while (s && *s && count < len) {
			if ( idStr::IsColor(s) ) {
				s += 2;
				continue;
			}
			else {
				glyph = &font->glyphs[*(const unsigned char*)s];
				if (max < glyph->height) {
					max = glyph->height;
				}

				s++;
				count++;
			}
		}
	}

	return idMath::FtoiFast( max * useScale );
}

int idDeviceContext::MaxCharWidth(float scale) {
	SetFontByScale(scale);
	float useScale = scale * useFont->glyphScale;
	return idMath::FtoiFast( activeFont->maxWidth * useScale );
}

int idDeviceContext::MaxCharHeight(float scale) {
	SetFontByScale(scale);
	float useScale = scale * useFont->glyphScale;
	return idMath::FtoiFast( activeFont->maxHeight * useScale );
}

const idMaterial *idDeviceContext::GetScrollBarImage(int index) {
	if (index >= SCROLLBAR_HBACK && index < SCROLLBAR_COUNT) {
		return scrollBarImages[index];
	}
	return scrollBarImages[SCROLLBAR_HBACK];
}

// this only supports left aligned text
idRegion *idDeviceContext::GetTextRegion(const char *text, float textScale, idRectangle rectDraw, float xStart, float yStart) {
#if 0
	const char	*p, *textPtr, *newLinePtr;
	char		buff[1024];
	int			len, textWidth, newLine, newLineWidth;
	float		y;

	float charSkip = MaxCharWidth(textScale) + 1;
	float lineSkip = MaxCharHeight(textScale);

	textWidth = 0;
	newLinePtr = NULL;
#endif
	return NULL;
/*
	if (text == NULL) {
		return;
	}

	textPtr = text;
	if (*textPtr == '\0') {
		return;
	}

	y = lineSkip + rectDraw.y + yStart;
	len = 0;
	buff[0] = '\0';
	newLine = 0;
	newLineWidth = 0;
	p = textPtr;

	textWidth = 0;
	while (p) {
		if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\0') {
			newLine = len;
			newLinePtr = p + 1;
			newLineWidth = textWidth;
		}

		if ((newLine && textWidth > rectDraw.w) || *p == '\n' || *p == '\0') {
			if (len) {

				float x = rectDraw.x ;

				buff[newLine] = '\0';
				DrawText(x, y, textScale, color, buff, 0, 0, 0);
				if (!wrap) {
					return;
				}
			}

			if (*p == '\0') {
				break;
			}

			y += lineSkip + 5;
			p = newLinePtr;
			len = 0;
			newLine = 0;
			newLineWidth = 0;
			continue;
		}

		buff[len++] = *p++;
		buff[len] = '\0';
		textWidth = TextWidth( buff, textScale, -1 );
	}
*/
}

void idDeviceContext::DrawEditCursor( float x, float y, float scale ) {
	if ( (int)( com_ticNumber >> 4 ) & 1 ) {
		return;
	}
	SetFontByScale(scale);
	float useScale = scale * useFont->glyphScale;
	const glyphInfo_t *glyph2 = &useFont->glyphs[overStrikeMode ? int('_') : int('|')];
	float	yadj = useScale * glyph2->top;
	PaintChar(x, y - yadj,glyph2->imageWidth,glyph2->imageHeight,useScale,glyph2->s,glyph2->t,glyph2->s2,glyph2->t2,glyph2->glyph);
}

int idDeviceContext::DrawText( const char *text, float textScale, int textAlign, idVec4 color, idRectangle rectDraw, bool wrap, int cursor, bool calcOnly, idList<int> *breaks, int limit ) {
	const char	*p, *textPtr, *newLinePtr;
	char		buff[1024];
	int			len, newLine, newLineWidth, count;
	float		y;
	float		textWidth;

	float		charSkip = MaxCharWidth( textScale ) + 1;
	float		lineSkip = MaxCharHeight( textScale );

	float		cursorSkip = ( cursor >= 0 ? charSkip : 0 );

	bool		lineBreak, wordBreak;

	SetFontByScale( textScale );

	textWidth = 0;
	newLinePtr = NULL;

	if (!calcOnly && !(text && *text)) {
		if (cursor == 0) {
			renderSystem->SetColor(color);
			DrawEditCursor(rectDraw.x, lineSkip + rectDraw.y, textScale);
		}
		return idMath::FtoiFast( rectDraw.w / charSkip );
	}

	textPtr = text;

	y = lineSkip + rectDraw.y;
	len = 0;
	buff[0] = '\0';
	newLine = 0;
	newLineWidth = 0;
	p = textPtr;

	if ( breaks ) {
		breaks->Append(0);
	}
	count = 0;
	textWidth = 0;
	lineBreak = false;
	wordBreak = false;
	while (p) {

		if ( *p == '\n' || *p == '\r' || *p == '\0' ) {
			lineBreak = true;
			if ((*p == '\n' && *(p + 1) == '\r') || (*p == '\r' && *(p + 1) == '\n')) {
				p++;
			}
		}

		int nextCharWidth = ( idStr::CharIsPrintable(*p) ? CharWidth( *p, textScale ) : cursorSkip );
		// FIXME: this is a temp hack until the guis can be fixed not not overflow the bounding rectangles
		//		  the side-effect is that list boxes and edit boxes will draw over their scroll bars
		//	The following line and the !linebreak in the if statement below should be removed
		nextCharWidth = 0;

		if ( !lineBreak && ( textWidth + nextCharWidth ) > rectDraw.w ) {
			// The next character will cause us to overflow, if we haven't yet found a suitable
			// break spot, set it to be this character
			if ( len > 0 && newLine == 0 ) {
				newLine = len;
				newLinePtr = p;
				newLineWidth = textWidth;
			}
			wordBreak = true;
		} else if ( lineBreak || ( wrap && (*p == ' ' || *p == '\t') ) ) {
			// The next character is in view, so if we are a break character, store our position
			newLine = len;
			newLinePtr = p + 1;
			newLineWidth = textWidth;
		}

		if ( lineBreak || wordBreak ) {
			float x = rectDraw.x;

			if (textAlign == ALIGN_RIGHT) {
				x = rectDraw.x + rectDraw.w - newLineWidth;
			} else if (textAlign == ALIGN_CENTER) {
				x = rectDraw.x + (rectDraw.w - newLineWidth) / 2;
			}

			if ( wrap || newLine > 0 ) {
				buff[newLine] = '\0';

				// This is a special case to handle breaking in the middle of a word.
				// if we didn't do this, the cursor would appear on the end of this line
				// and the beginning of the next.
				if ( wordBreak && cursor >= newLine && newLine == len ) {
					cursor++;
				}
			}

			if (!calcOnly) {
				count += DrawText(x, y, textScale, color, buff, 0, 0, 0, cursor);
			}

			if ( cursor < newLine ) {
				cursor = -1;
			} else if ( cursor >= 0 ) {
				cursor -= ( newLine + 1 );
			}

			if ( !wrap ) {
				return newLine;
			}

			if ( ( limit && count > limit ) || *p == '\0' ) {
				break;
			}

			y += lineSkip + 5;

			if ( !calcOnly && y > rectDraw.Bottom() ) {
				break;
			}

			p = newLinePtr;

			if (breaks) {
				breaks->Append(p - text);
			}

			len = 0;
			newLine = 0;
			newLineWidth = 0;
			textWidth = 0;
			lineBreak = false;
			wordBreak = false;
			continue;
		}

		buff[len++] = *p++;
		buff[len] = '\0';
		// update the width
		if ( *( buff + len - 1 ) != C_COLOR_ESCAPE && (len <= 1 || *( buff + len - 2 ) != C_COLOR_ESCAPE)) {
			textWidth += textScale * useFont->glyphScale * useFont->glyphs[ (const unsigned char)*( buff + len - 1 ) ].xSkip;
		}
	}

	return idMath::FtoiFast( rectDraw.w / charSkip );
}

/*
=============
idRectangle::String
=============
*/
char *idRectangle::String( void ) const {
	static	int		index = 0;
	static	char	str[ 8 ][ 48 ];
	char	*s;

	// use an array so that multiple toString's won't collide
	s = str[ index ];
	index = (index + 1)&7;

	sprintf( s, "%.2f %.2f %.2f %.2f", x, y, w, h );

	return s;
}
