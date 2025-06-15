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

#include "Game_local.h"

const idEventDef AI_EnableEyeFocus( "enableEyeFocus" );
const idEventDef AI_DisableEyeFocus( "disableEyeFocus" );
const idEventDef EV_Footstep( "footstep" );
const idEventDef EV_FootstepLeft( "leftFoot" );
const idEventDef EV_FootstepRight( "rightFoot" );
const idEventDef EV_EnableWalkIK( "EnableWalkIK" );
const idEventDef EV_DisableWalkIK( "DisableWalkIK" );
const idEventDef EV_EnableLegIK( "EnableLegIK", "d" );
const idEventDef EV_DisableLegIK( "DisableLegIK", "d" );
const idEventDef AI_StopAnim( "stopAnim", "dd" );
const idEventDef AI_PlayAnim( "playAnim", "ds", 'd' );
const idEventDef AI_PlayCycle( "playCycle", "ds", 'd' );
const idEventDef AI_IdleAnim( "idleAnim", "ds", 'd' );
const idEventDef AI_SetSyncedAnimWeight( "setSyncedAnimWeight", "ddf" );
const idEventDef AI_SetBlendFrames( "setBlendFrames", "dd" );
const idEventDef AI_GetBlendFrames( "getBlendFrames", "d", 'd' );
const idEventDef AI_AnimState( "animState", "dsd" );
const idEventDef AI_GetAnimState( "getAnimState", "d", 's' );
const idEventDef AI_InAnimState( "inAnimState", "ds", 'd' );
const idEventDef AI_FinishAction( "finishAction", "s" );
const idEventDef AI_AnimDone( "animDone", "dd", 'd' );
const idEventDef AI_OverrideAnim( "overrideAnim", "d" );
const idEventDef AI_EnableAnim( "enableAnim", "dd" );
const idEventDef AI_PreventPain( "preventPain", "f" );
const idEventDef AI_DisablePain( "disablePain" );
const idEventDef AI_EnablePain( "enablePain" );
const idEventDef AI_GetPainAnim( "getPainAnim", NULL, 's' );
const idEventDef AI_SetAnimPrefix( "setAnimPrefix", "s" );
const idEventDef AI_HasAnim( "hasAnim", "ds", 'f' );
const idEventDef AI_CheckAnim( "checkAnim", "ds" );
const idEventDef AI_ChooseAnim( "chooseAnim", "ds", 's' );
const idEventDef AI_AnimLength( "animLength", "ds", 'f' );
const idEventDef AI_AnimDistance( "animDistance", "ds", 'f' );
const idEventDef AI_HasEnemies( "hasEnemies", NULL, 'd' );
const idEventDef AI_NextEnemy( "nextEnemy", "E", 'e' );
const idEventDef AI_ClosestEnemyToPoint( "closestEnemyToPoint", "v", 'e' );
const idEventDef AI_SetNextState( "setNextState", "s" );
const idEventDef AI_SetState( "setState", "s" );
const idEventDef AI_GetState( "getState", NULL, 's' );
const idEventDef AI_GetHead( "getHead", NULL, 'e' );
#ifdef _D3XP
const idEventDef EV_SetDamageGroupScale( "setDamageGroupScale", "sf" );
const idEventDef EV_SetDamageGroupScaleAll( "setDamageGroupScaleAll", "f" );
const idEventDef EV_GetDamageGroupScale( "getDamageGroupScale", "s", 'f' );
const idEventDef EV_SetDamageCap( "setDamageCap", "f" );
const idEventDef EV_SetWaitState( "setWaitState" , "s" );
const idEventDef EV_GetWaitState( "getWaitState", NULL, 's' );
#endif

CLASS_DECLARATION( idAFEntity_Gibbable, idActor )
	EVENT( AI_EnableEyeFocus,			idActor::Script_EnableEyeFocus )
	EVENT( AI_DisableEyeFocus,			idActor::Script_DisableEyeFocus )
	EVENT( EV_Footstep,					idActor::Script_Footstep )
	EVENT( EV_FootstepLeft,				idActor::Script_Footstep )
	EVENT( EV_FootstepRight,			idActor::Script_Footstep )
	EVENT( EV_EnableWalkIK,				idActor::Script_EnableWalkIK )
	EVENT( EV_DisableWalkIK,			idActor::Script_DisableWalkIK )
	EVENT( EV_EnableLegIK,				idActor::Script_EnableLegIK )
	EVENT( EV_DisableLegIK,				idActor::Script_DisableLegIK )
	EVENT( AI_PreventPain,				idActor::Script_PreventPain )
	EVENT( AI_DisablePain,				idActor::Script_DisablePain )
	EVENT( AI_EnablePain,				idActor::Script_EnablePain )
	EVENT( AI_GetPainAnim,				idActor::Script_GetPainAnim )
	EVENT( AI_SetAnimPrefix,			idActor::Script_SetAnimPrefix )
	EVENT( AI_StopAnim,					idActor::Script_StopAnim )
	EVENT( AI_PlayAnim,					idActor::Script_PlayAnim )
	EVENT( AI_PlayCycle,				idActor::Script_PlayCycle )
	EVENT( AI_IdleAnim,					idActor::Script_IdleAnim )
	EVENT( AI_SetSyncedAnimWeight,		idActor::Script_SetSyncedAnimWeight )
	EVENT( AI_SetBlendFrames,			idActor::Script_SetBlendFrames )
	EVENT( AI_GetBlendFrames,			idActor::Script_GetBlendFrames )
	EVENT( AI_AnimState,				idActor::Script_AnimState )
	EVENT( AI_GetAnimState,				idActor::Script_GetAnimState )
	EVENT( AI_InAnimState,				idActor::Script_InAnimState )
	EVENT( AI_FinishAction,				idActor::Script_FinishAction )
	EVENT( AI_AnimDone,					idActor::Script_AnimDone )
	EVENT( AI_OverrideAnim,				idActor::Script_OverrideAnim )
	EVENT( AI_EnableAnim,				idActor::Script_EnableAnim )
	EVENT( AI_HasAnim,					idActor::Script_HasAnim )
	EVENT( AI_CheckAnim,				idActor::Script_CheckAnim )
	EVENT( AI_ChooseAnim,				idActor::Script_ChooseAnim )
	EVENT( AI_AnimLength,				idActor::Script_AnimLength )
	EVENT( AI_AnimDistance,				idActor::Script_AnimDistance )
	EVENT( AI_HasEnemies,				idActor::Script_HasEnemies )
	EVENT( AI_NextEnemy,				idActor::Script_NextEnemy )
	EVENT( AI_ClosestEnemyToPoint,		idActor::Script_ClosestEnemyToPoint )
	EVENT( EV_StopSound,				idActor::Script_StopSound )
	EVENT( AI_SetNextState,				idActor::Script_SetNextState )
	EVENT( AI_SetState,					idActor::Script_SetState )
	EVENT( AI_GetState,					idActor::Script_GetState )
	EVENT( AI_GetHead,					idActor::Script_GetHead )
#ifdef _D3XP
	EVENT( EV_SetDamageGroupScale,		idActor::Script_SetDamageGroupScale )
	EVENT( EV_SetDamageGroupScaleAll,	idActor::Script_SetDamageGroupScaleAll )
	EVENT( EV_GetDamageGroupScale,		idActor::Script_GetDamageGroupScale )
	EVENT( EV_SetDamageCap,				idActor::Script_SetDamageCap )
	EVENT( EV_SetWaitState,				idActor::Script_SetWaitState )
	EVENT( EV_GetWaitState,				idActor::Script_GetWaitState )
#endif
END_CLASS

/*
=====================
idActor::Script_EnableEyeFocus
=====================
*/
void idActor::Script_EnableEyeFocus( void ) {
	EnableEyeFocus();
}

/*
=====================
idActor::Script_DisableEyeFocus
=====================
*/
void idActor::Script_DisableEyeFocus( void ) {
	DisableEyeFocus();
}

/*
===============
idActor::Script_Footstep
===============
*/
void idActor::Script_Footstep( void ) {
	Footstep();
}

void idActor::Script_EnableWalkIK() {
	EnableWalkIK();
}

/*
=====================
idActor::Script_DisableWalkIK
=====================
*/
void idActor::Script_DisableWalkIK( void ) {
	DisableWalkIK();
}

/*
=====================
idActor::Script_EnableLegIK
=====================
*/
void idActor::Script_EnableLegIK( int num ) {
	EnableLegIK( num );
}

/*
=====================
idActor::Script_DisableLegIK
=====================
*/
void idActor::Script_DisableLegIK( int num ) {
	DisableLegIK( num );
}

/*
=====================
idActor::Script_PreventPain
=====================
*/
void idActor::Script_PreventPain( float duration ) {
	PreventPain( duration );
}

/*
===============
idActor::Script_DisablePain
===============
*/
void idActor::Script_DisablePain( void ) {
	DisablePain();
}

/*
===============
idActor::Script_EnablePain
===============
*/
void idActor::Script_EnablePain( void ) {
	EnablePain();
}

/*
=====================
idActor::Script_GetPainAnim
=====================
*/
void idActor::Script_GetPainAnim( void ) {
	idThread::ReturnString( GetPainAnim() );
}

/*
=====================
idActor::Script_SetAnimPrefix
=====================
*/
void idActor::Script_SetAnimPrefix( const char *prefix ) {
	SetAnimPrefix( prefix );
}

/*
===============
idActor::Script_StopAnim
===============
*/
void idActor::Script_StopAnim( int channel, int frames ) {
	StopAnim( channel, frames );
}

/*
===============
idActor::Script_PlayAnim
===============
*/
void idActor::Script_PlayAnim( int channel, const char *animname ) {
	idThread::ReturnInt( PlayAnim( channel, animname ) );
}

/*
===============
idActor::Script_PlayCycle
===============
*/
void idActor::Script_PlayCycle( int channel, const char *animname ) {
	idThread::ReturnInt( PlayCycle( channel, animname ) );
}

/*
===============
idActor::Script_IdleAnim
===============
*/
void idActor::Script_IdleAnim( int channel, const char *animname ) {
	idThread::ReturnInt( IdleAnim( channel, animname ) );
}

/*
================
idActor::Script_SetSyncedAnimWeight
================
*/
void idActor::Script_SetSyncedAnimWeight( int channel, int anim, float weight ) {
	SetSyncedAnimWeight( channel, anim, weight );
}

/*
===============
idActor::Script_OverrideAnim
===============
*/
void idActor::Script_OverrideAnim( int channel ) {
	OverrideAnim( channel );
}

/*
===============
idActor::Script_EnableAnim
===============
*/
void idActor::Script_EnableAnim( int channel, int blendFrames ) {
	EnableAnim( channel, blendFrames );
}

/*
===============
idActor::Script_SetBlendFrames
===============
*/
void idActor::Script_SetBlendFrames( int channel, int blendFrames ) {
	SetBlendFrames( channel, blendFrames );
}

/*
===============
idActor::Script_GetBlendFrames
===============
*/
void idActor::Script_GetBlendFrames( int channel ) {
	idThread::ReturnInt( GetBlendFrames( channel ) );
}

/*
===============
idActor::Script_AnimState
===============
*/
void idActor::Script_AnimState( int channel, const char* statename, int blendFrames ) {
	SetAnimState( channel, statename, blendFrames );
}

/*
===============
idActor::Script_GetAnimState
===============
*/
void idActor::Script_GetAnimState( int channel ) {
	idThread::ReturnString( GetAnimState( channel ) );
}

/*
===============
idActor::Script_InAnimState
===============
*/
void idActor::Script_InAnimState( int channel, const char *statename ) {
	idThread::ReturnInt( InAnimState( channel, statename ) );
}

/*
===============
idActor::Script_FinishAction
===============
*/
void idActor::Script_FinishAction( const char *actionname ) {
	FinishAction( actionname );
}

/*
===============
idActor::Script_AnimDone
===============
*/
void idActor::Script_AnimDone( int channel, int blendFrames ) {
	idThread::ReturnInt( AnimDone( channel, blendFrames ) );
}

/*
================
idActor::Script_HasAnim
================
*/
void idActor::Script_HasAnim( int channel, const char *animname ) {
	idThread::ReturnFloat( HasAnim( channel, animname ) );
}

/*
================
idActor::Script_CheckAnim
================
*/
void idActor::Script_CheckAnim( int channel, const char* animname) {
	CheckAnim( channel, animname );
}

/*
================
idActor::Script_ChooseAnim
================
*/
void idActor::Script_ChooseAnim( int channel, const char *animname ) {
	idThread::ReturnString( ChooseAnim( channel, animname ) );
}

/*
================
idActor::Script_AnimLength
================
*/
void idActor::Script_AnimLength( int channel, const char *animname ) {
	idThread::ReturnFloat( AnimLength( channel, animname ) );
}

/*
================
idActor::Script_AnimDistance
================
*/
void idActor::Script_AnimDistance( int channel, const char *animname ) {
	idThread::ReturnFloat( AnimDistance( channel, animname ) );
}

/*
================
idActor::Script_HasEnemies
================
*/
void idActor::Script_HasEnemies( void ) {
	idThread::ReturnInt( HasEnemies() );
}

/*
================
idActor::Script_NextEnemy
================
*/
void idActor::Script_NextEnemy( idEntity *ent ) {
	idThread::ReturnEntity( NextEnemy( ent ) );
}

/*
================
idActor::Script_ClosestEnemyToPoint
================
*/
void idActor::Script_ClosestEnemyToPoint( const idVec3 &pos ) {
	idThread::ReturnEntity( ClosestEnemyToPoint( pos ) );
}

/*
================
idActor::Script_StopSound
================
*/
void idActor::Script_StopSound( int channel, int netSync ) {
	StopSound( channel, netSync );
}

/*
================
idActor::Script_StopSound
================
*/
void idActor::Script_SetNextState( const char *name ) {
	SetNextState( name );
}

/*
================
idActor::Script_SetState
================
*/
void idActor::Script_SetState( const char *name ) {
	ScriptSetState( name );
}

/*
================
idActor::Script_GetState
================
*/
void idActor::Script_GetState( void ) {
	idThread::ReturnString( GetState() );
}

/*
================
idActor::Script_GetHead
================
*/
void idActor::Script_GetHead( void ) {
	idThread::ReturnEntity( GetHead() );
}

#ifdef _D3XP
/*
================
idActor::Script_SetDamageGroupScale
================
*/
void idActor::Script_SetDamageGroupScale( const char *groupName, float scale ) {
	SetDamageGroupScale( groupName, scale );
}

/*
================
idActor::Script_SetDamageGroupScaleAll
================
*/
void idActor::Script_SetDamageGroupScaleAll( float scale ) {
	SetDamageGroupScaleAll( scale );
}

/*
================
idActor::Script_GetDamageGroupScale
================
*/
void idActor::Script_GetDamageGroupScale( const char *groupName ) {
	idThread::ReturnFloat( GetDamageGroupScale( groupName ) );
}

/*
================
idActor::Script_SetDamageCap
================
*/
void idActor::Script_SetDamageCap( float _damageCap ) {
	SetDamageCap( _damageCap );
}

/*
================
idActor::Script_SetWaitState
================
*/
void idActor::Script_SetWaitState( const char *waitState ) {
	SetWaitState( waitState );
}

/*
================
idActor::Script_GetWaitState
================
*/
void idActor::Script_GetWaitState( void ) {
	idThread::ReturnString( GetWaitState() );
}
#endif
