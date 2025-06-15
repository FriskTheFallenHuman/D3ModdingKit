#/*
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

#include "Game_local.h"

// overridable events
const idEventDef EV_PostSpawn( "<postspawn>", NULL );
const idEventDef EV_FindTargets( "<findTargets>", NULL );
const idEventDef EV_Touch( "<touch>", "et" );
const idEventDef EV_GetName( "getName", NULL, 's' );
const idEventDef EV_SetName( "setName", "s" );
const idEventDef EV_Activate( "activate", "e" );
const idEventDef EV_ActivateTargets( "activateTargets", "e" );
const idEventDef EV_NumTargets( "numTargets", NULL, 'f' );
const idEventDef EV_GetTarget( "getTarget", "f", 'e' );
const idEventDef EV_RandomTarget( "randomTarget", "s", 'e' );
const idEventDef EV_Bind( "bind", "e" );
const idEventDef EV_BindPosition( "bindPosition", "e" );
const idEventDef EV_BindToJoint( "bindToJoint", "esf" );
const idEventDef EV_Unbind( "unbind", NULL );
const idEventDef EV_RemoveBinds( "removeBinds" );
const idEventDef EV_SpawnBind( "<spawnbind>", NULL );
const idEventDef EV_SetOwner( "setOwner", "e" );
const idEventDef EV_SetModel( "setModel", "s" );
const idEventDef EV_SetSkin( "setSkin", "s" );
const idEventDef EV_GetWorldOrigin( "getWorldOrigin", NULL, 'v' );
const idEventDef EV_SetWorldOrigin( "setWorldOrigin", "v" );
const idEventDef EV_GetOrigin( "getOrigin", NULL, 'v' );
const idEventDef EV_SetOrigin( "setOrigin", "v" );
const idEventDef EV_GetAngles( "getAngles", NULL, 'v' );
const idEventDef EV_SetAngles( "setAngles", "v" );
const idEventDef EV_GetLinearVelocity( "getLinearVelocity", NULL, 'v' );
const idEventDef EV_SetLinearVelocity( "setLinearVelocity", "v" );
const idEventDef EV_GetAngularVelocity( "getAngularVelocity", NULL, 'v' );
const idEventDef EV_SetAngularVelocity( "setAngularVelocity", "v" );
const idEventDef EV_GetSize( "getSize", NULL, 'v' );
const idEventDef EV_SetSize( "setSize", "vv" );
const idEventDef EV_GetMins( "getMins", NULL, 'v' );
const idEventDef EV_GetMaxs( "getMaxs", NULL, 'v' );
const idEventDef EV_IsHidden( "isHidden", NULL, 'd' );
const idEventDef EV_Hide( "hide", NULL );
const idEventDef EV_Show( "show", NULL );
const idEventDef EV_Touches( "touches", "E", 'd' );
const idEventDef EV_ClearSignal( "clearSignal", "d" );
const idEventDef EV_GetShaderParm( "getShaderParm", "d", 'f' );
const idEventDef EV_SetShaderParm( "setShaderParm", "df" );
const idEventDef EV_SetShaderParms( "setShaderParms", "ffff" );
const idEventDef EV_SetColor( "setColor", "fff" );
const idEventDef EV_GetColor( "getColor", NULL, 'v' );
const idEventDef EV_CacheSoundShader( "cacheSoundShader", "s" );
const idEventDef EV_StartSoundShader( "startSoundShader", "sd", 'f' );
const idEventDef EV_StartSound( "startSound", "sdd", 'f' );
const idEventDef EV_StopSound( "stopSound", "dd" );
const idEventDef EV_FadeSound( "fadeSound", "dff" );
const idEventDef EV_SetGuiParm( "setGuiParm", "ss" );
const idEventDef EV_SetGuiFloat( "setGuiFloat", "sf" );
const idEventDef EV_GetNextKey( "getNextKey", "ss", 's' );
const idEventDef EV_SetKey( "setKey", "ss" );
const idEventDef EV_GetKey( "getKey", "s", 's' );
const idEventDef EV_GetIntKey( "getIntKey", "s", 'f' );
const idEventDef EV_GetFloatKey( "getFloatKey", "s", 'f' );
const idEventDef EV_GetVectorKey( "getVectorKey", "s", 'v' );
const idEventDef EV_GetEntityKey( "getEntityKey", "s", 'e' );
const idEventDef EV_RestorePosition( "restorePosition" );
const idEventDef EV_UpdateCameraTarget( "<updateCameraTarget>", NULL );
const idEventDef EV_DistanceTo( "distanceTo", "E", 'f' );
const idEventDef EV_DistanceToPoint( "distanceToPoint", "v", 'f' );
const idEventDef EV_StartFx( "startFx", "s" );
const idEventDef EV_HasFunction( "hasFunction", "s", 'd' );
const idEventDef EV_CallFunction( "callFunction", "s" );
const idEventDef EV_SetNeverDormant( "setNeverDormant", "d" );
const idEventDef EV_GetMass( "getMass", "d" , 'f' );
const idEventDef EV_IsInLiquid( "isInLiquid", NULL, 'd' );

ABSTRACT_DECLARATION( idClass, idEntity )
	EVENT( EV_GetName,				idEntity::Script_GetName )
	EVENT( EV_SetName,				idEntity::Script_SetName )
	EVENT( EV_FindTargets,			idEntity::Script_FindTargets )
	EVENT( EV_ActivateTargets,		idEntity::Script_ActivateTargets )
	EVENT( EV_NumTargets,			idEntity::Script_NumTargets )
	EVENT( EV_GetTarget,			idEntity::Script_GetTarget )
	EVENT( EV_RandomTarget,			idEntity::Script_RandomTarget )
	EVENT( EV_BindToJoint,			idEntity::Script_BindToJoint )
	EVENT( EV_RemoveBinds,			idEntity::Script_RemoveBinds )
	EVENT( EV_Bind,					idEntity::Script_Bind )
	EVENT( EV_BindPosition,			idEntity::Script_BindPosition )
	EVENT( EV_Unbind,				idEntity::Script_Unbind )
	EVENT( EV_SpawnBind,			idEntity::Script_SpawnBind )
	EVENT( EV_SetOwner,				idEntity::Script_SetOwner )
	EVENT( EV_SetModel,				idEntity::Script_SetModel )
	EVENT( EV_SetSkin,				idEntity::Script_SetSkin )
	EVENT( EV_GetShaderParm,		idEntity::Script_GetShaderParm )
	EVENT( EV_SetShaderParm,		idEntity::Script_SetShaderParm )
	EVENT( EV_SetShaderParms,		idEntity::Script_SetShaderParms )
	EVENT( EV_SetColor,				idEntity::Script_SetColor )
	EVENT( EV_GetColor,				idEntity::Script_GetColor )
	EVENT( EV_IsHidden,				idEntity::Script_IsHidden )
	EVENT( EV_Hide,					idEntity::Script_Hide )
	EVENT( EV_Show,					idEntity::Script_Show )
	EVENT( EV_CacheSoundShader,		idEntity::Script_CacheSoundShader )
	EVENT( EV_StartSoundShader,		idEntity::Script_StartSoundShader )
	EVENT( EV_StartSound,			idEntity::Script_StartSound )
	EVENT( EV_StopSound,			idEntity::Script_StopSound )
	EVENT( EV_FadeSound,			idEntity::Script_FadeSound )
	EVENT( EV_GetWorldOrigin,		idEntity::Script_GetWorldOrigin )
	EVENT( EV_SetWorldOrigin,		idEntity::Script_SetWorldOrigin )
	EVENT( EV_GetOrigin,			idEntity::Script_GetOrigin )
	EVENT( EV_SetOrigin,			idEntity::Script_SetOrigin )
	EVENT( EV_GetAngles,			idEntity::Script_GetAngles )
	EVENT( EV_SetAngles,			idEntity::Script_SetAngles )
	EVENT( EV_GetLinearVelocity,	idEntity::Script_GetLinearVelocity )
	EVENT( EV_SetLinearVelocity,	idEntity::Script_SetLinearVelocity )
	EVENT( EV_GetAngularVelocity,	idEntity::Script_GetAngularVelocity )
	EVENT( EV_SetAngularVelocity,	idEntity::Script_SetAngularVelocity )
	EVENT( EV_GetSize,				idEntity::Script_GetSize )
	EVENT( EV_SetSize,				idEntity::Script_SetSize )
	EVENT( EV_GetMins,				idEntity::Script_GetMins)
	EVENT( EV_GetMaxs,				idEntity::Script_GetMaxs )
	EVENT( EV_Touches,				idEntity::Script_Touches )
	EVENT( EV_SetGuiParm, 			idEntity::Script_SetGuiParm )
	EVENT( EV_SetGuiFloat, 			idEntity::Script_SetGuiFloat )
	EVENT( EV_GetNextKey,			idEntity::Script_GetNextKey )
	EVENT( EV_SetKey,				idEntity::Script_SetKey )
	EVENT( EV_GetKey,				idEntity::Script_GetKey )
	EVENT( EV_GetIntKey,			idEntity::Script_GetIntKey )
	EVENT( EV_GetFloatKey,			idEntity::Script_GetFloatKey )
	EVENT( EV_GetVectorKey,			idEntity::Script_GetVectorKey )
	EVENT( EV_GetEntityKey,			idEntity::Script_GetEntityKey )
	EVENT( EV_RestorePosition,		idEntity::Script_RestorePosition )
	EVENT( EV_UpdateCameraTarget,	idEntity::Script_UpdateCameraTarget )
	EVENT( EV_DistanceTo,			idEntity::Script_DistanceTo )
	EVENT( EV_DistanceToPoint,		idEntity::Script_DistanceToPoint )
	EVENT( EV_StartFx,				idEntity::Script_StartFx )
	EVENT( EV_Thread_WaitFrame,		idEntity::Script_WaitFrame )
	EVENT( EV_Thread_Wait,			idEntity::Script_Wait )
	EVENT( EV_HasFunction,			idEntity::Script_HasFunction )
	EVENT( EV_CallFunction,			idEntity::Script_CallFunction )
	EVENT( EV_SetNeverDormant,		idEntity::Script_SetNeverDormant )
	EVENT( EV_GetMass,              idEntity::Script_GetMass )
	EVENT( EV_IsInLiquid,           idEntity::Script_IsInLiquid )
END_CLASS

/*
================
idEntity::Script_GetName
================
*/
void idEntity::Script_GetName( void ) {
	idThread::ReturnString( GetName() );
}

/*
================
idEntity::Script_SetName
================
*/
void idEntity::Script_SetName( const char*name ) {
	SetName( name );
}

/*
================
idEntity::Script_FindTargets
================
*/
void idEntity::Script_FindTargets( void ) {
	FindTargets();
}

/*
================
idEntity::Script_ActivateTargets
================
*/
void idEntity::Script_ActivateTargets( idEntity *activator ) {
	ActivateTargets( activator );
}

/*
================
idEntity::Script_NumTargets
================
*/
void idEntity::Script_NumTargets( void ) {
	idThread::ReturnInt( NumTargets() );
}

/*
================
idEntity::Script_GetTarget
================
*/
void idEntity::Script_GetTarget( float index) {
	idThread::ReturnEntity( GetTarget( index ) );
}

/*
================
idEntity::Script_RandomTarget
================
*/
void idEntity::Script_RandomTarget( const char *ignore ) {
	idThread::ReturnEntity( RandomTarget( ignore ) );
}

/*
================
idEntity::Script_BindToJoint
================
*/
void idEntity::Script_BindToJoint( idEntity *master, const char *jointname, float orientated ) {
	BindToJoint( master, jointname, orientated );
}

/*
================
idEntity::Script_RemoveBinds
================
*/
void idEntity::Script_RemoveBinds( void ) {
	RemoveBinds();
}

/*
================
idEntity::Script_Bind
================
*/
void idEntity::Script_Bind( idEntity *master ) {
	Bind( master );
}

/*
================
idEntity::Script_BindPosition
================
*/
void idEntity::Script_BindPosition( idEntity *master ) {
	BindPosition( master );
}

/*
================
idEntity::Script_Unbind
================
*/
void idEntity::Script_Unbind( void ) {
	Unbind();
}

/*
================
idEntity::Script_SpawnBind
================
*/
void idEntity::Script_SpawnBind( void ) {
	SpawnBind();
}

/*
================
idEntity::Script_SetOwner
================
*/
void idEntity::Script_SetOwner( idEntity *owner ) {
	SetOwner( owner );
}

/*
================
idEntity::Script_SetModel
================
*/
void idEntity::Script_SetModel( const char *modelname ) {
	SetModel( modelname );
}

/*
================
idEntity::Script_SetModel
================
*/
void idEntity::Script_SetSkin( const char *skinname ) {
	NativeEvent_SetSkin( skinname );
}

/*
================
idEntity::Script_GetShaderParm
================
*/
void idEntity::Script_GetShaderParm( int parmnum ) {
	idThread::ReturnFloat( GetShaderParm( parmnum ) );
}

/*
================
idEntity::Script_SetShaderParm
================
*/
void idEntity::Script_SetShaderParm( int parmnum, float value ) {
	SetShaderParm( parmnum, value );
}

/*
================
idEntity::Script_SetShaderParms
================
*/
void idEntity::Script_SetShaderParms( float parm0, float parm1, float parm2, float parm3 ) {
	SetShaderParms( parm0, parm1, parm2, parm3 );
}

/*
================
idEntity::Script_SetColor
================
*/
void idEntity::Script_SetColor( float red, float green, float blue ) {
	SetColor( red, green, blue );
}

/*
================
idEntity::Script_GetColor
================
*/
void idEntity::Script_GetColor( void ) {
	idThread::ReturnVector( GetColor() );
}

/*
================
idEntity::Script_IsHidden
================
*/
void idEntity::Script_IsHidden( void ) {
	idThread::ReturnInt( IsHidden() );
}

/*
================
idEntity::Script_Hide
================
*/
void idEntity::Script_Hide( void ) {
	Hide();
}

/*
================
idEntity::Script_Hide
================
*/
void idEntity::Script_Show( void ) {
	Show();
}

/*
================
idEntity::Script_CacheSoundShader
================
*/
void idEntity::Script_CacheSoundShader( const char *soundName ) {
	CacheSoundShader( soundName );
}

/*
================
idEntity::Script_StartSoundShader
================
*/
void idEntity::Script_StartSoundShader( const char *shaderName, int channel ) {
	idThread::ReturnInt( StartSoundShader( shaderName, channel ) );
}

/*
================
idEntity::Script_StartSound
================
*/
void idEntity::Script_StartSound( const char *soundName, int channel, int netSync ) {
	idThread::ReturnInt( StartSound( soundName, channel, netSync ) );
}

/*
================
idEntity::Script_StopSound
================
*/
void idEntity::Script_StopSound( int channel, int netSync ) {
	StopSound( channel, netSync );
	//StopSound( channel, ( netSync != 0 ) );
}

/*
================
idEntity::Script_FadeSound
================
*/
void idEntity::Script_FadeSound( int channel, float to, float over ) {
	FadeSound( channel, to, over );
}

/*
================
idEntity::Script_GetWorldOrigin
================
*/
void idEntity::Script_GetWorldOrigin( void ) {
	idThread::ReturnVector( GetWorldOrigin() );
}

/*
================
idEntity::Script_SetWorldOrigin
================
*/
void idEntity::Script_SetWorldOrigin( const idVec3 &pos ) {
	SetWorldOrigin( pos );
}

/*
================
idEntity::Script_GetOrigin
================
*/
void idEntity::Script_GetOrigin( void ) {
	idThread::ReturnVector( GetOrigin() );
}

/*
================
idEntity::Script_SetOrigin
================
*/
void idEntity::Script_SetOrigin( const idVec3 &pos ) {
	SetOrigin( pos );
}

/*
================
idEntity::Script_GetAngles
================
*/
void idEntity::Script_GetAngles( void ) {
	idAngles ang = GetPhysics()->GetAxis().ToAngles();
	idThread::ReturnVector( idVec3( ang[0], ang[1], ang[2] ) );
}

/*
================
idEntity::Script_SetAngles
================
*/
void idEntity::Script_SetAngles( const idAngles &ang ) {
	SetAngles( ang );
}

/*
================
idEntity::Script_GetLinearVelocity
================
*/
void idEntity::Script_GetLinearVelocity( void ) {
	idThread::ReturnVector( GetLinearVelocity() );
}

/*
================
idEntity::Script_SetLinearVelocity
================
*/
void idEntity::Script_SetLinearVelocity( const idVec3 &velocity ) {
	SetLinearVelocity(velocity);
}

/*
================
idEntity::Script_GetAngularVelocity
================
*/
void idEntity::Script_GetAngularVelocity( void ) {
	idThread::ReturnVector( GetAngularVelocity() );
}

/*
================
idEntity::Script_SetAngularVelocity
================
*/
void idEntity::Script_SetAngularVelocity( const idVec3 &velocity ) {
	SetAngularVelocity( velocity );
}

/*
================
idEntity::Script_GetSize
================
*/
void idEntity::Script_GetSize( void ) {
	idBounds bounds;

	bounds = GetPhysics()->GetBounds();
	idThread::ReturnVector( bounds[1] - bounds[0] );
}

/*
================
idEntity::Script_SetSize
================
*/
void idEntity::Script_SetSize( const idVec3 &mins, const idVec3 &maxs ) {
	SetSize( mins, maxs );
}

/*
================
idEntity::Script_GetMins
================
*/
void idEntity::Script_GetMins( void ) {
	idThread::ReturnVector( GetMins() );
}

/*
================
idEntity::Script_GetMaxs
================
*/
void idEntity::Script_GetMaxs( void ) {
	idThread::ReturnVector( GetMaxs() );
}

/*
================
idEntity::Script_Touches
================
*/
void idEntity::Script_Touches( idEntity *ent ) {
	idThread::ReturnInt( Touches( ent ) );
}

/*
================
idEntity::Script_SetGuiParm
================
*/
void idEntity::Script_SetGuiParm( const char *key, const char *val ) {
	SetGuiParm( key, val );
}

/*
================
idEntity::Script_SetGuiFloat
================
*/
void idEntity::Script_SetGuiFloat( const char *key, float val ) {
	SetGuiFloat( key, val );
}

/*
================
idEntity::Script_GetNextKey
================
*/
void idEntity::Script_GetNextKey( const char *prefix, const char *lastMatch ) {
	idThread::ReturnString( GetNextKey( prefix, lastMatch ) );
}

/*
================
idEntity::Script_SetKey
================
*/
void idEntity::Script_SetKey( const char *key, const char *value ) {
	SetKey( key, value );
}

/*
================
idEntity::Script_GetKey
================
*/
void idEntity::Script_GetKey( const char *key ) {
	idThread::ReturnString( GetKey( key ) );
}

/*
================
idEntity::Script_GetIntKey
================
*/
void idEntity::Script_GetIntKey( const char *key ) {
	idThread::ReturnInt( GetIntKey( key ) );
}

/*
================
idEntity::Script_GetFloatKey
================
*/
void idEntity::Script_GetFloatKey( const char *key ) {
	idThread::ReturnFloat( GetFloatKey( key ) );
}

/*
================
idEntity::Script_GetFloatKey
================
*/
void idEntity::Script_GetVectorKey( const char *key ) {
	idThread::ReturnVector( GetVectorKey( key ) );
}

/*
================
idEntity::Script_GetEntityKey
================
*/
void idEntity::Script_GetEntityKey( const char *key ) {
	idThread::ReturnEntity( GetEntityKey( key ) );
}

/*
================
idEntity::Script_RestorePosition
================
*/
void idEntity::Script_RestorePosition( void ) {
	RestorePosition();
}

/*
================
idEntity::Script_UpdateCameraTarget
================
*/
void idEntity::Script_UpdateCameraTarget( void ) {
	UpdateCameraTarget();
}

void idEntity::Script_DistanceTo( idEntity *ent ) {
	idThread::ReturnFloat( DistanceTo( ent ) );
}

/*
================
idEntity::Script_DistanceToPoint
================
*/
void idEntity::Script_DistanceToPoint( const idVec3 &point ) {
	idThread::ReturnFloat( DistanceToPoint( point ) );
}

/*
================
idEntity::Script_StartFx
================
*/
void idEntity::Script_StartFx( const char *fxName ) {
	StartFx( fxName );
}

/*
================
idEntity::Script_WaitFrame
================
*/
void idEntity::Script_WaitFrame( void ) {
	idThread *thread;

	thread = idThread::CurrentThread();
	if ( thread ) {
		thread->WaitFrame();
	}
}

/*
================
idEntity::Script_Wait
================
*/
void idEntity::Script_Wait( float time ) {
	idThread *thread = idThread::CurrentThread();

	if ( !thread ) {
		gameLocal.Error( "Event 'wait' called from outside thread" );
	}

	thread->WaitSec( time );
}

/*
================
idEntity::Script_HasFunction
================
*/
void idEntity::Script_HasFunction( const char *name ) {
	idThread::ReturnInt( HasFunction( name ) );
}

/*
================
idEntity::Script_CallFunction
================
*/
void idEntity::Script_CallFunction( const char *name ) {
	CallFunction( name );
}

/*
================
idEntity::Script_IsInLiquid
================
*/
void idEntity::Script_SetNeverDormant( int enable ) {
	SetNeverDormant( enable );
}

/*
================
idEntity::Script_GetMass
================
*/
void idEntity::Script_GetMass( int id ) {
	idThread::ReturnFloat( GetMass( id ) );
}

/*
================
idEntity::Script_IsInLiquid
================
*/
void idEntity::Script_IsInLiquid( void ) {
	idThread::ReturnInt( IsInLiquid() );
}
