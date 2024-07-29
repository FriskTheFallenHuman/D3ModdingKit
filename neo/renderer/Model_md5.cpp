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

static const char *MD5_SnapshotName = "_MD5_Snapshot_";

/***********************************************************************

	idMD5Mesh

***********************************************************************/

static int c_numVerts = 0;
static int c_numWeights = 0;
static int c_numWeightJoints = 0;

typedef struct vertexWeight_s {
	int							vert;
	int							joint;
	idVec3						offset;
	float						jointWeight;
} vertexWeight_t;

/*
====================
idMD5Mesh::idMD5Mesh
====================
*/
idMD5Mesh::idMD5Mesh() {

	scaledWeights	= NULL;
	weightIndex		= NULL;
	shader			= NULL;
	numTris			= 0;
	deformInfo		= NULL;
	surfaceNum		= 0;

	#if MD5_ENABLE_GIBS > 0
	gibZones		= 0;
	gibShown		= 0;
	gibSpurt		= 0;
	#endif

	#if MD5_ENABLE_LODS > 0
	lodLower		= 0.00f;
	lodUpper		= 0.00f;
	#endif

}

/*
====================
idMD5Mesh::~idMD5Mesh
====================
*/
idMD5Mesh::~idMD5Mesh() {
	Mem_Free16( scaledWeights );
	Mem_Free16( weightIndex );
	if ( deformInfo ) {
		R_FreeDeformInfo( deformInfo );
		deformInfo = NULL;
	}
}

#if MD5_ENABLE_GIBS > 0

/* ====================
idMD5Mesh::ZoneParse
==================== */
int idMD5Mesh::ZoneParse(const char* zone, int& step) {

	int bits = 0;

	if (isxdigit(zone[0])) {
		if (zone[0] == '0' && zone[1] == 'x') {
			char* next = NULL;
			bits = strtol(&zone[2], &next, 16) << 1;
			step += next - zone;
		} else if (strchr(zone, ',')) {
			char* copy = strdup(zone);
			char* part = NULL;
			char* rest = NULL;
			char* next = NULL;
			for (part = strtok_s(copy, ",", &rest); part && isxdigit(part[0]); part = strtok_s(NULL, ",", &rest)) {
				bits |= 1 << (strtol(part, &next, 16) + 1);
			}
			step += next - copy;
			free(copy);
		} else {
			int index = (zone[1] != '-' || zone[2] <= zone[0] || !isxdigit(zone[2]) ? 0 : 2);
			int start = (zone[0]     - 47); if (start > 17) start -= 7; // 1-based index (0-bit is 'unassigned').
			int until = (zone[index] - 47); if (until > 17) until -= 7;
			for (; start <= until; start++) bits |= (1 << start);
			step += index + 1;
		}
	}

	return bits & MD5_GIBBED_BITS;

}

/* ====================
idMD5Mesh::ParseZone
==================== */
void idMD5Mesh::ParseZone(const char* zone, int show) {

	int step = 0;

	if (zone) {
		gibZones = ZoneParse(&zone[step], step); gibShown = int(show != 0);
	}

//	if (step && zone[step] == ';') { step++;
//		gibOther = ZoneParse(&zone[step], step);
//	}

	if (step && show > 1 && gibZones && (gibZones & (gibZones - 1))) { // Needs more than one zone flag (or gibShown will be zero).
		gibShown = gibZones;
		gibZones = 1 << (int)log2f(show == 2 ? gibZones : gibZones - (gibZones & (gibZones - 1))); // show == 2 ? 'STUB' : 'STEM'
		gibShown = gibShown & ~gibZones;
	}

	if (step && zone[step]) {
		gibSpurt = ( // Character equivalent only safe to use with ?-? notation (or may be parsed as HEX with the preceding value).
			zone[step] == '@' ? 5 : zone[step] == 'C' ? 5 :		// Class
			zone[step] == '#' ? 4 : zone[step] == 'S' ? 4 :		// Spark
			zone[step] == '&' ? 3 : zone[step] == 'F' ? 3 :		// Flame
			zone[step] == '$' ? 2 : zone[step] == 'G' ? 2 :		// Gloop
			zone[step] == '%' ? 1 : zone[step] == 'B' ? 1 : 0	// Blood
		);
	}

}

#endif

/*
====================
idMD5Mesh::ParseMesh
====================
*/
#if   MD5_BINARY_MESH > 2 // WRITE+
bool idMD5Mesh::ParseMesh(idLexer &parser, int numJoints, const idJointMat* joints, idFile* data_fd, idFile* text_fd) {
#elif MD5_BINARY_MESH > 1 // WRITE
bool idMD5Mesh::ParseMesh(idLexer &parser, int numJoints, const idJointMat* joints, idFile* data_fd) {
#elif MD5_BINARY_MESH > 0
bool idMD5Mesh::ParseMesh(idLexer &parser, int numJoints, const idJointMat *joints) {
#else
void idMD5Mesh::ParseMesh(idLexer &parser, int numJoints, const idJointMat *joints) {
#endif

	idToken		token;
	idToken		name;
	int			num;
	int			count;
	int			jointnum;
	idStr		shaderName;
	int			i, j;
	idList<int>	tris;
	idList<int>	firstWeightForVertex;
	idList<int>	numWeightsForVertex;
	int			maxweight;
	idList<vertexWeight_t> tempWeights;

	parser.ExpectTokenString( "{" );

	//
	// parse name
	//
	if ( parser.CheckTokenString( "name" ) ) {
		parser.ReadToken( &name );
	}

	//
	// parse shader
	//
	parser.ExpectTokenString( "shader" );

	parser.ReadToken( &token );
	shaderName = token;

	#if MD5_ENABLE_GIBS > 0 || MD5_BINARY_MESH > 1 // WRITE
	idStr shaderComment; parser.ReadRestOfLine(shaderComment);
	#endif

	#if MD5_ENABLE_GIBS > 0
	int trim;
	trim = idStr::FindText(shaderComment.c_str(), "HIDE:");
	if (trim >= 0) ParseZone(&shaderComment.c_str()[trim + 5], 0);
	trim = idStr::FindText(shaderComment.c_str(), "SHOW:");
	if (trim >= 0) ParseZone(&shaderComment.c_str()[trim + 5], 1);
	trim = idStr::FindText(shaderComment.c_str(), "STUB:");
	if (trim >= 0) ParseZone(&shaderComment.c_str()[trim + 5], 2);
	trim = idStr::FindText(shaderComment.c_str(), "STEM:");
	if (trim >= 0) ParseZone(&shaderComment.c_str()[trim + 5], 3);
	trim = idStr::FindText(shaderComment.c_str(), "HIDE!");
	if (trim >= 0) {gibZones = MD5_GIBBED_BITS; gibShown = MD5_GIBBED_HIDE;}
	trim = idStr::FindText(shaderComment.c_str(), "SHOW!");
	if (trim >= 0) {gibZones = MD5_GIBBED_BITS; gibShown = MD5_GIBBED_SHOW;}
	trim = idStr::FindText(shaderComment.c_str(), "TRIM!") + 1;
	#endif

	#if MD5_BINARY_MESH > 2 // WRITE+
	if (text_fd) {
		#if MD5_ENABLE_GIBS > 0
		const char* spurts[] = {"", "%", "$", "&", "#", "@"};
		if /*el*/ (gibZones == MD5_GIBBED_ZERO && trim == 0) {
			text_fd->Printf("mesh {\n\tshader \"%s\"\n",                    shaderName.c_str()                                                                       );
		} else if (gibZones == MD5_GIBBED_ZERO) {
			text_fd->Printf("mesh {\n\tshader \"%s\" // TRIM!\n",           shaderName.c_str()                                                                       );
		} else if (gibZones == MD5_GIBBED_BITS && gibShown == MD5_GIBBED_HIDE) {
			text_fd->Printf("mesh {\n\tshader \"%s\" // HIDE!\n",           shaderName.c_str()                                                                       );
		} else if (gibZones == MD5_GIBBED_BITS && gibShown == MD5_GIBBED_SHOW) {
			text_fd->Printf("mesh {\n\tshader \"%s\" // SHOW!\n",           shaderName.c_str()                                                                       );
		} else if (gibShown == MD5_GIBBED_HIDE) {
			text_fd->Printf("mesh {\n\tshader \"%s\" // HIDE:0x%04X%s%s\n", shaderName.c_str(), (gibZones           ) >> 1, spurts[gibSpurt], trim ? " // TRIM!" : "");
		} else if (gibShown == MD5_GIBBED_SHOW) {
			text_fd->Printf("mesh {\n\tshader \"%s\" // SHOW:0x%04X%s%s\n", shaderName.c_str(), (gibZones           ) >> 1, spurts[gibSpurt], trim ? " // TRIM!" : "");
		} else if (gibShown <= gibZones) {
			text_fd->Printf("mesh {\n\tshader \"%s\" // STUB:0x%04X%s%s\n", shaderName.c_str(), (gibZones | gibShown) >> 1, spurts[gibSpurt], trim ? " // TRIM!" : "");
		} else {
			text_fd->Printf("mesh {\n\tshader \"%s\" // STEM:0x%04X%s%s\n", shaderName.c_str(), (gibZones | gibShown) >> 1, spurts[gibSpurt], trim ? " // TRIM!" : "");
		}
		#else
		text_fd->Printf("mesh {\n\tshader \"%s\"\n", shaderName.c_str());
		#endif
	}
	#endif

	#if MD5_ENABLE_LODS > 0
	if (shaderName.Right(8).Cmpn("/lod_", 5) == 0) { // Example; models/characters/player/body/lod_2_9
		const char* range = &shaderName.c_str()[shaderName.Length() - 3];
		if (range[0] >= '0' && range[0] <= 'Z' && range[1] == '_' && range[2] >= '0' && range[2] <= 'Z') {
			float  steps = r_lodRangeIncrements.GetFloat();
			int ndxLower = range[0] - (range[0] > 57 ? 55 : 48); // 0-9, A-Z >>> 0-35
			int ndxUpper = range[2] - (range[2] > 57 ? 55 : 48); // 0-9, A-Z >>> 0-35
			lodLower = ndxLower * steps; lodLower *= lodLower; // Stored (and compared) squared.
			lodUpper = ndxUpper * steps; lodUpper *= lodUpper;
			shaderName.CapLength(shaderName.Length() - 8);
		}
	}
	#endif

	shader = declManager->FindMaterial( shaderName );

	//
	// parse texture coordinates
	//
	parser.ExpectTokenString( "numverts" );
	count = parser.ParseInt();

	#if MD5_ENABLE_GIBS > 0 // HINTS
	int hintIndex = count;
	int hintFaces = 0;
	#endif

	#if MD5_BINARY_MESH > 0
	bool load_data = false;
	if (parser.PeekTokenString("numtris")) {
		texCoords.SetNum(count);
		parser.ExpectTokenString("numtris"); numTris = parser.ParseInt();
		parser.ExpectTokenString("numweights"); numWeights = parser.ParseInt(); // NB: Not actually used.
		parser.ExpectTokenString("}");
		assert(deformInfo == NULL);
		assert(scaledWeights == NULL);
		assert(weightIndex == NULL);
		load_data = true;
	} else {
	#endif

	if ( count < 0 ) {
		parser.Error( "Invalid size: %s", token.c_str() );
	}

	texCoords.SetNum( count );
	firstWeightForVertex.SetNum( count );
	numWeightsForVertex.SetNum( count );

	numWeights = 0;
	maxweight = 0;
	for( i = 0; i < texCoords.Num(); i++ ) {
		parser.ExpectTokenString( "vert" );
		parser.ParseInt();

		parser.Parse1DMatrix( 2, texCoords[ i ].ToFloatPtr() );

		firstWeightForVertex[ i ]	= parser.ParseInt();
		numWeightsForVertex[ i ]	= parser.ParseInt();

		#if MD5_ENABLE_GIBS > 0 // HINTS
		if (trim && hintIndex > i) {
			if (texCoords[i].x < -0.1250f || texCoords[i].x > +1.1250f || texCoords[i].y < -0.1250f || texCoords[i].y > +1.1250f) {
				hintIndex = i;
			}
		}
		#endif

		if ( !numWeightsForVertex[ i ] ) {
			parser.Error( "Vertex without any joint weights." );
		}

		numWeights += numWeightsForVertex[ i ];
		if ( numWeightsForVertex[ i ] + firstWeightForVertex[ i ] > maxweight ) {
			maxweight = numWeightsForVertex[ i ] + firstWeightForVertex[ i ];
		}
	}

	//
	// parse tris
	//
	parser.ExpectTokenString( "numtris" );
	count = parser.ParseInt();
	if ( count < 0 ) {
		parser.Error( "Invalid size: %d", count );
	}

	tris.SetNum( count * 3 );
	numTris = count;
	for( i = 0; i < count; i++ ) {
		parser.ExpectTokenString( "tri" );
		parser.ParseInt();

		tris[ i * 3 + 0 ] = parser.ParseInt();
		tris[ i * 3 + 1 ] = parser.ParseInt();
		tris[ i * 3 + 2 ] = parser.ParseInt();

		#if MD5_ENABLE_GIBS > 0 // HINTS
		if (trim && hintIndex < texCoords.Num()) {
			if (tris[i * 3 + 0] >= hintIndex) hintFaces++; else
			if (tris[i * 3 + 1] >= hintIndex) hintFaces++; else
			if (tris[i * 3 + 2] >= hintIndex) hintFaces++;
		}
		#endif
	}

	//
	// parse weights
	//
	parser.ExpectTokenString( "numweights" );
	count = parser.ParseInt();
	if ( count < 0 ) {
		parser.Error( "Invalid size: %d", count );
	}

	if ( maxweight > count ) {
		parser.Warning( "Vertices reference out of range weights in model (%d of %d weights).", maxweight, count );
	}

	tempWeights.SetNum( count );

	for( i = 0; i < count; i++ ) {
		parser.ExpectTokenString( "weight" );
		parser.ParseInt();

		jointnum = parser.ParseInt();
		if ( ( jointnum < 0 ) || ( jointnum >= numJoints ) ) {
			parser.Error( "Joint Index out of range(%d): %d", numJoints, jointnum );
		}

		tempWeights[ i ].joint			= jointnum;
		tempWeights[ i ].jointWeight	= parser.ParseFloat();

		parser.Parse1DMatrix( 3, tempWeights[ i ].offset.ToFloatPtr() );
	}

	// create pre-scaled weights and an index for the vertex/joint lookup
	scaledWeights = (idVec4 *) Mem_Alloc16( numWeights * sizeof( scaledWeights[0] ) );
	weightIndex = (int *) Mem_Alloc16( numWeights * 2 * sizeof( weightIndex[0] ) );
	memset( weightIndex, 0, numWeights * 2 * sizeof( weightIndex[0] ) );

	count = 0;
	for( i = 0; i < texCoords.Num(); i++ ) {
		num = firstWeightForVertex[i];
		for( j = 0; j < numWeightsForVertex[i]; j++, num++, count++ ) {
			scaledWeights[count].ToVec3() = tempWeights[num].offset * tempWeights[num].jointWeight;
			scaledWeights[count].w = tempWeights[num].jointWeight;
			weightIndex[count * 2 + 0] = tempWeights[num].joint * sizeof( idJointMat );
		}
		weightIndex[count * 2 - 1] = 1;
	}

	tempWeights.Clear();
	numWeightsForVertex.Clear();
	firstWeightForVertex.Clear();

	parser.ExpectTokenString( "}" );



	// update counters
	c_numVerts += texCoords.Num();
	c_numWeights += numWeights;
	c_numWeightJoints++;
	for ( i = 0; i < numWeights; i++ ) {
		c_numWeightJoints += weightIndex[i*2+1];
	}

	//
	// build the information that will be common to all animations of this mesh:
	// silhouette edge connectivity and normal / tangent generation information
	//
	#if 0 // DEFUNKT I think this has already been fixed (by enlarging the stack size).
	size_t allocaSize = texCoords.Num() * sizeof(idDrawVert);
	idDrawVert* verts;
	if (allocaSize < 600000) {
		verts = (idDrawVert*)_alloca16(allocaSize);
	} else {
		verts = (idDrawVert*)Mem_Alloc16(allocaSize);
	}
	#else
	idDrawVert* verts = (idDrawVert*)_alloca16(texCoords.Num() * sizeof(idDrawVert));
	#endif

	for ( i = 0; i < texCoords.Num(); i++ ) {
		verts[i].Clear();
		verts[i].st = texCoords[i];
	}
	TransformVerts( verts, joints );

	#if MD5_ENABLE_GIBS > 0 // HINTS
	#if MD5_BINARY_MESH > 1 // WRITE
	deformInfo = R_BuildDeformInfo(texCoords.Num(), verts, tris.Num(), tris.Ptr(), (data_fd == NULL && shader->UseUnsmoothedTangents()) || (data_fd != NULL && shaderComment.Icmp("NoUnsmoothedTangents")), hintFaces);
	#else
	deformInfo = R_BuildDeformInfo(texCoords.Num(), verts, tris.Num(), tris.Ptr(), shader->UseUnsmoothedTangents(), hintFaces);
	#endif
	#else
	#if MD5_BINARY_MESH > 1 // WRITE
	deformInfo = R_BuildDeformInfo(texCoords.Num(), verts, tris.Num(), tris.Ptr(), (data_fd == NULL && shader->UseUnsmoothedTangents()) || (data_fd != NULL && shaderComment.Icmp("NoUnsmoothedTangents")));
	#else
	deformInfo = R_BuildDeformInfo(texCoords.Num(), verts, tris.Num(), tris.Ptr(), shader->UseUnsmoothedTangents());
	#endif
	#endif

	#if 0 // DEFUNKT I think this has already been fixed (by enlarging the stack size).
	if (allocaSize >= 600000) Mem_Free16(verts);
	#endif

	#if MD5_ENABLE_GIBS > 0 // HINTS
	if (hintFaces) deformInfo->numHiddenTris = hintFaces * 3;
	#endif

	#if MD5_BINARY_MESH > 2 // WRITE+
	if (text_fd) text_fd->Printf("\tnumverts %d\n\tnumtris %d\n\tnumweights %d\n}\n\n", texCoords.Num(), tris.Num() / 3, numWeights);
	#endif

	#if MD5_BINARY_MESH > 1 // WRITE
	if (data_fd) WriteData(data_fd);
	#endif

	#if MD5_BINARY_MESH > 0
	}
	return load_data;
	#endif

}

#if MD5_BINARY_MESH > 1 // WRITE
/* ====================
idMD5Mesh::WriteData
==================== */
void idMD5Mesh::WriteData(idFile* data_fd) { // NB: Binarisation is not strictly portable; presumes int and float are each of 4-bytes (and little-endian).

	deformInfo->numDominantTris = deformInfo->dominantTris ? deformInfo->numOutputVerts : 0;

	data_fd->Write(deformInfo,      sizeof(int             ) * 8              );
	data_fd->Write(texCoords.Ptr(), sizeof(texCoords[0]    ) * texCoords.Num());
	data_fd->Write(scaledWeights,   sizeof(scaledWeights[0]) *     numWeights );
	data_fd->Write(weightIndex,     sizeof(weightIndex[0]  ) * 2 * numWeights );

	if (deformInfo->numIndexes         ) data_fd->Write(deformInfo->indexes,       sizeof(deformInfo->indexes[0]      ) * deformInfo->numIndexes      );
	if (deformInfo->numIndexes         ) data_fd->Write(deformInfo->silIndexes,    sizeof(deformInfo->silIndexes[0]   ) * deformInfo->numIndexes      );
	if (deformInfo->numSilEdges        ) data_fd->Write(deformInfo->silEdges,      sizeof(deformInfo->silEdges[0]     ) * deformInfo->numSilEdges     );
	if (deformInfo->numDupVerts        ) data_fd->Write(deformInfo->dupVerts,      sizeof(deformInfo->dupVerts[0]     ) * deformInfo->numDupVerts * 2 );
	if (deformInfo->numMirroredVerts   ) data_fd->Write(deformInfo->mirroredVerts, sizeof(deformInfo->mirroredVerts[0]) * deformInfo->numMirroredVerts);
	if (deformInfo->numDominantTris > 0) data_fd->Write(deformInfo->dominantTris,  sizeof(deformInfo->dominantTris[0] ) * deformInfo->numDominantTris );

	if (shader->UseUnsmoothedTangents() != true) R_FreeDeformInfoDominantTris(deformInfo);

}
#endif

#if MD5_BINARY_MESH > 0
/* ====================
idMD5Mesh::FetchData
==================== */
void idMD5Mesh::FetchData(idFile* data_fd) { if (deformInfo) return; // NB: Some unbinarized meshes might be expected.

	deformInfo = R_AllocDeformInfo();
	
	data_fd->Read(deformInfo,       sizeof(int             ) * 8              );
	data_fd->Read(texCoords.Ptr(),  sizeof(texCoords[0]    ) * texCoords.Num());

	#if MD5_ENABLE_LODS > 2 // DEBUG+
	if (deformInfo->numDominantTris == 0) common->Printf("NoDominantTris binarised for; %s\n", data_fd->GetFullPath());
	#endif

	#if MD5_ENABLE_LODS > 2 // DEBUG+
	if (shader->UseUnsmoothedTangents() != true && r_testUnsmoothedTangents.GetInteger() != 2) deformInfo->numDominantTris = -deformInfo->numDominantTris;
	#else
	if (shader->UseUnsmoothedTangents() != true) deformInfo->numDominantTris = -deformInfo->numDominantTris;
	#endif

	scaledWeights = (idVec4*)Mem_Alloc16(sizeof(scaledWeights[0]) * numWeights);
	weightIndex   = (int*)Mem_Alloc16(sizeof(weightIndex[0])  * 2 * numWeights);

	data_fd->Read(scaledWeights,    sizeof(scaledWeights[0])      * numWeights);
	data_fd->Read(weightIndex,      sizeof(weightIndex[0]  )  * 2 * numWeights);

	R_AllocDeformInfo(deformInfo);

	if (deformInfo->numIndexes         ) data_fd->Read(deformInfo->indexes,       sizeof(deformInfo->indexes[0]      ) * deformInfo->numIndexes      );
	if (deformInfo->numIndexes         ) data_fd->Read(deformInfo->silIndexes,    sizeof(deformInfo->silIndexes[0]   ) * deformInfo->numIndexes      );
	if (deformInfo->numSilEdges        ) data_fd->Read(deformInfo->silEdges,      sizeof(deformInfo->silEdges[0]     ) * deformInfo->numSilEdges     );
	if (deformInfo->numDupVerts        ) data_fd->Read(deformInfo->dupVerts,      sizeof(deformInfo->dupVerts[0]     ) * deformInfo->numDupVerts * 2 );
	if (deformInfo->numMirroredVerts   ) data_fd->Read(deformInfo->mirroredVerts, sizeof(deformInfo->mirroredVerts[0]) * deformInfo->numMirroredVerts);
	if (deformInfo->numDominantTris > 0) data_fd->Read(deformInfo->dominantTris,  sizeof(deformInfo->dominantTris[0] ) * deformInfo->numDominantTris ); else
	if (deformInfo->numDominantTris < 0) data_fd->Seek(                          -sizeof(deformInfo->dominantTris[0] ) * deformInfo->numDominantTris, FS_SEEK_CUR);

}
#endif

/*
====================
idMD5Mesh::TransformVerts
====================
*/
void idMD5Mesh::TransformVerts( idDrawVert *verts, const idJointMat *entJoints ) {
	SIMDProcessor->TransformVerts( verts, texCoords.Num(), entJoints, scaledWeights, weightIndex, numWeights );
}

/*
====================
idMD5Mesh::TransformScaledVerts

Special transform to make the mesh seem fat or skinny.  May be used for zombie deaths
====================
*/
void idMD5Mesh::TransformScaledVerts( idDrawVert *verts, const idJointMat *entJoints, float scale ) {
	idVec4 *scaledWeights = (idVec4 *) _alloca16( numWeights * sizeof( scaledWeights[0] ) );
	SIMDProcessor->Mul( scaledWeights[0].ToFloatPtr(), scale, scaledWeights[0].ToFloatPtr(), numWeights * 4 );
	SIMDProcessor->TransformVerts( verts, texCoords.Num(), entJoints, scaledWeights, weightIndex, numWeights );
}

/*
====================
idMD5Mesh::UpdateSurface
====================
*/
void idMD5Mesh::UpdateSurface( const struct renderEntity_s *ent, const idJointMat *entJoints, modelSurface_t *surf ) {
	int i, base;
	srfTriangles_t *tri;

	tr.pc.c_deformedSurfaces++;
	tr.pc.c_deformedVerts += deformInfo->numOutputVerts;
	tr.pc.c_deformedIndexes += deformInfo->numIndexes;

	surf->shader = shader;

	if ( surf->geometry ) {
		// if the number of verts and indexes are the same we can re-use the triangle surface
		// the number of indexes must be the same to assure the correct amount of memory is allocated for the facePlanes
		if ( surf->geometry->numVerts == deformInfo->numOutputVerts && surf->geometry->numIndexes == deformInfo->numIndexes ) {
			R_FreeStaticTriSurfVertexCaches( surf->geometry );
		} else {
			R_FreeStaticTriSurf( surf->geometry );
			surf->geometry = R_AllocStaticTriSurf();
		}
	} else {
		surf->geometry = R_AllocStaticTriSurf();
	}

	tri = surf->geometry;

	// note that some of the data is references, and should not be freed
	tri->deformedSurface = true;
	tri->tangentsCalculated = false;
	tri->facePlanesCalculated = false;

	tri->numIndexes = deformInfo->numIndexes;
	tri->indexes = deformInfo->indexes;
	tri->silIndexes = deformInfo->silIndexes;
	tri->numMirroredVerts = deformInfo->numMirroredVerts;
	tri->mirroredVerts = deformInfo->mirroredVerts;
	tri->numDupVerts = deformInfo->numDupVerts;
	tri->dupVerts = deformInfo->dupVerts;
	tri->numSilEdges = deformInfo->numSilEdges;
	tri->silEdges = deformInfo->silEdges;
	tri->dominantTris = deformInfo->dominantTris;
	tri->numVerts = deformInfo->numOutputVerts;

	if ( tri->verts == NULL ) {
		R_AllocStaticTriSurfVerts( tri, tri->numVerts );
		for ( i = 0; i < deformInfo->numSourceVerts; i++ ) {
			tri->verts[i].Clear();
			tri->verts[i].st = texCoords[i];
		}
	}

	if ( ent->shaderParms[ SHADERPARM_MD5_SKINSCALE ] != 0.0f ) {
		TransformScaledVerts( tri->verts, entJoints, ent->shaderParms[ SHADERPARM_MD5_SKINSCALE ] );
	} else {
		TransformVerts( tri->verts, entJoints );
	}

	// replicate the mirror seam vertexes
	base = deformInfo->numOutputVerts - deformInfo->numMirroredVerts;
	for ( i = 0; i < deformInfo->numMirroredVerts; i++ ) {
		tri->verts[base + i] = tri->verts[deformInfo->mirroredVerts[i]];
	}

	R_BoundTriSurf( tri );

	// If a surface is going to be have a lighting interaction generated, it will also have to call
	// R_DeriveTangents() to get normals, tangents, and face planes.  If it only
	// needs shadows generated, it will only have to generate face planes.  If it only
	// has ambient drawing, or is culled, no additional work will be necessary
	if ( !r_useDeferredTangents.GetBool() ) {
		// set face planes, vertex normals, tangents
		R_DeriveTangents( tri );
	}

	#if MD5_ENABLE_GIBS > 0 // HINTS
	if (deformInfo->numHiddenTris) {
		tri->numIndexes -= deformInfo->numHiddenTris;
		tri->silIndexes -= deformInfo->numHiddenTris;
	}
	#endif

}

/*
====================
idMD5Mesh::CalcBounds
====================
*/
idBounds idMD5Mesh::CalcBounds( const idJointMat *entJoints ) {
	idBounds	bounds;
	idDrawVert *verts = (idDrawVert *) _alloca16( texCoords.Num() * sizeof( idDrawVert ) );

	TransformVerts( verts, entJoints );

	SIMDProcessor->MinMax( bounds[0], bounds[1], verts, texCoords.Num() );

	return bounds;
}

/*
====================
idMD5Mesh::NearestJoint
====================
*/
int idMD5Mesh::NearestJoint( int a, int b, int c ) const {
	int i, bestJoint, vertNum, weightVertNum;
	float bestWeight;

	// duplicated vertices might not have weights
	if ( a >= 0 && a < texCoords.Num() ) {
		vertNum = a;
	} else if ( b >= 0 && b < texCoords.Num() ) {
		vertNum = b;
	} else if ( c >= 0 && c < texCoords.Num() ) {
		vertNum = c;
	} else {
		// all vertices are duplicates which shouldn't happen
		return 0;
	}

	// find the first weight for this vertex
	weightVertNum = 0;
	for( i = 0; weightVertNum < vertNum; i++ ) {
		weightVertNum += weightIndex[i*2+1];
	}

	// get the joint for the largest weight
	bestWeight = scaledWeights[i].w;
	bestJoint = weightIndex[i*2+0] / sizeof( idJointMat );
	for( ; weightIndex[i*2+1] == 0; i++ ) {
		if ( scaledWeights[i].w > bestWeight ) {
			bestWeight = scaledWeights[i].w;
			bestJoint = weightIndex[i*2+0] / sizeof( idJointMat );
		}
	}
	return bestJoint;
}

/*
====================
idMD5Mesh::NumVerts
====================
*/
int idMD5Mesh::NumVerts( void ) const {
	return texCoords.Num();
}

/*
====================
idMD5Mesh::NumTris
====================
*/
int	idMD5Mesh::NumTris( void ) const {
	return numTris;
}

/*
====================
idMD5Mesh::NumWeights
====================
*/
int	idMD5Mesh::NumWeights( void ) const {
	return numWeights;
}

/***********************************************************************

	idRenderModelMD5

***********************************************************************/

/*
====================
idRenderModelMD5::ParseJoint
====================
*/
void idRenderModelMD5::ParseJoint( idLexer &parser, idMD5Joint *joint, idJointQuat *defaultPose ) {
	idToken	token;
	int		num;

	//
	// parse name
	//
	parser.ReadToken( &token );
	joint->name = token;

	//
	// parse parent
	//
	num = parser.ParseInt();
	if ( num < 0 ) {
		joint->parent = NULL;
	} else {
		if ( num >= joints.Num() - 1 ) {
			parser.Error( "Invalid parent for joint '%s'", joint->name.c_str() );
		}
		joint->parent = &joints[ num ];
	}

	//
	// parse default pose
	//
	parser.Parse1DMatrix( 3, defaultPose->t.ToFloatPtr() );
	parser.Parse1DMatrix( 3, defaultPose->q.ToFloatPtr() );
	defaultPose->q.w = defaultPose->q.CalcW();
}

/*
====================
idRenderModelMD5::InitFromFile
====================
*/
void idRenderModelMD5::InitFromFile( const char *fileName ) {
	name = fileName;
	LoadModel();
}

/*
====================
idRenderModelMD5::LoadModel

used for initial loads, reloadModel, and reloading the data of purged models
Upon exit, the model will absolutely be valid, but possibly as a default model
====================
*/
void idRenderModelMD5::LoadModel() {
	int			version;
	int			i;
	int			num;
	int			parentNum;
	idToken		token;
	idLexer		parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS );
	idJointQuat	*pose;
	idMD5Joint	*joint;
	idJointMat *poseMat3;

	if ( !purged ) {
		PurgeModel();
	}
	purged = false;

	if ( !parser.LoadFile( name ) ) {
		MakeDefaultModel();
		return;
	}

	parser.ExpectTokenString( MD5_VERSION_STRING );
	version = parser.ParseInt();

	if ( version != MD5_VERSION ) {
		parser.Error( "Invalid version %d.  Should be version %d\n", version, MD5_VERSION );
	}

	//
	// skip commandline
	//
	parser.ExpectTokenString( "commandline" );
	parser.ReadToken( &token );

	#if MD5_BINARY_MESH > 1 // WRITE
	idFile* data_fd = (token.Icmpn("binary-export ", 14) ? NULL : fileSystem->OpenExplicitFileWrite(token.Right(token.Length() - 14)));
	#endif

	#if MD5_BINARY_MESH > 2 // WRITE+
	idFile* text_fd = NULL; const char* header = parser.GetPosition();
	if (data_fd) {
		#if MD5_BINARY_MESH > 3 // WRITE++
		idFile* save_fd = NULL;
		token.Replace(".md5data", ".md5save");
		if (save_fd = fileSystem->OpenExplicitFileWrite(token.Right(token.Length() - 14))) {
			save_fd->Write(parser.GetCommence(), parser.GetConclude() - parser.GetCommence()); fileSystem->CloseFile(save_fd); token.Replace(".md5save", ".md5data");
		}
		#endif
		token.Replace(".md5data", ".md5mesh");
		if (text_fd = fileSystem->OpenExplicitFileWrite(token.Right(token.Length() - 14))) {
			text_fd->Printf("MD5Version 10\ncommandline \"\"");
		}
	}
	#endif

	// parse num joints
	parser.ExpectTokenString( "numJoints" );
	num  = parser.ParseInt();
	joints.SetGranularity( 1 );
	joints.SetNum( num );
	defaultPose.SetGranularity( 1 );
	defaultPose.SetNum( num );
	poseMat3 = ( idJointMat * )_alloca16( num * sizeof( *poseMat3 ) );

	// parse num meshes
	parser.ExpectTokenString( "numMeshes" );
	num = parser.ParseInt();
	if ( num < 0 ) {
		parser.Error( "Invalid size: %d", num );
	}
	meshes.SetGranularity( 1 );
	meshes.SetNum( num );

	//
	// parse joints
	//
	parser.ExpectTokenString( "joints" );
	parser.ExpectTokenString( "{" );
	pose = defaultPose.Ptr();
	joint = joints.Ptr();
	for( i = 0; i < joints.Num(); i++, joint++, pose++ ) {
		ParseJoint( parser, joint, pose );
		poseMat3[ i ].SetRotation( pose->q.ToMat3() );
		poseMat3[ i ].SetTranslation( pose->t );
		if ( joint->parent ) {
			parentNum = joint->parent - joints.Ptr();
			pose->q = ( poseMat3[ i ].ToMat3() * poseMat3[ parentNum ].ToMat3().Transpose() ).ToQuat();
			pose->t = ( poseMat3[ i ].ToVec3() - poseMat3[ parentNum ].ToVec3() ) * poseMat3[ parentNum ].ToMat3().Transpose();
		}
	}
	parser.ExpectTokenString( "}" );

	#if MD5_BINARY_MESH > 2 // WRITE+
	if (text_fd) {text_fd->Write(header, parser.GetPosition() - header); text_fd->Printf("\n\n");}
	#endif

	#if MD5_ENABLE_LODS > 1 // DEBUG
	lodCount = 0;
	#endif

	#if MD5_BINARY_MESH > 0
	bool load_data = false;
	#endif

	for( i = 0; i < meshes.Num(); i++ ) {
		parser.ExpectTokenString( "mesh" );
		#if   MD5_BINARY_MESH > 2 // WRITE+
		load_data |= meshes[i].ParseMesh(parser, defaultPose.Num(), poseMat3, data_fd, text_fd);
		#elif MD5_BINARY_MESH > 1 // WRITE
		load_data |= meshes[i].ParseMesh(parser, defaultPose.Num(), poseMat3, data_fd         );
		#elif MD5_BINARY_MESH > 0
		load_data |= meshes[i].ParseMesh(parser, defaultPose.Num(), poseMat3                  );
		#else
		             meshes[i].ParseMesh(parser, defaultPose.Num(), poseMat3                  );
		#endif
		#if MD5_ENABLE_GIBS > 0
		if (meshes[i].gibZones) {
			gibParts |= meshes[i].gibZones;
			if (meshes[i].gibSpurt) {
				if (meshes[i].gibSpurt == 5) gibClass |= meshes[i].gibZones; else
				if (meshes[i].gibSpurt == 4) gibSpark |= meshes[i].gibZones; else
				if (meshes[i].gibSpurt == 3) gibFlame |= meshes[i].gibZones; else
				if (meshes[i].gibSpurt == 2) gibGloop |= meshes[i].gibZones; else
				if (meshes[i].gibSpurt == 1) gibBlood |= meshes[i].gibZones;
			}
		}
		#endif
		#if MD5_ENABLE_LODS > 1 // DEBUG
		if (meshes[i].lodLower == 0 && meshes[i].lodUpper != 0) lodCount++;
		#endif
	}

	#if MD5_BINARY_MESH > 2 // WRITE+
	if (text_fd) fileSystem->CloseFile(text_fd);
	#endif

	#if MD5_BINARY_MESH > 1 // WRITE
	if (data_fd) fileSystem->CloseFile(data_fd);
	#endif

	#if MD5_BINARY_MESH > 0
	if (load_data) {
		char  datafile[MAX_OSPATH]; strcpy(datafile, name);
		char* datatype = strstr(datafile, ".md5mesh"); // .MD5_MESH_EXT
		if (datatype) {
			datatype[4] = 'd';
			datatype[5] = 'a';
			datatype[6] = 't';
			datatype[7] = 'a';
			idFile* load_fd = fileSystem->OpenFileRead(datafile);
			if (load_fd) {
				for (i = 0; i < meshes.Num(); i++) {
					meshes[i].FetchData(load_fd);
				}
				fileSystem->CloseFile(load_fd);
			} else {
				common->Error("MD5 binary data not found; %s\n", datafile);
			}
		}
	}
	#endif

	//
	// calculate the bounds of the model
	//
	CalculateBounds( poseMat3 );

	// set the timestamp for reloadmodels
	fileSystem->ReadFile( name, NULL, &timeStamp );
}

/*
==============
idRenderModelMD5::Print
==============
*/
void idRenderModelMD5::Print() const {
	const idMD5Mesh	*mesh;
	int			i;

	common->Printf( "%s\n", name.c_str() );
	common->Printf( "Dynamic model.\n" );
	common->Printf( "Generated smooth normals.\n" );
	common->Printf( "    verts  tris weights material\n" );
	int	totalVerts = 0;
	int	totalTris = 0;
	int	totalWeights = 0;
	for( mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++ ) {
		totalVerts += mesh->NumVerts();
		totalTris += mesh->NumTris();
		totalWeights += mesh->NumWeights();
		common->Printf( "%2i: %5i %5i %7i %s\n", i, mesh->NumVerts(), mesh->NumTris(), mesh->NumWeights(), mesh->shader->GetName() );
	}
	common->Printf( "-----\n" );
	common->Printf( "%4i verts.\n", totalVerts );
	common->Printf( "%4i tris.\n", totalTris );
	common->Printf( "%4i weights.\n", totalWeights );
	common->Printf( "%4i joints.\n", joints.Num() );
}

/*
==============
idRenderModelMD5::List
==============
*/
void idRenderModelMD5::List() const {
	int			i;
	const idMD5Mesh	*mesh;
	int			totalTris = 0;
	int			totalVerts = 0;

	for( mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++ ) {
		totalTris += mesh->numTris;
		totalVerts += mesh->NumVerts();
	}
	common->Printf( " %4ik %3i %4i %4i %s(MD5)", Memory()/1024, meshes.Num(), totalVerts, totalTris, Name() );

	if ( defaulted ) {
		common->Printf( " (DEFAULTED)" );
	}

	common->Printf( "\n" );
}

/*
====================
idRenderModelMD5::CalculateBounds
====================
*/
void idRenderModelMD5::CalculateBounds( const idJointMat *entJoints ) {
	int			i;
	idMD5Mesh	*mesh;

	bounds.Clear();
	for( mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++ ) {
		bounds.AddBounds( mesh->CalcBounds( entJoints ) );
	}
}

/*
====================
idRenderModelMD5::Bounds

This calculates a rough bounds by using the joint radii without
transforming all the points
====================
*/
idBounds idRenderModelMD5::Bounds( const renderEntity_t *ent ) const {
#if 0
	// we can't calculate a rational bounds without an entity,
	// because joints could be positioned to deform it into an
	// arbitrarily large shape
	if ( !ent ) {
		common->Error( "idRenderModelMD5::Bounds: called without entity" );
	}
#endif

	if ( !ent ) {
		// this is the bounds for the reference pose
		return bounds;
	}

	return ent->bounds;
}

/*
====================
idRenderModelMD5::DrawJoints
====================
*/
void idRenderModelMD5::DrawJoints( const renderEntity_t *ent, const struct viewDef_s *view ) const {
	int					i;
	int					num;
	idVec3				pos;
	const idJointMat	*joint;
	const idMD5Joint	*md5Joint;
	int					parentNum;

	num = ent->numJoints;
	joint = ent->joints;
	md5Joint = joints.Ptr();
	for( i = 0; i < num; i++, joint++, md5Joint++ ) {
		pos = ent->origin + joint->ToVec3() * ent->axis;
		if ( md5Joint->parent ) {
			parentNum = md5Joint->parent - joints.Ptr();
			session->rw->DebugLine( colorWhite, ent->origin + ent->joints[ parentNum ].ToVec3() * ent->axis, pos );
		}

		session->rw->DebugLine( colorRed,	pos, pos + joint->ToMat3()[ 0 ] * 2.0f * ent->axis );
		session->rw->DebugLine( colorGreen,	pos, pos + joint->ToMat3()[ 1 ] * 2.0f * ent->axis );
		session->rw->DebugLine( colorBlue,	pos, pos + joint->ToMat3()[ 2 ] * 2.0f * ent->axis );
	}

	idBounds bounds;

	bounds.FromTransformedBounds( ent->bounds, vec3_zero, ent->axis );
	session->rw->DebugBounds( colorMagenta, bounds, ent->origin );

	if ( ( r_jointNameScale.GetFloat() != 0.0f ) && ( bounds.Expand( 128.0f ).ContainsPoint( view->renderView.vieworg - ent->origin ) ) ) {
		idVec3	offset( 0, 0, r_jointNameOffset.GetFloat() );
		float	scale;

		scale = r_jointNameScale.GetFloat();
		joint = ent->joints;
		num = ent->numJoints;
		for( i = 0; i < num; i++, joint++ ) {
			pos = ent->origin + joint->ToVec3() * ent->axis;
			session->rw->DrawText( joints[ i ].name, pos + offset, scale, colorWhite, view->renderView.viewaxis, 1 );
		}
	}
}

/*
====================
idRenderModelMD5::InstantiateDynamicModel
====================
*/
idRenderModel *idRenderModelMD5::InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel ) {

	int					i, surfaceNum;
	idMD5Mesh			*mesh;
	idRenderModelStatic	*staticModel;

	if (cachedModel && !r_useCachedDynamicModels.GetBool()) {
		delete cachedModel;
		cachedModel = NULL;
	}

	if ( purged ) {
		common->DWarning( "model %s instantiated while purged", Name() );
		LoadModel();
	}

	if ( !ent->joints ) {
		common->Printf( "idRenderModelMD5::InstantiateDynamicModel: NULL joints on renderEntity for '%s'\n", Name() );
		delete cachedModel;
		return NULL;
	} else if ( ent->numJoints != joints.Num() ) {
		common->Printf( "idRenderModelMD5::InstantiateDynamicModel: renderEntity has different number of joints than model for '%s'\n", Name() );
		delete cachedModel;
		return NULL;
	}

	tr.pc.c_generateMd5++;

	if ( cachedModel ) {
		assert( dynamic_cast<idRenderModelStatic *>(cachedModel) != NULL );
		assert( idStr::Icmp( cachedModel->Name(), MD5_SnapshotName ) == 0 );
		staticModel = static_cast<idRenderModelStatic *>(cachedModel);
	} else {
		staticModel = new idRenderModelStatic;
		staticModel->InitEmpty( MD5_SnapshotName );
	}

	staticModel->bounds.Clear();

	#if MD5_ENABLE_GIBS > 0
	staticModel->gibParts = gibParts;
//	staticModel->gibBlood = gibBlood;
//	staticModel->gibGloop = gibGloop;
//	staticModel->gibFlame = gibFlame;
//	staticModel->gibSpark = gibSpark;
//	staticModel->gibClass = gibClass;
	#endif

	if ( r_showSkel.GetInteger() ) {
		if ( ( view != NULL ) && ( !r_skipSuppress.GetBool() || !ent->suppressSurfaceInViewID || ( ent->suppressSurfaceInViewID != view->renderView.viewID ) ) ) {
			// only draw the skeleton
			DrawJoints( ent, view );
		}
		if ( r_showSkel.GetInteger() > 1 ) {
			// turn off the model when showing the skeleton
			staticModel->InitEmpty( MD5_SnapshotName );
			return staticModel;
		}
	}

	// create all the surfaces
	for( mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++ ) {
		// avoid deforming the surface if it will be a nodraw due to a skin remapping
		// FIXME: may have to still deform clipping hulls
		#if MD5_ENABLE_GIBS > 0
		const idMaterial* shader = R_RemapShaderBySkin(mesh->shader, ent->customSkin, ent->customShader);
		if /*el*/ (shader == NULL) {
			staticModel->DeleteSurfaceWithId(i); mesh->surfaceNum = -1; continue;
		} else if (mesh->gibZones) {
			if (gibParts == MD5_GIBBED_BITS && ent->gibbedZones < MD5_GIBBED_HEAD) { // We have a 'whole' mesh and no zones are gibbed (we may set gibbedZones|0x1 on death).
				if /*el*/ (mesh->gibZones != MD5_GIBBED_BITS || mesh->gibShown != MD5_GIBBED_HIDE) {
					staticModel->DeleteSurfaceWithId(i); mesh->surfaceNum = -1; continue;
				} else if (shader->IsDrawn() != true && shader->SurfaceCastsShadow() != true) {
					staticModel->DeleteSurfaceWithId(i); mesh->surfaceNum = -1; continue;
				}
			} else if (mesh->gibZones & ent->gibbedZones) { // A qualifying zone is gibbed.
				if /*el*/ (mesh->gibShown == MD5_GIBBED_HIDE || (mesh->gibShown & ent->gibbedZones) > 1) { // Always hide or ancestor gibbed (we may set gibbedZones|0x1 on death).
					staticModel->DeleteSurfaceWithId(i); mesh->surfaceNum = -1; continue;
				} else if (mesh->gibZones == MD5_GIBBED_BITS && shader->IsDrawn() != true && shader->SurfaceCastsShadow() != true) { // Suppress nodraw (to use the gib skeleton).
					staticModel->DeleteSurfaceWithId(i); mesh->surfaceNum = -1; continue;
				}
			} else { // No qualifying zones are gibbed.
				if /*el*/ (mesh->gibShown != MD5_GIBBED_HIDE) {
					staticModel->DeleteSurfaceWithId(i); mesh->surfaceNum = -1; continue;
				} else if (shader->IsDrawn() != true && shader->SurfaceCastsShadow() != true) {
					staticModel->DeleteSurfaceWithId(i); mesh->surfaceNum = -1; continue;
				}
			}
		} else if (shader->IsDrawn() != true && shader->SurfaceCastsShadow() != true) {
			staticModel->DeleteSurfaceWithId(i); mesh->surfaceNum = -1; continue;
		}
		#else
		const idMaterial *shader = mesh->shader;
		shader = R_RemapShaderBySkin(shader, ent->customSkin, ent->customShader);
		if (!shader || (!shader->IsDrawn() && !shader->SurfaceCastsShadow())) {
			staticModel->DeleteSurfaceWithId(i);
			mesh->surfaceNum = -1;
			continue;
		}
		#endif
		#if MD5_ENABLE_LODS > 0
		if (mesh->lodUpper > 0.00f) {
			float lodRange = (view ? (ent->origin - view->renderView.vieworg).LengthSqr() : 0.00f);
			#if MD5_ENABLE_LODS > 1 // DEBUG
			float lodSquare = r_lodRangeIncrements.GetFloat() * r_lodRangeIncrements.GetFloat();
			lodRange = fminf(lodRange, lodSquare * r_lodLevelMaximum.GetFloat() * r_lodLevelMaximum.GetFloat());
			lodRange = fmaxf(lodRange, lodSquare * r_lodLevelMinimum.GetFloat() * r_lodLevelMinimum.GetFloat());
			#endif
			if (lodRange < mesh->lodLower || lodRange >= mesh->lodUpper) {
				staticModel->DeleteSurfaceWithId(i);
				mesh->surfaceNum = -1;
				continue;
			}
			#if MD5_ENABLE_LODS > 1 // DEBUG
			if (ent->hModel->lodFrame < idLib::frameNumber) {
				ent->hModel->lodFrame = idLib::frameNumber;
				ent->hModel->lodIndex = ent->entityNum;
				ent->hModel->lodCalls = 1;
				ent->hModel->lodFaces = mesh->numTris;
				ent->hModel->lodLevel = int(sqrtf(lodRange) / r_lodRangeIncrements.GetFloat());
				ent->hModel->lodRange = lodRange;
			} else if (ent->hModel->lodIndex == ent->entityNum && ent->hModel->lodCalls < ent->hModel->lodCount) {
				ent->hModel->lodCalls = ent->hModel->lodCalls + 1;
				ent->hModel->lodFaces = ent->hModel->lodFaces + mesh->numTris;
				ent->hModel->lodLevel = int(sqrtf(lodRange) / r_lodRangeIncrements.GetFloat());
				ent->hModel->lodRange = lodRange;
			}
		#endif
		#if MD5_ENABLE_LODS > 1 && MD5_ENABLE_GIBS > 2 // DEBUG
		} else if (mesh->gibZones) {
			if (ent->hModel->lodFrame < idLib::frameNumber) {
				ent->hModel->lodFrame = idLib::frameNumber;
				ent->hModel->lodIndex = ent->entityNum;
				ent->hModel->lodCalls = 1;
				ent->hModel->lodFaces = mesh->numTris;
				ent->hModel->lodLevel = mesh->gibZones & (ent->gibbedZones ? ent->gibbedZones : MD5_GIBBED_BITS);
				ent->hModel->lodRange = 0;
			} else if (ent->hModel->lodIndex == ent->entityNum) {
				ent->hModel->lodCalls = ent->hModel->lodCalls + 1;
				ent->hModel->lodFaces = ent->hModel->lodFaces + (mesh->numTris);
				ent->hModel->lodLevel = ent->hModel->lodLevel | (mesh->gibZones & (ent->gibbedZones ? ent->gibbedZones : MD5_GIBBED_BITS));
				ent->hModel->lodRange = 0;
			}
		#endif
		}
		#endif

		modelSurface_t *surf;

		if ( staticModel->FindSurfaceWithId( i, surfaceNum ) ) {
			mesh->surfaceNum = surfaceNum;
			surf = &staticModel->surfaces[surfaceNum];
		} else {
			// Remove Overlays before adding new surfaces
			idRenderModelOverlay::RemoveOverlaySurfacesFromModel( staticModel );
			mesh->surfaceNum = staticModel->NumSurfaces();
			surf = &staticModel->surfaces.Alloc();
			surf->geometry = NULL;
			surf->shader = NULL;
			surf->id = i;
		}

		mesh->UpdateSurface( ent, ent->joints, surf );

		staticModel->bounds.AddPoint( surf->geometry->bounds[0] );
		staticModel->bounds.AddPoint( surf->geometry->bounds[1] );
	}

	return staticModel;
}

/*
====================
idRenderModelMD5::IsDynamicModel
====================
*/
dynamicModel_t idRenderModelMD5::IsDynamicModel() const {
	return DM_CACHED;
}

/*
====================
idRenderModelMD5::NumJoints
====================
*/
int idRenderModelMD5::NumJoints( void ) const {
	return joints.Num();
}

/*
====================
idRenderModelMD5::GetJoints
====================
*/
const idMD5Joint *idRenderModelMD5::GetJoints( void ) const {
	return joints.Ptr();
}

/*
====================
idRenderModelMD5::GetDefaultPose
====================
*/
const idJointQuat *idRenderModelMD5::GetDefaultPose( void ) const {
	return defaultPose.Ptr();
}

/*
====================
idRenderModelMD5::GetJointHandle
====================
*/
jointHandle_t idRenderModelMD5::GetJointHandle( const char *name ) const {
	const idMD5Joint *joint;
	int	i;

	joint = joints.Ptr();
	for( i = 0; i < joints.Num(); i++, joint++ ) {
		if ( idStr::Icmp( joint->name.c_str(), name ) == 0 ) {
			return ( jointHandle_t )i;
		}
	}

	return INVALID_JOINT;
}

/*
=====================
idRenderModelMD5::GetJointName
=====================
*/
const char *idRenderModelMD5::GetJointName( jointHandle_t handle ) const {
	if ( ( handle < 0 ) || ( handle >= joints.Num() ) ) {
		return "<invalid joint>";
	}

	return joints[ handle ].name;
}

/*
====================
idRenderModelMD5::NearestJoint
====================
*/
int idRenderModelMD5::NearestJoint( int surfaceNum, int a, int b, int c ) const {
	int i;
	const idMD5Mesh *mesh;

	if ( surfaceNum > meshes.Num() ) {
		common->Error( "idRenderModelMD5::NearestJoint: surfaceNum > meshes.Num()" );
	}

	for ( mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++ ) {
		if ( mesh->surfaceNum == surfaceNum ) {
			return mesh->NearestJoint( a, b, c );
		}
	}
	return 0;
}

/*
====================
idRenderModelMD5::TouchData

models that are already loaded at level start time
will still touch their materials to make sure they
are kept loaded
====================
*/
void idRenderModelMD5::TouchData() {
	idMD5Mesh	*mesh;
	int			i;

	for( mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++ ) {
		declManager->FindMaterial( mesh->shader->GetName() );
	}
}

/*
===================
idRenderModelMD5::PurgeModel

frees all the data, but leaves the class around for dangling references,
which can regenerate the data with LoadModel()
===================
*/
void idRenderModelMD5::PurgeModel() {
	purged = true;
	joints.Clear();
	defaultPose.Clear();
	meshes.Clear();
	#if MD5_ENABLE_GIBS > 0
	gibParts = 0;
	gibBlood = 0;
	gibGloop = 0;
	gibFlame = 0;
	gibSpark = 0;
	gibClass = 0;
	#endif
	#if MD5_ENABLE_LODS > 1 // DEBUG
	lodFrame = 0;
	lodIndex = 0;
	lodCount = 0;
	lodCalls = 0;
	lodFaces = 0;
	lodLevel = 0;
	lodRange = 0.00f;
	#endif
}

/*
===================
idRenderModelMD5::Memory
===================
*/
int	idRenderModelMD5::Memory() const {
	int		total, i;

	total = sizeof( *this );
	total += joints.MemoryUsed() + defaultPose.MemoryUsed() + meshes.MemoryUsed();

	// count up strings
	for ( i = 0; i < joints.Num(); i++ ) {
		total += joints[i].name.DynamicMemoryUsed();
	}

	// count up meshes
	for ( i = 0 ; i < meshes.Num() ; i++ ) {
		const idMD5Mesh *mesh = &meshes[i];

		total += mesh->texCoords.MemoryUsed() + mesh->numWeights * ( sizeof( mesh->scaledWeights[0] ) + sizeof( mesh->weightIndex[0] ) * 2 );

		// sum up deform info
		total += sizeof( mesh->deformInfo );
		total += R_DeformInfoMemoryUsed( mesh->deformInfo );
	}
	return total;
}
