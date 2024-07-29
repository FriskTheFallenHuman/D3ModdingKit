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

#include "tr_local.h"
#include "Model_local.h"
#include "Model_obj.h"

/*
====================
OBJ_LoadOBJ
====================
*/
bool OBJ_LoadOBJ( const char *filename, idList<idDrawVert> &vertices, idList<int> &indices ) {
	idFile* file = fileSystem->OpenFileRead( filename );
	if ( !file ) {
		common->Warning( "Failed to open OBJ file: %s", filename );
		return false;
	}

	idList<idVec3> positions;
	idList<idVec2> texCoords;
	idList<idVec3> normals;

	idStr line;
	while ( file->ReadLine( line ) ) {
		// Skip empty lines and comments
		if ( line.Length() == 0 || line[0] == '#' ) {
			continue;
		}

		if ( line.StartsWith( "v ") ) {
			OBJ_ParseVertex( line, positions );
		} else if ( line.StartsWith( "vt " ) ) {
			OBJ_ParseTexCoord( line, texCoords );
		} else if ( line.StartsWith( "vn " ) ) {
			OBJ_ParseNormal( line, normals );
		} else if ( line.StartsWith( "f " ) ) {
			OBJ_ParseFace( line, indices, positions, texCoords, normals, vertices );
		}
	}

	fileSystem->CloseFile(file);
	return true;
}

/*
====================
OBJ_ParseVertex
====================
*/
void OBJ_ParseVertex( const idStr &line, idList<idVec3> &positions ) {
	idVec3 position;
	sscanf( line.c_str(), "v %f %f %f", &position.x, &position.y, &position.z );
	positions.Append( position );
}

/*
====================
OBJ_ParseTexCoord
====================
*/
void OBJ_ParseTexCoord( const idStr& line, idList<idVec2>& texCoords) {
	idVec2 texCoord;
	sscanf(line.c_str(), "vt %f %f", &texCoord.x, &texCoord.y);
	texCoords.Append( texCoord );
}

/*
====================
OBJ_ParseNormal
====================
*/
void OBJ_ParseNormal( const idStr &line, idList<idVec3> &normals ) {
	idVec3 normal;
	sscanf( line.c_str(), "vn %f %f %f", &normal.x, &normal.y, &normal.z );
	normals.Append(normal);
}

/*
====================
OBJ_ParseFace
====================
*/
void OBJ_ParseFace( const idStr &line, idList<int> &indices, const idList<idVec3> &positions, const idList<idVec2> &texCoords, const idList<idVec3> &normals, idList<idDrawVert> &vertices ) {
	int vertexIndex[3], texCoordIndex[3], normalIndex[3];
	sscanf( line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d",
		&vertexIndex[0], &texCoordIndex[0], &normalIndex[0],
		&vertexIndex[1], &texCoordIndex[1], &normalIndex[1],
		&vertexIndex[2], &texCoordIndex[2], &normalIndex[2] );

	for ( int i = 0; i < 3; i++ ) {
		idDrawVert vert;
		vert.xyz = positions[vertexIndex[2 - i] - 1];
		vert.xyz = idVec3(vert.xyz.x, -vert.xyz.z, vert.xyz.y);
		vert.st = texCoords[texCoordIndex[2 - i] - 1];
		vert.st.y = 1.0f - vert.st.y;
		vert.normal = normals[normalIndex[2 - i] - 1];
		vertices.Append( vert );
		indices.Append( vertices.Num() - 1 );
	}
}
