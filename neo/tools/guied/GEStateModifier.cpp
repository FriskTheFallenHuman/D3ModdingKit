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

#include "GEApp.h"
#include "GEStateModifier.h"

rvGEStateModifier::rvGEStateModifier ( const char* name, idWindow* window, idDict& dict ) :
	rvGEModifier ( name, window ),
	mDict ( dict )
{
	//Ross T 1/6/2015 - commented out this mDict copy because it seems completely
	//redundant (copy constructor happens two lines above) and was causing a bug with adding keys
	//mDict.Copy ( dict );

	// Make a copy of the current dictionary
	mUndoDict.Copy ( mWrapper->GetStateDict() );
}

/*
================
rvGEStateModifier::Apply

Applys the new state dictionary to the window
================
*/
bool rvGEStateModifier::Apply ( void )
{
	return SetState ( mDict );
}

/*
================
rvGEStateModifier::Undo

Applies the undo dictionary to the window
================
*/
bool rvGEStateModifier::Undo ( void )
{
	return SetState ( mUndoDict );
}

/*
================
rvGEStateModifier::Apply

Applys the given dictionary to the window
================
*/
bool rvGEStateModifier::SetState ( idDict& dict )
{
	const idKeyValue*	key;
	int					i;

	// Delete any key thats gone in the new dict
	for ( i = 0; i < mWrapper->GetStateDict().GetNumKeyVals(); i ++ )
	{
		key = mWrapper->GetStateDict().GetKeyVal ( i );
		if ( !key )
		{
			continue;
		}
	}

	mWrapper->SetState ( dict );

	return true;
}
