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

#include "../Game_local.h"

/***********************************************************************

	AI Events

***********************************************************************/

const idEventDef AI_FindEnemy( "findEnemy", "d", 'e' );
const idEventDef AI_FindEnemyAI( "findEnemyAI", "d", 'e' );
const idEventDef AI_FindEnemyInCombatNodes( "findEnemyInCombatNodes", NULL, 'e' );
const idEventDef AI_ClosestReachableEnemyOfEntity( "closestReachableEnemyOfEntity", "E", 'e' );
const idEventDef AI_HeardSound( "heardSound", "d", 'e' );
const idEventDef AI_SetEnemy( "setEnemy", "E" );
const idEventDef AI_ClearEnemy( "clearEnemy" );
const idEventDef AI_MuzzleFlash( "muzzleFlash", "s" );
const idEventDef AI_CreateMissile( "createMissile", "s", 'e' );
const idEventDef AI_AttackMissile( "attackMissile", "s", 'e' );
const idEventDef AI_FireMissileAtTarget( "fireMissileAtTarget", "ss", 'e' );
const idEventDef AI_LaunchMissile( "launchMissile", "vv", 'e' );
#ifdef _D3XP
const idEventDef AI_LaunchProjectile( "launchProjectile", "s" );
#endif
const idEventDef AI_AttackMelee( "attackMelee", "s", 'd' );
const idEventDef AI_DirectDamage( "directDamage", "es" );
const idEventDef AI_RadiusDamageFromJoint( "radiusDamageFromJoint", "ss" );
const idEventDef AI_BeginAttack( "attackBegin", "s" );
const idEventDef AI_EndAttack( "attackEnd" );
const idEventDef AI_MeleeAttackToJoint( "meleeAttackToJoint", "ss", 'd' );
const idEventDef AI_RandomPath( "randomPath", NULL, 'e' );
const idEventDef AI_CanBecomeSolid( "canBecomeSolid", NULL, 'f' );
const idEventDef AI_BecomeSolid( "becomeSolid" );
const idEventDef AI_BecomeRagdoll( "becomeRagdoll", NULL, 'd' );
const idEventDef AI_StopRagdoll( "stopRagdoll" );
const idEventDef AI_SetHealth( "setHealth", "f" );
const idEventDef AI_GetHealth( "getHealth", NULL, 'f' );
const idEventDef AI_AllowDamage( "allowDamage" );
const idEventDef AI_IgnoreDamage( "ignoreDamage" );
const idEventDef AI_GetCurrentYaw( "getCurrentYaw", NULL, 'f' );
const idEventDef AI_TurnTo( "turnTo", "f" );
const idEventDef AI_TurnToPos( "turnToPos", "v" );
const idEventDef AI_TurnToEntity( "turnToEntity", "E" );
const idEventDef AI_MoveStatus( "moveStatus", NULL, 'd' );
const idEventDef AI_StopMove( "stopMove" );
const idEventDef AI_MoveToCover( "moveToCover" );
const idEventDef AI_MoveToEnemy( "moveToEnemy" );
const idEventDef AI_MoveToEnemyHeight( "moveToEnemyHeight" );
const idEventDef AI_MoveOutOfRange( "moveOutOfRange", "ef" );
const idEventDef AI_MoveToAttackPosition( "moveToAttackPosition", "es" );
const idEventDef AI_Wander( "wander" );
const idEventDef AI_MoveToEntity( "moveToEntity", "e" );
const idEventDef AI_MoveToPosition( "moveToPosition", "v" );
const idEventDef AI_SlideTo( "slideTo", "vf" );
const idEventDef AI_FacingIdeal( "facingIdeal", NULL, 'd' );
const idEventDef AI_FaceEnemy( "faceEnemy" );
const idEventDef AI_FaceEntity( "faceEntity", "E" );
const idEventDef AI_GetCombatNode( "getCombatNode", NULL, 'e' );
const idEventDef AI_EnemyInCombatCone( "enemyInCombatCone", "Ed", 'd' );
const idEventDef AI_WaitMove( "waitMove" );
const idEventDef AI_GetJumpVelocity( "getJumpVelocity", "vff", 'v' );
const idEventDef AI_EntityInAttackCone( "entityInAttackCone", "E", 'd' );
const idEventDef AI_CanSeeEntity( "canSee", "E", 'd' );
const idEventDef AI_SetTalkTarget( "setTalkTarget", "E" );
const idEventDef AI_GetTalkTarget( "getTalkTarget", NULL, 'e' );
const idEventDef AI_SetTalkState( "setTalkState", "d" );
const idEventDef AI_EnemyRange( "enemyRange", NULL, 'f' );
const idEventDef AI_EnemyRange2D( "enemyRange2D", NULL, 'f' );
const idEventDef AI_GetEnemy( "getEnemy", NULL, 'e' );
const idEventDef AI_GetEnemyPos( "getEnemyPos", NULL, 'v' );
const idEventDef AI_GetEnemyEyePos( "getEnemyEyePos", NULL, 'v' );
const idEventDef AI_PredictEnemyPos( "predictEnemyPos", "f", 'v' );
const idEventDef AI_CanHitEnemy( "canHitEnemy", NULL, 'd' );
const idEventDef AI_CanHitEnemyFromAnim( "canHitEnemyFromAnim", "s", 'd' );
const idEventDef AI_CanHitEnemyFromJoint( "canHitEnemyFromJoint", "s", 'd' );
const idEventDef AI_EnemyPositionValid( "enemyPositionValid", NULL, 'd' );
const idEventDef AI_ChargeAttack( "chargeAttack", "s" );
const idEventDef AI_TestChargeAttack( "testChargeAttack", NULL, 'f' );
const idEventDef AI_TestMoveToPosition( "testMoveToPosition", "v", 'd' );
const idEventDef AI_TestAnimMoveTowardEnemy( "testAnimMoveTowardEnemy", "s", 'd' );
const idEventDef AI_TestAnimMove( "testAnimMove", "s", 'd' );
const idEventDef AI_TestMeleeAttack( "testMeleeAttack", NULL, 'd' );
const idEventDef AI_TestAnimAttack( "testAnimAttack", "s", 'd' );
const idEventDef AI_Shrivel( "shrivel", "f" );
const idEventDef AI_Burn( "burn" );
const idEventDef AI_ClearBurn( "clearBurn" );
const idEventDef AI_PreBurn( "preBurn" );
const idEventDef AI_SetSmokeVisibility( "setSmokeVisibility", "dd" );
const idEventDef AI_NumSmokeEmitters( "numSmokeEmitters", NULL, 'd' );
const idEventDef AI_WaitAction( "waitAction", "s" );
const idEventDef AI_StopThinking( "stopThinking" );
const idEventDef AI_GetTurnDelta( "getTurnDelta", NULL, 'f' );
const idEventDef AI_GetMoveType( "getMoveType", NULL, 'd' );
const idEventDef AI_SetMoveType( "setMoveType", "d" );
const idEventDef AI_SaveMove( "saveMove" );
const idEventDef AI_RestoreMove( "restoreMove" );
const idEventDef AI_AllowMovement( "allowMovement", "f" );
const idEventDef AI_JumpFrame( "<jumpframe>" );
const idEventDef AI_EnableClip( "enableClip" );
const idEventDef AI_DisableClip( "disableClip" );
const idEventDef AI_EnableGravity( "enableGravity" );
const idEventDef AI_DisableGravity( "disableGravity" );
const idEventDef AI_EnableAFPush( "enableAFPush" );
const idEventDef AI_DisableAFPush( "disableAFPush" );
const idEventDef AI_SetFlySpeed( "setFlySpeed", "f" );
const idEventDef AI_SetFlyOffset( "setFlyOffset", "d" );
const idEventDef AI_ClearFlyOffset( "clearFlyOffset" );
const idEventDef AI_GetClosestHiddenTarget( "getClosestHiddenTarget", "s", 'e' );
const idEventDef AI_GetRandomTarget( "getRandomTarget", "s", 'e' );
const idEventDef AI_TravelDistanceToPoint( "travelDistanceToPoint", "v", 'f' );
const idEventDef AI_TravelDistanceToEntity( "travelDistanceToEntity", "e", 'f' );
const idEventDef AI_TravelDistanceBetweenPoints( "travelDistanceBetweenPoints", "vv", 'f' );
const idEventDef AI_TravelDistanceBetweenEntities( "travelDistanceBetweenEntities", "ee", 'f' );
const idEventDef AI_LookAtEntity( "lookAt", "Ef" );
const idEventDef AI_LookAtEnemy( "lookAtEnemy", "f" );
const idEventDef AI_SetJointMod( "setBoneMod", "d" );
const idEventDef AI_ThrowMoveable( "throwMoveable" );
const idEventDef AI_ThrowAF( "throwAF" );
const idEventDef AI_RealKill( "<kill>" );
const idEventDef AI_Kill( "kill" );
const idEventDef AI_WakeOnFlashlight( "wakeOnFlashlight", "d" );
const idEventDef AI_LocateEnemy( "locateEnemy" );
const idEventDef AI_KickObstacles( "kickObstacles", "Ef" );
const idEventDef AI_GetObstacle( "getObstacle", NULL, 'e' );
const idEventDef AI_PushPointIntoAAS( "pushPointIntoAAS", "v", 'v' );
const idEventDef AI_GetTurnRate( "getTurnRate", NULL, 'f' );
const idEventDef AI_SetTurnRate( "setTurnRate", "f" );
const idEventDef AI_AnimTurn( "animTurn", "f" );
const idEventDef AI_AllowHiddenMovement( "allowHiddenMovement", "d" );
const idEventDef AI_TriggerParticles( "triggerParticles", "s" );
const idEventDef AI_FindActorsInBounds( "findActorsInBounds", "vv", 'e' );
const idEventDef AI_CanReachPosition( "canReachPosition", "v", 'd' );
const idEventDef AI_CanReachEntity( "canReachEntity", "E", 'd' );
const idEventDef AI_CanReachEnemy( "canReachEnemy", NULL, 'd' );
const idEventDef AI_GetReachableEntityPosition( "getReachableEntityPosition", "e", 'v' );
#ifdef _D3XP
const idEventDef AI_MoveToPositionDirect( "moveToPositionDirect", "v" );
const idEventDef AI_AvoidObstacles( "avoidObstacles", "d" );
const idEventDef AI_TriggerFX( "triggerFX", "ss" );
const idEventDef AI_StartEmitter( "startEmitter", "sss", 'e' );
const idEventDef AI_GetEmitter( "getEmitter", "s", 'e' );
const idEventDef AI_StopEmitter( "stopEmitter", "s" );
#endif

CLASS_DECLARATION( idActor, idAI )
	EVENT( EV_Activate,							idAI::Script_Activate )
	EVENT( EV_Touch,							idAI::Script_Touch )
	EVENT( AI_FindEnemy,						idAI::Script_FindEnemy )
	EVENT( AI_FindEnemyAI,						idAI::Script_FindEnemyAI )
	EVENT( AI_FindEnemyInCombatNodes,			idAI::Script_FindEnemyInCombatNodes )
	EVENT( AI_ClosestReachableEnemyOfEntity,	idAI::Script_ClosestReachableEnemyOfEntity )
	EVENT( AI_HeardSound,						idAI::Script_HeardSound )
	EVENT( AI_SetEnemy,							idAI::Script_SetEnemy )
	EVENT( AI_ClearEnemy,						idAI::Script_ClearEnemy )
	EVENT( AI_MuzzleFlash,						idAI::Script_MuzzleFlash )
	EVENT( AI_CreateMissile,					idAI::Script_CreateMissile )
	EVENT( AI_AttackMissile,					idAI::Script_AttackMissile )
	EVENT( AI_FireMissileAtTarget,				idAI::Script_FireMissileAtTarget )
	EVENT( AI_LaunchMissile,					idAI::Script_LaunchMissile )
#ifdef _D3XP
	EVENT( AI_LaunchProjectile,					idAI::Script_LaunchProjectile )
#endif
	EVENT( AI_AttackMelee,						idAI::Script_AttackMelee )
	EVENT( AI_DirectDamage,						idAI::Script_DirectDamage )
	EVENT( AI_RadiusDamageFromJoint,			idAI::Script_RadiusDamageFromJoint )
	EVENT( AI_BeginAttack,						idAI::Script_BeginAttack )
	EVENT( AI_EndAttack,						idAI::Script_EndAttack )
	EVENT( AI_MeleeAttackToJoint,				idAI::Script_MeleeAttackToJoint )
	EVENT( AI_RandomPath,						idAI::Script_RandomPath )
	EVENT( AI_CanBecomeSolid,					idAI::Script_CanBecomeSolid )
	EVENT( AI_BecomeSolid,						idAI::Script_BecomeSolid )
	EVENT( EV_BecomeNonSolid,					idAI::Script_BecomeNonSolid )
	EVENT( AI_BecomeRagdoll,					idAI::Script_BecomeRagdoll )
	EVENT( AI_StopRagdoll,						idAI::Script_StopRagdoll )
	EVENT( AI_SetHealth,						idAI::Script_SetHealth )
	EVENT( AI_GetHealth,						idAI::Script_GetHealth )
	EVENT( AI_AllowDamage,						idAI::Script_AllowDamage )
	EVENT( AI_IgnoreDamage,						idAI::Script_IgnoreDamage )
	EVENT( AI_GetCurrentYaw,					idAI::Script_GetCurrentYaw )
	EVENT( AI_TurnTo,							idAI::Script_TurnTo )
	EVENT( AI_TurnToPos,						idAI::Script_TurnToPos )
	EVENT( AI_TurnToEntity,						idAI::Script_TurnToEntity )
	EVENT( AI_MoveStatus,						idAI::Script_MoveStatus )
	EVENT( AI_StopMove,							idAI::Script_StopMove )
	EVENT( AI_MoveToCover,						idAI::Script_MoveToCover )
	EVENT( AI_MoveToEnemy,						idAI::Script_MoveToEnemy )
	EVENT( AI_MoveToEnemyHeight,				idAI::Script_MoveToEnemyHeight )
	EVENT( AI_MoveOutOfRange,					idAI::Script_MoveOutOfRange )
	EVENT( AI_MoveToAttackPosition,				idAI::Script_MoveToAttackPosition )
	EVENT( AI_Wander,							idAI::Script_Wander )
	EVENT( AI_MoveToEntity,						idAI::Script_MoveToEntity )
	EVENT( AI_MoveToPosition,					idAI::Script_MoveToPosition )
	EVENT( AI_SlideTo,							idAI::Script_SlideTo )
	EVENT( AI_FacingIdeal,						idAI::Script_FacingIdeal )
	EVENT( AI_FaceEnemy,						idAI::Script_FaceEnemy )
	EVENT( AI_FaceEntity,						idAI::Script_FaceEntity )
	EVENT( AI_WaitAction,						idAI::Script_WaitAction )
	EVENT( AI_GetCombatNode,					idAI::Script_GetCombatNode )
	EVENT( AI_EnemyInCombatCone,				idAI::Script_EnemyInCombatCone )
	EVENT( AI_WaitMove,							idAI::Script_WaitMove )
	EVENT( AI_GetJumpVelocity,					idAI::Script_GetJumpVelocity )
	EVENT( AI_EntityInAttackCone,				idAI::Script_EntityInAttackCone )
	EVENT( AI_CanSeeEntity,						idAI::Script_CanSeeEntity )
	EVENT( AI_SetTalkTarget,					idAI::Script_SetTalkTarget )
	EVENT( AI_GetTalkTarget,					idAI::Script_GetTalkTarget )
	EVENT( AI_SetTalkState,						idAI::Script_SetTalkState )
	EVENT( AI_EnemyRange,						idAI::Script_EnemyRange )
	EVENT( AI_EnemyRange2D,						idAI::Script_EnemyRange2D )
	EVENT( AI_GetEnemy,							idAI::Script_GetEnemy )
	EVENT( AI_GetEnemyPos,						idAI::Script_GetEnemyPos )
	EVENT( AI_GetEnemyEyePos,					idAI::Script_GetEnemyEyePos )
	EVENT( AI_PredictEnemyPos,					idAI::Script_PredictEnemyPos )
	EVENT( AI_CanHitEnemy,						idAI::Script_CanHitEnemy )
	EVENT( AI_CanHitEnemyFromAnim,				idAI::Script_CanHitEnemyFromAnim )
	EVENT( AI_CanHitEnemyFromJoint,				idAI::Script_CanHitEnemyFromJoint )
	EVENT( AI_EnemyPositionValid,				idAI::Script_EnemyPositionValid )
	EVENT( AI_ChargeAttack,						idAI::Script_ChargeAttack )
	EVENT( AI_TestChargeAttack,					idAI::Script_TestChargeAttack )
	EVENT( AI_TestAnimMoveTowardEnemy,			idAI::Script_TestAnimMoveTowardEnemy )
	EVENT( AI_TestAnimMove,						idAI::Script_TestAnimMove )
	EVENT( AI_TestMoveToPosition,				idAI::Script_TestMoveToPosition )
	EVENT( AI_TestMeleeAttack,					idAI::Script_TestMeleeAttack )
	EVENT( AI_TestAnimAttack,					idAI::Script_TestAnimAttack )
	EVENT( AI_Shrivel,							idAI::Script_Shrivel )
	EVENT( AI_Burn,								idAI::Script_Burn )
	EVENT( AI_PreBurn,							idAI::Script_PreBurn )
	EVENT( AI_SetSmokeVisibility,				idAI::Script_SetSmokeVisibility )
	EVENT( AI_NumSmokeEmitters,					idAI::Script_NumSmokeEmitters )
	EVENT( AI_ClearBurn,						idAI::Script_ClearBurn )
	EVENT( AI_StopThinking,						idAI::Script_StopThinking )
	EVENT( AI_GetTurnDelta,						idAI::Script_GetTurnDelta )
	EVENT( AI_GetMoveType,						idAI::Script_GetMoveType )
	EVENT( AI_SetMoveType,						idAI::Script_SetMoveType )
	EVENT( AI_SaveMove,							idAI::Script_SaveMove )
	EVENT( AI_RestoreMove,						idAI::Script_RestoreMove )
	EVENT( AI_AllowMovement,					idAI::Script_AllowMovement )
	EVENT( AI_JumpFrame,						idAI::Script_JumpFrame )
	EVENT( AI_EnableClip,						idAI::Script_EnableClip )
	EVENT( AI_DisableClip,						idAI::Script_DisableClip )
	EVENT( AI_EnableGravity,					idAI::Script_EnableGravity )
	EVENT( AI_DisableGravity,					idAI::Script_DisableGravity )
	EVENT( AI_EnableAFPush,						idAI::Script_EnableAFPush )
	EVENT( AI_DisableAFPush,					idAI::Script_DisableAFPush )
	EVENT( AI_SetFlySpeed,						idAI::Script_SetFlySpeed )
	EVENT( AI_SetFlyOffset,						idAI::Script_SetFlyOffset )
	EVENT( AI_ClearFlyOffset,					idAI::Script_ClearFlyOffset )
	EVENT( AI_GetClosestHiddenTarget,			idAI::Script_GetClosestHiddenTarget )
	EVENT( AI_GetRandomTarget,					idAI::Script_GetRandomTarget )
	EVENT( AI_TravelDistanceToPoint,			idAI::Script_TravelDistanceToPoint )
	EVENT( AI_TravelDistanceToEntity,			idAI::Script_TravelDistanceToEntity )
	EVENT( AI_TravelDistanceBetweenPoints,		idAI::Script_TravelDistanceBetweenPoints )
	EVENT( AI_TravelDistanceBetweenEntities,	idAI::Script_TravelDistanceBetweenEntities )
	EVENT( AI_LookAtEntity,						idAI::Script_LookAtEntity )
	EVENT( AI_LookAtEnemy,						idAI::Script_LookAtEnemy )
	EVENT( AI_SetJointMod,						idAI::Script_SetJointMod )
	EVENT( AI_ThrowMoveable,					idAI::Script_ThrowMoveable )
	EVENT( AI_ThrowAF,							idAI::Script_ThrowAF )
	EVENT( EV_GetAngles,						idAI::Script_GetAngles )
	EVENT( EV_SetAngles,						idAI::Script_SetAngles )
	EVENT( AI_RealKill,							idAI::Script_RealKill )
	EVENT( AI_Kill,								idAI::Script_Kill )
	EVENT( AI_WakeOnFlashlight,					idAI::Script_WakeOnFlashlight )
	EVENT( AI_LocateEnemy,						idAI::Script_LocateEnemy )
	EVENT( AI_KickObstacles,					idAI::Script_KickObstacles )
	EVENT( AI_GetObstacle,						idAI::Script_GetObstacle )
	EVENT( AI_PushPointIntoAAS,					idAI::Script_PushPointIntoAAS )
	EVENT( AI_GetTurnRate,						idAI::Script_GetTurnRate )
	EVENT( AI_SetTurnRate,						idAI::Script_SetTurnRate )
	EVENT( AI_AnimTurn,							idAI::Script_AnimTurn )
	EVENT( AI_AllowHiddenMovement,				idAI::Script_AllowHiddenMovement )
	EVENT( AI_TriggerParticles,					idAI::Script_TriggerParticles )
	EVENT( AI_FindActorsInBounds,				idAI::Script_FindActorsInBounds )
	EVENT( AI_CanReachPosition,					idAI::Script_CanReachPosition )
	EVENT( AI_CanReachEntity,					idAI::Script_CanReachEntity )
	EVENT( AI_CanReachEnemy,					idAI::Script_CanReachEnemy )
	EVENT( AI_GetReachableEntityPosition,		idAI::Script_GetReachableEntityPosition )
#ifdef _D3XP
	EVENT( AI_MoveToPositionDirect,				idAI::Script_MoveToPositionDirect )
	EVENT( AI_AvoidObstacles,					idAI::Script_AvoidObstacles )
	EVENT( AI_TriggerFX,						idAI::Script_TriggerFX )
	EVENT( AI_StartEmitter,						idAI::Script_StartEmitter )
	EVENT( AI_GetEmitter,						idAI::Script_GetEmitter )
	EVENT( AI_StopEmitter,						idAI::Script_StopEmitter )
#endif
END_CLASS

/*
=====================
idAI::Script_Activate
=====================
*/
void idAI::Script_Activate( idEntity *activator ) {
	Activate( activator );
}

/*
=====================
idAI::Script_Touch
=====================
*/
void idAI::Script_Touch( idEntity *other, trace_t *trace ) {
	Touch( other, trace );
}

/*
=====================
idAI::Script_FindEnemy
=====================
*/
void idAI::Script_FindEnemy( int useFOV ) {
	idEntity *result = FindEnemy( useFOV );
	idThread::ReturnEntity( result );
}

/*
=====================
idAI::Script_FindEnemyAI
=====================
*/
void idAI::Script_FindEnemyAI( int useFOV ) {
	idEntity *result = FindEnemyAI( useFOV );
	idThread::ReturnEntity( result );
}

/*
=====================
idAI::Script_FindEnemyInCombatNodes
=====================
*/
void idAI::Script_FindEnemyInCombatNodes( void ) {
	idEntity *result = FindEnemyInCombatNodes();
	idThread::ReturnEntity( result );
}

/*
=====================
idAI::Script_ClosestReachableEnemyOfEntity
=====================
*/
void idAI::Script_ClosestReachableEnemyOfEntity( idEntity *team_mate ) {
	idActor *result = ClosestReachableEnemyOfEntity( team_mate );
	idThread::ReturnEntity(result);
}

/*
=====================
idAI::Script_HeardSound
=====================
*/
void idAI::Script_HeardSound( int ignore_team ) {
	idEntity *result = HeardSound( ignore_team );
	idThread::ReturnEntity( result );
}

/*
=====================
idAI::Script_SetEnemy
=====================
*/
void idAI::Script_SetEnemy( idEntity *ent ) {
	SetEnemy( ent );
}

/*
=====================
idAI::Script_ClearEnemy
=====================
*/
void idAI::Script_ClearEnemy( void ) {
	ClearEnemy();
}

/*
=====================
idAI::Script_MuzzleFlash
=====================
*/
void idAI::Script_MuzzleFlash( const char *jointname ) {
	MuzzleFlash( jointname );
}

/*
=====================
idAI::Script_CreateMissile
=====================
*/
void idAI::Script_CreateMissile( const char *jointname ) {
	idEntity *result = CreateMissile( jointname );
	idThread::ReturnEntity( result );
}

/*
=====================
idAI::Script_AttackMissile
=====================
*/
void idAI::Script_AttackMissile( const char *jointname ) {
	idProjectile *result = AttackMissile( jointname );
	idThread::ReturnEntity( result );
}

/*
=====================
idAI::Script_FireMissileAtTarget
=====================
*/
void idAI::Script_FireMissileAtTarget( const char *jointname, const char *targetname ) {
	idProjectile *result = FireMissileAtTarget( jointname, targetname );
	idThread::ReturnEntity( result );
}

/*
=====================
idAI::Script_LaunchMissile
=====================
*/
void idAI::Script_LaunchMissile( const idVec3 &org, const idAngles &ang ) {
	idEntity *result = LaunchMissile( org, ang );
	idThread::ReturnEntity( result );
}

#ifdef _D3XP
/*
=====================
idAI::Script_LaunchProjectile
=====================
*/
void idAI::Script_LaunchProjectile( const char *entityDefName ) {
	LaunchProjectile( entityDefName );
}
#endif

/*
=====================
idAI::Script_AttackMelee
=====================
*/
void idAI::Script_AttackMelee( const char *meleeDefName ) {
	bool result = AttackMelee( meleeDefName );
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_DirectDamage
=====================
*/
void idAI::Script_DirectDamage( idEntity *damageTarget, const char *damageDefName ) {
	DirectDamage(damageTarget, damageDefName);
}

/*
=====================
idAI::Script_RadiusDamageFromJoint
=====================
*/
void idAI::Script_RadiusDamageFromJoint( const char *jointname, const char *damageDefName ) {
	RadiusDamageFromJoint( jointname, damageDefName );
}

/*
=====================
idAI::Script_RandomPath
=====================
*/
void idAI::Script_RandomPath( void ) {
	idPathCorner *result = RandomPath();
	idThread::ReturnEntity( result );
}

/*
=====================
idAI::Script_BeginAttack
=====================
*/
void idAI::Script_BeginAttack( const char *name ) {
	BeginAttack( name );
}

/*
=====================
idAI::Script_EndAttack
=====================
*/
void idAI::Script_EndAttack( void ) {
	EndAttack();
}

/*
=====================
idAI::Script_MeleeAttackToJoint
=====================
*/
void idAI::Script_MeleeAttackToJoint( const char *jointname, const char *meleeDefName ) {
	bool result = MeleeAttackToJoint( jointname, meleeDefName );
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_CanBecomeSolid
=====================
*/
void idAI::Script_CanBecomeSolid( void ) {
	bool result = CanBecomeSolid();
	idThread::ReturnInt(result);
}

/*
=====================
idAI::Script_BecomeSolid
=====================
*/
void idAI::Script_BecomeSolid( void ) {
	BecomeSolid();
}

/*
=====================
idAI::Script_BecomeNonSolid
=====================
*/
void idAI::Script_BecomeNonSolid( void ) {
	BecomeNonSolid();
}

/*
=====================
idAI::Script_BecomeRagdoll
=====================
*/
void idAI::Script_BecomeRagdoll( void ) {
	bool result = BecomeRagdoll();
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_StopRagdoll
=====================
*/
void idAI::Script_StopRagdoll( void ) {
	StopRagdoll();
}

/*
=====================
idAI::Script_SetHealth
=====================
*/
void idAI::Script_SetHealth( float newHealth ) {
	SetHealth( newHealth );
}

/*
=====================
idAI::Script_GetHealth
=====================
*/
void idAI::Script_GetHealth( void ) {
	float result = GetHealth();
	idThread::ReturnFloat( result );
}

/*
=====================
idAI::Script_AllowDamage
=====================
*/
void idAI::Script_AllowDamage( void ) {
	AllowDamage();
}

/*
=====================
idAI::Script_IgnoreDamage
=====================
*/
void idAI::Script_IgnoreDamage( void ) {
	IgnoreDamage();
}

/*
=====================
idAI::Script_GetCurrentYaw
=====================
*/
void idAI::Script_GetCurrentYaw( void ) {
	float result = GetCurrentYaw();
	idThread::ReturnFloat( result );
}

/*
=====================
idAI::Script_TurnTo
=====================
*/
void idAI::Script_TurnTo( float angle ) {
	TurnTo( angle );
}

/*
=====================
idAI::Script_TurnTo
=====================
*/
void idAI::Script_TurnToPos( const idVec3 &pos ) {
	TurnToPos( pos );
}

/*
=====================
idAI::Script_TurnToEntity
=====================
*/
void idAI::Script_TurnToEntity( idEntity *ent ) {
	TurnToEntity( ent );
}

/*
=====================
idAI::Script_MoveStatus
=====================
*/
void idAI::Script_MoveStatus( void ) {
	int result = MoveStatus();
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_StopMove
=====================
*/
void idAI::Script_StopMove( void ) {
	StopMove();
}

/*
=====================
idAI::Script_MoveToCover
=====================
*/
void idAI::Script_MoveToCover( void ) {
	MoveToCover();
}

/*
=====================
idAI::Script_MoveToEnemy
=====================
*/
void idAI::Script_MoveToEnemy( void ) {
	MoveToEnemy();
}

/*
=====================
idAI::Script_MoveToEnemyHeight
=====================
*/
void idAI::Script_MoveToEnemyHeight( void ) {
	MoveToEnemyHeight();
}

/*
=====================
idAI::Script_MoveOutOfRange
=====================
*/
void idAI::Script_MoveOutOfRange( idEntity *entity, float range ) {
	MoveOutOfRange( entity, range );
}

/*
=====================
idAI::Script_MoveToAttackPosition
=====================
*/
void idAI::Script_MoveToAttackPosition( idEntity *entity, const char *attack_anim ) {
	MoveToAttackPosition( entity, attack_anim );
}

/*
=====================
idAI::Script_MoveToEntity
=====================
*/
void idAI::Script_MoveToEntity( idEntity *ent ) {
	MoveToEntity( ent );
}

/*
=====================
idAI::Script_MoveToPosition
=====================
*/
void idAI::Script_MoveToPosition( const idVec3 &pos ) {
	MoveToPosition( pos );
}

/*
=====================
idAI::Script_SlideTo
=====================
*/
void idAI::Script_SlideTo( const idVec3 &pos, float time ) {
	SlideTo( pos, time );
}

/*
=====================
idAI::Script_Wander
=====================
*/
void idAI::Script_Wander( void ) {
	Wander();
}

/*
=====================
idAI::Script_FacingIdeal
=====================
*/
void idAI::Script_FacingIdeal( void ) {
	bool result = FacingIdeal();
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_FaceEnemy
=====================
*/
void idAI::Script_FaceEnemy( void ) {
	FaceEnemy();
}

/*
=====================
idAI::Script_FaceEnemy
=====================
*/
void idAI::Script_FaceEntity( idEntity *ent ) {
	FaceEntity( ent );
}

/*
=====================
idAI::Script_WaitAction
=====================
*/
void idAI::Script_WaitAction( const char *waitForState ) {
	if ( idThread::BeginMultiFrameEvent( this, &AI_WaitAction ) ) {
		SetWaitState( waitForState );
	}

	if ( !WaitState() ) {
		idThread::EndMultiFrameEvent( this, &AI_WaitAction );
	}
}

/*
=====================
idAI::Script_GetCombatNode
=====================
*/
void idAI::Script_GetCombatNode( void ) {
	idCombatNode *result = GetCombatNode();
	idThread::ReturnEntity( result );
}

/*
=====================
idAI::Script_EnemyInCombatCone
=====================
*/
void idAI::Script_EnemyInCombatCone( idEntity *ent, int use_current_enemy_location ) {
	bool result = EnemyInCombatCone( ent, use_current_enemy_location );
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_WaitMove
=====================
*/
void idAI::Script_WaitMove( void ) {
	idThread::BeginMultiFrameEvent( this, &AI_WaitMove );

	if ( MoveDone() ) {
		idThread::EndMultiFrameEvent( this, &AI_WaitMove );
	}
}

/*
=====================
idAI::Script_GetJumpVelocity
=====================
*/
void idAI::Script_GetJumpVelocity( const idVec3 &pos, float speed, float max_height ) {
	idVec3 result = GetJumpVelocity( pos, speed, max_height );
	idThread::ReturnVector( result );
}

/*
=====================
idAI::Script_EntityInAttackCone
=====================
*/
void idAI::Script_EntityInAttackCone( idEntity *ent ) {
	bool result = EntityInAttackCone( ent );
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_CanSeeEntity
=====================
*/
void idAI::Script_CanSeeEntity( idEntity *ent ) {
	bool result = CanSeeEntity( ent );
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_SetTalkTarget
=====================
*/
void idAI::Script_SetTalkTarget( idEntity *target ) {
	SetTalkTarget( target );
}

/*
=====================
idAI::Script_GetTalkTarget
=====================
*/
void idAI::Script_GetTalkTarget( void ) {
	idEntity *result = GetTalkTarget();
	idThread::ReturnEntity( result );
}

/*
=====================
idAI::Script_SetTalkState
=====================
*/
void idAI::Script_SetTalkState( int state ) {
	SetTalkState( state );
}

/*
=====================
idAI::Script_EnemyRange
=====================
*/
void idAI::Script_EnemyRange( void ) {
	float result = EnemyRange();
	idThread::ReturnFloat( result );
}

/*
=====================
idAI::Script_EnemyRange2D
=====================
*/
void idAI::Script_EnemyRange2D( void ) {
	float result = EnemyRange2D();
	idThread::ReturnFloat( result );
}

/*
=====================
idAI::Script_GetEnemy
=====================
*/
void idAI::Script_GetEnemy( void ) {
	idEntity *result = GetEnemy();
	idThread::ReturnEntity( result );
}

/*
=====================
idAI::Script_GetEnemyPos
=====================
*/
void idAI::Script_GetEnemyPos( void ) {
	idVec3 result = GetEnemyPos();
	idThread::ReturnVector( result );
}

/*
=====================
idAI::Script_GetEnemyEyePos
=====================
*/
void idAI::Script_GetEnemyEyePos( void ) {
	idVec3 result = GetEnemyEyePos();
	idThread::ReturnVector( result );
}

/*
=====================
idAI::Script_PredictEnemyPos
=====================
*/
void idAI::Script_PredictEnemyPos( float time ) {
	idVec3 result = PredictEnemyPos( time );
	idThread::ReturnVector( result );
}

/*
=====================
idAI::Script_CanHitEnemy
=====================
*/
void idAI::Script_CanHitEnemy( void ) {
	bool result = CanHitEnemy();
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_CanHitEnemyFromAnim
=====================
*/
void idAI::Script_CanHitEnemyFromAnim( const char *animname ) {
	bool result = CanHitEnemyFromAnim( animname );
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_CanHitEnemyFromJoint
=====================
*/
void idAI::Script_CanHitEnemyFromJoint( const char *jointname ) {
	bool result = CanHitEnemyFromJoint( jointname );
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_EnemyPositionValid
=====================
*/
void idAI::Script_EnemyPositionValid( void ) {
	bool result = EnemyPositionValid();
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_ChargeAttack
=====================
*/
void idAI::Script_ChargeAttack( const char *damageDef ) {
	ChargeAttack( damageDef );
}

/*
=====================
idAI::Script_TestChargeAttack
=====================
*/
void idAI::Script_TestChargeAttack( void ) {
	float result = TestChargeAttack();
	idThread::ReturnFloat( result );
}

/*
=====================
idAI::Script_TestAnimMoveTowardEnemy
=====================
*/
void idAI::Script_TestAnimMoveTowardEnemy( const char *animname ) {
	bool result = TestAnimMoveTowardEnemy( animname );
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_TestAnimMove
=====================
*/
void idAI::Script_TestAnimMove( const char *animname ) {
	bool result = TestAnimMove( animname );
	idThread::ReturnInt(result);
}

/*
=====================
idAI::Script_TestMoveToPosition
=====================
*/
void idAI::Script_TestMoveToPosition( const idVec3 &position ) {
	bool result = TestMoveToPosition( position );
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_TestMeleeAttack
=====================
*/
void idAI::Script_TestMeleeAttack( void ) {
	bool result = TestMeleeAttack();
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_TestAnimAttack
=====================
*/
void idAI::Script_TestAnimAttack( const char *animname ) {
	bool result = TestAnimAttack( animname );
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_Shrivel
=====================
*/
void idAI::Script_Shrivel( float shrivel_time ) {
	float t;

	if ( idThread::BeginMultiFrameEvent( this, &AI_Shrivel ) ) {
		if ( shrivel_time <= 0.0f ) {
			idThread::EndMultiFrameEvent( this, &AI_Shrivel );
			return;
		}

		shrivel_rate = 0.001f / shrivel_time;
		shrivel_start = gameLocal.time;
	}

	t = ( gameLocal.time - shrivel_start ) * shrivel_rate;
	if ( t > 0.25f ) {
		renderEntity.noShadow = true;
	}
	if ( t > 1.0f ) {
		t = 1.0f;
		idThread::EndMultiFrameEvent( this, &AI_Shrivel );
	}

	renderEntity.shaderParms[ SHADERPARM_MD5_SKINSCALE ] = 1.0f - t * 0.5f;
	UpdateVisuals();
}

/*
=====================
idAI::Script_PreBurn
=====================
*/
void idAI::Script_PreBurn( void ) {
	PreBurn();
}

/*
=====================
idAI::Script_Burn
=====================
*/
void idAI::Script_Burn( void ) {
	Burn();
}

/*
=====================
idAI::Script_ClearBurn
=====================
*/
void idAI::Script_ClearBurn( void ) {
	ClearBurn();
}

/*
=====================
idAI::Script_SetSmokeVisibility
=====================
*/
void idAI::Script_SetSmokeVisibility( int num, int on ) {
	SetSmokeVisibility(num, on);
}

/*
=====================
idAI::Script_SetSmokeVisibility
=====================
*/
void idAI::Script_NumSmokeEmitters( void ) {
	int result = NumSmokeEmitters();
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_StopThinking
=====================
*/
void idAI::Script_StopThinking( void ) {
	StopThinking();
}

/*
=====================
idAI::Script_GetTurnDelta
=====================
*/
void idAI::Script_GetTurnDelta( void ) {
	float result = GetTurnDelta();
	idThread::ReturnFloat( result );
}

/*
=====================
idAI::Script_GetMoveType
=====================
*/
void idAI::Script_GetMoveType( void ) {
	int result = GetMoveType();
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_SetMoveType
=====================
*/
void idAI::Script_SetMoveType( int moveType ) {
	SetMoveType( moveType );
}

/*
=====================
idAI::Script_SaveMove
=====================
*/
void idAI::Script_SaveMove( void ) {
	SaveMove();
}

/*
=====================
idAI::Script_RestoreMove
=====================
*/
void idAI::Script_RestoreMove( void ) {
	RestoreMove();
}

/*
=====================
idAI::Script_AllowMovement
=====================
*/
void idAI::Script_AllowMovement( float flag ) {
	AllowMovement( flag );
}

/*
=====================
idAI::Script_JumpFrame
=====================
*/
void idAI::Script_JumpFrame( void ) {
	JumpFrame();
}

/*
=====================
idAI::Script_EnableClip
=====================
*/
void idAI::Script_EnableClip( void ) {
	EnableClip();
}

/*
=====================
idAI::Script_DisableClip
=====================
*/
void idAI::Script_DisableClip( void ) {
	DisableClip();
}

/*
=====================
idAI::Script_EnableGravity
=====================
*/
void idAI::Script_EnableGravity( void ) {
	EnableGravity();
}

/*
=====================
idAI::Script_DisableGravity
=====================
*/
void idAI::Script_DisableGravity( void ) {
	DisableGravity();
}

/*
=====================
idAI::Script_EnableAFPush
=====================
*/
void idAI::Script_EnableAFPush( void ) {
	EnableAFPush();
}

/*
=====================
idAI::Script_DisableAFPush
=====================
*/
void idAI::Script_DisableAFPush( void ) {
	DisableAFPush();
}

/*
=====================
idAI::Script_SetFlySpeed
=====================
*/
void idAI::Script_SetFlySpeed( float speed ) {
	SetFlySpeed( speed );
}

/*
=====================
idAI::Script_SetFlySpeed
=====================
*/
void idAI::Script_SetFlyOffset( int offset ) {
	SetFlyOffset( offset );
}

/*
=====================
idAI::Script_ClearFlyOffset
=====================
*/
void idAI::Script_ClearFlyOffset( void ) {
	ClearFlyOffset();
}

/*
=====================
idAI::Script_GetClosestHiddenTarget
=====================
*/
void idAI::Script_GetClosestHiddenTarget( const char *type ) {
	idEntity *result = GetClosestHiddenTarget( type );
	idThread::ReturnEntity( result );
}

/*
=====================
idAI::Script_GetRandomTarget
=====================
*/
void idAI::Script_GetRandomTarget( const char *type ) {
	idEntity *result = GetRandomTarget( type );
	idThread::ReturnEntity( result );
}

/*
=====================
idAI::Script_TravelDistanceToPoint
=====================
*/
void idAI::Script_TravelDistanceToPoint( const idVec3 &pos ) {
	float result = TravelDistanceToPoint( pos );
	idThread::ReturnFloat( result );
}

/*
=====================
idAI::Script_TravelDistanceToEntity
=====================
*/
void idAI::Script_TravelDistanceToEntity( idEntity *ent ) {
	float result = TravelDistanceToEntity( ent );
	idThread::ReturnFloat( result );
}

/*
=====================
idAI::Script_TravelDistanceBetweenPoints
=====================
*/
void idAI::Script_TravelDistanceBetweenPoints( const idVec3 &source, const idVec3 &dest ) {
	float result = TravelDistanceBetweenPoints( source, dest );
	idThread::ReturnFloat( result );
}

/*
=====================
idAI::Script_TravelDistanceBetweenEntities
=====================
*/
void idAI::Script_TravelDistanceBetweenEntities( idEntity *source, idEntity *dest ) {
	float result = TravelDistanceBetweenEntities( source, dest );
	idThread::ReturnFloat( result );
}

/*
=====================
idAI::Script_LookAtEnemy
=====================
*/
void idAI::Script_LookAtEntity( idEntity *ent, float duration ) {
	LookAtEntity( ent, duration );
}

/*
=====================
idAI::Script_LookAtEnemy
=====================
*/
void idAI::Script_LookAtEnemy( float duration ) {
	LookAtEnemy( duration );
}

/*
=====================
idAI::Script_SetJointMod
=====================
*/
void idAI::Script_SetJointMod( int allow ) {
	SetJointMod( allow );
}

/*
=====================
idAI::Script_ThrowMoveable
=====================
*/
void idAI::Script_ThrowMoveable( void ) {
	ThrowMoveable();
}

/*
=====================
idAI::Script_ThrowAF
=====================
*/
void idAI::Script_ThrowAF( void ) {
	ThrowAF();
}

/*
=====================
idAI::Script_SetAngles
=====================
*/
void idAI::Script_SetAngles( const idAngles &ang ) {
	SetAngles( ang );
}

/*
=====================
idAI::Script_GetAngles
=====================
*/
void idAI::Script_GetAngles( void ) {
	idVec3 result = GetAngles();
	idThread::ReturnVector( result );
}

/*
=====================
idAI::Script_RealKill
=====================
*/
void idAI::Script_RealKill( void ) {
	RealKill();
}

/*
=====================
idAI::Script_Kill
=====================
*/
void idAI::Script_Kill( void ) {
	Kill();
}

/*
=====================
idAI::Script_WakeOnFlashlight
=====================
*/
void idAI::Script_WakeOnFlashlight( int enable ) {
	WakeOnFlashlight( enable );
}

/*
=====================
idAI::Script_LocateEnemy
=====================
*/
void idAI::Script_LocateEnemy( void ) {
	LocateEnemy();
}

/*
=====================
idAI::Script_KickObstacles
=====================
*/
void idAI::Script_KickObstacles( idEntity *kickEnt, float force ) {
	KickObstacles( kickEnt, force );
}

/*
=====================
idAI::Script_GetObstacle
=====================
*/
void idAI::Script_GetObstacle( void ) {
	idEntity *result = GetObstacle();
	idThread::ReturnEntity( result );
}

/*
=====================
idAI::Script_PushPointIntoAAS
=====================
*/
void idAI::Script_PushPointIntoAAS( const idVec3 &pos ) {
	idVec3 result = PushPointIntoAAS( pos );
	idThread::ReturnVector( result );
}

/*
=====================
idAI::Script_GetTurnRate
=====================
*/
void idAI::Script_GetTurnRate( void ) {
	float result = GetTurnRate();
	idThread::ReturnFloat( result );
}

/*
=====================
idAI::Script_SetTurnRate
=====================
*/
void idAI::Script_SetTurnRate( float rate ) {
	SetTurnRate( rate );
}

/*
=====================
idAI::Script_AllowHiddenMovement
=====================
*/
void idAI::Script_AnimTurn(float angles) {
	AnimTurn(angles);
}

/*
=====================
idAI::Script_AllowHiddenMovement
=====================
*/
void idAI::Script_AllowHiddenMovement( int enable ) {
	AllowHiddenMovement( enable );
}

/*
=====================
idAI::Script_TriggerParticles
=====================
*/
void idAI::Script_TriggerParticles( const char *jointName ) {
	TriggerParticles( jointName );
}

/*
=====================
idAI::Script_FindActorsInBounds
=====================
*/
void idAI::Script_FindActorsInBounds( const idVec3 &mins, const idVec3 &maxs ) {
	idEntity* result = FindActorsInBounds( mins, maxs );
	idThread::ReturnEntity( result );
}

/*
=====================
idAI::Script_CanReachPosition
=====================
*/
void idAI::Script_CanReachPosition( const idVec3 &pos ) {
	bool result = CanReachPosition( pos );
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_CanReachEntity
=====================
*/
void idAI::Script_CanReachEntity( idEntity *ent ) {
	bool result = CanReachEntity( ent );
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_CanReachEnemy
=====================
*/
void idAI::Script_CanReachEnemy( void ) {
	bool result = CanReachEnemy();
	idThread::ReturnInt( result );
}

/*
=====================
idAI::Script_GetReachableEntityPosition
=====================
*/
void idAI::Script_GetReachableEntityPosition( idEntity *ent ) {
	idVec3 result = GetReachableEntityPosition( ent );
	idThread::ReturnVector( result );
}

#ifdef _D3XP
/*
================
idAI::Script_MoveToPositionDirect
================
*/
void idAI::Script_MoveToPositionDirect( const idVec3 &pos ) {
	MoveToPositionDirect( pos );
}

/*
================
idAI::Script_AvoidObstacles
================
*/
void idAI::Script_AvoidObstacles( int ignore ) {
	AvoidObstacles( ignore );
}

/*
================
idAI::Script_TriggerFX
================
*/
void idAI::Script_TriggerFX( const char *joint, const char *fx ) {
	TriggerFX( joint, fx );
}

/*
================
idAI::Script_StartEmitter
================
*/
void idAI::Script_StartEmitter( const char* name, const char *joint, const char *particle ) {
	idEntity *ent = StartEmitter( name, joint, particle );
	idThread::ReturnEntity( ent );
}

/*
================
idAI::Script_GetEmitter
================
*/
void idAI::Script_GetEmitter( const char *name ) {
	idThread::ReturnEntity( GetEmitter( name ) );
}

/*
================
idAI::Script_StopEmitter
================
*/
void idAI::Script_StopEmitter( const char *name ) {
	StopEmitter( name );
}
#endif