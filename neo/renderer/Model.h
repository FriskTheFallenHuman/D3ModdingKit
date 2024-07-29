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

#ifndef __MODEL_H__
#define __MODEL_H__

/*
===============================================================================

	Render Model

===============================================================================
*/

// shared between the renderer, game, and Maya export DLL
#define MD5_VERSION_STRING		"MD5Version"
#define MD5_MESH_EXT			"md5mesh"
#define MD5_ANIM_EXT			"md5anim"
#define MD5_CAMERA_EXT			"md5camera"
#define MD5_VERSION				10

#define MD5_ENABLE_LODS			3 // 0=Disable, 1=Enable, 2=1+ai_showLevelOfDetail, 3=2+r_testUnsmoothedTangents
#define MD5_ENABLE_GIBS			3 // 0=Disable, 1=Enable, 2=Enable+NoDraw 3=Enable+NoDraw+Debug
#define MD5_GIBBED_ZERO			0x0000
#define MD5_GIBBED_HEAD			0x0002
#define MD5_GIBBED_BODY			0x0004
#define MD5_GIBBED_CORE			0x0006 // MD5_GIBBED_BODY | MD5_GIBBED_HEAD
#define MD5_GIBBED_BITS			0x1FFE
#define MD5_GIBBED_HIDE			0x0000 // For gibShown (always hide when gibbed; 'HIDE').
#define MD5_GIBBED_SHOW			0x0001 // For gibShown (always show when gibbed; 'SHOW'). Other bits are dependencies (show only if those are NOT gibbed; 'STUB').
#define MD5_GIBFX_BLOOD			0x2000 // Used as a divisor also (to get effect ordinal 0-7, instead of >> 14).
#define MD5_GIBFX_GLOOP			0x4000
#define MD5_GIBFX_FLAME			0x6000
#define MD5_GIBFX_SPARK			0x8000
#define MD5_GIBFX_CLASS			0xA000
#define MD5_GIBFX_INDEX			0xE000 // Used for masking.
#define MD5_IS_FALLBACK			0x2000 // Permit animation ONLY if no other is allowed.
#define MD5_OR_FALLBACK			0x4000 // Permit animation if no other is allowed.
#define MD5_OR_HEADLESS			0x8000 // Permit animation if headless.
#define MD5_BINARY_MESH			4 // 0=Disable, 1=Enable, 2=1+binaryExport, 3=1+binaryExport+Text, 4=1+binaryExport+Text+Save
#define MD5_BINARY_ANIM			1 // 0=Disable, 1=Enable, 2=Enable-bigEndian
#define IMG_ENABLE_PNGS			1 // 0=Disable, 1=Enable

/* ===================================================================================================
MD5_ENABLE_GIBS: TODO
default				  1
cyberdemon	1.00	150
sabaoth				 50
vagary		2.00
archvile	2.00
cherub		1.25
bruiser		2.00
hellknight	2.00	 25
imp			2.00
imp_crawler	1.25
maggot		1.50
mancubus	1.50	 16
pinky		1.00
revenant	2.00
sentry		1.25
tick		1.00
trite		1.00
vulgar		2.00
wraith		2.00
bernie		2.00
boney		2.00
chainsaw	2.00
commando	2.00
commando_cg	2.00
fatty		2.00
zombie		2.00
jumpsuit	2.00
maintenance	2.00
morgue		1.25
sawyer		2.00
zsec_pistol	1.25
zsec_shield	1.25
?			1.25
------------------------------------------------------------------------------------------------------
The ai_testDismemberment implementation is not pretty (passing the mesh/face count out via properties
added to idRenderModel) but is sufficient for development/testing. Set MD5_ENABLE_GIBS=2 for releases.
=================================================================================================== */

/* ===================================================================================================
MD5_ENABLE_LODS: A simple level-of-detail mechanism based on decorating MD5 mesh names as shown below;
	models/characters/male_npc/marine/marine/lod_0_2
	models/characters/male_npc/marine/marine/lod_2_5
	models/characters/male_npc/marine/marine/lod_5_9
------------------------------------------------------------------------------------------------------
Multiple meshes should cover steps 0-9 which by default (per r_lodRangeIncrements) extend out 90 feet.
Once parsed the decoration is stripped from the name with the remainder becoming the effective shader.
UPDATED: Range steps may now encompass 0-Z (use uppercase) yielding 0-35 steps * r_lodRangeIncrements.
------------------------------------------------------------------------------------------------------
Content may be packaged such that only the first LOD is shown on systems without this patch by either;
a) Including a materials file which defines a shader for the first (decorated) LOD then all others as;
	nonsolid
	noshadows
b) Including a skin file aliasing the first (decorated) LOD to some existing shader and all others to;
	"textures/common/nodraw"
------------------------------------------------------------------------------------------------------
The ai_showLevelOfDetail implementation is not pretty (passing the mesh/face count out via properties
added to idRenderModel) but is sufficient for development/testing. Set MD5_ENABLE_LODS=1 for releases.
=================================================================================================== */

/* ===================================================================================================
MD5_BINARY_MESH: A mechanism to read binarised mesh data from an md5data file adjacent to the md5mesh.
In which case vert/tri/weight data is removed from the md5mesh leaving a simplified mesh block like;
mesh {
	shader "models/characters/male_npc/marine/marine/lod_0_2"
	numverts 2362
	numtris 4158
	numweights 6439
}
It should be possible to mix conventional and binarised mesh blocks in the same md5mesh file. The
order of the binarised blocks is significant when the binary data is read back (must be retained).
MD5_BINARY_MESH > 0 Enable reading of a binarised MD5 model.
MD5_BINARY_MESH > 1 Enable writing of a binarised MD5 model. If the 'commandline' key/pair value
					starts with "binary-export" the remainder is expected to specify a DOS path to a
					target file for the binarised mesh data. For example;
					commandline "binary-export E:\DOOM3\OUT\pak452\models\md5\chars\marine.md5data"
MD5_BINARY_MESH > 2 As above but will also write the simplified md5mesh file alongside the data file.
MD5_BINARY_MESH > 3 As above but will also write the original md5mesh with an 'md5save' extension.
=================================================================================================== */

/* ===================================================================================================
MD5_BINARY_ANIM: Adds support for loading binarised md5anim files. If the load fails an attempt will
then be made to parse the original text format.
_MD5_BINARY_ANIM_ > 0 Load binarised MD5 animations in the BFG layout with Little Endian values only.
_MD5_BINARY_ANIM_ > 1 Load binarised MD5 animations in the BFG layout with mixed Endianness (per-BFG).
=================================================================================================== */

/* ===================================================================================================
IMG_ENABLE_PNGS: Quick hack to make PNG the default texture format with TGA or JPG as fallbacks. There
are reasonable arguments as to why this is a bad idea for releases but it suits my development needs.
Dhewm3 Issue https://github.com/dhewm/dhewm3/issues/171
=================================================================================================== */

/* ===================================================================================================
RBMIKKT_TANGENT: My attempt to port the support for mikkt tangent-space from RB-DOOM3-BFG. As far as I
can see this should be working but comparisons made with bakes provided by Arl suggest some divergence
from RenderBump - though that may just be down to differences in the baking - no further testing done.
Enabled by including 'mikktspace' in the material definition.
idTech4 Discord https://discord.com/channels/488393111014342656/488393514690805790/1053987460452982865
=================================================================================================== */

// using shorts for triangle indexes can save a significant amount of traffic, but
// to support the large models that renderBump loads, they need to be 32 bits
#if 1

#define GL_INDEX_TYPE		GL_UNSIGNED_INT
typedef int glIndex_t;

#else

#define GL_INDEX_TYPE		GL_UNSIGNED_SHORT
typedef short glIndex_t;

#endif


typedef struct {
	// NOTE: making this a glIndex is dubious, as there can be 2x the faces as verts
	glIndex_t					p1, p2;					// planes defining the edge
	glIndex_t					v1, v2;					// verts defining the edge
} silEdge_t;

// this is used for calculating unsmoothed normals and tangents for deformed models
typedef struct dominantTri_s {
	glIndex_t					v2, v3;
	float						normalizationScale[3];
} dominantTri_t;

typedef struct lightingCache_s {
	idVec3						localLightVector;		// this is the statically computed vector to the light
														// in texture space for cards without vertex programs
} lightingCache_t;

typedef struct shadowCache_s {
	idVec4						xyz;					// we use homogenous coordinate tricks
} shadowCache_t;

const int SHADOW_CAP_INFINITE	= 64;

// our only drawing geometry type
typedef struct srfTriangles_s {
	idBounds					bounds;					// for culling

	int							ambientViewCount;		// if == tr.viewCount, it is visible this view

	bool						generateNormals;		// create normals from geometry, instead of using explicit ones
	bool						tangentsCalculated;		// set when the vertex tangents have been calculated
	bool						facePlanesCalculated;	// set when the face planes have been calculated
	bool						perfectHull;			// true if there aren't any dangling edges
	bool						deformedSurface;		// if true, indexes, silIndexes, mirrorVerts, and silEdges are
														// pointers into the original surface, and should not be freed

	int							numVerts;				// number of vertices
	idDrawVert *				verts;					// vertices, allocated with special allocator

	int							numIndexes;				// for shadows, this has both front and rear end caps and silhouette planes
	glIndex_t *					indexes;				// indexes, allocated with special allocator

	glIndex_t *					silIndexes;				// indexes changed to be the first vertex with same XYZ, ignoring normal and texcoords

	int							numMirroredVerts;		// this many verts at the end of the vert list are tangent mirrors
	int *						mirroredVerts;			// tri->mirroredVerts[0] is the mirror of tri->numVerts - tri->numMirroredVerts + 0

	int							numDupVerts;			// number of duplicate vertexes
	int *						dupVerts;				// pairs of the number of the first vertex and the number of the duplicate vertex

	int							numSilEdges;			// number of silhouette edges
	silEdge_t *					silEdges;				// silhouette edges

	idPlane *					facePlanes;				// [numIndexes/3] plane equations

	dominantTri_t *				dominantTris;			// [numVerts] for deformed surface fast tangent calculation

	int							numShadowIndexesNoFrontCaps;	// shadow volumes with front caps omitted
	int							numShadowIndexesNoCaps;			// shadow volumes with the front and rear caps omitted

	int							shadowCapPlaneBits;		// bits 0-5 are set when that plane of the interacting light has triangles
														// projected on it, which means that if the view is on the outside of that
														// plane, we need to draw the rear caps of the shadow volume
														// turboShadows will have SHADOW_CAP_INFINITE

	shadowCache_t *				shadowVertexes;			// these will be copied to shadowCache when it is going to be drawn.
														// these are NULL when vertex programs are available

	struct srfTriangles_s *		ambientSurface;			// for light interactions, point back at the original surface that generated
														// the interaction, which we will get the ambientCache from

	struct srfTriangles_s *		nextDeferredFree;		// chain of tris to free next frame

	// data in vertex object space, not directly readable by the CPU
	struct vertCache_s *		indexCache;				// int
	struct vertCache_s *		ambientCache;			// idDrawVert
	struct vertCache_s *		lightingCache;			// lightingCache_t
	struct vertCache_s *		shadowCache;			// shadowCache_t
} srfTriangles_t;

typedef idList<srfTriangles_t *> idTriList;

typedef struct modelSurface_s {
	int							id;
	const idMaterial *			shader;
	srfTriangles_t *			geometry;
} modelSurface_t;

typedef enum {
	DM_STATIC,		// never creates a dynamic model
	DM_CACHED,		// once created, stays constant until the entity is updated (animating characters)
	DM_CONTINUOUS	// must be recreated for every single view (time dependent things like particles)
} dynamicModel_t;

typedef enum {
	INVALID_JOINT				= -1
} jointHandle_t;

class idMD5Joint {
public:
								idMD5Joint() { parent = NULL; }
	idStr						name;
	const idMD5Joint *			parent;
};


// the init methods may be called again on an already created model when
// a reloadModels is issued

class idRenderModel {
public:
	virtual						~idRenderModel() {};

	// Loads static models only, dynamic models must be loaded by the modelManager
	virtual void				InitFromFile( const char *fileName ) = 0;

	// renderBump uses this to load the very high poly count models, skipping the
	// shadow and tangent generation, along with some surface cleanup to make it load faster
	virtual void				PartialInitFromFile( const char *fileName ) = 0;

	// this is used for dynamically created surfaces, which are assumed to not be reloadable.
	// It can be called again to clear out the surfaces of a dynamic model for regeneration.
	virtual void				InitEmpty( const char *name ) = 0;

	// dynamic model instantiations will be created with this
	// the geometry data will be owned by the model, and freed when it is freed
	// the geoemtry should be raw triangles, with no extra processing
	virtual void				AddSurface( modelSurface_t surface ) = 0;

	// cleans all the geometry and performs cross-surface processing
	// like shadow hulls
	// Creates the duplicated back side geometry for two sided, alpha tested, lit materials
	// This does not need to be called if none of the surfaces added with AddSurface require
	// light interaction, and all the triangles are already well formed.
	virtual void				FinishSurfaces() = 0;

	// frees all the data, but leaves the class around for dangling references,
	// which can regenerate the data with LoadModel()
	virtual void				PurgeModel() = 0;

	// resets any model information that needs to be reset on a same level load etc..
	// currently only implemented for liquids
	virtual void				Reset() = 0;

	// used for initial loads, reloadModel, and reloading the data of purged models
	// Upon exit, the model will absolutely be valid, but possibly as a default model
	virtual void				LoadModel() = 0;

	// internal use
	virtual bool				IsLoaded() const = 0;
	virtual void				SetLevelLoadReferenced( bool referenced ) = 0;
	virtual bool				IsLevelLoadReferenced() = 0;

	// models that are already loaded at level start time
	// will still touch their data to make sure they
	// are kept loaded
	virtual void				TouchData() = 0;

	// dump any ambient caches on the model surfaces
	virtual void				FreeVertexCache() = 0;

	// returns the name of the model
	virtual const char	*		Name() const = 0;

	// prints a detailed report on the model for printModel
	virtual void				Print() const = 0;

	// prints a single line report for listModels
	virtual void				List() const = 0;

	// reports the amount of memory (roughly) consumed by the model
	virtual int					Memory() const = 0;

	// for reloadModels
	virtual ID_TIME_T			Timestamp() const = 0;

	// returns the number of surfaces
	virtual int					NumSurfaces() const = 0;

	// NumBaseSurfaces will not count any overlays added to dynamic models
	virtual int					NumBaseSurfaces() const = 0;

	// get a pointer to a surface
	virtual const modelSurface_t *Surface( int surfaceNum ) const = 0;

	// Allocates surface triangles.
	// Allocates memory for srfTriangles_t::verts and srfTriangles_t::indexes
	// The allocated memory is not initialized.
	// srfTriangles_t::numVerts and srfTriangles_t::numIndexes are set to zero.
	virtual srfTriangles_t *	AllocSurfaceTriangles( int numVerts, int numIndexes ) const = 0;

	// Frees surfaces triangles.
	virtual void				FreeSurfaceTriangles( srfTriangles_t *tris ) const = 0;

	// created at load time by stitching together all surfaces and sharing
	// the maximum number of edges.  This may be incorrect if a skin file
	// remaps surfaces between shadow casting and non-shadow casting, or
	// if some surfaces are noSelfShadow and others aren't
	virtual srfTriangles_t	*	ShadowHull() const = 0;

	// models of the form "_area*" may have a prelight shadow model associated with it
	virtual bool				IsStaticWorldModel() const = 0;

	// models parsed from inside map files or dynamically created cannot be reloaded by
	// reloadmodels
	virtual bool				IsReloadable() const = 0;

	// md3, md5, particles, etc
	virtual dynamicModel_t		IsDynamicModel() const = 0;

	// if the load failed for any reason, this will return true
	virtual bool				IsDefaultModel() const = 0;

	// dynamic models should return a fast, conservative approximation
	// static models should usually return the exact value
	virtual idBounds			Bounds( const struct renderEntity_s *ent = NULL ) const = 0;

	// returns value != 0.0f if the model requires the depth hack
	virtual float				DepthHack() const = 0;

	// returns a static model based on the definition and view
	// currently, this will be regenerated for every view, even though
	// some models, like character meshes, could be used for multiple (mirror)
	// views in a frame, or may stay static for multiple frames (corpses)
	// The renderer will delete the returned dynamic model the next view
	// This isn't const, because it may need to reload a purged model if it
	// wasn't precached correctly.
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel ) = 0;

	// Returns the number of joints or 0 if the model is not an MD5
	virtual int					NumJoints( void ) const = 0;

	// Returns the MD5 joints or NULL if the model is not an MD5
	virtual const idMD5Joint *	GetJoints( void ) const = 0;

	// Returns the handle for the joint with the given name.
	virtual jointHandle_t		GetJointHandle( const char *name ) const = 0;

	// Returns the name for the joint with the given handle.
	virtual const char *		GetJointName( jointHandle_t handle ) const = 0;

	// Returns the default animation pose or NULL if the model is not an MD5.
	virtual const idJointQuat *	GetDefaultPose( void ) const = 0;

	// Returns number of the joint nearest to the given triangle.
	virtual int					NearestJoint( int surfaceNum, int a, int c, int b ) const = 0;

	// Writing to and reading from a demo file.
	virtual void				ReadFromDemoFile( class idDemoFile *f ) = 0;
	virtual void				WriteToDemoFile( class idDemoFile *f ) = 0;

	#if MD5_ENABLE_GIBS > 0
	int   gibParts = 0;
	int   gibBlood = 0;
	int   gibGloop = 0;
	int   gibFlame = 0;
	int   gibSpark = 0;
	int   gibClass = 0;
	#endif

	#if MD5_ENABLE_LODS > 1 // DEBUG
	int   lodFrame = 0;
	int   lodIndex = 0;
	int   lodCount = 0;
	int   lodCalls = 0;
	int   lodFaces = 0;
	int   lodLevel = 0;
	float lodRange = 0;
	#endif

};

#endif /* !__MODEL_H__ */
