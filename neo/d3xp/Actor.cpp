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

/***********************************************************************

	idActor

***********************************************************************/

/*
=====================
idActor::idActor
=====================
*/
idActor::idActor( void ) {
	viewAxis.Identity();

	scriptThread		= NULL;		// initialized by ConstructScriptObject, which is called by idEntity::Spawn

	use_combat_bbox		= false;
	head				= NULL;

	team				= 0;
	rank				= 0;
	fovDot				= 0.0f;
	eyeOffset.Zero();
	pain_debounce_time	= 0;
	pain_delay			= 0;
	pain_threshold		= 0;

	#if MD5_ENABLE_GIBS > 0
	damageEmitSever = NULL;
	damageEmitSpray = NULL;
	damageEmitStage = 0;
	damageEmitStart = 0;
	damageEmitDeath = 0;
	damageEmitJoint = jointHandle_t::INVALID_JOINT;
	damageEmitAngle = mat3_identity;
	damageEmitShift = vec3_zero;
	#endif

	state				= NULL;
	idealState			= NULL;

	leftEyeJoint		= INVALID_JOINT;
	rightEyeJoint		= INVALID_JOINT;
	soundJoint			= INVALID_JOINT;

	modelOffset.Zero();
	deltaViewAngles.Zero();

	painTime			= 0;
	allowPain			= false;
	allowEyeFocus		= false;

	waitState			= "";

	blink_anim			= 0;
	blink_time			= 0;
	blink_min			= 0;
	blink_max			= 0;

	finalBoss			= false;

	attachments.SetGranularity( 1 );

	enemyNode.SetOwner( this );
	enemyList.SetOwner( this );

#ifdef _D3XP
	damageCap = -1;
#endif
}

/*
=====================
idActor::~idActor
=====================
*/
idActor::~idActor( void ) {
	int i;
	idEntity *ent;

	DeconstructScriptObject();
	scriptObject.Free();

	StopSound( SND_CHANNEL_ANY, false );

	delete combatModel;
	combatModel = NULL;

	if ( head.GetEntity() ) {
		head.GetEntity()->ClearBody();
		head.GetEntity()->PostEventMS( &EV_Remove, 0 );
	}

	// remove any attached entities
	for( i = 0; i < attachments.Num(); i++ ) {
		ent = attachments[ i ].ent.GetEntity();
		if ( ent ) {
			ent->PostEventMS( &EV_Remove, 0 );
		}
	}

	ShutdownThreads();
}

/*
=====================
idActor::Spawn
=====================
*/
void idActor::Spawn( void ) {
	idEntity		*ent;
	idStr			jointName;
	float			fovDegrees;
	copyJoints_t	copyJoint;

	animPrefix	= "";
	state		= NULL;
	idealState	= NULL;

	spawnArgs.GetInt( "rank", "0", rank );
	spawnArgs.GetInt( "team", "0", team );
	spawnArgs.GetVector( "offsetModel", "0 0 0", modelOffset );

	spawnArgs.GetBool( "use_combat_bbox", "0", use_combat_bbox );

	viewAxis = GetPhysics()->GetAxis();

	spawnArgs.GetFloat( "fov", "90", fovDegrees );
	SetFOV( fovDegrees );

	pain_debounce_time	= 0;

	pain_delay		= SEC2MS( spawnArgs.GetFloat( "pain_delay" ) );
	pain_threshold	= spawnArgs.GetInt( "pain_threshold" );

	#if MD5_ENABLE_GIBS > 0
	damageEmitSever = NULL;
	damageEmitSpray = NULL;
	damageEmitStage = 0;
	damageEmitStart = 0;
	damageEmitDeath = 0;
	damageEmitJoint = jointHandle_t::INVALID_JOINT;
	damageEmitAngle = mat3_identity;
	damageEmitShift = vec3_zero;
	#endif

	LoadAF();

	walkIK.Init( this, IK_ANIM, modelOffset );

	// the animation used to be set to the IK_ANIM at this point, but that was fixed, resulting in
	// attachments not binding correctly, so we're stuck setting the IK_ANIM before attaching things.
	animator.ClearAllAnims( gameLocal.time, 0 );
	animator.SetFrame( ANIMCHANNEL_ALL, animator.GetAnim( IK_ANIM ), 0, 0, 0 );

	// spawn any attachments we might have
	const idKeyValue *kv = spawnArgs.MatchPrefix( "def_attach", NULL );
	while ( kv ) {
		idDict args;

		args.Set( "classname", kv->GetValue().c_str() );

		// make items non-touchable so the player can't take them out of the character's hands
		args.Set( "no_touch", "1" );

		// don't let them drop to the floor
		args.Set( "dropToFloor", "0" );

		gameLocal.SpawnEntityDef( args, &ent );
		if ( !ent ) {
			gameLocal.Error( "Couldn't spawn '%s' to attach to entity '%s'", kv->GetValue().c_str(), name.c_str() );
		} else {
			Attach( ent );
		}
		kv = spawnArgs.MatchPrefix( "def_attach", kv );
	}

	SetupDamageGroups();
	SetupHead();

	// clear the bind anim
	animator.ClearAllAnims( gameLocal.time, 0 );

	idEntity *headEnt = head.GetEntity();
	idAnimator *headAnimator;
	if ( headEnt ) {
		headAnimator = headEnt->GetAnimator();
	} else {
		headAnimator = &animator;
	}

	if ( headEnt ) {
		// set up the list of joints to copy to the head
		for( kv = spawnArgs.MatchPrefix( "copy_joint", NULL ); kv != NULL; kv = spawnArgs.MatchPrefix( "copy_joint", kv ) ) {
			if ( kv->GetValue() == "" ) {
				// probably clearing out inherited key, so skip it
				continue;
			}

			jointName = kv->GetKey();
			if ( jointName.StripLeadingOnce( "copy_joint_world " ) ) {
				copyJoint.mod = JOINTMOD_WORLD_OVERRIDE;
			} else {
				jointName.StripLeadingOnce( "copy_joint " );
				copyJoint.mod = JOINTMOD_LOCAL_OVERRIDE;
			}

			copyJoint.from = animator.GetJointHandle( jointName );
			if ( copyJoint.from == INVALID_JOINT ) {
				gameLocal.Warning( "Unknown copy_joint '%s' on entity %s", jointName.c_str(), name.c_str() );
				continue;
			}

			jointName = kv->GetValue();
			copyJoint.to = headAnimator->GetJointHandle( jointName );
			if ( copyJoint.to == INVALID_JOINT ) {
				gameLocal.Warning( "Unknown copy_joint '%s' on head of entity %s", jointName.c_str(), name.c_str() );
				continue;
			}

			copyJoints.Append( copyJoint );
		}
	}

	// set up blinking
	blink_anim = headAnimator->GetAnim( "blink" );
	blink_time = 0;	// it's ok to blink right away
	blink_min = SEC2MS( spawnArgs.GetFloat( "blink_min", "0.5" ) );
	blink_max = SEC2MS( spawnArgs.GetFloat( "blink_max", "8" ) );

	// set up the head anim if necessary
	int headAnim = headAnimator->GetAnim( "def_head" );
	if ( headAnim ) {
		if ( headEnt ) {
			headAnimator->CycleAnim( ANIMCHANNEL_ALL, headAnim, gameLocal.time, 0 );
		} else {
			headAnimator->CycleAnim( ANIMCHANNEL_HEAD, headAnim, gameLocal.time, 0 );
		}
	}

	if ( spawnArgs.GetString( "sound_bone", "", jointName ) ) {
		soundJoint = animator.GetJointHandle( jointName );
		if ( soundJoint == INVALID_JOINT ) {
			gameLocal.Warning( "idAnimated '%s' at (%s): cannot find joint '%s' for sound playback", name.c_str(), GetPhysics()->GetOrigin().ToString(0), jointName.c_str() );
		}
	}

	finalBoss = spawnArgs.GetBool( "finalBoss" );

	FinishSetup();
}

/*
================
idActor::FinishSetup
================
*/
void idActor::FinishSetup( void ) {
	const char	*scriptObjectName;

	// setup script object
	if ( spawnArgs.GetString( "scriptobject", NULL, &scriptObjectName ) ) {
		if ( !scriptObject.SetType( scriptObjectName ) ) {
			gameLocal.Error( "Script object '%s' not found on entity '%s'.", scriptObjectName, name.c_str() );
		}

		ConstructScriptObject();
	}

	SetupBody();
}

/*
================
idActor::SetupHead
================
*/
void idActor::SetupHead( void ) {
	idAFAttachment		*headEnt;
	idStr				jointName;
	const char			*headModel;
	jointHandle_t		joint;
	jointHandle_t		damageJoint;
	int					i;
	const idKeyValue	*sndKV;

	if ( gameLocal.isClient ) {
		return;
	}

	headModel = spawnArgs.GetString( "def_head", "" );
	if ( headModel[ 0 ] ) {
		jointName = spawnArgs.GetString( "head_joint" );
		joint = animator.GetJointHandle( jointName );
		if ( joint == INVALID_JOINT ) {
			gameLocal.Error( "Joint '%s' not found for 'head_joint' on '%s'", jointName.c_str(), name.c_str() );
		}

		// set the damage joint to be part of the head damage group
		damageJoint = joint;
		#if MD5_ENABLE_GIBS > 0
		for (i = 0; i < damageZonesName.Num(); i++) {
			if (damageZonesName[i] == "head") {
				damageJoint = static_cast<jointHandle_t>(damageZonesBone[i]);
				break;
			}
		}
		#else
		for (i = 0; i < damageGroups.Num(); i++) {
			if (damageGroups[i] == "head") {
				damageJoint = static_cast<jointHandle_t>(i);
				break;
			}
		}
		#endif

		// copy any sounds in case we have frame commands on the head
		idDict	args;
		sndKV = spawnArgs.MatchPrefix( "snd_", NULL );
		while( sndKV ) {
			args.Set( sndKV->GetKey(), sndKV->GetValue() );
			sndKV = spawnArgs.MatchPrefix( "snd_", sndKV );
		}

#ifdef _D3XP
		// copy slowmo param to the head
		args.SetBool( "slowmo", spawnArgs.GetBool( "slowmo", "1" ) );
#endif

		headEnt = static_cast<idAFAttachment *>( gameLocal.SpawnEntityType( idAFAttachment::GetClassType(), &args ) );
		headEnt->SetName( va( "%s_head", name.c_str() ) );
		headEnt->SetBody( this, headModel, damageJoint );
		head = headEnt;

#ifdef _D3XP
		idStr xSkin;
		if ( spawnArgs.GetString( "skin_head_xray", "", xSkin ) ) {
			headEnt->xraySkin = declManager->FindSkin( xSkin.c_str() );
			headEnt->UpdateModel();
		}
#endif

		idVec3		origin;
		idMat3		axis;
		idAttachInfo &attach = attachments.Alloc();
		attach.channel = animator.GetChannelForJoint( joint );
		animator.GetJointTransform( joint, gameLocal.time, origin, axis );
		origin = renderEntity.origin + ( origin + modelOffset ) * renderEntity.axis;
		attach.ent = headEnt;
		headEnt->SetOrigin( origin );
		headEnt->SetAxis( renderEntity.axis );
		headEnt->BindToJoint( this, joint, true );
	}
}

/*
================
idActor::CopyJointsFromBodyToHead
================
*/
void idActor::CopyJointsFromBodyToHead( void ) {
	idEntity	*headEnt = head.GetEntity();
	idAnimator	*headAnimator;
	int			i;
	idMat3		mat;
	idMat3		axis;
	idVec3		pos;

	if ( !headEnt ) {
		return;
	}

	headAnimator = headEnt->GetAnimator();

	// copy the animation from the body to the head
	for( i = 0; i < copyJoints.Num(); i++ ) {
		if ( copyJoints[ i ].mod == JOINTMOD_WORLD_OVERRIDE ) {
			mat = headEnt->GetPhysics()->GetAxis().Transpose();
			GetJointWorldTransform( copyJoints[ i ].from, gameLocal.time, pos, axis );
			pos -= headEnt->GetPhysics()->GetOrigin();
			headAnimator->SetJointPos( copyJoints[ i ].to, copyJoints[ i ].mod, pos * mat );
			headAnimator->SetJointAxis( copyJoints[ i ].to, copyJoints[ i ].mod, axis * mat );
		} else {
			animator.GetJointLocalTransform( copyJoints[ i ].from, gameLocal.time, pos, axis );
			headAnimator->SetJointPos( copyJoints[ i ].to, copyJoints[ i ].mod, pos );
			headAnimator->SetJointAxis( copyJoints[ i ].to, copyJoints[ i ].mod, axis );
		}
	}
}

/*
================
idActor::Restart
================
*/
void idActor::Restart( void ) {
	assert( !head.GetEntity() );
	SetupHead();
	FinishSetup();
}

/*
================
idActor::Save

archive object for savegame file
================
*/
void idActor::Save( idSaveGame *savefile ) const {
	idActor *ent;
	int i;

	savefile->WriteInt( team );
	savefile->WriteInt( rank );
	savefile->WriteMat3( viewAxis );

	savefile->WriteInt( enemyList.Num() );
	for ( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
		savefile->WriteObject( ent );
	}

	savefile->WriteFloat( fovDot );
	savefile->WriteVec3( eyeOffset );
	savefile->WriteVec3( modelOffset );
	savefile->WriteAngles( deltaViewAngles );

	savefile->WriteInt( pain_debounce_time );
	savefile->WriteInt( pain_delay );
	savefile->WriteInt( pain_threshold );

	#if MD5_ENABLE_GIBS > 0
	savefile->WriteInt(damageBonesZone.Num());
	for (i = 0; i < damageBonesZone.Num(); i++) savefile->WriteInt(damageBonesZone[i]);
	savefile->WriteInt(damageZonesBone.Num());
	for (i = 0; i < damageZonesBone.Num(); i++) savefile->WriteInt(damageZonesBone[i]);
	savefile->WriteInt(damageZonesKill.Num());
	for (i = 0; i < damageZonesKill.Num(); i++) savefile->WriteInt(damageZonesKill[i]);
	savefile->WriteInt(damageZonesHeap.Num());
	for (i = 0; i < damageZonesHeap.Num(); i++) savefile->WriteInt(damageZonesHeap[i]);
	savefile->WriteInt(damageZonesDrop.Num());
	for (i = 0; i < damageZonesDrop.Num(); i++) savefile->WriteVec4(damageZonesDrop[i]);
	savefile->WriteInt(damageZonesRate.Num());
	for (i = 0; i < damageZonesRate.Num(); i++) savefile->WriteFloat(damageZonesRate[i]);
	savefile->WriteInt(damageZonesName.Num());
	for (i = 0; i < damageZonesName.Num(); i++) savefile->WriteString(damageZonesName[i]);
	#else
	savefile->WriteInt(damageGroups.Num());
	for (i = 0; i < damageGroups.Num(); i++) savefile->WriteString(damageGroups[i]);
	savefile->WriteInt(damageScales.Num());
	for (i = 0; i < damageScales.Num(); i++) savefile->WriteFloat(damageScales[i]);
	#endif

	#if MD5_ENABLE_GIBS > 0
	savefile->WriteParticle(damageEmitSever);
	savefile->WriteParticle(damageEmitSpray);
	savefile->WriteInt(damageEmitStage);
	savefile->WriteInt(damageEmitStart);
	savefile->WriteInt(damageEmitDeath);
	savefile->WriteJoint(damageEmitJoint);
	savefile->WriteMat3(damageEmitAngle);
	savefile->WriteVec3(damageEmitShift);
	#endif

	savefile->WriteBool( use_combat_bbox );
	head.Save( savefile );

	savefile->WriteInt( copyJoints.Num() );
	for( i = 0; i < copyJoints.Num(); i++ ) {
		savefile->WriteInt( copyJoints[i].mod );
		savefile->WriteJoint( copyJoints[i].from );
		savefile->WriteJoint( copyJoints[i].to );
	}

	savefile->WriteJoint( leftEyeJoint );
	savefile->WriteJoint( rightEyeJoint );
	savefile->WriteJoint( soundJoint );

	walkIK.Save( savefile );

	savefile->WriteString( animPrefix );
	savefile->WriteString( painAnim );

	savefile->WriteInt( blink_anim );
	savefile->WriteInt( blink_time );
	savefile->WriteInt( blink_min );
	savefile->WriteInt( blink_max );

	// script variables
	savefile->WriteObject( scriptThread );

	savefile->WriteString( waitState );

	headAnim.Save( savefile );
	torsoAnim.Save( savefile );
	legsAnim.Save( savefile );

	savefile->WriteBool( allowPain );
	savefile->WriteBool( allowEyeFocus );

	savefile->WriteInt( painTime );

	savefile->WriteInt( attachments.Num() );
	for ( i = 0; i < attachments.Num(); i++ ) {
		attachments[i].ent.Save( savefile );
		savefile->WriteInt( attachments[i].channel );
	}

	savefile->WriteBool( finalBoss );

	idToken token;

	//FIXME: this is unneccesary
	if ( state ) {
		idLexer src( state->Name(), idStr::Length( state->Name() ), "idAI::Save" );

		src.ReadTokenOnLine( &token );
		src.ExpectTokenString( "::" );
		src.ReadTokenOnLine( &token );

		savefile->WriteString( token );
	} else {
		savefile->WriteString( "" );
	}

	if ( idealState ) {
		idLexer src( idealState->Name(), idStr::Length( idealState->Name() ), "idAI::Save" );

		src.ReadTokenOnLine( &token );
		src.ExpectTokenString( "::" );
		src.ReadTokenOnLine( &token );

		savefile->WriteString( token );
	} else {
		savefile->WriteString( "" );
	}

#ifdef _D3XP
	savefile->WriteInt( damageCap );
#endif

}

/*
================
idActor::Restore

unarchives object from save game file
================
*/
void idActor::Restore( idRestoreGame *savefile ) {
	int i, num;
	idActor *ent;

	savefile->ReadInt( team );
	savefile->ReadInt( rank );
	savefile->ReadMat3( viewAxis );

	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		savefile->ReadObject( reinterpret_cast<idClass *&>( ent ) );
		assert( ent );
		if ( ent ) {
			ent->enemyNode.AddToEnd( enemyList );
		}
	}

	savefile->ReadFloat( fovDot );
	savefile->ReadVec3( eyeOffset );
	savefile->ReadVec3( modelOffset );
	savefile->ReadAngles( deltaViewAngles );

	savefile->ReadInt( pain_debounce_time );
	savefile->ReadInt( pain_delay );
	savefile->ReadInt( pain_threshold );

	#if MD5_ENABLE_GIBS > 0
	savefile->ReadInt(num); damageBonesZone.SetNum(num);
	for (i = 0; i < num; i++) savefile->ReadInt(damageBonesZone[i]);
	savefile->ReadInt(num); damageZonesBone.SetNum(num);
	for (i = 0; i < num; i++) savefile->ReadInt(damageZonesBone[i]);
	savefile->ReadInt(num); damageZonesKill.SetNum(num);
	for (i = 0; i < num; i++) savefile->ReadInt(damageZonesKill[i]);
	savefile->ReadInt(num); damageZonesHeap.SetNum(num);
	for (i = 0; i < num; i++) savefile->ReadInt(damageZonesHeap[i]);
	savefile->ReadInt(num); damageZonesDrop.SetNum(num);
	for (i = 0; i < num; i++) savefile->ReadVec4(damageZonesDrop[i]);
	savefile->ReadInt(num); damageZonesRate.SetNum(num);
	for (i = 0; i < num; i++) savefile->ReadFloat(damageZonesRate[i]);
	savefile->ReadInt(num); damageZonesName.SetGranularity(1); damageZonesName.SetNum(num);
	for (i = 0; i < num; i++) savefile->ReadString(damageZonesName[i]);
	#else
	savefile->ReadInt(num); damageGroups.SetGranularity(1); damageGroups.SetNum(num);
	for (i = 0; i < num; i++) savefile->ReadString(damageGroups[i]);
	savefile->ReadInt(num); damageScales.SetNum(num);
	for (i = 0; i < num; i++) savefile->ReadFloat(damageScales[i]);
	#endif

	#if MD5_ENABLE_GIBS > 0
	savefile->ReadParticle(damageEmitSever);
	savefile->ReadParticle(damageEmitSpray);
	savefile->ReadInt(damageEmitStage);
	savefile->ReadInt(damageEmitStart);
	savefile->ReadInt(damageEmitDeath);
	savefile->ReadJoint(damageEmitJoint);
	savefile->ReadMat3(damageEmitAngle);
	savefile->ReadVec3(damageEmitShift);
	#endif

	savefile->ReadBool( use_combat_bbox );
	head.Restore( savefile );

	savefile->ReadInt( num );
	copyJoints.SetNum( num );
	for( i = 0; i < num; i++ ) {
		int val;
		savefile->ReadInt( val );
		copyJoints[i].mod = static_cast<jointModTransform_t>( val );
		savefile->ReadJoint( copyJoints[i].from );
		savefile->ReadJoint( copyJoints[i].to );
	}

	savefile->ReadJoint( leftEyeJoint );
	savefile->ReadJoint( rightEyeJoint );
	savefile->ReadJoint( soundJoint );

	walkIK.Restore( savefile );

	savefile->ReadString( animPrefix );
	savefile->ReadString( painAnim );

	savefile->ReadInt( blink_anim );
	savefile->ReadInt( blink_time );
	savefile->ReadInt( blink_min );
	savefile->ReadInt( blink_max );

	savefile->ReadObject( reinterpret_cast<idClass *&>( scriptThread ) );

	savefile->ReadString( waitState );

	headAnim.Restore( savefile );
	torsoAnim.Restore( savefile );
	legsAnim.Restore( savefile );

	savefile->ReadBool( allowPain );
	savefile->ReadBool( allowEyeFocus );

	savefile->ReadInt( painTime );

	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		idAttachInfo &attach = attachments.Alloc();
		attach.ent.Restore( savefile );
		savefile->ReadInt( attach.channel );
	}

	savefile->ReadBool( finalBoss );

	idStr statename;

	savefile->ReadString( statename );
	if ( statename.Length() > 0 ) {
		state = GetScriptFunction( statename );
	}

	savefile->ReadString( statename );
	if ( statename.Length() > 0 ) {
		idealState = GetScriptFunction( statename );
	}

#ifdef _D3XP
	savefile->ReadInt( damageCap );
#endif
}

/*
================
idActor::Hide
================
*/
void idActor::Hide( void ) {
	idEntity *ent;
	idEntity *next;

	idAFEntity_Base::Hide();
	if ( head.GetEntity() ) {
		head.GetEntity()->Hide();
	}

	for( ent = GetNextTeamEntity(); ent != NULL; ent = next ) {
		next = ent->GetNextTeamEntity();
		if ( ent->GetBindMaster() == this ) {
			ent->Hide();
			if ( ent->IsType( idLight::GetClassType() ) ) {
				static_cast<idLight *>( ent )->Off();
			}
		}
	}
	UnlinkCombat();
}

/*
================
idActor::Show
================
*/
void idActor::Show( void ) {
	idEntity *ent;
	idEntity *next;

	idAFEntity_Base::Show();
	if ( head.GetEntity() ) {
		head.GetEntity()->Show();
	}
	for( ent = GetNextTeamEntity(); ent != NULL; ent = next ) {
		next = ent->GetNextTeamEntity();
		if ( ent->GetBindMaster() == this ) {
			ent->Show();
			if ( ent->IsType( idLight::GetClassType() ) ) {
#ifdef _D3XP
				if ( !spawnArgs.GetBool( "lights_off", "0" ) ) {
					static_cast<idLight *>( ent )->On();
				}
#endif
			}
		}
	}
	LinkCombat();
}

/*
==============
idActor::GetDefaultSurfaceType
==============
*/
int	idActor::GetDefaultSurfaceType( void ) const {
	return SURFTYPE_FLESH;
}

/*
================
idActor::ProjectOverlay
================
*/
void idActor::ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material ) {
	idEntity *ent;
	idEntity *next;

	idEntity::ProjectOverlay( origin, dir, size, material );

	for( ent = GetNextTeamEntity(); ent != NULL; ent = next ) {
		next = ent->GetNextTeamEntity();
		if ( ent->GetBindMaster() == this ) {
			if ( ent->fl.takedamage && ent->spawnArgs.GetBool( "bleed" ) ) {
				ent->ProjectOverlay( origin, dir, size, material );
			}
		}
	}
}

/*
================
idActor::LoadAF
================
*/
bool idActor::LoadAF( void ) {
	idStr fileName;

	if ( !spawnArgs.GetString( "ragdoll", "*unknown*", fileName ) || !fileName.Length() ) {
		return false;
	}
	af.SetAnimator( GetAnimator() );
	return af.Load( this, fileName );
}

/*
=====================
idActor::SetupBody
=====================
*/
void idActor::SetupBody( void ) {
	const char *jointname;

	animator.ClearAllAnims( gameLocal.time, 0 );
	animator.ClearAllJoints();

	idEntity *headEnt = head.GetEntity();
	if ( headEnt ) {
		jointname = spawnArgs.GetString( "bone_leftEye" );
		leftEyeJoint = headEnt->GetAnimator()->GetJointHandle( jointname );

		jointname = spawnArgs.GetString( "bone_rightEye" );
		rightEyeJoint = headEnt->GetAnimator()->GetJointHandle( jointname );

		// set up the eye height.  check if it's specified in the def.
		if ( !spawnArgs.GetFloat( "eye_height", "0", eyeOffset.z ) ) {
			// if not in the def, then try to base it off the idle animation
			int anim = headEnt->GetAnimator()->GetAnim( "idle" );
			if ( anim && ( leftEyeJoint != INVALID_JOINT ) ) {
				idVec3 pos;
				idMat3 axis;
				headEnt->GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, 0 );
				headEnt->GetAnimator()->GetJointTransform( leftEyeJoint, gameLocal.time, pos, axis );
				headEnt->GetAnimator()->ClearAllAnims( gameLocal.time, 0 );
				headEnt->GetAnimator()->ForceUpdate();
				pos += headEnt->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
				eyeOffset = pos + modelOffset;
			} else {
				// just base it off the bounding box size
				eyeOffset.z = GetPhysics()->GetBounds()[ 1 ].z - 6;
			}
		}
		headAnim.Init( this, headEnt->GetAnimator(), ANIMCHANNEL_ALL );
	} else {
		jointname = spawnArgs.GetString( "bone_leftEye" );
		leftEyeJoint = animator.GetJointHandle( jointname );

		jointname = spawnArgs.GetString( "bone_rightEye" );
		rightEyeJoint = animator.GetJointHandle( jointname );

		// set up the eye height.  check if it's specified in the def.
		if ( !spawnArgs.GetFloat( "eye_height", "0", eyeOffset.z ) ) {
			// if not in the def, then try to base it off the idle animation
			int anim = animator.GetAnim( "idle" );
			if ( anim && ( leftEyeJoint != INVALID_JOINT ) ) {
				idVec3 pos;
				idMat3 axis;
				animator.PlayAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, 0 );
				animator.GetJointTransform( leftEyeJoint, gameLocal.time, pos, axis );
				animator.ClearAllAnims( gameLocal.time, 0 );
				animator.ForceUpdate();
				eyeOffset = pos + modelOffset;
			} else {
				// just base it off the bounding box size
				eyeOffset.z = GetPhysics()->GetBounds()[ 1 ].z - 6;
			}
		}
		headAnim.Init( this, &animator, ANIMCHANNEL_HEAD );
	}

	waitState = "";

	torsoAnim.Init( this, &animator, ANIMCHANNEL_TORSO );
	legsAnim.Init( this, &animator, ANIMCHANNEL_LEGS );
}

/*
=====================
idActor::CheckBlink
=====================
*/
void idActor::CheckBlink( void ) {
	// check if it's time to blink
	if ( !blink_anim || ( health <= 0 ) || !allowEyeFocus || ( blink_time > gameLocal.time ) ) {
		return;
	}

	idEntity *headEnt = head.GetEntity();
	if ( headEnt ) {
		headEnt->GetAnimator()->PlayAnim( ANIMCHANNEL_EYELIDS, blink_anim, gameLocal.time, 1 );
	} else {
		animator.PlayAnim( ANIMCHANNEL_EYELIDS, blink_anim, gameLocal.time, 1 );
	}

	// set the next blink time
	blink_time = gameLocal.time + blink_min + gameLocal.random.RandomFloat() * ( blink_max - blink_min );
}

/*
================
idActor::GetPhysicsToVisualTransform
================
*/
bool idActor::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	if ( af.IsActive() ) {
		af.GetPhysicsToVisualTransform( origin, axis );
		return true;
	}
	origin = modelOffset;
	axis = viewAxis;
	return true;
}

/*
================
idActor::GetPhysicsToSoundTransform
================
*/
bool idActor::GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) {
	if ( soundJoint != INVALID_JOINT ) {
		animator.GetJointTransform( soundJoint, gameLocal.time, origin, axis );
		origin += modelOffset;
		axis = viewAxis;
	} else {
		origin = GetPhysics()->GetGravityNormal() * -eyeOffset.z;
		axis.Identity();
	}
	return true;
}

/***********************************************************************

	script state management

***********************************************************************/

/*
================
idActor::ShutdownThreads
================
*/
void idActor::ShutdownThreads( void ) {
	headAnim.Shutdown();
	torsoAnim.Shutdown();
	legsAnim.Shutdown();

	if ( scriptThread ) {
		scriptThread->EndThread();
		scriptThread->PostEventMS( &EV_Remove, 0 );
		delete scriptThread;
		scriptThread = NULL;
	}
}

/*
================
idActor::ShouldConstructScriptObjectAtSpawn

Called during idEntity::Spawn to see if it should construct the script object or not.
Overridden by subclasses that need to spawn the script object themselves.
================
*/
bool idActor::ShouldConstructScriptObjectAtSpawn( void ) const {
	return false;
}

/*
================
idActor::ConstructScriptObject

Called during idEntity::Spawn.  Calls the constructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
================
*/
idThread *idActor::ConstructScriptObject( void ) {
	const function_t *constructor;

	// make sure we have a scriptObject
	if ( !scriptObject.HasObject() ) {
		gameLocal.Error( "No scriptobject set on '%s'.  Check the '%s' entityDef.", name.c_str(), GetEntityDefName() );
	}

	if ( !scriptThread ) {
		// create script thread
		scriptThread = new idThread();
		scriptThread->ManualDelete();
		scriptThread->ManualControl();
		scriptThread->SetThreadName( name.c_str() );
	} else {
		scriptThread->EndThread();
	}

	// call script object's constructor
	constructor = scriptObject.GetConstructor();
	if ( !constructor ) {
		gameLocal.Error( "Missing constructor on '%s' for entity '%s'", scriptObject.GetTypeName(), name.c_str() );
	}

	// init the script object's data
	scriptObject.ClearObject();

	// just set the current function on the script.  we'll execute in the subclasses.
	scriptThread->CallFunction( this, constructor, true );

	return scriptThread;
}

/*
=====================
idActor::GetScriptFunction
=====================
*/
const function_t *idActor::GetScriptFunction( const char *funcname ) {
	const function_t *func;

	func = scriptObject.GetFunction( funcname );
	if ( !func ) {
		scriptThread->Error( "Unknown function '%s' in '%s'", funcname, scriptObject.GetTypeName() );
	}

	return func;
}

/*
=====================
idActor::SetState
=====================
*/
void idActor::SetState( const function_t *newState ) {
	if ( !newState ) {
		gameLocal.Error( "idActor::SetState: Null state" );
	}

	if ( ai_debugScript.GetInteger() == entityNumber ) {
		gameLocal.Printf( "%d: %s: State: %s\n", gameLocal.time, name.c_str(), newState->Name() );
	}

	state = newState;
	idealState = state;
	scriptThread->CallFunction( this, state, true );
}

/*
=====================
idActor::SetState
=====================
*/
void idActor::SetState( const char *statename ) {
	const function_t *newState;

	newState = GetScriptFunction( statename );
	SetState( newState );
}

/*
=====================
idActor::UpdateScript
=====================
*/
void idActor::UpdateScript( void ) {
	int	i;

	if ( ai_debugScript.GetInteger() == entityNumber ) {
		scriptThread->EnableDebugInfo();
	} else {
		scriptThread->DisableDebugInfo();
	}

	// a series of state changes can happen in a single frame.
	// this loop limits them in case we've entered an infinite loop.
	for( i = 0; i < 20; i++ ) {
		if ( idealState != state ) {
			SetState( idealState );
		}

		// don't call script until it's done waiting
		if ( scriptThread->IsWaiting() ) {
			break;
		}

		scriptThread->Execute();
		if ( idealState == state ) {
			break;
		}
	}

	if ( i == 20 ) {
		scriptThread->Warning( "idActor::UpdateScript: exited loop to prevent lockup" );
	}
}

/***********************************************************************

	vision

***********************************************************************/

/*
=====================
idActor::setFov
=====================
*/
void idActor::SetFOV( float fov ) {
	fovDot = (float)cos( DEG2RAD( fov * 0.5f ) );
}

/*
=====================
idActor::SetEyeHeight
=====================
*/
void idActor::SetEyeHeight( float height ) {
	eyeOffset.z = height;
}

/*
=====================
idActor::EyeHeight
=====================
*/
float idActor::EyeHeight( void ) const {
	return eyeOffset.z;
}

/*
=====================
idActor::EyeOffset
=====================
*/
idVec3 idActor::EyeOffset( void ) const {
	return GetPhysics()->GetGravityNormal() * -eyeOffset.z;
}

/*
=====================
idActor::GetEyePosition
=====================
*/
idVec3 idActor::GetEyePosition( void ) const {
	return GetPhysics()->GetOrigin() + ( GetPhysics()->GetGravityNormal() * -eyeOffset.z );
}

/*
=====================
idActor::GetViewPos
=====================
*/
void idActor::GetViewPos( idVec3 &origin, idMat3 &axis ) const {
	origin = GetEyePosition();
	axis = viewAxis;
}

/*
=====================
idActor::CheckFOV
=====================
*/
bool idActor::CheckFOV( const idVec3 &pos ) const {
	if ( fovDot == 1.0f ) {
		return true;
	}

	float	dot;
	idVec3	delta;

	delta = pos - GetEyePosition();

	// get our gravity normal
	const idVec3 &gravityDir = GetPhysics()->GetGravityNormal();

	// infinite vertical vision, so project it onto our orientation plane
	delta -= gravityDir * ( gravityDir * delta );

	delta.Normalize();
	dot = viewAxis[ 0 ] * delta;

	return ( dot >= fovDot );
}

/*
=====================
idActor::CanSee
=====================
*/
bool idActor::CanSee( idEntity *ent, bool useFov ) const {
	trace_t		tr;
	idVec3		eye;
	idVec3		toPos;

	if ( ent->IsHidden() ) {
		return false;
	}

	if ( ent->IsType( idActor::GetClassType() ) ) {
		toPos = ( ( idActor * )ent )->GetEyePosition();
	} else {
		toPos = ent->GetPhysics()->GetOrigin();
	}

	if ( useFov && !CheckFOV( toPos ) ) {
		return false;
	}

	eye = GetEyePosition();

	gameLocal.clip.TracePoint( tr, eye, toPos, MASK_OPAQUE, this );
	if ( tr.fraction >= 1.0f || ( gameLocal.GetTraceEntity( tr ) == ent ) ) {
		return true;
	}

	return false;
}

/*
=====================
idActor::PointVisible
=====================
*/
bool idActor::PointVisible( const idVec3 &point ) const {
	trace_t results;
	idVec3 start, end;

	start = GetEyePosition();
	end = point;
	end[2] += 1.0f;

	gameLocal.clip.TracePoint( results, start, end, MASK_OPAQUE, this );
	return ( results.fraction >= 1.0f );
}

/*
=====================
idActor::GetAIAimTargets

Returns positions for the AI to aim at.
=====================
*/
void idActor::GetAIAimTargets( const idVec3 &lastSightPos, idVec3 &headPos, idVec3 &chestPos ) {
	headPos = lastSightPos + EyeOffset();
	chestPos = ( headPos + lastSightPos + GetPhysics()->GetBounds().GetCenter() ) * 0.5f;
}

/*
=====================
idActor::GetRenderView
=====================
*/
renderView_t *idActor::GetRenderView() {
	renderView_t *rv = idEntity::GetRenderView();
	rv->viewaxis = viewAxis;
	rv->vieworg = GetEyePosition();
	return rv;
}

/***********************************************************************

	Model/Ragdoll

***********************************************************************/

/*
================
idActor::SetCombatModel
================
*/
void idActor::SetCombatModel( void ) {
	idAFAttachment *headEnt;

	if ( !use_combat_bbox ) {
		if ( combatModel ) {
			combatModel->Unlink();
			combatModel->LoadModel( modelDefHandle );
		} else {
			combatModel = new idClipModel( modelDefHandle );
		}

		headEnt = head.GetEntity();
		if ( headEnt ) {
			headEnt->SetCombatModel();
		}
	}
}

/*
================
idActor::GetCombatModel
================
*/
idClipModel *idActor::GetCombatModel( void ) const {
	return combatModel;
}

/*
================
idActor::LinkCombat
================
*/
void idActor::LinkCombat( void ) {
	idAFAttachment *headEnt;

	if ( fl.hidden || use_combat_bbox ) {
		return;
	}

	if ( combatModel ) {
		combatModel->Link( gameLocal.clip, this, 0, renderEntity.origin, renderEntity.axis, modelDefHandle );
	}
	headEnt = head.GetEntity();
	if ( headEnt ) {
		headEnt->LinkCombat();
	}
}

/*
================
idActor::UnlinkCombat
================
*/
void idActor::UnlinkCombat( void ) {
	idAFAttachment *headEnt;

	if ( combatModel ) {
		combatModel->Unlink();
	}
	headEnt = head.GetEntity();
	if ( headEnt ) {
		headEnt->UnlinkCombat();
	}
}

/*
================
idActor::StartRagdoll
================
*/
bool idActor::StartRagdoll( void ) {
	float slomoStart, slomoEnd;
	float jointFrictionDent, jointFrictionDentStart, jointFrictionDentEnd;
	float contactFrictionDent, contactFrictionDentStart, contactFrictionDentEnd;

	// if no AF loaded
	if ( !af.IsLoaded() ) {
		return false;
	}

	// if the AF is already active
	if ( af.IsActive() ) {
		return true;
	}

	// disable the monster bounding box
	GetPhysics()->DisableClip();

	// start using the AF
	af.StartFromCurrentPose( spawnArgs.GetInt( "velocityTime", "0" ) );

	slomoStart = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_slomoStart", "-1.6" );
	slomoEnd = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_slomoEnd", "0.8" );

	#if MD5_ENABLE_GIBS > 0 // DEATH
	if (damageEmitDeath && damageEmitDeath < gameLocal.time) {
		slomoStart += 1.60f;
		slomoEnd   += 1.60f;
	}
	#endif

	// do the first part of the death in slow motion
	af.GetPhysics()->SetTimeScaleRamp( slomoStart, slomoEnd );

	jointFrictionDent = spawnArgs.GetFloat( "ragdoll_jointFrictionDent", "0.1" );
	jointFrictionDentStart = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_jointFrictionStart", "0.2" );
	jointFrictionDentEnd = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_jointFrictionEnd", "1.2" );

	// set joint friction dent
	af.GetPhysics()->SetJointFrictionDent( jointFrictionDent, jointFrictionDentStart, jointFrictionDentEnd );

	contactFrictionDent = spawnArgs.GetFloat( "ragdoll_contactFrictionDent", "0.1" );
	contactFrictionDentStart = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_contactFrictionStart", "1.0" );
	contactFrictionDentEnd = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_contactFrictionEnd", "2.0" );

	// set contact friction dent
	af.GetPhysics()->SetContactFrictionDent( contactFrictionDent, contactFrictionDentStart, contactFrictionDentEnd );

	// drop any items the actor is holding
	idMoveableItem::DropItems( this, "death", NULL );

	// drop any articulated figures the actor is holding
	idAFEntity_Base::DropAFs( this, "death", NULL );

	RemoveAttachments();

	return true;
}

/*
================
idActor::StopRagdoll
================
*/
void idActor::StopRagdoll( void ) {
	if ( af.IsActive() ) {
		af.Stop();
	}
}

/*
================
idActor::UpdateAnimationControllers
================
*/
bool idActor::UpdateAnimationControllers( void ) {

	if ( af.IsActive() ) {
		return idAFEntity_Base::UpdateAnimationControllers();
	} else {
		animator.ClearAFPose();
	}

	if ( walkIK.IsInitialized() ) {
		walkIK.Evaluate();
		return true;
	}

	return false;
}

/*
================
idActor::RemoveAttachments
================
*/
void idActor::RemoveAttachments( void ) {
	int i;
	idEntity *ent;

	// remove any attached entities
	for( i = 0; i < attachments.Num(); i++ ) {
		ent = attachments[ i ].ent.GetEntity();
		if ( ent && ent->spawnArgs.GetBool( "remove" ) ) {
			ent->PostEventMS( &EV_Remove, 0 );
		}
	}
}

/*
================
idActor::Attach
================
*/
void idActor::Attach( idEntity *ent ) {
	idVec3			origin;
	idMat3			axis;
	jointHandle_t	joint;
	idStr			jointName;
	idAttachInfo	&attach = attachments.Alloc();
	idAngles		angleOffset;
	idVec3			originOffset;

	jointName = ent->spawnArgs.GetString( "joint" );
	joint = animator.GetJointHandle( jointName );
	if ( joint == INVALID_JOINT ) {
		gameLocal.Error( "Joint '%s' not found for attaching '%s' on '%s'", jointName.c_str(), ent->GetClassname(), name.c_str() );
	}

	angleOffset = ent->spawnArgs.GetAngles( "angles" );
	originOffset = ent->spawnArgs.GetVector( "origin" );

	attach.channel = animator.GetChannelForJoint( joint );
	GetJointWorldTransform( joint, gameLocal.time, origin, axis );
	attach.ent = ent;

	ent->SetOrigin( origin + originOffset * renderEntity.axis );
	idMat3 rotate = angleOffset.ToMat3();
	idMat3 newAxis = rotate * axis;
	ent->SetAxis( newAxis );
	ent->BindToJoint( this, joint, true );
	ent->cinematic = cinematic;
}

/*
================
idActor::Teleport
================
*/
void idActor::Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination ) {
	GetPhysics()->SetOrigin( origin + idVec3( 0, 0, CM_CLIP_EPSILON ) );
	GetPhysics()->SetLinearVelocity( vec3_origin );

	viewAxis = angles.ToMat3();

	UpdateVisuals();

	if ( !IsHidden() ) {
		// kill anything at the new position
		gameLocal.KillBox( this );
	}
}

/*
================
idActor::GetDeltaViewAngles
================
*/
const idAngles &idActor::GetDeltaViewAngles( void ) const {
	return deltaViewAngles;
}

/*
================
idActor::SetDeltaViewAngles
================
*/
void idActor::SetDeltaViewAngles( const idAngles &delta ) {
	deltaViewAngles = delta;
}

/*
================
idActor::HasEnemies
================
*/
bool idActor::HasEnemies( void ) const {
	idActor *ent;

	for( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
		if ( !ent->fl.hidden ) {
			return true;
		}
	}

	return false;
}

/*
================
idActor::ClosestEnemyToPoint
================
*/
idActor *idActor::ClosestEnemyToPoint( const idVec3 &pos ) {
	idActor		*ent;
	idActor		*bestEnt;
	float		bestDistSquared;
	float		distSquared;
	idVec3		delta;

	bestDistSquared = idMath::INFINITY;
	bestEnt = NULL;
	for( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
		if ( ent->fl.hidden ) {
			continue;
		}
		delta = ent->GetPhysics()->GetOrigin() - pos;
		distSquared = delta.LengthSqr();
		if ( distSquared < bestDistSquared ) {
			bestEnt = ent;
			bestDistSquared = distSquared;
		}
	}

	return bestEnt;
}

/*
================
idActor::EnemyWithMostHealth
================
*/
idActor *idActor::EnemyWithMostHealth() {
	idActor		*ent;
	idActor		*bestEnt;

	int most = -9999;
	bestEnt = NULL;
	for( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
		if ( !ent->fl.hidden && ( ent->health > most ) ) {
			bestEnt = ent;
			most = ent->health;
		}
	}
	return bestEnt;
}

/*
================
idActor::OnLadder
================
*/
bool idActor::OnLadder( void ) const {
	return false;
}

/*
==============
idActor::GetAASLocation
==============
*/
void idActor::GetAASLocation( idAAS *aas, idVec3 &pos, int &areaNum ) const {
	idVec3		size;
	idBounds	bounds;

	GetFloorPos( 64.0f, pos );
	if ( !aas ) {
		areaNum = 0;
		return;
	}

	size = aas->GetSettings()->boundingBoxes[0][1];
	bounds[0] = -size;
	size.z = 32.0f;
	bounds[1] = size;

	areaNum = aas->PointReachableAreaNum( pos, bounds, AREA_REACHABLE_WALK );
	if ( areaNum ) {
		aas->PushPointIntoAreaNum( areaNum, pos );
	}
}

/***********************************************************************

	animation state

***********************************************************************/

/*
=====================
idActor::SetAnimState
=====================
*/
void idActor::SetAnimState( int channel, const char *statename, int blendFrames ) {
	const function_t *func;

	func = scriptObject.GetFunction( statename );
	if ( !func ) {
		assert( 0 );
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, scriptObject.GetTypeName() );
	}

	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.SetState( statename, blendFrames );
		allowEyeFocus = true;
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.SetState( statename, blendFrames );
		legsAnim.Enable( blendFrames );
		allowPain = true;
		allowEyeFocus = true;
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.SetState( statename, blendFrames );
		torsoAnim.Enable( blendFrames );
		allowPain = true;
		allowEyeFocus = true;
		break;

	default:
		gameLocal.Error( "idActor::SetAnimState: Unknown anim group" );
		break;
	}
}

/*
=====================
idActor::GetAnimState
=====================
*/
const char *idActor::GetAnimState( int channel ) const {
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		return headAnim.state;
		break;

	case ANIMCHANNEL_TORSO :
		return torsoAnim.state;
		break;

	case ANIMCHANNEL_LEGS :
		return legsAnim.state;
		break;

	default:
		gameLocal.Error( "idActor::GetAnimState: Unknown anim group" );
		return NULL;
		break;
	}
}

/*
=====================
idActor::InAnimState
=====================
*/
bool idActor::InAnimState( int channel, const char *statename ) const {
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		if ( headAnim.state == statename ) {
			return true;
		}
		break;

	case ANIMCHANNEL_TORSO :
		if ( torsoAnim.state == statename ) {
			return true;
		}
		break;

	case ANIMCHANNEL_LEGS :
		if ( legsAnim.state == statename ) {
			return true;
		}
		break;

	default:
		gameLocal.Error( "idActor::InAnimState: Unknown anim group" );
		break;
	}

	return false;
}

/*
=====================
idActor::WaitState
=====================
*/
const char *idActor::WaitState( void ) const {
	if ( waitState.Length() ) {
		return waitState;
	} else {
		return NULL;
	}
}

/*
=====================
idActor::SetWaitState
=====================
*/
void idActor::SetWaitState( const char *_waitstate ) {
	waitState = _waitstate;
}

/*
=====================
idActor::UpdateAnimState
=====================
*/
void idActor::UpdateAnimState( void ) {
	headAnim.UpdateState();
	torsoAnim.UpdateState();
	legsAnim.UpdateState();
}

/*
=====================
idActor::GetAnim
=====================
*/
int idActor::GetAnim( int channel, const char *animname ) {
	int			anim;
	const char *temp;
	idAnimator *animatorPtr;

	if ( channel == ANIMCHANNEL_HEAD ) {
		if ( !head.GetEntity() ) {
			return 0;
		}
		animatorPtr = head.GetEntity()->GetAnimator();
	} else {
		animatorPtr = &animator;
	}

	if ( animPrefix.Length() ) {
		temp = va( "%s_%s", animPrefix.c_str(), animname );
		#if MD5_ENABLE_GIBS > 0 // ANIMS PERMIT
		anim = animatorPtr->GetAnim(temp, GetRenderEntity()->gibbedZones);
		#else
		anim = animatorPtr->GetAnim(temp);
		#endif
		if ( anim ) {
			return anim;
		}
	}

	#if MD5_ENABLE_GIBS > 0 // ANIMS PERMIT
	anim = animatorPtr->GetAnim(animname, GetRenderEntity()->gibbedZones);
	#else
	anim = animatorPtr->GetAnim(animname);
	#endif

	return anim;
}

/*
===============
idActor::SyncAnimChannels
===============
*/
void idActor::SyncAnimChannels( int channel, int syncToChannel, int blendFrames ) {
	idAnimator		*headAnimator;
	idAFAttachment	*headEnt;
	int				anim;
	idAnimBlend		*syncAnim;
	int				starttime;
	int				blendTime;
	int				cycle;

	blendTime = FRAME2MS( blendFrames );
	if ( channel == ANIMCHANNEL_HEAD ) {
		headEnt = head.GetEntity();
		if ( headEnt ) {
			headAnimator = headEnt->GetAnimator();
			syncAnim = animator.CurrentAnim( syncToChannel );
			if ( syncAnim ) {
				anim = headAnimator->GetAnim( syncAnim->AnimFullName() );
				if ( !anim ) {
					anim = headAnimator->GetAnim( syncAnim->AnimName() );
				}
				if ( anim ) {
					cycle = animator.CurrentAnim( syncToChannel )->GetCycleCount();
					starttime = animator.CurrentAnim( syncToChannel )->GetStartTime();
					headAnimator->PlayAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, blendTime );
					headAnimator->CurrentAnim( ANIMCHANNEL_ALL )->SetCycleCount( cycle );
					headAnimator->CurrentAnim( ANIMCHANNEL_ALL )->SetStartTime( starttime );
				} else {
					headEnt->PlayIdleAnim( blendTime );
				}
			}
		}
	} else if ( syncToChannel == ANIMCHANNEL_HEAD ) {
		headEnt = head.GetEntity();
		if ( headEnt ) {
			headAnimator = headEnt->GetAnimator();
			syncAnim = headAnimator->CurrentAnim( ANIMCHANNEL_ALL );
			if ( syncAnim ) {
				anim = GetAnim( channel, syncAnim->AnimFullName() );
				if ( !anim ) {
					anim = GetAnim( channel, syncAnim->AnimName() );
				}
				if ( anim ) {
					cycle = headAnimator->CurrentAnim( ANIMCHANNEL_ALL )->GetCycleCount();
					starttime = headAnimator->CurrentAnim( ANIMCHANNEL_ALL )->GetStartTime();
					animator.PlayAnim( channel, anim, gameLocal.time, blendTime );
					animator.CurrentAnim( channel )->SetCycleCount( cycle );
					animator.CurrentAnim( channel )->SetStartTime( starttime );
				}
			}
		}
	} else {
		animator.SyncAnimChannels( channel, syncToChannel, gameLocal.time, blendTime );
	}
}

/***********************************************************************

	Damage

***********************************************************************/

/*
============
idActor::Gib
============
*/
void idActor::Gib( const idVec3 &dir, const char *damageDefName ) {
	// no gibbing in multiplayer - by self damage or by moving objects
	if ( gameLocal.isMultiplayer ) {
		return;
	}
	// only gib once
	if ( gibbed ) {
		return;
	}
	idAFEntity_Gibbable::Gib( dir, damageDefName );
	if ( head.GetEntity() ) {
		head.GetEntity()->Hide();
	}
	StopSound( SND_CHANNEL_VOICE, false );
}


/*
============
idActor::Damage

this		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: this=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback in global space
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted

inflictor, attacker, dir, and point can be NULL for environmental effects

Bleeding wounds and surface overlays are applied in the collision code that
calls Damage()
============
*/
void idActor::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir,
					  const char *damageDefName, const float damageScale, const int location ) {
	if ( !fl.takedamage ) {
		return;
	}

	if ( !inflictor ) {
		inflictor = gameLocal.world;
	}
	if ( !attacker ) {
		attacker = gameLocal.world;
	}

#ifdef _D3XP
	SetTimeState ts( timeGroup );

	// Helltime boss is immune to all projectiles except the helltime killer
	if ( finalBoss && idStr::Icmp( inflictor->GetEntityDefName(), "projectile_helltime_killer" ) ) {
		return;
	}

	// Maledict is immume to the falling asteroids
	if ( !idStr::Icmp( GetEntityDefName(), "monster_boss_d3xp_maledict" ) &&
		(!idStr::Icmp( damageDefName, "damage_maledict_asteroid" ) || !idStr::Icmp( damageDefName, "damage_maledict_asteroid_splash" ) ) ) {
		return;
	}
#else
	if ( finalBoss && !inflictor->IsType( idSoulCubeMissile::GetClassType() ) ) {
		return;
	}
#endif

	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( !damageDef ) {
		gameLocal.Error( "Unknown damageDef '%s'", damageDefName );
	}

	int	damage = damageDef->GetInt( "damage" ) * damageScale;
	damage = GetDamageForLocation( damage, location );

	// inform the attacker that they hit someone
	attacker->DamageFeedback( this, inflictor, damage );
	if ( damage > 0 ) {
		#if MD5_ENABLE_GIBS > 0
		int gibbedZone = 0;
		int gibbedFlag = 0;
		int gibbedPart = 0;
		if (location > INVALID_JOINT && pain_threshold && pain_threshold <= damage) {
			gibbedZone = damageBonesZone[location];
			if (damageZonesHeap[0] < gameLocal.time) {
				memset(damageZonesHeap.Ptr(), 0, damageZonesHeap.MemoryUsed());
			}
			damageZonesHeap[0] = gameLocal.time + 1000; // Accumulate damage received within one second of the previous (mostly for shotguns).
			if (gibbedZone) {
				damageZonesHeap[gibbedZone] += damage;
				#if MD5_ENABLE_GIBS > 2 // DEBUG
				if (
					(ai_testDismemberment.GetInteger() == 4) ||
					(ai_testDismemberment.GetInteger() == 3 && (damage >= health * (gibbedZone == 2 ? 2 : 1))) || // Require 2x current health to gib body.
					(ai_testDismemberment.GetInteger() == 2 && (damageZonesHeap[gibbedZone] >= 50 || (damageZonesHeap[gibbedZone] >= 25 && gibbedZone >= 3))) || // Require 2x to gib head/body (given head usually receives 2x).
					(ai_testDismemberment.GetInteger() <= 1 && (damageZonesHeap[gibbedZone] >= health * (gibbedZone == 2 ? 4 : 1))) // Require 3x to gib body.
				) {
				#else
				if (damageZonesHeap[gibbedZone] >= health * (gibbedZone == 2 ? 4 : 1)) {
				#endif
					renderEntity_t* gibbedBody = &renderEntity;
					renderEntity_t* gibbedHead = head.GetEntity() ? head.GetEntity()->GetRenderEntity() : NULL;
					gibbedFlag |= (1 << gibbedZone);
					if (gibbedBody) gibbedPart |= (gibbedBody->hModel->gibParts & gibbedFlag);
					if (gibbedHead) gibbedPart |= (gibbedHead->hModel->gibParts & gibbedFlag);
					if (gibbedPart) {
						if (gibbedBody) Sever(gibbedBody, gibbedPart);
						if (gibbedHead) Sever(gibbedHead, gibbedPart);
						if (gibbedPart) health += damage;
					}
				}
			}
		}
		#endif
		health -= damage;

#ifdef _D3XP
		//Check the health against any damage cap that is currently set
		if ( damageCap >= 0 && health < damageCap ) {
			health = damageCap;
		}
#endif

		if ( health <= 0 ) {
			if ( health < -999 ) {
				health = -999;
			}
			#if MD5_ENABLE_GIBS > 0
			renderEntity.gibbedZones |= 0x0001;
			#endif
			Killed( inflictor, attacker, damage, dir, location );
			if ( ( health < -20 ) && spawnArgs.GetBool( "gib" ) && damageDef->GetBool( "gib" ) ) {
				Gib( dir, damageDefName );
			}
		} else {
			#if MD5_ENABLE_GIBS > 0
			if (gibbedPart & 0xF000) Bleed(gibbedPart, damageBonesZone[location]);
			#endif
			Pain( inflictor, attacker, damage, dir, location );
		}
	} else {
		// don't accumulate knockback
		if ( af.IsLoaded() ) {
			// clear impacts
			af.Rest();

			// physics is turned off by calling af.Rest()
			BecomeActive( TH_PHYSICS );
		}
	}
}

/*
=====================
idActor::ClearPain
=====================
*/
void idActor::ClearPain( void ) {
	pain_debounce_time = 0;
}

/*
=====================
idActor::Pain
=====================
*/
bool idActor::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( af.IsLoaded() ) {
		// clear impacts
		af.Rest();

		// physics is turned off by calling af.Rest()
		BecomeActive( TH_PHYSICS );
	}

	if ( gameLocal.time < pain_debounce_time ) {
		return false;
	}

	// don't play pain sounds more than necessary
	pain_debounce_time = gameLocal.time + pain_delay;

	if ( health > 75  ) {
		StartSound( "snd_pain_small", SND_CHANNEL_VOICE, 0, false, NULL );
	} else if ( health > 50 ) {
		StartSound( "snd_pain_medium", SND_CHANNEL_VOICE, 0, false, NULL );
	} else if ( health > 25 ) {
		StartSound( "snd_pain_large", SND_CHANNEL_VOICE, 0, false, NULL );
	} else {
		StartSound( "snd_pain_huge", SND_CHANNEL_VOICE, 0, false, NULL );
	}

	if ( !allowPain || ( gameLocal.time < painTime ) ) {
		// don't play a pain anim
		return false;
	}

	if ( pain_threshold && ( damage < pain_threshold ) ) {
		return false;
	}

	// set the pain anim
	idStr damageGroup = GetDamageGroup( location );

	painAnim = "";
	if ( animPrefix.Length() ) {
		if ( damageGroup.Length() && ( damageGroup != "legs" ) ) {
			sprintf( painAnim, "%s_pain_%s", animPrefix.c_str(), damageGroup.c_str() );
			if ( !animator.HasAnim( painAnim ) ) {
				sprintf( painAnim, "pain_%s", damageGroup.c_str() );
				if ( !animator.HasAnim( painAnim ) ) {
					painAnim = "";
				}
			}
		}

		if ( !painAnim.Length() ) {
			sprintf( painAnim, "%s_pain", animPrefix.c_str() );
			if ( !animator.HasAnim( painAnim ) ) {
				painAnim = "";
			}
		}
	} else if ( damageGroup.Length() && ( damageGroup != "legs" ) ) {
		sprintf( painAnim, "pain_%s", damageGroup.c_str() );
		if ( !animator.HasAnim( painAnim ) ) {
			sprintf( painAnim, "pain_%s", damageGroup.c_str() );
			if ( !animator.HasAnim( painAnim ) ) {
				painAnim = "";
			}
		}
	}

	if ( !painAnim.Length() ) {
		painAnim = "pain";
	}

	if ( g_debugDamage.GetBool() ) {
		gameLocal.Printf( "Damage: joint: '%s', zone '%s', anim '%s'\n", animator.GetJointName( ( jointHandle_t )location ),
			damageGroup.c_str(), painAnim.c_str() );
	}

	return true;
}

#if MD5_ENABLE_GIBS > 0

/* =====================
idActor::Sever
===================== */
void idActor::Sever(renderEntity_t* entity, int& zone) {

	if /*el*/ (entity->gibbedZones      & zone) { // Formerly gibbed.
		zone  = MD5_GIBBED_ZERO;
	} else if (entity->hModel->gibBlood & zone) { // Zone will bleed.
		zone |= MD5_GIBFX_BLOOD;
	} else if (entity->hModel->gibGloop & zone) { // Zone will gloop.
		zone |= MD5_GIBFX_GLOOP;
	} else if (entity->hModel->gibFlame & zone) { // Zone will flame.
		zone |= MD5_GIBFX_FLAME;
	} else if (entity->hModel->gibSpark & zone) { // Zone will spark.
		zone |= MD5_GIBFX_SPARK;
	} else if (entity->hModel->gibClass & zone) { // Custom particle.
		zone |= MD5_GIBFX_CLASS;
	}

	if (zone) entity->gibbedZones |= zone & MD5_GIBBED_BITS;

}

/* =====================
idActor::Bleed
===================== */
void idActor::Bleed(int gibbedPart, int gibbedZone) {

	const char* const bodySever[] = {"sever_body_blood", "sever_body_gloop", "sever_body_flame", "sever_body_spark"};
	const char* const bodySpray[] = {"spray_body_blood", "spray_body_gloop", "spray_body_flame", "spray_body_spark"};
	const char* const headSever[] = {"sever_head_blood", "sever_head_gloop", "sever_head_flame", "sever_head_spark"};
	const char* const headSpray[] = {"spray_head_blood", "spray_head_gloop", "spray_head_flame", "spray_head_spark"};
	const char* const limbSever[] = {"sever_limb_blood", "sever_limb_gloop", "sever_limb_flame", "sever_limb_spark"};
	const char* const limbSpray[] = {"spray_limb_blood", "spray_limb_gloop", "spray_limb_flame", "spray_limb_spark"};

	if (gibbedPart && gibbedZone) { // New gib.
		if /*el*/ ((gibbedPart & MD5_GIBBED_BODY) != 0) { // BODY
			damageEmitShift = idVec3(0.00f, 0.00f, damageZonesDrop[gibbedZone][3]);
			damageEmitAngle = idAngles(damageZonesDrop[gibbedZone].ToVec3()).ToMat3();
			damageEmitJoint = jointHandle_t(damageZonesBone[gibbedZone]);
			damageEmitStart = gameLocal.time;
			damageEmitStage = gibbedPart; gibbedPart /= MD5_GIBFX_BLOOD; gibbedPart--;
			damageEmitSpray = static_cast<const idDeclParticle*>(declManager->FindType(DECL_PARTICLE, gibbedPart < 4 ? bodySpray[gibbedPart] : va("spray_body_%s", spawnArgs.GetString("classname"))));
			damageEmitSever = static_cast<const idDeclParticle*>(declManager->FindType(DECL_PARTICLE, gibbedPart < 4 ? bodySever[gibbedPart] : va("sever_body_%s", spawnArgs.GetString("classname"))));
			if (damageZonesKill[gibbedZone] && (damageEmitDeath == 0 || damageEmitDeath > gameLocal.time + damageZonesKill[gibbedZone])) {
				damageEmitDeath = gameLocal.time + damageZonesKill[gibbedZone];
			}
		} else if ((gibbedPart & MD5_GIBBED_HEAD) != 0 && (renderEntity.gibbedZones & MD5_GIBBED_BODY) == 0) { // HEAD & not already truncated.
			damageEmitShift = idVec3(0.00f, 0.00f, damageZonesDrop[gibbedZone][3]);
			damageEmitAngle = idAngles(damageZonesDrop[gibbedZone].ToVec3()).ToMat3();
			damageEmitJoint = jointHandle_t(damageZonesBone[gibbedZone]);
			damageEmitStart = gameLocal.time;
			damageEmitStage = gibbedPart; gibbedPart /= MD5_GIBFX_BLOOD; gibbedPart--;
			damageEmitSpray = static_cast<const idDeclParticle*>(declManager->FindType(DECL_PARTICLE, gibbedPart < 4 ? headSpray[gibbedPart] : va("spray_head_%s", spawnArgs.GetString("classname"))));
			damageEmitSever = static_cast<const idDeclParticle*>(declManager->FindType(DECL_PARTICLE, gibbedPart < 4 ? headSever[gibbedPart] : va("sever_head_%s", spawnArgs.GetString("classname"))));
			if (damageZonesKill[gibbedZone] && (damageEmitDeath == 0 || damageEmitDeath > gameLocal.time + damageZonesKill[gibbedZone])) {
				damageEmitDeath = gameLocal.time + damageZonesKill[gibbedZone];
			}
		} else if ((renderEntity.gibbedZones & MD5_GIBBED_CORE) == 0 && (damageEmitStage == 0 || damageEmitStage > (gibbedPart & 0x0FFF))) { // LIMB & not already truncated or decapitated & lower index than prior gibs this frame.
			damageEmitShift = idVec3(0.00f, 0.00f, damageZonesDrop[gibbedZone][3]);
			damageEmitAngle = idAngles(damageZonesDrop[gibbedZone].ToVec3()).ToMat3();
			damageEmitJoint = jointHandle_t(damageZonesBone[gibbedZone]);
			damageEmitStart = gameLocal.time;
			damageEmitStage = gibbedPart; gibbedPart /= MD5_GIBFX_BLOOD; gibbedPart--;
			damageEmitSpray = static_cast<const idDeclParticle*>(declManager->FindType(DECL_PARTICLE, gibbedPart < 4 ? limbSpray[gibbedPart] : va("spray_limb_%s", spawnArgs.GetString("classname"))));
			damageEmitSever = static_cast<const idDeclParticle*>(declManager->FindType(DECL_PARTICLE, gibbedPart < 4 ? limbSever[gibbedPart] : va("sever_limb_%s", spawnArgs.GetString("classname"))));
			if (damageZonesKill[gibbedZone] && (damageEmitDeath == 0 || damageEmitDeath > gameLocal.time + damageZonesKill[gibbedZone])) {
				damageEmitDeath = gameLocal.time + damageZonesKill[gibbedZone];
			}
		} else return; // Don't want multiple gibs in the same frame to prematurely run down the stage variable.
	}

	if (damageEmitStart && damageEmitJoint > INVALID_JOINT && damageEmitSever) {
		idVec3 emitOrigin;
		idMat3 emitMatrix;
		animator.GetJointTransform(damageEmitJoint, gameLocal.time, emitOrigin, emitMatrix);
		emitOrigin  = emitOrigin * renderEntity.axis + renderEntity.origin;
		emitMatrix  = emitMatrix * renderEntity.axis;
		emitMatrix  = damageEmitAngle * emitMatrix;
		emitOrigin += damageEmitShift * emitMatrix;
		#if MD5_ENABLE_GIBS > 2 // DEBUG
		if (ai_testDismemberment.GetInteger() >= 1) gameRenderWorld->DebugLine(colorYellow,  emitOrigin, idVec3(0.00f, 0.00f, 9.00f) * emitMatrix + emitOrigin, gameLocal.msec);
		if (ai_testDismemberment.GetInteger() >= 2) gameRenderWorld->DebugLine(colorCyan,    emitOrigin, idVec3(0.00f, 9.00f, 0.00f) * emitMatrix + emitOrigin, gameLocal.msec);
		if (ai_testDismemberment.GetInteger() >= 3) gameRenderWorld->DebugLine(colorMagenta, emitOrigin, idVec3(9.00f, 0.00f, 0.00f) * emitMatrix + emitOrigin, gameLocal.msec);
		#endif
		#ifdef _D3XP
		SetTimeState ts(timeGroup);
		if (gameLocal.smokeParticles->EmitSmoke(damageEmitSever, damageEmitStart, gameLocal.random.RandomFloat(), emitOrigin, emitMatrix, timeGroup) != true) {
		#else
		if (gameLocal.smokeParticles->EmitSmoke(damageEmitSever, damageEmitStart, gameLocal.random.RandomFloat(), emitOrigin, emitMatrix           ) != true) {
		#endif
			if /*el*/ (damageEmitDeath && damageEmitDeath < gameLocal.time) {
				damageEmitStart = 0;
				damageEmitStage = MD5_GIBBED_ZERO;
				Damage(this, this, GetPhysics()->GetGravityNormal() * 0.10f, "damage_suicide", 1.00f, INVALID_JOINT);
			} else if (damageEmitStage & MD5_GIBFX_INDEX) {
				damageEmitStart = gameLocal.time;
				damageEmitStage = MD5_GIBBED_BITS & damageEmitStage;
				damageEmitSever = damageEmitSpray;
			} else {
				damageEmitStart = gameLocal.time;
				damageEmitStage = MD5_GIBBED_ZERO;
			//	damageEmitSever = damageEmitSpray;
			}
		}
	}

}

#endif

/*
=====================
idActor::SpawnGibs
=====================
*/
void idActor::SpawnGibs( const idVec3 &dir, const char *damageDefName ) {
	idAFEntity_Gibbable::SpawnGibs( dir, damageDefName );
	RemoveAttachments();
}

/*
=====================
idActor::SetupDamageGroups

FIXME: only store group names once and store an index for each joint
=====================
*/
void idActor::SetupDamageGroups( void ) {
	int						i;
	const idKeyValue		*arg;
	idStr					groupname;
	idList<jointHandle_t>	jointList;
#if MD5_ENABLE_GIBS < 0
	int						jointnum;
	float					scale;
#endif

	// create damage zones
	#if MD5_ENABLE_GIBS > 0
	damageBonesZone.SetNum(animator.NumJoints());
	for (i = 0; i < damageBonesZone.Num(); i++) damageBonesZone[i] = 0;
	int zoneCount = 1; arg = spawnArgs.MatchPrefix("damage_zone ", NULL);
	while (arg) { zoneCount++; arg = spawnArgs.MatchPrefix("damage_zone ", arg); }
	damageZonesBone.SetNum(zoneCount); damageZonesBone[0] = INVALID_JOINT;
	damageZonesKill.SetNum(zoneCount); damageZonesKill[0] = 0;
	damageZonesHeap.SetNum(zoneCount); damageZonesHeap[0] = 0;
	damageZonesDrop.SetNum(zoneCount); damageZonesDrop[0] = vec4_zero;
	damageZonesRate.SetNum(zoneCount); damageZonesRate[0] = 1.00f;
	damageZonesName.SetNum(zoneCount); damageZonesName[0] = "";
	int zoneIndex = 1; arg = spawnArgs.MatchPrefix("damage_zone ", NULL);
	int zoneSplit;
	int boneIndex;
	while (arg) {
		groupname = arg->GetKey();
		groupname.Strip("damage_zone ");
		zoneSplit = groupname.Find(':'); if (zoneSplit >= 0) groupname = groupname.Left(zoneSplit);
		boneIndex = -1;
		animator.GetJointList(arg->GetValue(), jointList);
		for (i = 0; i < jointList.Num(); i++) {
			damageBonesZone[jointList[i]] = zoneIndex;
			if (boneIndex < 0) boneIndex = jointList[i];
		}
		damageZonesBone[zoneIndex] = boneIndex;
		damageZonesKill[zoneIndex] = zoneIndex == 1 ? 30000 : zoneIndex == 2 ? 7500 : 0;
		damageZonesHeap[zoneIndex] = 0;
		damageZonesDrop[zoneIndex] = zoneIndex == 1 ? idVec4(0.00f, 0.00f, -82.50f, 0.00f) : idVec4(0.00f, 0.00f, -90.00f, 4.00f);
		damageZonesRate[zoneIndex] = 1.00f;
		damageZonesName[zoneIndex] = groupname;
		jointList.Clear();
		zoneIndex++; arg = spawnArgs.MatchPrefix("damage_zone ", arg);
	}
	#else
	damageGroups.SetNum(animator.NumJoints());
	arg = spawnArgs.MatchPrefix("damage_zone ", NULL);
	while (arg) {
		groupname = arg->GetKey();
		groupname.Strip("damage_zone ");
		animator.GetJointList(arg->GetValue(), jointList);
		for (i = 0; i < jointList.Num(); i++) {
			jointnum = jointList[i];
			damageGroups[jointnum] = groupname;
		}
		jointList.Clear();
		arg = spawnArgs.MatchPrefix("damage_zone ", arg);
	}
	#endif

	#if MD5_ENABLE_GIBS > 0
	arg = spawnArgs.MatchPrefix("damage_spurt ", NULL);
	while (arg) {
		groupname = arg->GetKey();
		groupname.Strip("damage_spurt ");
		if (isdigit(groupname.c_str()[0]) && (i = groupname.c_str()[0] - 47) < damageZonesName.Num()) { // 1-based index (currently only accepts 0-9 where MD5s allow 0-B).
			#pragma warning(suppress:6031)
			sscanf(arg->GetValue().c_str(), "%f %f %f %f", &damageZonesDrop[i][0], &damageZonesDrop[i][1], &damageZonesDrop[i][2], &damageZonesDrop[i][3]);
		} else {
			for (i = 1; i < damageZonesName.Num(); i++) {
				if (damageZonesName[i] == groupname) {
					#pragma warning(suppress:6031)
					sscanf(arg->GetValue().c_str(), "%f %f %f %f", &damageZonesDrop[i][0], &damageZonesDrop[i][1], &damageZonesDrop[i][2], &damageZonesDrop[i][3]);
				}
			}
		}
		arg = spawnArgs.MatchPrefix("damage_spurt ", arg);
	}
	arg = spawnArgs.MatchPrefix("damage_scale ", NULL);
	while (arg) {
		groupname = arg->GetKey();
		groupname.Strip("damage_scale ");
		if (isdigit(groupname.c_str()[0]) && (i = groupname.c_str()[0] - 47) < damageZonesName.Num()) { // 1-based index (currently only accepts 0-9 where MD5s allow 0-B).
			damageZonesRate[i] = atof(arg->GetValue());
		} else {
			for (i = 1; i < damageZonesName.Num(); i++) {
				if (damageZonesName[i] == groupname) {
					damageZonesRate[i] = atof(arg->GetValue());
				}
			}
		}
		arg = spawnArgs.MatchPrefix("damage_scale ", arg);
	}
	#if MD5_ENABLE_GIBS > 2 // DEBUG
	if (ai_testDismemberment.GetBool()) {
		for (i = 0; i < damageBonesZone.Num(); i++) {
			zoneIndex = damageBonesZone[i];
			common->Printf("i:%i z:%s d:%f b:%i g:%i\n", i, damageZonesName[zoneIndex].c_str(), damageZonesRate[zoneIndex], damageZonesBone[zoneIndex], zoneIndex);
		}
	}
	#endif
	#else
	// initilize the damage zones to normal damage
	damageScales.SetNum(animator.NumJoints());
	for (i = 0; i < damageScales.Num(); i++) {
		damageScales[i] = 1.0f;
	}
	// set the percentage on damage zones
	arg = spawnArgs.MatchPrefix("damage_scale ", NULL);
	while (arg) {
		scale = atof(arg->GetValue());
		groupname = arg->GetKey();
		groupname.Strip("damage_scale ");
		for (i = 0; i < damageScales.Num(); i++) {
			if (damageGroups[i] == groupname) {
				damageScales[i] = scale;
			}
		}
		arg = spawnArgs.MatchPrefix("damage_scale ", arg);
	}
	#if MD5_ENABLE_GIBS < 0 // DEBUG
	for (i = 0; i < damageGroups.Num(); i++) {
		common->Printf("i:%i z:%s d:%f\n", i, damageGroups[i].c_str(), damageScales[i]);
	}
	#endif
	#endif

}

/*
=====================
idActor::GetDamageForLocation
=====================
*/
int idActor::GetDamageForLocation( int damage, int location ) {
	#if MD5_ENABLE_GIBS > 0
	if (location < 0 || location >= damageBonesZone.Num()) {
		return damage;
	}
	return (int)ceil(damage * damageZonesRate[damageBonesZone[location]]);
	#else
	if (location < 0 || location >= damageScales.Num()) {
		return damage;
	}
	return (int)ceil(damage * damageScales[location]);
	#endif
}

/*
=====================
idActor::GetDamageGroup
=====================
*/
const char *idActor::GetDamageGroup( int location ) {
	#if MD5_ENABLE_GIBS > 0
	if (location < 0 || location >= damageBonesZone.Num()) {
		return "";
	}
	return damageZonesName[damageBonesZone[location]];
	#else
	if (location < 0 || location >= damageGroups.Num()) {
		return "";
	}
	return damageGroups[location];
	#endif
}


/***********************************************************************

	Events

***********************************************************************/

/*
=====================
idActor::PlayFootStepSound
=====================
*/
void idActor::PlayFootStepSound( void ) {
	trace_t trace;
	int num = GetPhysics()->GetNumContacts();
	memset( &trace, 0, sizeof( trace_t ) );

	for( int i = 0; i < num; i++ ) {
		trace.c = GetPhysics()->GetContact( i );
		if( GetPhysics()->IsGroundEntity( trace.c.entityNum ) ) {
			GetFootstepSoundMaterial( trace );
			return;
		}
	}
}

/*
===============
idActor::GetFootstepSoundMaterial
===============
*/
void idActor::GetFootstepSoundMaterial( const trace_t& trace ) {
	if( !trace.c.material ) {
		return;
	}

	if ( trace.c.material->GetSurfaceFlags() & SURF_NOSTEPS ) {
		return;
	}

	surfTypes_t type = gameLocal.GetMaterialType( trace, "idActor::GetFootstepSoundMaterial" );

	const char* soundKey = gameLocal.MaterialTypeToKey( "snd_footstep", type );
	StartSound( soundKey, SND_CHANNEL_BODY3, 0, false, NULL );
}

/*
=====================
idActor::Event_EnableEyeFocus
=====================
*/
void idActor::EnableEyeFocus( void ) {
	allowEyeFocus = true;
	blink_time = gameLocal.time + blink_min + gameLocal.random.RandomFloat() * ( blink_max - blink_min );
}

/*
=====================
idActor::Event_DisableEyeFocus
=====================
*/
void idActor::DisableEyeFocus( void ) {
	allowEyeFocus = false;

	idEntity *headEnt = head.GetEntity();
	if ( headEnt ) {
		headEnt->GetAnimator()->Clear( ANIMCHANNEL_EYELIDS, gameLocal.time, FRAME2MS( 2 ) );
	} else {
		animator.Clear( ANIMCHANNEL_EYELIDS, gameLocal.time, FRAME2MS( 2 ) );
	}
}

/*
===============
idActor::Event_Footstep
===============
*/
void idActor::Footstep( void ) {
	PlayFootStepSound();
}

/*
=====================
idActor::Event_EnableWalkIK
=====================
*/
void idActor::EnableWalkIK( void ) {
	walkIK.EnableAll();
}

/*
=====================
idActor::Event_DisableWalkIK
=====================
*/
void idActor::DisableWalkIK( void ) {
	walkIK.DisableAll();
}

/*
=====================
idActor::Event_EnableLegIK
=====================
*/
void idActor::EnableLegIK( int num ) {
	walkIK.EnableLeg( num );
}

/*
=====================
idActor::Event_DisableLegIK
=====================
*/
void idActor::DisableLegIK( int num ) {
	walkIK.DisableLeg( num );
}

/*
=====================
idActor::Event_PreventPain
=====================
*/
void idActor::PreventPain( float duration ) {
	painTime = gameLocal.time + SEC2MS( duration );
}

/*
===============
idActor::Event_DisablePain
===============
*/
void idActor::DisablePain( void ) {
	allowPain = false;
}

/*
===============
idActor::Event_EnablePain
===============
*/
void idActor::EnablePain( void ) {
	allowPain = true;
}

/*
=====================
idActor::Event_GetPainAnim
=====================
*/
const char *idActor::GetPainAnim( void ) {
	if ( !painAnim.Length() ) {
		return "pain";
	} else {
		return painAnim.c_str();
	}
}

/*
=====================
idActor::Event_SetAnimPrefix
=====================
*/
void idActor::SetAnimPrefix( const char *prefix ) {
	animPrefix = prefix;
}

/*
===============
idActor::Event_StopAnim
===============
*/
void idActor::StopAnim( int channel, int frames ) {
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.StopAnim( frames );
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.StopAnim( frames );
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.StopAnim( frames );
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
		break;
	}
}

/*
===============
idActor::Event_PlayAnim
===============
*/
int idActor::PlayAnim( int channel, const char *animname ) {
	animFlags_t	flags;
	idEntity *headEnt;
	int anim = GetAnim( channel, animname );
	if ( !anim ) {
		if ( ( channel == ANIMCHANNEL_HEAD ) && head.GetEntity() ) {
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), spawnArgs.GetString( "def_head", "" ) );
		} else {
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), GetEntityDefName() );
		}
		return 0;
	}

	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headEnt = head.GetEntity();
		if ( headEnt ) {
			headAnim.idleAnim = false;
			headAnim.PlayAnim( anim );
			flags = headAnim.GetAnimFlags();
			if ( !flags.prevent_idle_override ) {
				if ( torsoAnim.IsIdle() ) {
					torsoAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
					SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
					if ( legsAnim.IsIdle() ) {
						legsAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
						SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
					}
				}
			}
		}
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.idleAnim = false;
		torsoAnim.PlayAnim( anim );
		flags = torsoAnim.GetAnimFlags();
		if ( !flags.prevent_idle_override ) {
			if ( headAnim.IsIdle() ) {
				headAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
			}
			if ( legsAnim.IsIdle() ) {
				legsAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
			}
		}
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.idleAnim = false;
		legsAnim.PlayAnim( anim );
		flags = legsAnim.GetAnimFlags();
		if ( !flags.prevent_idle_override ) {
			if ( torsoAnim.IsIdle() ) {
				torsoAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
				if ( headAnim.IsIdle() ) {
					headAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
					SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
				}
			}
		}
		break;

	default :
		gameLocal.Error( "Unknown anim group" );
		break;
	}
	return 1;
}

/*
===============
idActor::Event_PlayCycle
===============
*/
int idActor::PlayCycle( int channel, const char *animname ) {
	animFlags_t	flags;
	int			anim = GetAnim( channel, animname );
	if ( !anim ) {
		if ( ( channel == ANIMCHANNEL_HEAD ) && head.GetEntity() ) {
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), spawnArgs.GetString( "def_head", "" ) );
		} else {
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), GetEntityDefName() );
		}
		return false;
	}

	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.idleAnim = false;
		headAnim.CycleAnim( anim );
		flags = headAnim.GetAnimFlags();
		if ( !flags.prevent_idle_override ) {
			if ( torsoAnim.IsIdle() && legsAnim.IsIdle() ) {
				torsoAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
				legsAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
			}
		}
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.idleAnim = false;
		torsoAnim.CycleAnim( anim );
		flags = torsoAnim.GetAnimFlags();
		if ( !flags.prevent_idle_override ) {
			if ( headAnim.IsIdle() ) {
				headAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
			}
			if ( legsAnim.IsIdle() ) {
				legsAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
			}
		}
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.idleAnim = false;
		legsAnim.CycleAnim( anim );
		flags = legsAnim.GetAnimFlags();
		if ( !flags.prevent_idle_override ) {
			if ( torsoAnim.IsIdle() ) {
				torsoAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
				if ( headAnim.IsIdle() ) {
					headAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
					SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
				}
			}
		}
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
		break;
	}
	return true;
}

/*
===============
idActor::Event_IdleAnim
===============
*/
int idActor::IdleAnim( int channel, const char* animname ) {
	int anim = GetAnim( channel, animname );
	if ( !anim ) {
		if ( ( channel == ANIMCHANNEL_HEAD ) && head.GetEntity() ) {
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), spawnArgs.GetString( "def_head", "" ) );
		} else {
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), GetEntityDefName() );
		}

		switch( channel ) {
		case ANIMCHANNEL_HEAD :
			headAnim.BecomeIdle();
			break;

		case ANIMCHANNEL_TORSO :
			torsoAnim.BecomeIdle();
			break;

		case ANIMCHANNEL_LEGS :
			legsAnim.BecomeIdle();
			break;

		default:
			gameLocal.Error( "Unknown anim group" );
		}
		return false;
	}

	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.BecomeIdle();
		if ( torsoAnim.GetAnimFlags().prevent_idle_override ) {
			// don't sync to torso body if it doesn't override idle anims
			headAnim.CycleAnim( anim );
		} else if ( torsoAnim.IsIdle() && legsAnim.IsIdle() ) {
			// everything is idle, so play the anim on the head and copy it to the torso and legs
			headAnim.CycleAnim( anim );
			torsoAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
			SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
			legsAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
			SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
		} else if ( torsoAnim.IsIdle() ) {
			// sync the head and torso to the legs
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_LEGS, headAnim.animBlendFrames );
			torsoAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
			SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, torsoAnim.animBlendFrames );
		} else {
			// sync the head to the torso
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, headAnim.animBlendFrames );
		}
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.BecomeIdle();
		if ( legsAnim.GetAnimFlags().prevent_idle_override ) {
			// don't sync to legs if legs anim doesn't override idle anims
			torsoAnim.CycleAnim( anim );
		} else if ( legsAnim.IsIdle() ) {
			// play the anim in both legs and torso
			torsoAnim.CycleAnim( anim );
			legsAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
			SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
		} else {
			// sync the anim to the legs
			SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, torsoAnim.animBlendFrames );
		}

		if ( headAnim.IsIdle() ) {
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
		}
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.BecomeIdle();
		if ( torsoAnim.GetAnimFlags().prevent_idle_override ) {
			// don't sync to torso if torso anim doesn't override idle anims
			legsAnim.CycleAnim( anim );
		} else if ( torsoAnim.IsIdle() ) {
			// play the anim in both legs and torso
			legsAnim.CycleAnim( anim );
			torsoAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
			SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
			if ( headAnim.IsIdle() ) {
				SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
			}
		} else {
			// sync the anim to the torso
			SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, legsAnim.animBlendFrames );
		}
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
		break;
	}
	return true;
}

/*
================
idActor::Event_SetSyncedAnimWeight
================
*/
void idActor::SetSyncedAnimWeight( int channel, int anim, float weight ) {
	idEntity *headEnt = head.GetEntity();
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		if ( headEnt ) {
			animator.CurrentAnim( ANIMCHANNEL_ALL )->SetSyncedAnimWeight( anim, weight );
		} else {
			animator.CurrentAnim( ANIMCHANNEL_HEAD )->SetSyncedAnimWeight( anim, weight );
		}
		if ( torsoAnim.IsIdle() ) {
			animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( anim, weight );
			if ( legsAnim.IsIdle() ) {
				animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( anim, weight );
			}
		}
		break;

	case ANIMCHANNEL_TORSO :
		animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( anim, weight );
		if ( legsAnim.IsIdle() ) {
			animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( anim, weight );
		}
		if ( headEnt && headAnim.IsIdle() ) {
			animator.CurrentAnim( ANIMCHANNEL_ALL )->SetSyncedAnimWeight( anim, weight );
		}
		break;

	case ANIMCHANNEL_LEGS :
		animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( anim, weight );
		if ( torsoAnim.IsIdle() ) {
			animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( anim, weight );
			if ( headEnt && headAnim.IsIdle() ) {
				animator.CurrentAnim( ANIMCHANNEL_ALL )->SetSyncedAnimWeight( anim, weight );
			}
		}
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
		break;
	}
}

/*
===============
idActor::Event_OverrideAnim
===============
*/
void idActor::OverrideAnim( int channel ) {
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.Disable();
		if ( !torsoAnim.IsIdle() ) {
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
		} else {
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
		}
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.Disable();
		SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
		if ( headAnim.IsIdle() ) {
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
		}
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.Disable();
		SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
		break;
	}
}

/*
===============
idActor::Event_EnableAnim
===============
*/
void idActor::EnableAnim( int channel, int blendFrames ) {
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.Enable( blendFrames );
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.Enable( blendFrames );
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.Enable( blendFrames );
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
		break;
	}
}

/*
===============
idActor::Event_SetBlendFrames
===============
*/
void idActor::SetBlendFrames( int channel, int blendFrames ) {
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.animBlendFrames = blendFrames;
		headAnim.lastAnimBlendFrames = blendFrames;
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.animBlendFrames = blendFrames;
		torsoAnim.lastAnimBlendFrames = blendFrames;
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.animBlendFrames = blendFrames;
		legsAnim.lastAnimBlendFrames = blendFrames;
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
		break;
	}
}

/*
===============
idActor::Event_GetBlendFrames
===============
*/
int idActor::GetBlendFrames( int channel ) {
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		return headAnim.animBlendFrames;
	case ANIMCHANNEL_TORSO :
		return torsoAnim.animBlendFrames;
	case ANIMCHANNEL_LEGS :
		return legsAnim.animBlendFrames;
	default:
		gameLocal.Error( "Unknown anim group" );
		return 0;
	}
}

/*
===============
idActor::Event_FinishAction
===============
*/
void idActor::FinishAction( const char *actionname ) {
	if ( waitState == actionname ) {
		SetWaitState( "" );
	}
}

/*
===============
idActor::Event_AnimDone
===============
*/
bool idActor::AnimDone( int channel, int blendFrames ) {
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		return headAnim.AnimDone( blendFrames );
	case ANIMCHANNEL_TORSO :
		return torsoAnim.AnimDone( blendFrames );
	case ANIMCHANNEL_LEGS :
		return legsAnim.AnimDone( blendFrames );
	default:
		gameLocal.Error( "Unknown anim group" );
		return false;
	}
}

/*
================
idActor::Event_HasAnim
================
*/
float idActor::HasAnim( int channel, const char *animname ) {
	return ( GetAnim( channel, animname ) != NULL ) ? 1.0f : 0.0f;
}

/*
================
idActor::Event_CheckAnim
================
*/
void idActor::CheckAnim( int channel, const char *animname ) {
	if ( !GetAnim( channel, animname ) ) {
		if ( animPrefix.Length() ) {
			gameLocal.Error( "Can't find anim '%s_%s' for '%s'", animPrefix.c_str(), animname, name.c_str() );
		} else {
			gameLocal.Error( "Can't find anim '%s' for '%s'", animname, name.c_str() );
		}
	}
}

/*
================
idActor::Event_ChooseAnim
================
*/
const char *idActor::ChooseAnim( int channel, const char *animname ) {
	int anim = GetAnim( channel, animname );
	if ( anim ) {
		if ( channel == ANIMCHANNEL_HEAD ) {
			if ( head.GetEntity() ) {
				return head.GetEntity()->GetAnimator()->AnimFullName( anim );
			}
		} else {
			return animator.AnimFullName( anim );
		}
	}
	return "";
}

/*
================
idActor::Event_AnimLength
================
*/
float idActor::AnimLength( int channel, const char *animname ) {
	int anim = GetAnim( channel, animname );
	if ( anim ) {
		if ( channel == ANIMCHANNEL_HEAD ) {
			if ( head.GetEntity() ) {
				return MS2SEC( head.GetEntity()->GetAnimator()->AnimLength( anim ) );
			}
		} else {
			return MS2SEC( animator.AnimLength( anim ) );
		}
	}
	return 0.0f;
}

/*
================
idActor::Event_AnimDistance
================
*/
float idActor::AnimDistance( int channel, const char *animname ) {
	int anim = GetAnim( channel, animname );
	if ( anim ) {
		if ( channel == ANIMCHANNEL_HEAD ) {
			if ( head.GetEntity() ) {
				return head.GetEntity()->GetAnimator()->TotalMovementDelta( anim ).Length();
			}
		} else {
			return animator.TotalMovementDelta( anim ).Length();
		}
	}
	return 0.0f;
}

/*
================
idActor::Event_NextEnemy
================
*/
idActor *idActor::NextEnemy( idEntity *ent ) {
	idActor *actor;
	if ( !ent || ( ent == this ) ) {
		actor = enemyList.Next();
	} else {
		if ( !ent->IsType( idActor::GetClassType() ) ) {
			gameLocal.Error( "'%s' cannot be an enemy", ent->name.c_str() );
		}

		actor = static_cast<idActor *>( ent );
		if ( actor->enemyNode.ListHead() != &enemyList ) {
			gameLocal.Error( "'%s' is not in '%s' enemy list", actor->name.c_str(), name.c_str() );
		}
	}

	for( ; actor != NULL; actor = actor->enemyNode.Next() ) {
		if ( !actor->fl.hidden ) {
			return actor;
		}
	}
	return NULL;
}

/*
================
idActor::Event_StopSound
================
*/
void idActor::StopSound( int channel, int netSync ) {
	if ( channel == SND_CHANNEL_VOICE ) {
		idEntity *headEnt = head.GetEntity();
		if ( headEnt ) {
			headEnt->StopSound( channel, ( netSync != 0 ) );
		}
	}
	idEntity::StopSound( channel, ( netSync != 0 ) );
}

/*
=====================
idActor::Event_SetNextState
=====================
*/
void idActor::SetNextState( const char *name ) {
	idealState = GetScriptFunction( name );
	if ( idealState == state ) {
		state = NULL;
	}
}

/*
=====================
idActor::Event_SetState
=====================
*/
void idActor::ScriptSetState( const char *name ) {
	idealState = GetScriptFunction( name );
	if ( idealState == state ) {
		state = NULL;
	}
	scriptThread->DoneProcessing();
}

/*
=====================
idActor::Event_GetState
=====================
*/
const char *idActor::GetState( void ) {
	if ( state ) {
		return state->Name();
	} else {
		return "";
	}
}

/*
=====================
idActor::Event_GetHead
=====================
*/
idEntity* idActor::GetHead( void ) {
	return head.GetEntity();
}

#ifdef _D3XP
/*
================
idActor::Event_SetDamageGroupScale
================
*/
void idActor::SetDamageGroupScale( const char *groupName, float scale ) {
#if MD5_ENABLE_GIBS > 0
	for ( int i = 0; i < damageZonesName.Num(); i++ ) {
		if ( damageZonesName[ i ] == groupName ) {
			damageZonesRate[ i ] = scale;
		}
	}
#else
	for ( int i = 0; i < damageScales.Num(); i++ ) {
		if ( damageGroups[ i ] == groupName ) {
			damageScales[ i ] = scale;
		}
	}
#endif
}

/*
================
idActor::Event_SetDamageGroupScaleAll
================
*/
void idActor::SetDamageGroupScaleAll( float scale ) {
#if MD5_ENABLE_GIBS > 0
	for ( int i = 0; i < damageZonesRate.Num(); i++ ) {
		damageZonesRate[ i ] = scale;
	}
#else
	for ( int i = 0; i < damageScales.Num(); i++ ) {
		damageScales[ i ] = scale;
	}
#endif
}

/*
================
idActor::Event_GetDamageGroupScale
================
*/
float idActor::GetDamageGroupScale( const char *groupName ) {
#if MD5_ENABLE_GIBS > 0
	for ( int i = 0; i < damageZonesName.Num(); i++ ) {
		if ( damageZonesName[ i ] == groupName ) {
			return damageZonesRate[ i ];
		}
	}
#else
	for( int i = 0; i < damageScales.Num(); i++ ) {
		if ( damageGroups[ i ] == groupName ) {
			return damageScales[ i ];
		}
	}
#endif
	return 0.0f;
}

/*
================
idActor::Event_SetDamageCap
================
*/
void idActor::SetDamageCap( float _damageCap ) {
	damageCap = _damageCap;
}

/*
================
idActor::Event_GetWaitState
================
*/
const char *idActor::GetWaitState( void ) {
	if( WaitState() ) {
		return WaitState();
	} else {
		return "";
	}
}
#endif