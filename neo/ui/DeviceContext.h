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

#ifndef __DEVICECONTEXT_H__
#define __DEVICECONTEXT_H__

// device context support for gui stuff
//

#include "Rectangle.h"

const int VIRTUAL_WIDTH = 640;
const int VIRTUAL_HEIGHT = 480;
const int BLINK_DIVISOR = 200;

class idDeviceContext {
public:
	idDeviceContext();
	~idDeviceContext() { }

	void				Init();
	void				Shutdown();
	bool				Initialized() { return initialized; }
	void				EnableLocalization();

	void				GetTransformInfo(idVec3& origin, idMat3& mat );

	void				SetTransformInfo(const idVec3 &origin, const idMat3 &mat);
	void				DrawMaterial(float x, float y, float w, float h, const idMaterial *mat, const idVec4 &color, float scalex = 1.0, float scaley = 1.0);
	void				DrawRect(float x, float y, float width, float height, float size, const idVec4 &color);
	void				DrawFilledRect(float x, float y, float width, float height, const idVec4 &color);
	int					DrawText(const char *text, float textScale, int textAlign, idVec4 color, idRectangle rectDraw, bool wrap, int cursor = -1, bool calcOnly = false, idList<int> *breaks = NULL, int limit = 0 );
	void				DrawMaterialRect( float x, float y, float w, float h, float size, const idMaterial *mat, const idVec4 &color);
	// fva/DG: added adjustCoords argument for CstDoom3 anchored GUIs and our old scale-menus-to-4:3-fix
	void				DrawStretchPic(float x, float y, float w, float h, float s0, float t0, float s1, float t1, const idMaterial *mat, bool adjustCoords = false);

	void				DrawMaterialRotated(float x, float y, float w, float h, const idMaterial *mat, const idVec4 &color, float scalex = 1.0, float scaley = 1.0, float angle = 0.0f);
	// fva/DG: added adjustCoords argument for CstDoom3 anchored GUIs and our old scale-menus-to-4:3-fix
	void				DrawStretchPicRotated(float x, float y, float w, float h, float s0, float t0, float s1, float t1, const idMaterial *mat, float angle = 0.0f, bool adjustCoords = false);

	int					CharWidth( const char c, float scale );
	int					TextWidth(const char *text, float scale, int limit);
	int					TextHeight(const char *text, float scale, int limit);
	int					MaxCharHeight(float scale);
	int					MaxCharWidth(float scale);

	int					FindFont( const char *name );
	void				SetupFonts();

	idRegion			*GetTextRegion(const char *text, float textScale, idRectangle rectDraw, float xStart, float yStart);

	void				SetSize(float width, float height);

	//#modified-fva; BEGIN
	void				CstSetSize(int anchor, int anchorTo, float factor);
	//#modified-fva; END

	const idMaterial	*GetScrollBarImage(int index);

	void				DrawCursor(float *x, float *y, float size);
	void				SetCursor(int n);
	// DG: Note: not sure if AdjustCoords() works entirely as it should, but it seems
	//     good enough for the idRenderWindow with the mars globe in the main menu
	void				AdjustCoords(float *x, float *y, float *w, float *h);
	bool				ClippedCoords(float *x, float *y, float *w, float *h);
	bool				ClippedCoords(float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2);

	void				PushClipRect(float x, float y, float w, float h);
	void				PushClipRect(idRectangle r);
	void				PopClipRect();

	void				EnableClipping(bool b) { enableClipping = b; };
	void				SetFont( int num );

	void				SetOverStrike(bool b) { overStrikeMode = b; }

	bool				GetOverStrike() { return overStrikeMode; }

	void				DrawEditCursor(float x, float y, float scale);

	// DG: this is used for the "make sure menus are rendered as 4:3" hack
	void				SetMenuScaleFix(bool enable);
	bool				IsMenuScaleFixActive() const {
		// also using this for cstAdjustCoords, it basically means "you need to call AdjustCoords()
		return fixOffsetForMenu.x != 0.0f || fixOffsetForMenu.y != 0.0f || cstAdjustCoords;
	}

	// DG: used in idWindow::Contains(), so it can adjust coordinates
	static bool			CstGetParams(int anchor, int anchorTo, float anchorFactor, idVec2& out_Scale, idVec2& out_Offset);

	enum {
		CURSOR_ARROW,
		CURSOR_HAND,
		CURSOR_COUNT
	};

	enum {
		ALIGN_LEFT,
		ALIGN_CENTER,
		ALIGN_RIGHT
	};

	enum {
		SCROLLBAR_HBACK,
		SCROLLBAR_VBACK,
		SCROLLBAR_THUMB,
		SCROLLBAR_RIGHT,
		SCROLLBAR_LEFT,
		SCROLLBAR_UP,
		SCROLLBAR_DOWN,
		SCROLLBAR_COUNT
	};

	//#modified-fva; BEGIN
	enum {
		CST_ANCHOR_NONE = -1,
		CST_ANCHOR_TOP_LEFT,
		CST_ANCHOR_TOP_CENTER,
		CST_ANCHOR_TOP_RIGHT,
		CST_ANCHOR_CENTER_LEFT,
		CST_ANCHOR_CENTER_CENTER,
		CST_ANCHOR_CENTER_RIGHT,
		CST_ANCHOR_BOTTOM_LEFT,
		CST_ANCHOR_BOTTOM_CENTER,
		CST_ANCHOR_BOTTOM_RIGHT,
		CST_ANCHOR_TOP,
		CST_ANCHOR_VCENTER,
		CST_ANCHOR_BOTTOM,
		CST_ANCHOR_LEFT,
		CST_ANCHOR_HCENTER,
		CST_ANCHOR_RIGHT
	};
	//#modified-fva; END

	static idVec4 colorPurple;
	static idVec4 colorOrange;
	static idVec4 colorYellow;
	static idVec4 colorGreen;
	static idVec4 colorBlue;
	static idVec4 colorRed;
	static idVec4 colorWhite;
	static idVec4 colorBlack;
	static idVec4 colorNone;

private:
	int					DrawText(float x, float y, float scale, idVec4 color, const char *text, float adjust, int limit, int style, int cursor = -1);
	void				PaintChar(float x,float y,float width,float height,float scale,float	s,float	t,float	s2,float t2,const idMaterial *hShader);
	void				SetFontByScale( float scale );
	void				Clear( void );

	const idMaterial	*cursorImages[CURSOR_COUNT];
	const idMaterial	*scrollBarImages[SCROLLBAR_COUNT];
	const idMaterial	*whiteImage;
	fontInfoEx_t		*activeFont;
	fontInfo_t			*useFont;
	idStr				fontName;
	float				xScale;
	float				yScale;

	//#modified-fva; BEGIN
	float				cst_xOffset;
	float				cst_yOffset;
	bool				cstAdjustCoords;
	//#modified-fva; END

	float				vidHeight;
	float				vidWidth;

	int					cursor;

	idList<idRectangle>	clipRects;

	static idList<fontInfoEx_t> fonts;
	idStr fontLang;

	bool				enableClipping;

	bool				overStrikeMode;

	idMat3				mat;
	idVec3				origin;
	bool				initialized;

	bool				mbcs;

	// DG: this is used for the "make sure menus are rendered as 4:3" hack
	idVec2				fixScaleForMenu;
	idVec2				fixOffsetForMenu;
};

#endif /* !__DEVICECONTEXT_H__ */
