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

//
// event defs
//
const idEventDef EV_Weapon_Clear( "<clear>" );
const idEventDef EV_Weapon_GetOwner( "getOwner", NULL, 'e' );
const idEventDef EV_Weapon_Next( "nextWeapon" );
const idEventDef EV_Weapon_State( "weaponState", "sd" );
const idEventDef EV_Weapon_UseAmmo( "useAmmo", "d" );
const idEventDef EV_Weapon_AddToClip( "addToClip", "d" );
const idEventDef EV_Weapon_AmmoInClip( "ammoInClip", NULL, 'f' );
const idEventDef EV_Weapon_AmmoAvailable( "ammoAvailable", NULL, 'f' );
const idEventDef EV_Weapon_TotalAmmoCount( "totalAmmoCount", NULL, 'f' );
const idEventDef EV_Weapon_ClipSize( "clipSize", NULL, 'f' );
const idEventDef EV_Weapon_WeaponOutOfAmmo( "weaponOutOfAmmo" );
const idEventDef EV_Weapon_WeaponReady( "weaponReady" );
const idEventDef EV_Weapon_WeaponReloading( "weaponReloading" );
const idEventDef EV_Weapon_WeaponHolstered( "weaponHolstered" );
const idEventDef EV_Weapon_WeaponRising( "weaponRising" );
const idEventDef EV_Weapon_WeaponLowering( "weaponLowering" );
const idEventDef EV_Weapon_Flashlight( "flashlight", "d" );
const idEventDef EV_Weapon_LaunchProjectiles( "launchProjectiles", "dffff" );
const idEventDef EV_Weapon_CreateProjectile( "createProjectile", NULL, 'e' );
const idEventDef EV_Weapon_EjectBrass( "ejectBrass" );
const idEventDef EV_Weapon_Melee( "melee", NULL, 'd' );
const idEventDef EV_Weapon_GetWorldModel( "getWorldModel", NULL, 'e' );
const idEventDef EV_Weapon_AllowDrop( "allowDrop", "d" );
const idEventDef EV_Weapon_AutoReload( "autoReload", NULL, 'f' );
const idEventDef EV_Weapon_NetReload( "netReload" );
const idEventDef EV_Weapon_IsInvisible( "isInvisible", NULL, 'f' );
const idEventDef EV_Weapon_NetEndReload( "netEndReload" );
#ifdef _D3XP
const idEventDef EV_Weapon_GrabberHasTarget( "grabberHasTarget", NULL, 'd' );
const idEventDef EV_Weapon_Grabber( "grabber", "d" );
const idEventDef EV_Weapon_Grabber_SetGrabDistance( "grabberGrabDistance", "f" );
const idEventDef EV_Weapon_LaunchProjectilesEllipse( "launchProjectilesEllipse", "dffff" );
const idEventDef EV_Weapon_LaunchPowerup( "launchPowerup", "sfd" );
const idEventDef EV_Weapon_StartWeaponSmoke( "startWeaponSmoke" );
const idEventDef EV_Weapon_StopWeaponSmoke( "stopWeaponSmoke" );
const idEventDef EV_Weapon_StartWeaponParticle( "startWeaponParticle", "s" );
const idEventDef EV_Weapon_StopWeaponParticle( "stopWeaponParticle", "s" );
const idEventDef EV_Weapon_StartWeaponLight( "startWeaponLight", "s" );
const idEventDef EV_Weapon_StopWeaponLight( "stopWeaponLight", "s" );
#endif

//
// class def
//
CLASS_DECLARATION( idAnimatedEntity, idWeapon )
	EVENT( EV_Weapon_Clear,						idWeapon::Script_Clear )
	EVENT( EV_Weapon_GetOwner,					idWeapon::Script_GetOwner )
	EVENT( EV_Weapon_State,						idWeapon::Script_WeaponState )
	EVENT( EV_Weapon_WeaponReady,				idWeapon::Script_WeaponReady )
	EVENT( EV_Weapon_WeaponOutOfAmmo,			idWeapon::Script_WeaponOutOfAmmo )
	EVENT( EV_Weapon_WeaponReloading,			idWeapon::Script_WeaponReloading )
	EVENT( EV_Weapon_WeaponHolstered,			idWeapon::Script_WeaponHolstered )
	EVENT( EV_Weapon_WeaponRising,				idWeapon::Script_WeaponRising )
	EVENT( EV_Weapon_WeaponLowering,			idWeapon::Script_WeaponLowering )
	EVENT( EV_Weapon_UseAmmo,					idWeapon::Script_UseAmmo )
	EVENT( EV_Weapon_AddToClip,					idWeapon::Script_AddToClip )
	EVENT( EV_Weapon_AmmoInClip,				idWeapon::Script_AmmoInClip )
	EVENT( EV_Weapon_AmmoAvailable,				idWeapon::Script_AmmoAvailable )
	EVENT( EV_Weapon_TotalAmmoCount,			idWeapon::Script_TotalAmmoCount )
	EVENT( EV_Weapon_ClipSize,					idWeapon::Script_ClipSize )
	EVENT( AI_PlayAnim,							idWeapon::Script_PlayAnim )
	EVENT( AI_PlayCycle,						idWeapon::Script_PlayCycle )
	EVENT( AI_SetBlendFrames,					idWeapon::Script_SetBlendFrames )
	EVENT( AI_GetBlendFrames,					idWeapon::Script_GetBlendFrames )
	EVENT( AI_AnimDone,							idWeapon::Script_AnimDone )
	EVENT( EV_Weapon_Next,						idWeapon::Script_Next )
	EVENT( EV_SetSkin,							idWeapon::Script_SetSkin )
	EVENT( EV_Weapon_Flashlight,				idWeapon::Script_Flashlight )
	EVENT( EV_Light_GetLightParm,				idWeapon::Script_GetLightParm )
	EVENT( EV_Light_SetLightParm,				idWeapon::Script_SetLightParm )
	EVENT( EV_Light_SetLightParms,				idWeapon::Script_SetLightParms )
	EVENT( EV_Weapon_LaunchProjectiles,			idWeapon::Script_LaunchProjectiles )
	EVENT( EV_Weapon_CreateProjectile,			idWeapon::Script_CreateProjectile )
	EVENT( EV_Weapon_EjectBrass,				idWeapon::Script_EjectBrass )
	EVENT( EV_Weapon_Melee,						idWeapon::Script_Melee )
	EVENT( EV_Weapon_GetWorldModel,				idWeapon::Script_GetWorldModel )
	EVENT( EV_Weapon_AllowDrop,					idWeapon::Script_AllowDrop )
	EVENT( EV_Weapon_AutoReload,				idWeapon::Script_AutoReload )
	EVENT( EV_Weapon_NetReload,					idWeapon::Script_NetReload )
	EVENT( EV_Weapon_IsInvisible,				idWeapon::Script_IsInvisible )
	EVENT( EV_Weapon_NetEndReload,				idWeapon::Script_NetEndReload )
#ifdef _D3XP
	EVENT( EV_Weapon_Grabber,					idWeapon::Script_Grabber )
	EVENT( EV_Weapon_GrabberHasTarget,			idWeapon::Script_GrabberHasTarget )
	EVENT( EV_Weapon_Grabber_SetGrabDistance,	idWeapon::Script_GrabberSetGrabDistance )
	EVENT( EV_Weapon_LaunchProjectilesEllipse,	idWeapon::Script_LaunchProjectilesEllipse )
	EVENT( EV_Weapon_LaunchPowerup,				idWeapon::Script_LaunchPowerup )
	EVENT( EV_Weapon_StartWeaponSmoke,			idWeapon::Script_StartWeaponSmoke )
	EVENT( EV_Weapon_StopWeaponSmoke,			idWeapon::Script_StopWeaponSmoke )
	EVENT( EV_Weapon_StartWeaponParticle,		idWeapon::Script_StartWeaponParticle )
	EVENT( EV_Weapon_StopWeaponParticle,		idWeapon::Script_StopWeaponParticle )
	EVENT( EV_Weapon_StartWeaponLight,			idWeapon::Script_StartWeaponLight )
	EVENT( EV_Weapon_StopWeaponLight,			idWeapon::Script_StopWeaponLight )
#endif
END_CLASS

/*
================
idWeapon::Script_Clear
================
*/
void idWeapon::Script_Clear( void ) {
	Clear();
}

/*
================
idWeapon::Script_GetOwner
================
*/
void idWeapon::Script_GetOwner( void ) {
	idThread::ReturnEntity( WeaponGetOwner() );
}

/*
================
idWeapon::Script_WeaponState
================
*/
void idWeapon::Script_WeaponState( const char *statename, int blendFrames ) {
	WeaponState( statename, blendFrames );
}

/*
================
idWeapon::Script_UseAmmo
================
*/
void idWeapon::Script_UseAmmo( int amount ) {
	UseAmmo( amount );
}

/*
================
idWeapon::Script_AddToClip
================
*/
void idWeapon::Script_AddToClip( int amount ) {
	AddToClip( amount );
}

/*
================
idWeapon::Script_AmmoInClip
================
*/
void idWeapon::Script_AmmoInClip( void ) {
	idThread::ReturnInt( AmmoInClip() );
}

/*
================
idWeapon::Script_AmmoAvailable
================
*/
void idWeapon::Script_AmmoAvailable( void ) {
	idThread::ReturnInt( AmmoAvailable() );
}

/*
================
idWeapon::Script_TotalAmmoCount
================
*/
void idWeapon::Script_TotalAmmoCount( void ) {
	idThread::ReturnInt( TotalAmmoCount() );
}

/*
================
idWeapon::Script_ClipSize
================
*/
void idWeapon::Script_ClipSize( void ) {
	idThread::ReturnInt( ClipSize() );
}

/*
================
idWeapon::Script_AutoReload
================
*/
void idWeapon::Script_AutoReload( void ) {
	idThread::ReturnFloat( AutoReload() );
}

/*
================
idWeapon::Script_NetReload
================
*/
void idWeapon::Script_NetReload( void ) {
	NetReload();
}

/*
================
idWeapon::Script_NetEndReload
================
*/
void idWeapon::Script_NetEndReload( void ) {
	NetEndReload();
}

/*
================
idWeapon::Script_PlayAnim
================
*/
void idWeapon::Script_PlayAnim( int channel, const char *animname ) {
	PlayAnim( channel, animname );
}

/*
================
idWeapon::Script_PlayCycle
================
*/
void idWeapon::Script_PlayCycle( int channel, const char *animname ) {
	PlayCycle( channel, animname );
}

/*
================
idWeapon::Script_AnimDone
================
*/
void idWeapon::Script_AnimDone( int channel, int blendFrames ) {
	idThread::ReturnInt( AnimDone( channel, blendFrames ) );
}

/*
================
idWeapon::Script_SetBlendFrames
================
*/
void idWeapon::Script_SetBlendFrames( int channel, int blendFrames ) {
	SetBlendFrames( blendFrames );
}

/*
================
idWeapon::Script_GetBlendFrames
================
*/
void idWeapon::Script_GetBlendFrames( int channel ) {
	idThread::ReturnInt( GetBlendFrames() );
}

/*
================
idWeapon::Script_Next
================
*/
void idWeapon::Script_Next( void ) {
	Next();
}

/*
================
idWeapon::Script_SetSkin
================
*/
void idWeapon::Script_SetSkin( const char *skinname ) {
	SetSkin( skinname );
}

/*
================
idWeapon::Script_Flashlight
================
*/
void idWeapon::Script_Flashlight( int enable ) {
	Flashlight( enable );
}

/*
================
idWeapon::Script_GetLightParm
================
*/
void idWeapon::Script_GetLightParm( int parmnum ) {
	idThread::ReturnFloat( GetLightParm( parmnum ) );
}

/*
================
idWeapon::Script_SetLightParm
================
*/
void idWeapon::Script_SetLightParm( int parmnum, float value ) {
	SetLightParm( parmnum, value );
}

/*
================
idWeapon::Script_SetLightParms
================
*/
void idWeapon::Script_SetLightParms( float parm0, float parm1, float parm2, float parm3 ) {
	SetLightParms( parm0, parm1, parm2, parm3 );
}

#ifdef _D3XP
/*
================
idWeapon::Script_Grabber
================
*/
void idWeapon::Script_Grabber( int enable ) {
	SetGrabberState( enable );
}

/*
================
idWeapon::Script_GrabberHasTarget
================
*/
void idWeapon::Script_GrabberHasTarget() {
	idThread::ReturnInt( GrabberHasTarget() );
}

/*
================
idWeapon::Script_GrabberSetGrabDistance
================
*/
void idWeapon::Script_GrabberSetGrabDistance( float dist ) {

	GrabberSetGrabDistance( dist );
}
#endif

/*
================
idWeapon::Script_CreateProjectile
================
*/
void idWeapon::Script_CreateProjectile( void ) {
	idThread::ReturnEntity( CreateProjectile() );
}

/*
================
idWeapon::Script_LaunchProjectiles
================
*/
void idWeapon::Script_LaunchProjectiles( int num_projectiles, float spread, float fuseOffset, float launchPower, float dmgPower ) {
	LaunchProjectiles( num_projectiles, spread, fuseOffset, launchPower, dmgPower );
}

#ifdef _D3XP
/*
================
idWeapon::Script_LaunchProjectilesEllipse
================
*/
void idWeapon::Script_LaunchProjectilesEllipse( int num_projectiles, float spreada, float spreadb, float fuseOffset, float power ) {
	LaunchProjectilesEllipse( num_projectiles, spreada, spreadb, fuseOffset, power );
}

/*
================
idWeapon::Script_LaunchPowerup
================
*/
void idWeapon::Script_LaunchPowerup( const char *powerup, float duration, int useAmmo ) {
	LaunchPowerup( powerup, duration, useAmmo );
}

/*
================
idWeapon::Script_StartWeaponSmoke
================
*/
void idWeapon::Script_StartWeaponSmoke( void ) {
	StartWeaponSmoke();
}

/*
================
idWeapon::Script_StopWeaponSmoke
================
*/
void idWeapon::Script_StopWeaponSmoke( void ) {
	StopWeaponSmoke();
}

/*
================
idWeapon::Script_StartWeaponParticle
================
*/
void idWeapon::Script_StartWeaponParticle( const char *name ) {
	StartWeaponParticle( name );
}

/*
================
idWeapon::Script_StopWeaponParticle
================
*/
void idWeapon::Script_StopWeaponParticle( const char *name ) {
	StopWeaponParticle( name );
}

/*
================
idWeapon::Script_StartWeaponLight
================
*/
void idWeapon::Script_StartWeaponLight( const char *name ) {
	StartWeaponLight( name );
}

/*
================
idWeapon::Script_StopWeaponLight
================
*/
void idWeapon::Script_StopWeaponLight( const char *name ) {
	StopWeaponLight( name );
}
#endif

/*
================
idWeapon::Script_Melee
================
*/
void idWeapon::Script_Melee( void ) {
	idThread::ReturnInt( Melee() );
}

/*
================
idWeapon::Script_GetWorldModel
================
*/
void idWeapon::Script_GetWorldModel( void ) {
	idThread::ReturnEntity( GetWorldModel() );
}

/*
================
idWeapon::Script_AllowDrop
================
*/
void idWeapon::Script_AllowDrop( int allow ) {
	AllowDrop( allow );
}

/*
================
idWeapon::Script_EjectBrass
================
*/
void idWeapon::Script_EjectBrass( void ) {
	EjectBrass();
}

/*
================
idWeapon::Script_IsInvisible
================
*/
void idWeapon::Script_IsInvisible( void ) {
	idThread::ReturnFloat( IsInvisible() );
}

/*
================
idWeapon::Script_WeaponReady
================
*/
void idWeapon::Script_WeaponReady( void ) {
	WeaponReady();
}

/*
================
idWeapon::Script_WeaponOutOfAmmo
================
*/
void idWeapon::Script_WeaponOutOfAmmo( void ) {
	WeaponOutOfAmmo();
}

/*
================
idWeapon::Script_WeaponReloading
================
*/
void idWeapon::Script_WeaponReloading( void ) {
	WeaponReloading();
}

/*
================
idWeapon::Script_WeaponHolstered
================
*/
void idWeapon::Script_WeaponHolstered( void ) {
	WeaponHolstered();
}

/*
================
idWeapon::Script_WeaponRising
================
*/
void idWeapon::Script_WeaponRising( void ) {
	WeaponRising();
}

/*
================
idWeapon::Script_WeaponLowering
================
*/
void idWeapon::Script_WeaponLowering( void ) {
	WeaponLowering();
}
