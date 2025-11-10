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

#ifndef __GAME_ACTOR_H__
#define __GAME_ACTOR_H__

/*
===============================================================================

	idActor

===============================================================================
*/

extern const idEventDef AI_EnableEyeFocus;
extern const idEventDef AI_DisableEyeFocus;
extern const idEventDef EV_Footstep;
extern const idEventDef EV_FootstepLeft;
extern const idEventDef EV_FootstepRight;
extern const idEventDef EV_EnableWalkIK;
extern const idEventDef EV_DisableWalkIK;
extern const idEventDef EV_EnableLegIK;
extern const idEventDef EV_DisableLegIK;
extern const idEventDef AI_SetAnimPrefix;
extern const idEventDef AI_PlayAnim;
extern const idEventDef AI_PlayCycle;
extern const idEventDef AI_AnimDone;
extern const idEventDef AI_SetBlendFrames;
extern const idEventDef AI_GetBlendFrames;

#ifdef _D3XP
extern const idEventDef AI_SetState;
#endif

class idDeclParticle;

class idAttachInfo {
public:
	idEntityPtr<idEntity>	ent;
	int						channel;
};

typedef struct {
	jointModTransform_t		mod;
	jointHandle_t			from;
	jointHandle_t			to;
} copyJoints_t;

class idActor : public idAFEntity_Gibbable {
public:
	CLASS_PROTOTYPE( idActor );

	int						team;
	int						rank;				// monsters don't fight back if the attacker's rank is higher
	idMat3					viewAxis;			// view axis of the actor

	idLinkList<idActor>		enemyNode;			// node linked into an entity's enemy list for quick lookups of who is attacking him
	idLinkList<idActor>		enemyList;			// list of characters that have targeted the player as their enemy

public:
							idActor( void );
	virtual					~idActor( void );

	void					Spawn( void );
	virtual void			Restart( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Hide( void );
	virtual void			Show( void );
	virtual int				GetDefaultSurfaceType( void ) const;
	virtual void			ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material );

	virtual bool			LoadAF( void );
	void					SetupBody( void );

	void					CheckBlink( void );

	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
	virtual bool			GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );

							// script state management
	void					ShutdownThreads( void );
	virtual bool			ShouldConstructScriptObjectAtSpawn( void ) const;
	virtual idThread *		ConstructScriptObject( void );
	void					UpdateScript( void );
	const function_t		*GetScriptFunction( const char *funcname );
	void					SetState( const function_t *newState );
	void					SetState( const char *statename );

							// vision testing
	void					SetEyeHeight( float height );
	float					EyeHeight( void ) const;
	idVec3					EyeOffset( void ) const;
	idVec3					GetEyePosition( void ) const;
	virtual void			GetViewPos( idVec3 &origin, idMat3 &axis ) const;
	void					SetFOV( float fov );
	bool					CheckFOV( const idVec3 &pos ) const;
	bool					CanSee( idEntity *ent, bool useFOV ) const;
	bool					PointVisible( const idVec3 &point ) const;
	virtual void			GetAIAimTargets( const idVec3 &lastSightPos, idVec3 &headPos, idVec3 &chestPos );

							// damage
	void					SetupDamageGroups( void );
	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	int						GetDamageForLocation( int damage, int location );
	const char *			GetDamageGroup( int location );
	void					ClearPain( void );
	virtual bool			Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

							// model/combat model/ragdoll
	void					SetCombatModel( void );
	idClipModel *			GetCombatModel( void ) const;
	virtual void			LinkCombat( void );
	virtual void			UnlinkCombat( void );
	bool					StartRagdoll( void );
	void					StopRagdoll( void );
	virtual bool			UpdateAnimationControllers( void );

							// delta view angles to allow movers to rotate the view of the actor
	const idAngles &		GetDeltaViewAngles( void ) const;
	void					SetDeltaViewAngles( const idAngles &delta );

	bool					HasEnemies( void ) const;
	idActor *				ClosestEnemyToPoint( const idVec3 &pos );
	idActor *				EnemyWithMostHealth();

	virtual bool			OnLadder( void ) const;

	virtual void			GetAASLocation( idAAS *aas, idVec3 &pos, int &areaNum ) const;

	void					Attach( idEntity *ent );

	virtual void			Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination );

	virtual	renderView_t *	GetRenderView();

							// animation state control
	int						GetAnim( int channel, const char *name );
	void					UpdateAnimState( void );
	void					SetAnimState( int channel, const char *name, int blendFrames );
	const char *			GetAnimState( int channel ) const;
	bool					InAnimState( int channel, const char *name ) const;
	const char *			WaitState( void ) const;
	void					SetWaitState( const char *_waitstate );
	bool					AnimDone( int channel, int blendFrames ) const;
	virtual void			SpawnGibs( const idVec3 &dir, const char *damageDefName );

#ifdef _D3XP
	idEntity*				GetHeadEntity() { return head.GetEntity(); };
#endif

protected:
	friend class			idAnimState;

	float					fovDot;				// cos( fovDegrees )
	idVec3					eyeOffset;			// offset of eye relative to physics origin
	idVec3					modelOffset;		// offset of visual model relative to the physics origin

	idAngles				deltaViewAngles;	// delta angles relative to view input angles

	int						pain_debounce_time;	// next time the actor can show pain
	int						pain_delay;			// time between playing pain sound
	int						pain_threshold;		// how much damage monster can take at any one time before playing pain animation

	idStrList				damageGroups;		// body damage groups
	idList<float>			damageScale;		// damage scale per damage gruop

	bool						use_combat_bbox;	// whether to use the bounding box for combat collision
	idEntityPtr<idAFAttachment>	head;
	idList<copyJoints_t>		copyJoints;			// copied from the body animation to the head model

	// state variables
	const function_t		*state;
	const function_t		*idealState;

	// joint handles
	jointHandle_t			leftEyeJoint;
	jointHandle_t			rightEyeJoint;
	jointHandle_t			soundJoint;

	idIK_Walk				walkIK;

	idStr					animPrefix;
	idStr					painAnim;

	// blinking
	int						blink_anim;
	int						blink_time;
	int						blink_min;
	int						blink_max;

	// script variables
	idThread *				scriptThread;
	idStr					waitState;
	idAnimState				headAnim;
	idAnimState				torsoAnim;
	idAnimState				legsAnim;

	bool					allowPain;
	bool					allowEyeFocus;
	bool					finalBoss;

	int						painTime;

	idList<idAttachInfo>	attachments;

#ifdef _D3XP
	int						damageCap;
#endif

	virtual void			Gib( const idVec3 &dir, const char *damageDefName );

							// removes attachments with "remove" set for when character dies
	void					RemoveAttachments( void );

							// copies animation from body to head joints
	void					CopyJointsFromBodyToHead( void );

public:
	void					EnableEyeFocus( void );
	void					DisableEyeFocus( void );
	void					Footstep( void );
	void					EnableWalkIK( void );
	void					DisableWalkIK( void );
	void					EnableLegIK( int num );
	void					DisableLegIK( int num );
	void					PreventPain( float duration );
	void					DisablePain( void );
	void					EnablePain( void );
	const char *			GetPainAnim( void );
	void					SetAnimPrefix( const char *prefix );
	void					StopAnim( int channel, int frames );
	int						PlayAnim( int channel, const char *animname );
	int						PlayCycle( int channel, const char *animname );
	int						IdleAnim( int channel, const char *animname );
	void					SetSyncedAnimWeight( int channel, int anim, float weight );
	void					OverrideAnim( int channel );
	void					EnableAnim( int channel, int blendFrames );
	void					SetBlendFrames( int channel, int blendFrames );
	int						GetBlendFrames( int channel );
	void					FinishAction( const char *actionname );
	bool					AnimDone( int channel, int blendFrames );
	float					HasAnim( int channel, const char *animname );
	void					CheckAnim( int channel, const char *animname );
	const char *			ChooseAnim( int channel, const char *animname );
	float					AnimLength( int channel, const char *animname );
	float					AnimDistance( int channel, const char *animname );
	idActor *				NextEnemy( idEntity *ent );
	void					StopSound( int channel, int netSync );
	void					SetNextState( const char *name );
	void					ScriptSetState( const char *name );
	const char *			GetState( void );
	idEntity *				GetHead( void );
#ifdef _D3XP
	void					SetDamageGroupScale( const char *groupName, float scale );
	void					SetDamageGroupScaleAll( float scale );
	float					GetDamageGroupScale( const char *groupName );
	void					SetDamageCap( float _damageCap );
	const char *			GetWaitState( void );
#endif

	void					SyncAnimChannels( int channel, int syncToChannel, int blendFrames );
	void					FinishSetup( void );
	void					SetupHead( void );
	void					PlayFootStepSound( void );

private:
	void					Script_EnableEyeFocus( void );
	void					Script_DisableEyeFocus( void );
	void					Script_Footstep( void );
	void					Script_EnableWalkIK( void );
	void					Script_DisableWalkIK( void );
	void					Script_EnableLegIK( int num );
	void					Script_DisableLegIK( int num );
	void					Script_SetAnimPrefix( const char *name );
	void					Script_LookAtEntity( idEntity *ent, float duration );
	void					Script_PreventPain( float duration );
	void					Script_DisablePain( void );
	void					Script_EnablePain( void );
	void					Script_GetPainAnim( void );
	void					Script_StopAnim( int channel, int frames );
	void					Script_PlayAnim( int channel, const char *name );
	void					Script_PlayCycle( int channel, const char *name );
	void					Script_IdleAnim( int channel, const char *name );
	void					Script_SetSyncedAnimWeight( int channel, int anim, float weight );
	void					Script_OverrideAnim( int channel );
	void					Script_EnableAnim( int channel, int blendFrames );
	void					Script_SetBlendFrames( int channel, int blendFrames );
	void					Script_GetBlendFrames( int channel );
	void					Script_AnimState( int channel, const char *name, int blendFrames );
	void					Script_GetAnimState( int channel );
	void					Script_InAnimState( int channel, const char *name );
	void					Script_FinishAction( const char *name );
	void					Script_AnimDone( int channel, int blendFrames );
	void					Script_HasAnim( int channel, const char *name );
	void					Script_CheckAnim( int channel, const char *animname );
	void					Script_ChooseAnim( int channel, const char *animname );
	void					Script_AnimLength( int channel, const char *animname );
	void					Script_AnimDistance( int channel, const char *animname );
	void					Script_HasEnemies( void );
	void					Script_NextEnemy( idEntity *ent );
	void					Script_ClosestEnemyToPoint( const idVec3 &pos );
	void					Script_StopSound( int channel, int netsync );
	void					Script_SetNextState( const char *name );
	void					Script_SetState( const char *name );
	void					Script_GetState( void );
	void					Script_GetHead( void );
#ifdef _D3XP
	void					Script_SetDamageGroupScale( const char* groupName, float scale);
	void					Script_SetDamageGroupScaleAll( float scale );
	void					Script_GetDamageGroupScale( const char* groupName );
	void					Script_SetDamageCap( float _damageCap );
	void					Script_SetWaitState( const char* waitState);
	void					Script_GetWaitState();
#endif
};

#endif /* !__GAME_ACTOR_H__ */
