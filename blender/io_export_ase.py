## ***** BEGIN GPL LICENSE BLOCK ***** 
# 
# This program is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License 
# as published by the Free Software Foundation; either version 2 
# of the License, or (at your option) any later version. 
# 
# This program is distributed in the hope that it will be useful, 
# but WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
# GNU General Public License for more details. 
# 
# You should have received a copy of the GNU General Public License 
# along with this program; if not, write to the Free Software Foundation, 
# Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. 
# 
# ***** END GPL LICENCE BLOCK *****
# 
#
# REQUIRED OPTIONS -
# - Make Normals Consistent
# - Remove Doubles
# **********************************
bl_info = {
    "name": "ASCII Scene Exporter",
    "author": "Richard Bartlett, MCampagnini, The Dark Mod team",
    "version": ( 2, 8, 0 ),
    "blender": ( 2, 80, 0 ),
    "api": 36079,
    "location": "File > Export > ASCII Scene Export(.ase)",
    "description": "ASCII Scene Export(.ase)",
    "warning": "",
    "category": "Import-Export"
}

"""
--  This script is intended to export in the ASE file format for STATIC MESHES ONLY.
--  This script WILL NOT export skeletal meshes, these should be exported using a
--  different file format.

--  More Information at http://code.google.com/p/ase-export-vmc/
--  UDK Thread at http://forums.epicgames.com/threads/776986-Blender-2-57-ASE-export
"""

# rewritten code dealing with UVs to only store unique vertices
# changed vertex normal export to store vertex-per-face normals instead
# changed bitmap assignment to be based on material name
# added options to separate by material / apply modifiers on export
# tweaked collision mesh detection criteria
# added a missing brace to enclose submaterials
# reformatted strings and cleaned up whitespace spam
# broken something, probably
# - chedap, V.2018

import os
import bpy
import math
import time

# settings
aseFloat = lambda x: '''{0:0.4f}'''.format( x )
optionScale = 16.0
optionSubmaterials = False
optionSmoothingGroups = True
optionAllowMultiMats = True

# ASE components
aseHeader = ''
aseScene = ''
aseMaterials = ''
aseGeometry = ''

# Other
matList = []
numMats = 0
currentMatId = 0

#== Error ==================================================================
class Error( Exception ):

    def __init__( self, message ):
        self.message = message
        print( '\n\n' + message + '\n\n' )

#== Header =================================================================
class cHeader:
    def __init__( self ):
        self.comment = "Ascii Scene Exporter v2.52"

    def __repr__( self ):
        return '''*3DSMAX_ASCIIEXPORT\t200\n*COMMENT "{0}"\n'''.format( self.comment )

#== Scene ==================================================================
class cScene:
    def __init__( self ):
        self.filename = bpy.path.basename(bpy.data.filepath)
        self.firstframe = 0
        self.lastframe = 100
        self.framespeed = 30
        self.ticksperframe = 160
        self.backgroundstatic = '\t'.join( [aseFloat( x ) for x in [0.0, 0.0, 0.0]] )
        self.ambientstatic = '\t'.join( [aseFloat( x ) for x in [0.0, 0.0, 0.0]] )

    def __repr__( self ):
        return ("*SCENE {{\n\t*SCENE_FILENAME \"{0}\""+
               "\n\t*SCENE_FIRSTFRAME {1}"+
               "\n\t*SCENE_LASTFRAME {2}"+
               "\n\t*SCENE_FRAMESPEED {3}"+
               "\n\t*SCENE_TICKSPERFRAME {4}"+
               "\n\t*SCENE_BACKGROUND_STATIC {5}"+
               "\n\t*SCENE_AMBIENT_STATIC {6}"+
               "\n}}\n").format( self.filename, self.firstframe, self.lastframe, self.framespeed, self.ticksperframe, self.backgroundstatic, self.ambientstatic )

#== Materials ==============================================================
class cMaterials:
    def __init__( self, objects ):
        global optionSubmaterials
        global matList
        global numMats

        self.material_list = []

        # Get all of the materials used by non-collision object meshes  
        for object in objects:
            if collisionObject( object ) == 2:
                continue
            elif object.type != 'MESH':
                continue
            else:
                print( object.name + ': Constructing Materials' )
                for slot in object.material_slots:
                    # if the material is not in the material_list, add it
                    if self.material_list.count( slot.material ) == 0:
                        self.material_list.append( slot.material )
                        matList.append( slot.material.name )

        self.material_count = len( self.material_list )
        numMats = self.material_count

        # Raise an error if there are no materials found
        if self.material_count == 0:
            raise Error( 'Mesh must have at least one applied material' )
        else:
            if ( optionSubmaterials ):
                self.dump = cSubMaterials( self.material_list )
            else:
                self.dump = cMultiMaterials( self.material_list )

    def __repr__( self ):
        return str( self.dump )
class cMultiMaterials:
    def __init__( self, material_list ):
        self.numMtls = len( material_list )
        # Initialize material information
        self.dump = ("*MATERIAL_LIST {{"+
                    "\n\t*MATERIAL_COUNT {0}").format( str( self.numMtls ) )

        for index, slot in enumerate( material_list ):
            self.dump += ("\n\t*MATERIAL {0} {{"+
                         "{1}\n\t}}").format( index, cMaterial( slot ) )

        self.dump += '\n}'

    def __repr__( self ):
        return self.dump
class cSubMaterials:
    def __init__( self, material_list ):
        slot = material_list[0]
        # Initialize material information
        self.dump = ("*MATERIAL_LIST {"+
                    "\n\t*MATERIAL_COUNT 1"+
                    "\n\t*MATERIAL 0 {")
        self.matDump = ''
        self.name = material_list[0].name
        self.numSubMtls = len( material_list )
        self.diffusemap = cDiffusemap( slot )
        if ( self.numSubMtls > 1 ):
            self.matClass = 'Multi/Sub-Object'
            self.diffuseDump = ''
        else:
            self.matClass = 'Standard'
            self.numSubMtls = 0
            self.diffuseDump = self.diffdump()
        self.ambient = '\t'.join( [aseFloat( x ) for x in [0.0, 0.0, 0.0]] )
        self.diffuse = '\t'.join( [aseFloat( x ) for x in slot.diffuse_color] )
        self.specular = '\t'.join( [aseFloat( x ) for x in slot.specular_color] )
        self.shine = aseFloat( 1.0 / slot.roughness )
        self.shinestrength = aseFloat( slot.specular_intensity )
        self.transparency = aseFloat( 0.0 )
        self.wiresize = aseFloat( 1.0 )
        self.shading = 'Blinn'
        self.xpfalloff = aseFloat( 0.0 )
        self.xptype = 'Filter'
        self.falloff = 'In'
        self.soften = False
        self.submtls = []
        self.selfillum = aseFloat( 0.0 )

        if ( len( material_list ) > 1 ):
            # Build SubMaterials
            for index, slot in enumerate( material_list ):
                self.matDump += ("\n\t\t*SUBMATERIAL {0} {{"+
                                "{1}"+
                                "\n\t\t}}").format( index, cMaterial( slot ) )

        self.dump += ("\n\t\t*MATERIAL_NAME \"{0}\""+
                     "\n\t\t*MATERIAL_CLASS \"{1}\""+
                     "\n\t\t*MATERIAL_AMBIENT {2}"+
                     "\n\t\t*MATERIAL_DIFFUSE {3}"+
                     "\n\t\t*MATERIAL_SPECULAR {4}"+
                     "\n\t\t*MATERIAL_SHINE {5}"+
                     "\n\t\t*MATERIAL_SHINESTRENGTH {6}"+
                     "\n\t\t*MATERIAL_TRANSPARENCY {7}"+
                     "\n\t\t*MATERIAL_WIRESIZE {8}"+
                     "\n\t\t*MATERIAL_SHADING {9}"+
                     "\n\t\t*MATERIAL_XP_FALLOFF {10}"+
                     "\n\t\t*MATERIAL_SELFILLUM {11}"+
                     "\n\t\t*MATERIAL_FALLOFF {12}"+
                     "\n\t\t*MATERIAL_XP_TYPE {13}"+
                     "{14}"+
                     "\n\t\t*NUMSUBMTLS {15}"+
                     "{16}").format( self.name, self.matClass, self.ambient, self.diffuse, self.specular, self.shine, self.shinestrength, self.transparency, self.wiresize, self.shading, self.xpfalloff, self.selfillum, self.falloff, self.xptype, self.diffuseDump, self.numSubMtls, self.matDump )

        self.dump += '\n\t}'
        self.dump += '\n}'

    def diffdump( self ):
        for x in [self.diffusemap]:
            return x

    def __repr__( self ):
        return self.dump
class cMaterial:
    def __init__( self, slot ):
        self.dump = ''
        self.name = slot.name
        self.matClass = 'Standard'
        self.ambient = '\t'.join( [aseFloat( x ) for x in [0.0, 0.0, 0.0]] )
        self.diffuse = '\t'.join( [aseFloat( x ) for x in slot.diffuse_color] )
        self.specular = '\t'.join( [aseFloat( x ) for x in slot.specular_color] )
        self.shine = aseFloat( 1.0 / slot.roughness )
        self.shinestrength = aseFloat( slot.specular_intensity )
        self.transparency = aseFloat( 0.0 )
        self.wiresize = aseFloat( 1.0 )

        # Material Definition
        self.shading = 'Blinn'
        self.xpfalloff = aseFloat( 0.0 )
        self.xptype = 'Filter'
        self.falloff = 'In'
        self.soften = False
        self.diffusemap = cDiffusemap( slot )
        self.submtls = []
        self.selfillum = aseFloat( 0.0 )
        self.dump = ("\n\t\t*MATERIAL_NAME \"{0}\""+
                    "\n\t\t*MATERIAL_CLASS \"{1}\""+
                    "\n\t\t*MATERIAL_AMBIENT {2}"+
                    "\n\t\t*MATERIAL_DIFFUSE {3}"+
                    "\n\t\t*MATERIAL_SPECULAR {4}"+
                    "\n\t\t*MATERIAL_SHINE {5}"+
                    "\n\t\t*MATERIAL_SHINESTRENGTH {6}"+
                    "\n\t\t*MATERIAL_TRANSPARENCY {7}"+
                    "\n\t\t*MATERIAL_WIRESIZE {8}"+
                    "\n\t\t*MATERIAL_SHADING {9}"+
                    "\n\t\t*MATERIAL_XP_FALLOFF {10}"+
                    "\n\t\t*MATERIAL_SELFILLUM {11}"+
                    "\n\t\t*MATERIAL_FALLOFF {12}"+
                    "\n\t\t*MATERIAL_XP_TYPE {13}"+
                    "{14}").format( self.name, self.matClass, self.ambient, self.diffuse, self.specular, self.shine, self.shinestrength, self.transparency, self.wiresize, self.shading, self.xpfalloff, self.selfillum, self.falloff, self.xptype, self.diffdump() )

    def diffdump( self ):
        for x in [self.diffusemap]:
            return x

    def __repr__( self ):
        return self.dump
class cDiffusemap:
    def __init__( self, slot ):
        import os
        self.dump = ''
        if slot is None:
            self.name = 'default'
            self.mapclass = 'Bitmap'
            self.bitmap = 'None'
        else:
            self.name = slot.name
            self.mapclass = 'Bitmap'
            matname = '//base/' + self.name.replace( '\\', '/' )
            self.bitmap = matname.lower()
        self.subno = 1
        self.amount = aseFloat( 1.0 )
        self.type = 'Screen'
        self.uoffset = aseFloat( 0.0 )
        self.voffset = aseFloat( 0.0 )
        self.utiling = aseFloat( 1.0 )
        self.vtiling = aseFloat( 1.0 )
        self.angle = aseFloat( 0.0 )
        self.blur = aseFloat( 1.0 )
        self.bluroffset = aseFloat( 0.0 )
        self.noiseamt = aseFloat( 1.0 )
        self.noisesize = aseFloat( 1.0 )
        self.noiselevel = 1
        self.noisephase = aseFloat( 0.0 )
        self.bitmapfilter = 'Pyramidal'

        self.dump = ("\n\t\t*MAP_DIFFUSE {{"+
                    "\n\t\t\t*MAP_NAME \"{0}\""+
                    "\n\t\t\t*MAP_CLASS \"{1}\""+
                    "\n\t\t\t*MAP_SUBNO {2}"+
                    "\n\t\t\t*MAP_AMOUNT {3}"+
                    "\n\t\t\t*BITMAP \"{4}\""+
                    "\n\t\t\t*MAP_TYPE {5}"+
                    "\n\t\t\t*UVW_U_OFFSET {6}"+
                    "\n\t\t\t*UVW_V_OFFSET {7}"+
                    "\n\t\t\t*UVW_U_TILING {8}"+
                    "\n\t\t\t*UVW_V_TILING {9}"+
                    "\n\t\t\t*UVW_ANGLE {10}"+
                    "\n\t\t\t*UVW_BLUR {11}"+
                    "\n\t\t\t*UVW_BLUR_OFFSET {12}"+
                    "\n\t\t\t*UVW_NOUSE_AMT {13}"+
                    "\n\t\t\t*UVW_NOISE_SIZE {14}"+
                    "\n\t\t\t*UVW_NOISE_LEVEL {15}"+
                    "\n\t\t\t*UVW_NOISE_PHASE {16}"+
                    "\n\t\t\t*BITMAP_FILTER {17}"+
                    "\n\t\t}}").format( self.name, self.mapclass, self.subno, self.amount, self.bitmap, self.type, self.uoffset, self.voffset, self.utiling, self.vtiling, self.angle, self.blur, self.bluroffset, self.noiseamt, self.noisesize, self.noiselevel, self.noisephase, self.bitmapfilter )

    def __repr__( self ):
        return self.dump

#== Geometry ===============================================================
class cGeomObject:
    def __init__( self, object ):
        print( object.name + ": Constructing Geometry" )
        global optionAllowMultiMats
        global currentMatId

        self.name = object.name
        self.prop_motionblur = 0
        self.prop_castshadow = 1
        self.prop_recvshadow = 1

        if optionAllowMultiMats:
            self.material_ref = 0
        else:
            self.material_ref = currentMatId

        self.nodetm = cNodeTM( object )
        self.mesh = cMesh( object )

        self.dump = '''\n*GEOMOBJECT {{\n\t*NODE_NAME "{0}"\n{1}\n{2}\n\t*PROP_MOTIONBLUR {3}\n\t*PROP_CASTSHADOW {4}\n\t*PROP_RECVSHADOW {5}\n\t*MATERIAL_REF {6}\n}}'''.format( self.name, self.nodetm, self.mesh, self.prop_motionblur, self.prop_castshadow, self.prop_recvshadow, self.material_ref )

    def __repr__( self ):
        return self.dump
class cNodeTM:
    def __init__( self, object ):
        self.name = object.name
        self.inherit_pos = '0 0 0'
        self.inherit_rot = '0 0 0'
        self.inherit_scl = '0 0 0'
        self.tm_row0 = '\t'.join( [aseFloat( x ) for x in [1.0, 0.0, 0.0]] )
        self.tm_row1 = '\t'.join( [aseFloat( x ) for x in [0.0, 1.0, 0.0]] )
        self.tm_row2 = '\t'.join( [aseFloat( x ) for x in [0.0, 0.0, 1.0]] )
        self.tm_row3 = '\t'.join( [aseFloat( x ) for x in [0.0, 0.0, 0.0]] )
        self.tm_pos = '\t'.join( [aseFloat( x ) for x in [0.0, 0.0, 0.0]] )
        self.tm_rotaxis = '\t'.join( [aseFloat( x ) for x in [0.0, 0.0, 0.0]] )
        self.tm_rotangle = aseFloat( 0.0 )
        self.tm_scale = '\t'.join( [aseFloat( x ) for x in [1.0, 1.0, 1.0]] )
        self.tm_scaleaxis = '\t'.join( [aseFloat( x ) for x in [0.0, 0.0, 0.0]] )
        self.tm_scaleaxisang = aseFloat( 0.0 )

        self.dump = ("\t*NODE_TM {{"+
                    "\n\t\t*NODE_NAME \"{0}\""+
                    "\n\t\t*INHERIT_POS {1}"+
                    "\n\t\t*INHERIT_ROT {2}"+
                    "\n\t\t*INHERIT_SCL {3}"+
                    "\n\t\t*TM_ROW0 {4}"+
                    "\n\t\t*TM_ROW1 {5}"+
                    "\n\t\t*TM_ROW2 {6}"+
                    "\n\t\t*TM_ROW3 {7}"+
                    "\n\t\t*TM_POS {8}"+
                    "\n\t\t*TM_ROTAXIS {9}"+
                    "\n\t\t*TM_ROTANGLE {10}"+
                    "\n\t\t*TM_SCALE {11}"+
                    "\n\t\t*TM_SCALEAXIS {12}"+
                    "\n\t\t*TM_SCALEAXISANG {13}"+
                    "\n\t}}").format( self.name, self.inherit_pos, self.inherit_rot, self.inherit_scl, self.tm_row0, self.tm_row1, self.tm_row2, self.tm_row3, self.tm_pos, self.tm_rotaxis, self.tm_rotangle, self.tm_scale, self.tm_scaleaxis, self.tm_scaleaxisang )

    def __repr__( self ):
        return self.dump
class cMesh:
    def __init__( self, obj ):
        bpy.ops.mesh.reveal
        
        self.uvdata = cUVdata( obj )

        self.timevalue = '0'
        self.numvertex = len( obj.data.vertices )
        self.numfaces = len( obj.data.polygons )
        self.vertlist = cVertlist( obj )
        self.facelist = cFacelist( obj )


        # Vertex Paint
        if len( obj.data.vertex_colors ) > 0:
            self.cvertlist = cCVertlist( obj )
            self.numcvertex = self.cvertlist.length
            self.numcvfaces = len( obj.data.vertex_colors.data.polygons )
            self.cfacelist = cCFacelist( self.numcvfaces )
            # change them into strings now
            self.numcvertex = '\n\t\t*MESH_NUMCVERTEX {0}'.format( self.numcvertex )
            self.cvertlist = '\n{0}'.format( self.cvertlist )
            self.numcvfaces = '\n\t\t*MESH_NUMCVFACES {0}'.format( self.numcvfaces )
            self.cfacelist = '\n{0}'.format( self.cfacelist )
        else:
            self.numcvertex = '\n\t\t*MESH_NUMCVERTEX 0'
            self.cvertlist = ''
            self.numcvfaces = ''
            self.cfacelist = ''

        self.normals = cNormallist( obj )
       
    def __repr__( self ):
        temp = '''\t*MESH {{\n\t\t*TIMEVALUE {0}\n\t\t*MESH_NUMVERTEX {1}\n\t\t*MESH_NUMFACES {2}\n\t\t*MESH_VERTEX_LIST {3}\n\t\t*MESH_FACE_LIST {4}{5}{6}{7}{8}{9}\n{10}\n\t}}'''.format( self.timevalue, self.numvertex, self.numfaces, self.vertlist, self.facelist, self.uvdata, self.numcvertex, self.cvertlist, self.numcvfaces, self.cfacelist, self.normals )
        return temp
class cVertlist:
    def __init__( self, object ):
        self.vertlist = []
        for data in object.data.vertices:
            temp = cVert( data.index, data.co.to_tuple( 4 ) )
            self.vertlist.append( temp )

    def dump( self ):
        temp = ''
        for x in self.vertlist:
            temp += str( x )
        return temp

    def __repr__( self ):
        return '''{{\n{0}\t\t}}'''.format( self.dump() )
class cVert:
    def __init__( self, index, coord ):
        global optionScale

        self.index = index
        self.x = aseFloat( coord[0] * optionScale )
        self.y = aseFloat( coord[1] * optionScale )
        self.z = aseFloat( coord[2] * optionScale )

    def __repr__( self ):
        return '''\t\t\t*MESH_VERTEX {0:4d}\t{1}\t{2}\t{3}\n'''.format(self.index,self.x,self.y,self.z)
class cFacelist:
    def __init__( self, object ):
        global optionAllowMultiMats
        global matList
        global numMats
        global currentMatId

        self.facelist = []
        sgID = 0

        # Define smoothing groups (if enabled)
        if ( collisionObject( object ) == 0 ):
            if ( optionSmoothingGroups ):
                self.smoothing_groups = defineSmoothing( self, object )
            else:
                self.smoothing_groups = ''

        for face in object.data.polygons:
            if optionAllowMultiMats:
                if ( collisionObject( object ) < 2 ):
                    self.matid = matList.index( object.material_slots[face.material_index].material.name )
                else:
                    self.matid = 0
            else:
                self.matid = currentMatId
            if ( collisionObject( object ) == 0 ):
                if ( optionSmoothingGroups ):
                    for group in self.smoothing_groups:
                        if group.count( face.index ) == 0:
                            continue
                        else:
                            #TODO: Compress sg's
                            index = self.smoothing_groups.index( group )
                            sgID = index % 32

            temp = '''\t\t\t*MESH_FACE {0:4d}:    A: {1:4d} B: {2:4d} C: {3:4d} AB:    0 BC:    0 CA:    0\t *MESH_SMOOTHING {4}\t *MESH_MTLID {5}\n'''.format( face.index, face.vertices[0], face.vertices[1], face.vertices[2], sgID, self.matid )
            self.facelist.append( temp )

        if currentMatId < numMats - 1:
            currentMatId += 1
        else:
            currentMatId = 0

    def dump( self ):
        temp = ''
        for x in self.facelist:
            temp = temp + str( x )
        return temp

    def __repr__( self ):
        return '''{{\n{0}\t\t}}'''.format( self.dump() )

class cUVdata:
    "Representation of mesh UV coordinates"

    def __init__( self, obj ):
        self.uvdata = ''
        mesh = obj.data
        mesh.calc_loop_triangles()
        
        if ( len( mesh.uv_layers ) == 0 ) or ( collisionObject( obj ) > 0 ):
            self.uvdata = "\n\t\t*MESH_NUMTVERTEX 0"
        else:
            # For each UV layer
            for uv_layer_num, uv_layer in enumerate( mesh.uv_layers ):
                tvlist = []
                tvdata = ''
                tfdata = ''
                # For each tri in the mesh
                for tri_num, tri in enumerate( mesh.loop_triangles ):
                    tfdata += "\n\t\t\t*MESH_TFACE {0}".format(tri_num)
                    # For each index in the triangle loop (should be 3)
                    for vert_index in tri.loops:
                        # Look up corresponding UV data from the uv_layer array
                        uvvert = uv_layer.data[vert_index].uv
                        if uvvert not in tvlist: # TODO bad performance
                            tvlist.append( uvvert ) #only append vertices with unique uvs
                            tvdata += ("\n\t\t\t*MESH_TVERT {0}\t{1}\t{2}\t{3}"
                                      ).format(len(tvlist)-1,aseFloat(uvvert[0]),aseFloat(uvvert[1]),aseFloat(0.0))
                        tfdata += "\t" + str (tvlist.index(uvvert))
                tvdata = ("\n\t\t*MESH_NUMTVERTEX " + str(len(tvlist)) +
                          "\n\t\t*MESH_TVERTLIST {" + tvdata + "\n\t\t}")
                tfdata = ("\n\t\t*MESH_NUMTVFACES " + str(len(mesh.loop_triangles)) +
                          "\n\t\t*MESH_TFACELIST {" + tfdata + "\n\t\t}")
                if uv_layer_num > 0:
                    tvdata = "\n\t\t*MESH_MAPPINGCHANNEL " + str(uv_layer_num + 1) + " {" + tvdata.replace("\n","\n\t")
                    tfdata = tfdata.replace("\n","\n\t") + "\n\t\t}"
                self.uvdata = self.uvdata + tvdata + tfdata

    def __repr__( self ):
        return self.uvdata

class cCVertlist:
    "Representation of mesh vertex colour information"

    def __init__( self, obj ):
        self.vertlist = []
        index = 0

        # Blender 2.63+
        bpy.ops.object.mode_set( mode = 'OBJECT' )
        mesh = obj.data
        mesh.calc_loop_triangles()

        # Look up vertex colors for loop triangle indices
        for tri in mesh.loop_triangles:
            for vert_index in tri.loops:
                vcol = mesh.vertex_colors.active.data[vert_index].color
                self.vertlist.append(cCVert(index, vcol))
                index += 1

        self.length = len( self.vertlist )

    def dump( self ):
        temp = ''
        for x in self.vertlist:
            temp = temp + str( x )
        return temp

    def __repr__( self ):
        return '''\t\t*MESH_CVERTLIST {{\n{0}\t\t}}'''.format( self.dump() )

class cCVert:
    def __init__( self, index, temp ):
        self.index = index
        self.r = aseFloat( float( temp[0] ) )
        self.g = aseFloat( float( temp[1] ) )
        self.b = aseFloat( float( temp[2] ) )

    def __repr__( self ):
        return '''\t\t\t*MESH_VERTCOL {0} {1} {2} {3}\n'''.format( self.index, self.r, self.g, self.b )

class cCFacelist:
    def __init__( self, facecount ):
        temp = [0 for x in range( facecount )]
        self.facelist = []
        for index, data in enumerate( temp ):
            self.facelist.append( cCFace( index, data ) )

    def dump( self ):
        temp = ''
        for x in self.facelist:
            temp = temp + str( x )
        return temp

    def __repr__( self ):
        return '''\t\t*MESH_CFACELIST {{\n{0}\t\t}}'''.format( self.dump() )
class cCFace:
    def __init__( self, index, data ):
        self.index = index
        self.vertices = []
        self.vertices.append( index * 3 )
        self.vertices.append( ( index * 3 ) + 1 )
        self.vertices.append( ( index * 3 ) + 2 )

    def __repr__( self ):
        return '''\t\t\t*MESH_CFACE {0} {1} {2} {3}\n'''.format( self.index, self.vertices[0], self.vertices[1], self.vertices[2] )
class cNormallist:
    def __init__( self, object ):
        self.normallist = []
        for face in object.data.polygons:
            self.normallist.append( cNormal( face, object ) )

    def dump( self ):
        temp = ''
        for x in self.normallist:
            temp = temp + str( x )
        return temp

    def __repr__( self ):
        return '''\t\t*MESH_NORMALS {{\n{0}\t\t}}'''.format( self.dump() )
class cNormal:
    def __init__( self, face, object ):
        self.faceindex = face.index
        self.facenormal = [aseFloat( x ) for x in face.normal.to_tuple( 4 )]
        self.vertnormals = []
        object.data.calc_normals_split()
        for x in [object.data.loops[i] for i in face.loop_indices]:
            self.vertnormals.append( [str(x.vertex_index), [aseFloat(y) for y in x.normal]] )

    def __repr__( self ):
        return '''\t\t\t*MESH_FACENORMAL {0}\t{1}\t{2}\t{3}\n\t\t\t\t*MESH_VERTEXNORMAL {4}\t{5}\n\t\t\t\t*MESH_VERTEXNORMAL {6}\t{7}\n\t\t\t\t*MESH_VERTEXNORMAL {8}\t{9}\n'''.format( self.faceindex, self.facenormal[0], self.facenormal[1], self.facenormal[2], self.vertnormals[0][0], '\t'.join(self.vertnormals[0][1]), self.vertnormals[1][0], '\t'.join(self.vertnormals[1][1]), self.vertnormals[2][0], '\t'.join(self.vertnormals[2][1]))

#== Smoothing Groups and Helper Methods =================================
def defineSmoothing( self, object ):
    print( object.name + ": Constructing Smoothing Groups" )

    seam_edge_list = []
    sharp_edge_list = []

    _mode = bpy.context.active_object.mode
    bpy.ops.object.mode_set( mode = 'EDIT' )
    bpy.ops.mesh.select_all( action = 'DESELECT' )
    setSelMode( 'EDGE' )

    # Get seams and clear them
    bpy.ops.object.mode_set( mode = 'OBJECT' )
    for edge in object.data.edges:
        if edge.use_seam:
            seam_edge_list.append( edge.index )
            edge.select = True

    bpy.ops.object.mode_set( mode = 'EDIT' )
    bpy.ops.mesh.select_all( action = 'SELECT' )
    bpy.ops.mesh.mark_seam( clear = True )

    # Get sharp edges, convert them to seams
    bpy.ops.mesh.select_all( action = 'DESELECT' )
    bpy.ops.object.mode_set( mode = 'OBJECT' )
    for edge in object.data.edges:
        if edge.use_edge_sharp:
            sharp_edge_list.append( edge )
            edge.select = True

    bpy.ops.object.mode_set( mode = 'EDIT' )
    bpy.ops.mesh.mark_seam()

    bpy.ops.mesh.select_all( action = 'DESELECT' )

    smoothing_groups = []
    face_list = []

    mode = getSelMode( self, False )
    setSelMode( 'FACE' )

    for face in object.data.polygons:
        face_list.append( face.index )

    while len( face_list ) > 0:
        bpy.ops.object.mode_set( mode = 'OBJECT' )
        object.data.polygons[face_list[0]].select = True
        bpy.ops.object.mode_set( mode = 'EDIT' )
        bpy.ops.mesh.select_linked( delimit = {'SEAM'} )

        # TODO - update when API is updated
        selected_faces = getSelectedFaces( self, True )
        smoothing_groups.append( selected_faces )
        for face_index in selected_faces:
            if face_list.count( face_index ) > 0:
                face_list.remove( face_index )
        bpy.ops.mesh.select_all( action = 'DESELECT' )

    setSelMode( mode, False )

    # Clear seams created by sharp edges
    bpy.ops.object.mode_set( mode = 'OBJECT' )
    for edge in object.data.edges:
        if edge.use_seam:
            edge.select = True

    bpy.ops.object.mode_set( mode = 'EDIT' )
    bpy.ops.mesh.mark_seam( clear = True )

    bpy.ops.mesh.select_all( action = 'DESELECT' )
    # Restore original uv seams
    bpy.ops.object.mode_set( mode = 'OBJECT' )
    for edge_index in seam_edge_list:
        object.data.edges[edge_index].select = True

    bpy.ops.object.mode_set( mode = 'EDIT' )
    bpy.ops.mesh.mark_seam()

    print( '\t' + str( len( smoothing_groups ) ) + ' smoothing groups found.' )
    return smoothing_groups

#===========================================================================
# // General Helpers
#===========================================================================

# Check if the mesh is a collider and what kind
# 2 - skip materials, smoothing and uvs
# 1 - skip smoothing and uvs
# 0 - not a collision object
def collisionObject( object ):
    collisionPrefixes = ['UCX_', 'UBX_', 'USX_']
    for prefix in collisionPrefixes:
        if prefix in object.name:
            return 2
    collisionPrefixesAlt = ['collision_', 'shadow_']
    for prefix in collisionPrefixesAlt:
        if prefix in object.name:
            return 1
    return 0

# Set the selection mode    
def setSelMode( mode, default = True ):
    if default:
        if mode == 'VERT':
            bpy.context.tool_settings.mesh_select_mode = [True, False, False]
        elif mode == 'EDGE':
            bpy.context.tool_settings.mesh_select_mode = [False, True, False]
        elif mode == 'FACE':
            bpy.context.tool_settings.mesh_select_mode = [False, False, True]
        else:
            return False
    else:
        bpy.context.tool_settings.mesh_select_mode = mode
        return True
def getSelMode( self, default = True ):
    if default:
        if bpy.context.tool_settings.mesh_select_mode[0] == True:
            return 'VERT'
        elif bpy.context.tool_settings.mesh_select_mode[1] == True:
            return 'EDGE'
        elif bpy.context.tool_settings.mesh_select_mode[2] == True:
            return 'FACE'
        return False
    else:
        mode = []
        for value in bpy.context.tool_settings.mesh_select_mode:
            mode.append( value )

        return mode
def getSelectedFaces( self, index = False ):
    selected_faces = []
    # Update mesh data
    bpy.ops.object.editmode_toggle()
    bpy.ops.object.editmode_toggle()

    _mode = bpy.context.active_object.mode
    bpy.ops.object.mode_set( mode = 'EDIT' )

    object = bpy.context.active_object
    for face in object.data.polygons:
        if face.select == True:
            if index == False:
                selected_faces.append( face )
            else:
                selected_faces.append( face.index )

    bpy.ops.object.mode_set( mode = _mode )

    return selected_faces

#== Core ===================================================================

from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, FloatProperty

class ExportAse( bpy.types.Operator, ExportHelper ):
    '''Load an Ascii Scene Export File'''
    bl_idname = "export.ase"
    bl_label = "Export"
    __doc__ = "Ascii Scene Exporter (.ase)"
    filename_ext = ".ase"
    filter_glob : StringProperty( default = "*.ase", options = {'HIDDEN'} )

    filepath : StringProperty( 
        name = "File Path",
        description = "File path used for exporting the ASE file",
        maxlen = 1024,
        default = "" )

    option_apply_stack : BoolProperty( 
            name = "Apply modifiers",
            description = "Apply modifier stack before exporting",
            default = True )

    option_separate_by_material : BoolProperty( 
            name = "Separate by material",
            description = "Separates objects by material",
            default = True )

    option_triangulate : BoolProperty( 
            name = "Triangulate",
            description = "Triangulates all exportable objects",
            default = True )

    option_normals : BoolProperty( 
            name = "Recalculate Normals",
            description = "Recalculate normals before exporting",
            default = False )

    option_remove_doubles : BoolProperty( 
            name = "Remove Doubles",
            description = "Remove any duplicate vertices before exporting",
            default = False )

    option_apply_scale : BoolProperty( 
            name = "Scale",
            description = "Apply scale transformation",
            default = True )

    option_apply_location : BoolProperty( 
            name = "Location",
            description = "Apply location transformation",
            default = True )

    option_apply_rotation : BoolProperty( 
            name = "Rotation",
            description = "Apply rotation transformation",
            default = True )

    option_smoothinggroups : BoolProperty( 
            name = "Smoothing Groups",
            description = "Construct hard edge islands as smoothing groups",
            default = True )

    option_separate : BoolProperty( 
            name = "Separate",
            description = "A separate ASE file for every selected object",
            default = False )

    option_submaterials : BoolProperty( 
            name = "Use Submaterials (UDK)",
            description = "Export a single material with multiple sub materials",
            default = False )

    option_allowmultimats : BoolProperty( 
            name = "Allow Multiple Materials (UDK)",
            description = "Allow multiple materials per geometry object",
            default = False )

    option_scale : FloatProperty( 
            name = "Scale",
            description = "Object scaling factor (default: 1.0)",
            min = 0.01,
            max = 1000.0,
            soft_min = 0.01,
            soft_max = 1000.0,
            default = 1.0 )

    def draw( self, context ):
        layout = self.layout

        box = layout.box()
        box.label( text = 'Essentials:' )
        box.prop( self, 'option_apply_stack' )
        box.prop( self, 'option_separate_by_material' )
        box.prop( self, 'option_triangulate' )
        box.prop( self, 'option_normals' )
        box.prop( self, 'option_remove_doubles' )
        box.label( text = "Transformations:" )
        box.prop( self, 'option_apply_scale' )
        box.prop( self, 'option_apply_rotation' )
        box.prop( self, 'option_apply_location' )
        box.label( text = "Materials:" )
        box.prop( self, 'option_submaterials' )
        box.prop( self, 'option_allowmultimats' )
        box.label( text = "Advanced:" )
        box.prop( self, 'option_scale' )
        box.prop( self, 'option_smoothinggroups' )

    @classmethod
    def poll( cls, context ):
        active = context.active_object
        selected = context.selected_objects
        camera = context.scene.camera
        ok = selected or camera
        return ok

    def writeASE( self, filename, data ):
        print( '\nWriting', filename )
        try:
            file = open( filename, 'w' )
        except IOError:
            print( 'Error: The file could not be written to. Aborting.' )
        else:
            file.write( data )
            file.close()

    def execute( self, context ):
        start = time.time()

        global optionScale
        global optionSubmaterials
        global optionSmoothingGroups
        global optionAllowMultiMats

        global aseHeader
        global aseScene
        global aseMaterials
        global aseGeometry

        global currentMatId
        global numMats
        global matList

        # Set globals and reinitialize ase components
        aseHeader = ''
        aseScene = ''
        aseMaterials = ''
        aseGeometry = ''

        optionScale = self.option_scale
        optionSubmaterials = self.option_submaterials
        optionSmoothingGroups = self.option_smoothinggroups
        optionAllowMultiMats = self.option_allowmultimats

        matList = []
        currentMatId = 0
        numMats = 0

        # Build ASE Header, Scene
        print( '\nAscii Scene Export by MCampagnini\n' )
        print( 'Objects selected: ' + str( len( bpy.context.selected_objects ) ) )
        aseHeader = str( cHeader() )
        aseScene = str( cScene() )

        # Back up duplicates, work on originals
        objects = []
        bup_objs = []
        bup_obj_names = {}
        bup_mesh_names = {}
        for object in bpy.context.selected_objects:
            if object.type == 'MESH':
                bpy.ops.object.select_all( action = 'DESELECT' )
                bpy.context.view_layer.objects.active = object
                objects.append(object)
                object.select_set( state = True)
                bpy.ops.object.duplicate()
                dup = bpy.context.active_object
                dup.name = "temp"
                dup.data.name = "temp"
                bup_objs.append(dup)
                bup_obj_names[dup] = object.name
                bup_mesh_names[dup] = object.data.name
            # Apply modifiers
            if self.option_apply_stack:
                bpy.context.view_layer.objects.active = object
                while (len(object.modifiers)):
                    bpy.ops.object.modifier_apply(apply_as='DATA', modifier = object.modifiers[0].name)

        # Separate by material
        bpy.ops.object.select_all( action = 'DESELECT' )
        if self.option_separate_by_material:
            for object in objects:
                bpy.context.view_layer.objects.active = object
                object.select_set( state = True)
                bpy.ops.object.mode_set( mode = 'EDIT' )
                bpy.ops.mesh.separate( type = 'MATERIAL' )
            for object in bpy.context.selected_objects:
                if object.type == 'MESH':
                    if object not in objects:
                        objects.append(object)

        objects.sort( key = lambda a: a.name )

        aseMaterials = str( cMaterials(objects) )

        for object in objects:
            bpy.context.view_layer.objects.active = object
            object.select_set( state = True)

            # Apply options
            bpy.ops.object.mode_set( mode = 'EDIT' )
            if self.option_remove_doubles:
                bpy.ops.object.mode_set( mode = 'EDIT' )
                bpy.ops.mesh.select_all( action = 'SELECT' )
                bpy.ops.mesh.remove_doubles()
            if self.option_triangulate:
                bpy.ops.object.mode_set( mode = 'EDIT' )
                bpy.ops.mesh.select_all( action = 'SELECT' )
                bpy.ops.mesh.quads_convert_to_tris()
            if self.option_normals:
                bpy.ops.object.mode_set( mode = 'EDIT' )
                bpy.ops.mesh.select_all( action = 'SELECT' )
                bpy.ops.mesh.normals_make_consistent()


            # Transformations
            bpy.ops.object.mode_set( mode = 'OBJECT' )
            bpy.ops.object.transform_apply( location = self.option_apply_location, rotation = self.option_apply_rotation, scale = self.option_apply_scale )

            #Construct ASE Geometry Nodes
            aseGeometry += str( cGeomObject( object ) )
            
        # Clean up
        bpy.ops.object.mode_set( mode = 'OBJECT' )
        bpy.ops.object.select_all(action='DESELECT')
        for object in objects:
            bpy.context.view_layer.objects.active = object
            object.select_set( state = True)
            bpy.ops.object.delete()

        # Restore scene from duplicates
        for object in bup_objs:
            object.name = bup_obj_names[object]
            object.data.name = bup_mesh_names[object]
            bpy.context.view_layer.objects.active = object
            object.select_set( state = True)

        aseModel = ''
        aseModel += aseHeader
        aseModel += aseScene
        aseModel += aseMaterials
        aseModel += aseGeometry

        # Write the ASE file
        self.writeASE( self.filepath, aseModel )

        lapse = ( time.time() - start )
        print( 'Completed in ' + str( lapse ) + ' seconds' )

        return {'FINISHED'}

def menu_func( self, context ):
    self.layout.operator( ExportAse.bl_idname, text = "ASCII Scene Exporter (.ase)" )

def register():
    bpy.utils.register_class( ExportAse )
    bpy.types.TOPBAR_MT_file_export.append( menu_func )

def unregister():
    bpy.utils.unregister_class( ExportAse )
    bpy.types.TOPBAR_MT_file_export.remove( menu_func )

if __name__ == "__main__":
    register()
