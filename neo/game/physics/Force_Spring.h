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

#ifndef __FORCE_SPRING_H__
#define __FORCE_SPRING_H__

/*
===============================================================================

	Spring force

===============================================================================
*/

class idForce_Spring : public idForce {

public:
	CLASS_PROTOTYPE( idForce_Spring );

						idForce_Spring( void );
	virtual				~idForce_Spring( void );

	//ivan start
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );
	//ivan end

						// initialize the spring - ivan added some arguments for FraggingFree
	void				InitSpring( float Kstretch, float Kcompress, float damping, float restLength, float maxLength, bool pullEntity1 );
						// set the entities and positions on these entities the spring is attached to
	void				SetPosition(	idPhysics *physics1, int id1, const idVec3 &p1,
										idPhysics *physics2, int id2, const idVec3 &p2 );

public: // common force interface
	virtual void		Evaluate( int time );
	virtual void		RemovePhysics( const idPhysics *phys );

private:

	// spring properties
	float				Kstretch;
	float				Kcompress;
	float				damping;
	float				restLength;
	float				maxLength; // added by FraggingFree
	bool				pullEntity1; // added by FraggingFree

	// positioning
	idPhysics *			physics1;	// first physics object
	int					id1;		// clip model id of first physics object
	idVec3				p1;			// position on clip model
	idPhysics *			physics2;	// second physics object
	int					id2;		// clip model id of second physics object
	idVec3				p2;			// position on clip model

};

#endif /* !__FORCE_SPRING_H__ */
