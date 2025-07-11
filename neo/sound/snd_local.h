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

#ifndef __SND_LOCAL_H__
#define __SND_LOCAL_H__

#ifdef ID_DEDICATED
// stub-only mode: AL_API and ALC_API shouldn't refer to any dll-stuff
// because the implemenations are in openal_stub.cpp
// this is ensured by defining AL_LIBTYPE_STATIC before including the AL headers
#define AL_LIBTYPE_STATIC
// newer versions of openal-soft set the noexcept attribute to functions, older ones didn't
// just disable that so the stub functions continue to match the prototypes in the header
#define AL_DISABLE_NOEXCEPT
#endif

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

// DG: make this code build with older OpenAL headers that don't know about ALC_SOFT_HRTF
//     which provides LPALCRESETDEVICESOFT for idSoundSystemLocal::alcResetDeviceSOFT()
#ifndef ALC_SOFT_HRTF
  typedef ALCboolean (ALC_APIENTRY*LPALCRESETDEVICESOFT)(ALCdevice *device, const ALCint *attribs);
#endif

// DG: ALC_SOFT_output_mode is pretty new, provide compatibility..
#ifndef ALC_SOFT_output_mode
  #define ALC_SOFT_output_mode
  #define ALC_OUTPUT_MODE_SOFT                     0x19AC
  #define ALC_ANY_SOFT                             0x19AD

  #define ALC_STEREO_BASIC_SOFT                    0x19AE
  #define ALC_STEREO_UHJ_SOFT                      0x19AF
  #define ALC_STEREO_HRTF_SOFT                     0x19B2

  #define ALC_SURROUND_5_1_SOFT                    0x1504
  #define ALC_SURROUND_6_1_SOFT                    0x1505
  #define ALC_SURROUND_7_1_SOFT                    0x1506
#endif
// the following formats are defined in https://openal-soft.org/openal-extensions/SOFT_output_mode.txt
// but commented out in OpenAL Softs current AL/alext.h
#ifndef ALC_MONO_SOFT
  #define ALC_MONO_SOFT                            0x1500
#endif
#ifndef ALC_STEREO_SOFT
  #define ALC_STEREO_SOFT                          0x1501
#endif
#ifndef ALC_QUAD_SOFT
  #define ALC_QUAD_SOFT                            0x1503
#endif

// DG: in case ALC_SOFT_output_limiter is not available in some headers..
#ifndef ALC_SOFT_output_limiter
  #define ALC_SOFT_output_limiter
  #define ALC_OUTPUT_LIMITER_SOFT                  0x199A
#endif

#include "sound/efxlib.h"
#include "sound/snd_reverb.h"

// demo sound commands
typedef enum {
	SCMD_STATE,				// followed by a load game state
	SCMD_PLACE_LISTENER,
	SCMD_ALLOC_EMITTER,

	SCMD_FREE,
	SCMD_UPDATE,
	SCMD_START,
	SCMD_MODIFY,
	SCMD_STOP,
	SCMD_FADE
} soundDemoCommand_t;

const int SOUND_MAX_CHANNELS		= 8;
const int SOUND_DECODER_FREE_DELAY	= 1000 * MIXBUFFER_SAMPLES / USERCMD_MSEC;		// four seconds

const int PRIMARYFREQ				= 44100;			// samples per second
const float SND_EPSILON				= 1.0f / 32768.0f;	// if volume is below this, it will always multiply to zero

const int ROOM_SLICES_IN_BUFFER		= 10;

class idAudioBuffer;
class idWaveFile;
class idSoundCache;
class idSoundSample;
class idSampleDecoder;
class idSoundWorldLocal;

/*
===================================================================================

	General extended waveform format structure.
	Use this for all NON PCM formats.

===================================================================================
*/

#ifdef WIN32
#pragma pack(1)
#endif
struct waveformatex_s {
	word    wFormatTag;        /* format type */
	word    nChannels;         /* number of channels (i.e. mono, stereo...) */
	dword   nSamplesPerSec;    /* sample rate */
	dword   nAvgBytesPerSec;   /* for buffer estimation */
	word    nBlockAlign;       /* block size of data */
	word    wBitsPerSample;    /* Number of bits per sample of mono data */
	word    cbSize;            /* The count in bytes of the size of
									extra information (after cbSize) */
} PACKED;

typedef waveformatex_s waveformatex_t;

/* OLD general waveform format structure (information common to all formats) */
struct waveformat_s {
	word    wFormatTag;        /* format type */
	word    nChannels;         /* number of channels (i.e. mono, stereo, etc.) */
	dword   nSamplesPerSec;    /* sample rate */
	dword   nAvgBytesPerSec;   /* for buffer estimation */
	word    nBlockAlign;       /* block size of data */
} PACKED;

typedef waveformat_s waveformat_t;

/* flags for wFormatTag field of WAVEFORMAT */
enum {
	WAVE_FORMAT_TAG_PCM		= 1,
	WAVE_FORMAT_TAG_OGG		= 2
};

/* specific waveform format structure for PCM data */
struct pcmwaveformat_s {
	waveformat_t	wf;
	word			wBitsPerSample;
} PACKED;

typedef pcmwaveformat_s pcmwaveformat_t;

#ifndef mmioFOURCC
#define mmioFOURCC( ch0, ch1, ch2, ch3 )				\
		( (dword)(byte)(ch0) | ( (dword)(byte)(ch1) << 8 ) |	\
		( (dword)(byte)(ch2) << 16 ) | ( (dword)(byte)(ch3) << 24 ) )
#endif

#define fourcc_riff     mmioFOURCC('R', 'I', 'F', 'F')

struct waveformatextensible_s {
	waveformatex_t    Format;
	union {
		word wValidBitsPerSample;       /* bits of precision  */
		word wSamplesPerBlock;          /* valid if wBitsPerSample==0*/
		word wReserved;                 /* If neither applies, set to zero*/
	} Samples;
	dword           dwChannelMask;      /* which channels are */
										/* present in stream  */
	int            SubFormat;
} PACKED;

typedef waveformatextensible_s waveformatextensible_t;

typedef dword fourcc;

/* RIFF chunk information data structure */
struct mminfo_s {
	fourcc			ckid;           /* chunk ID */
	dword			cksize;         /* chunk size */
	fourcc			fccType;        /* form type or list type */
	dword			dwDataOffset;   /* offset of data portion of chunk */
} PACKED;

typedef mminfo_s mminfo_t;

#ifdef WIN32
#pragma pack()
#endif

/*
===================================================================================

idWaveFile

===================================================================================
*/

class idWaveFile {
public:
					idWaveFile( void );
					~idWaveFile( void );

	int				Open( const char* strFileName, waveformatex_t* pwfx = NULL );
	int				OpenFromMemory( short* pbData, int ulDataSize, waveformatextensible_t* pwfx );
	int				Read( byte* pBuffer, int dwSizeToRead, int *pdwSizeRead );
	int				Seek( int offset );
	int				Close( void );
	int				ResetFile( void );

	int				GetOutputSize( void ) { return mdwSize; }
	int				GetMemorySize( void ) { return mMemSize; }

	waveformatextensible_t	mpwfx;        // Pointer to waveformatex structure

private:
	idFile *		mhmmio;			// I/O handle for the WAVE
	mminfo_t		mck;			// Multimedia RIFF chunk
	mminfo_t		mckRiff;		// used when opening a WAVE file
	dword			mdwSize;		// size in samples
	dword			mMemSize;		// size of the wave data in memory
	dword			mseekBase;
	ID_TIME_T			mfileTime;

	bool			mbIsReadingFromMemory;
	short *			mpbData;
	short *			mpbDataCur;
	dword			mulDataSize;

	void *			ogg;			// only !NULL when !s_realTimeDecoding
	byte*			oggData; // the contents of the .ogg for stbi_vorbis (it doesn't support custom reading callbacks)
	bool			isOgg;

private:
	int				ReadMMIO( void );

	int				OpenOGG( const char* strFileName, waveformatex_t* pwfx = NULL );
	int				ReadOGG( byte* pBuffer, int dwSizeToRead, int *pdwSizeRead );
	int				CloseOGG( void );
};


/*
===================================================================================

Encapsulates functionality of a DirectSound buffer.

===================================================================================
*/

class idAudioBuffer {
public:
	virtual int		Play( dword dwPriority=0, dword dwFlags=0 ) = 0;
	virtual int			Stop( void ) = 0;
	virtual int			Reset( void ) = 0;
	virtual bool		IsSoundPlaying( void ) = 0;
	virtual void		SetVolume( float x ) = 0;
};


/*
===================================================================================

idSoundEmitterLocal

===================================================================================
*/

typedef enum {
	REMOVE_STATUS_INVALID				= -1,
	REMOVE_STATUS_ALIVE					=  0,
	REMOVE_STATUS_WAITSAMPLEFINISHED	=  1,
	REMOVE_STATUS_SAMPLEFINISHED		=  2
} removeStatus_t;

class idSoundFade {
public:
	int					fadeStart44kHz;
	int					fadeEnd44kHz;
	float				fadeStartVolume;		// in dB
	float				fadeEndVolume;			// in dB

	void				Clear();
	float				FadeDbAt44kHz( int current44kHz );
};

class SoundFX {
protected:
	bool				initialized;

	int					channel;
	int					maxlen;

	float*				buffer;
	float				continuitySamples[4];

	float				param;

public:
						SoundFX()										{ channel = 0; buffer = NULL; initialized = false; maxlen = 0; memset( continuitySamples, 0, sizeof( float ) * 4 ); };
	virtual				~SoundFX()										{ if ( buffer ) delete buffer; };

	virtual void		Initialize()									{ };
	virtual void		ProcessSample( float* in, float* out ) = 0;

	void				SetChannel( int chan )							{ channel = chan; };
	int					GetChannel()									{ return channel; };

	void				SetContinuitySamples( float in1, float in2, float out1, float out2 )		{ continuitySamples[0] = in1; continuitySamples[1] = in2; continuitySamples[2] = out1; continuitySamples[3] = out2; };		// FIXME?
	void				GetContinuitySamples( float& in1, float& in2, float& out1, float& out2 )	{ in1 = continuitySamples[0]; in2 = continuitySamples[1]; out1 = continuitySamples[2]; out2 = continuitySamples[3]; };

	void				SetParameter( float val )						{ param = val; };
};

class SoundFX_Lowpass : public SoundFX {
public:
	virtual void		ProcessSample( float* in, float* out );
};

class SoundFX_LowpassFast : public SoundFX {
	float				freq;
	float				res;
	float				a1, a2, a3;
	float				b1, b2;

public:
	virtual void		ProcessSample( float* in, float* out );
	void				SetParms( float p1 = 0, float p2 = 0, float p3 = 0 );

	void				Clear() {
		freq = res = 0.0f;
		a1 = a2 = a3 = 0.0f;
		b1 = b2 = 0.0f;
	}
};

class SoundFX_Comb : public SoundFX {
	int					currentTime;

public:
	virtual void		Initialize();
	virtual void		ProcessSample( float* in, float* out );
};

class FracTime {
public:
	int			time;
	float		frac;

	void		Set( int val )					{ time = val; frac = 0; };
	void		Increment( float val )			{ frac += val; while ( frac >= 1.f ) { time++; frac--; } };
};

enum {
	PLAYBACK_RESET,
	PLAYBACK_ADVANCING
};

class idSoundChannel;

class idSlowChannel {
	bool					active;
	const idSoundChannel*	chan;

	int						playbackState;
	int						triggerOffset;

	FracTime				newPosition;
	int						newSampleOffset;

	FracTime				curPosition;
	int						curSampleOffset;

	SoundFX_LowpassFast		lowpass;

	// functions
	void					GenerateSlowChannel( FracTime& playPos, int sampleCount44k, float* finalBuffer );

	float					GetSlowmoSpeed();

public:

	void					AttachSoundChannel( const idSoundChannel *chan );
	void					Reset();

	void					GatherChannelSamples( int sampleOffset44k, int sampleCount44k, float *dest );

	bool					IsActive()				{ return active; };
	FracTime				GetCurrentPosition()	{ return curPosition; };
};

class idSoundChannel {
public:
						idSoundChannel( void );
						~idSoundChannel( void );

	void				Clear( void );
	void				Start( void );
	void				Stop( void );
	void				GatherChannelSamples( int sampleOffset44k, int sampleCount44k, float *dest ) const;
	void				ALStop( void );			// free OpenAL resources if any

	bool				triggerState;
	int					trigger44kHzTime;		// hardware time sample the channel started
	int					triggerGame44kHzTime;	// game time sample time the channel started
	soundShaderParms_t	parms;					// combines the shader parms and the per-channel overrides
	idSoundSample *		leadinSample;			// if not looped, this is the only sample
	s_channelType		triggerChannel;
	const idSoundShader *soundShader;
	idSampleDecoder *	decoder;
	float				diversity;
	float				lastVolume;				// last calculated volume based on distance
	float				lastV[6];				// last calculated volume for each speaker, so we can smoothly fade
	idSoundFade			channelFade;
	bool				triggered;
	ALuint				openalSource;
	ALuint				openalStreamingOffset;
	ALuint				openalStreamingBuffer[3];
	ALuint				lastopenalStreamingBuffer[3];
	bool				stopped;

	bool				paused;					// DG: currently paused, but generally still playing - for when menu is open etc

	bool				disallowSlow;

};

class idSoundEmitterLocal : public idSoundEmitter {
public:

						idSoundEmitterLocal( void );
	virtual				~idSoundEmitterLocal( void );

	//----------------------------------------------

	// the "time" parameters should be game time in msec, which is used to make queries
	// return deterministic values regardless of async buffer scheduling

	// a non-immediate free will let all currently playing sounds complete
	virtual void		Free( bool immediate );

	// the parms specified will be the default overrides for all sounds started on this emitter.
	// NULL is acceptable for parms
	virtual void		UpdateEmitter( const idVec3 &origin, int listenerId, const soundShaderParms_t *parms );

	// returns the length of the started sound in msec
	virtual int			StartSound( const idSoundShader *shader, const s_channelType channel, float diversity = 0, int shaderFlags = 0, bool allowSlow = true /* D3XP */ );

	// can pass SCHANNEL_ANY
	virtual void		ModifySound( const s_channelType channel, const soundShaderParms_t *parms );
	virtual void		StopSound( const s_channelType channel );
	virtual void		FadeSound( const s_channelType channel, float to, float over );

	virtual bool		CurrentlyPlaying( void ) const;

	// can pass SCHANNEL_ANY
	virtual	float		CurrentAmplitude( void );

	// used for save games
	virtual	int			Index( void ) const;

	//----------------------------------------------

	void				Clear( void );

	void				PauseAll( void );   // DG: to pause active OpenAL sources when entering menu etc
	void				UnPauseAll( void ); // DG: to resume active OpenAL sources when leaving menu etc

	void				OverrideParms( const soundShaderParms_t *base, const soundShaderParms_t *over, soundShaderParms_t *out );
	void				CheckForCompletion( int current44kHzTime );
	void				Spatialize( idVec3 listenerPos, int listenerArea, idRenderWorld *rw );

	idSoundWorldLocal *	soundWorld;				// the world that holds this emitter

	int					index;						// in world emitter list
	removeStatus_t		removeStatus;

	idVec3				origin;
	int					listenerId;
	soundShaderParms_t	parms;						// default overrides for all channels


	// the following are calculated in UpdateEmitter, and don't need to be archived
	float				maxDistance;				// greatest of all playing channel distances
	int					lastValidPortalArea;		// so an emitter that slides out of the world continues playing
	bool				playing;					// if false, no channel is active
	bool				hasShakes;
	idVec3				spatializedOrigin;			// the virtual sound origin, either the real sound origin,
													// or a point through a portal chain
	float				realDistance;				// in meters
	float				distance;					// in meters, this may be the straight-line distance, or
													// it may go through a chain of portals.  If there
													// is not an open-portal path, distance will be > maxDistance

	// a single soundEmitter can have many channels playing from the same point
	idSoundChannel		channels[SOUND_MAX_CHANNELS];

	idSlowChannel		slowChannels[SOUND_MAX_CHANNELS];

	idSlowChannel		GetSlowChannel( const idSoundChannel *chan );
	void				SetSlowChannel( const idSoundChannel *chan, idSlowChannel slow );
	void				ResetSlowChannel( const idSoundChannel *chan );

	// this is just used for feedback to the game or rendering system:
	// flashing lights and screen shakes.  Because the material expression
	// evaluation doesn't do common subexpression removal, we cache the
	// last generated value
	int					ampTime;
	float				amplitude;
};


/*
===================================================================================

idSoundWorldLocal

===================================================================================
*/

class s_stats {
public:
	s_stats( void ) {
		rinuse = 0;
		runs = 1;
		timeinprocess = 0;
		missedWindow = 0;
		missedUpdateWindow = 0;
		activeSounds = 0;
	}
	int		rinuse;
	int		runs;
	int		timeinprocess;
	int		missedWindow;
	int		missedUpdateWindow;
	int		activeSounds;
};

typedef struct soundPortalTrace_s {
	int		portalArea;
	const struct soundPortalTrace_s	*prevStack;
} soundPortalTrace_t;

class idSoundWorldLocal : public idSoundWorld {
public:
	virtual					~idSoundWorldLocal( void );

	// call at each map start
	virtual void			ClearAllSoundEmitters( void );
	virtual void			StopAllSounds( void );

	// get a new emitter that can play sounds in this world
	virtual idSoundEmitter *AllocSoundEmitter( void );

	// for load games
	virtual idSoundEmitter *EmitterForIndex( int index );

	// query data from all emitters in the world
	virtual float			CurrentShakeAmplitudeForPosition( const int time, const idVec3 &listererPosition );

	// where is the camera/microphone
	// listenerId allows listener-private sounds to be added
	virtual void			PlaceListener( const idVec3 &origin, const idMat3 &axis, const int listenerId, const int gameTime, const idStr& areaName );

	// fade all sounds in the world with a given shader soundClass
	// to is in Db (sigh), over is in seconds
	virtual void			FadeSoundClasses( const int soundClass, const float to, const float over );

	// dumps the current state and begins archiving commands
	virtual void			StartWritingDemo( idDemoFile *demo );
	virtual void			StopWritingDemo( void );

	// read a sound command from a demo file
	virtual void			ProcessDemoCommand( idDemoFile *readDemo );

	// background music
	virtual void			PlayShaderDirectly( const char *name, int channel = -1 );

	// pause and unpause the sound world
	virtual void			Pause( void );
	virtual void			UnPause( void );
	virtual bool			IsPaused( void );

	// avidump
	virtual void			AVIOpen( const char *path, const char *name );
	virtual void			AVIClose( void );

	// SaveGame Support
	virtual void			WriteToSaveGame( idFile *savefile );
	virtual void			ReadFromSaveGame( idFile *savefile );

	virtual void			ReadFromSaveGameSoundChannel( idFile *saveGame, idSoundChannel *ch );
	virtual void			ReadFromSaveGameSoundShaderParams( idFile *saveGame, soundShaderParms_t *params );
	virtual void			WriteToSaveGameSoundChannel( idFile *saveGame, idSoundChannel *ch );
	virtual void			WriteToSaveGameSoundShaderParams( idFile *saveGame, soundShaderParms_t *params );

	virtual void			SetSlowmo( bool active );
	virtual void			SetSlowmoSpeed( float speed );
	virtual void			SetEnviroSuit( bool active );

	//=======================================

							idSoundWorldLocal( void );

	void					Shutdown( void );
	void					Init( idRenderWorld *rw );

	// update
	void					ForegroundUpdate( int currentTime );
	void					OffsetSoundTime( int offset44kHz );

	idSoundEmitterLocal *	AllocLocalSoundEmitter();
	void					CalcEars( int numSpeakers, idVec3 realOrigin, idVec3 listenerPos, idMat3 listenerAxis, float ears[6], float spatialize );
	void					AddChannelContribution( idSoundEmitterLocal *sound, idSoundChannel *chan,
												int current44kHz, int numSpeakers, float *finalMixBuffer );
	void					MixLoop( int current44kHz, int numSpeakers, float *finalMixBuffer );
	void					AVIUpdate( void );
	void					ResolveOrigin( const int stackDepth, const soundPortalTrace_t *prevStack, const int soundArea, const float dist, const idVec3& soundOrigin, idSoundEmitterLocal *def );
	float					FindAmplitude( idSoundEmitterLocal *sound, const int localTime, const idVec3 *listenerPosition, const s_channelType channel, bool shakesOnly );

	//============================================

	idRenderWorld *			rw;				// for portals and debug drawing
	idDemoFile *			writeDemo;			// if not NULL, archive commands here

	idMat3					listenerAxis;
	idVec3					listenerPos;		// position in meters
	int						listenerPrivateId;
	idVec3					listenerQU;			// position in "quake units"
	int						listenerArea;
	idStr					listenerAreaName;
	ALuint					listenerEffect;
	ALuint					listenerSlot;
	bool					listenerAreFiltersInitialized;
	ALuint					listenerFilters[2]; // 0 - direct; 1 - send.
	float					listenerSlotReverbGain;

	int						gameMsec;
	int						game44kHz;
	int						pause44kHz;
	int						lastAVI44kHz;		// determine when we need to mix and write another block

	idList<idSoundEmitterLocal *>emitters;

	idSoundFade				soundClassFade[SOUND_MAX_CLASSES];	// for global sound fading

	// avi stuff
	idFile *				fpa[6];
	idStr					aviDemoPath;
	idStr					aviDemoName;

	idSoundEmitterLocal *	localSound;		// just for playShaderDirectly()

	bool					slowmoActive;
	float					slowmoSpeed;
	bool					enviroSuitActive;
};

/*
===================================================================================

idSoundSystemLocal

===================================================================================
*/

typedef struct {
	ALuint			handle;
	int				startTime;
	idSoundChannel	*chan;
	bool			inUse;
	bool			looping;
	bool			stereo;
} openalSource_t;

class idSoundSystemLocal : public idSoundSystem {
public:
	idSoundSystemLocal( ) {
		isInitialized = false;
	}

	// all non-hardware initialization
	virtual void			Init( void );

	// shutdown routine
	virtual	void			Shutdown( void );

	// sound is attached to the window, and must be recreated when the window is changed
	virtual bool			ShutdownHW( void );
	virtual bool			InitHW( void );

	// async loop, called at 60Hz
	virtual int				AsyncUpdate( int time );
	// async loop, when the sound driver uses a write strategy
	virtual int				AsyncUpdateWrite( int time );
	// direct mixing called from the sound driver thread for OSes that support it
	virtual int				AsyncMix( int soundTime, float *mixBuffer );

	virtual void			SetMute( bool mute );

	virtual cinData_t		ImageForTime( const int milliseconds, const bool waveform );

	int						GetSoundDecoderInfo( int index, soundDecoderInfo_t &decoderInfo );

	// if rw == NULL, no portal occlusion or rendered debugging is available
	virtual idSoundWorld	*AllocSoundWorld( idRenderWorld *rw );

	// specifying NULL will cause silence to be played
	virtual void			SetPlayingSoundWorld( idSoundWorld *soundWorld );

	// some tools, like the sound dialog, may be used in both the game and the editor
	// This can return NULL, so check!
	virtual idSoundWorld	*GetPlayingSoundWorld( void );

	virtual	void			BeginLevelLoad( void );
	virtual	void			EndLevelLoad( const char *mapString );

	virtual void			PrintMemInfo( MemInfo_t *mi );

	virtual int				IsEFXAvailable( void );

	virtual	const char		*GetReverbName( int reverb );
	virtual	int				GetNumAreas( void );
	virtual	int				GetReverb( int area );
	virtual	bool			SetReverb( int area, const char *reverbName, const char *fileName );

private:
	idMapReverb reverb;

public:

	//-------------------------

	int						GetCurrent44kHzTime( void ) const;
	float					dB2Scale( const float val ) const;
	int						SamplesToMilliseconds( int samples ) const;
	int						MillisecondsToSamples( int ms ) const;

	void					DoEnviroSuit( float* samples, int numSamples, int numSpeakers );

	ALuint					AllocOpenALSource( idSoundChannel *chan, bool looping, bool stereo );
	void					FreeOpenALSource( ALuint handle );

	// returns true if openalDevice is still available,
	// otherwise it will try to recover the device and return false while it's gone
	// (display audio sound devices sometimes disappear for a few seconds when switching resolution)
	bool					CheckDeviceAndRecoverIfNeeded();
	// resets the OpenAL device, applying the settings of s_alHRTF and s_alOutputLimiter
	// returns false if that failed, or the necessary OpenAL extension isn't available
	bool					ResetALDevice();

	idSoundCache *			soundCache;

	idSoundWorldLocal *		currentSoundWorld;	// the one to mix each async tic

	int						olddwCurrentWritePos;	// statistics
	int						buffers;				// statistics
	int						CurrentSoundTime;		// set by the async thread and only used by the main thread

	unsigned int			nextWriteBlock;

	float					realAccum[6*MIXBUFFER_SAMPLES+16];
	float *					finalMixBuffer;			// points inside realAccum at a 16 byte aligned boundary

	bool					isInitialized;
	bool					muted;
	bool					shutdown;

	s_stats					soundStats;				// NOTE: updated throughout the code, not displayed anywhere

	int						meterTops[256];
	int						meterTopsTime[256];

	dword *					graph;

	float					volumesDB[1200];		// dB to float volume conversion

	idList<SoundFX*>		fxList;

	ALCdevice				*openalDevice;
	ALCcontext				*openalContext;
	ALsizei					openalSourceCount;
	openalSource_t			openalSources[256];

	LPALGENEFFECTS			alGenEffects;
	LPALDELETEEFFECTS		alDeleteEffects;
	LPALISEFFECT			alIsEffect;
	LPALEFFECTI				alEffecti;
	LPALEFFECTF				alEffectf;
	LPALEFFECTFV			alEffectfv;
	LPALGENFILTERS			alGenFilters;
	LPALDELETEFILTERS		alDeleteFilters;
	LPALISFILTER			alIsFilter;
	LPALFILTERI				alFilteri;
	LPALFILTERF				alFilterf;
	LPALGENAUXILIARYEFFECTSLOTS		alGenAuxiliaryEffectSlots;
	LPALDELETEAUXILIARYEFFECTSLOTS	alDeleteAuxiliaryEffectSlots;
	LPALISAUXILIARYEFFECTSLOT		alIsAuxiliaryEffectSlot;
	LPALAUXILIARYEFFECTSLOTI		alAuxiliaryEffectSloti;
	LPALAUXILIARYEFFECTSLOTF		alAuxiliaryEffectSlotf;

	idEFXFile				EFXDatabase;
	bool					efxloaded;
							// latches
	static bool				useEFXReverb;
							// mark available during initialization, or through an explicit test
	static int				EFXAvailable;

	static bool				alHRTFavailable; // needs ALC_SOFT_HRTF extension
	static bool				alOutputLimiterAvailable; // needs ALC_SOFT_output_limiter extension (+ HRTF extension)
	static bool				alEnumerateAllAvailable;  // needs ALC_ENUMERATE_ALL_EXT
	static bool				alIsDisconnectAvailable;  // needs ALC_EXT_disconnect
	static bool				alOutputModeAvailable;    // needs ALC_SOFT_output_mode

	// DG: for CheckDeviceAndRecoverIfNeeded()
	LPALCRESETDEVICESOFT	alcResetDeviceSOFT; // needs ALC_SOFT_HRTF extension
	int						resetRetryCount;
	unsigned int			lastCheckTime;

	static idCVar			s_noSound;
	static idCVar			s_device;
	static idCVar			s_quadraticFalloff;
	static idCVar			s_drawSounds;
	static idCVar			s_minVolume6;
	static idCVar			s_dotbias6;
	static idCVar			s_minVolume2;
	static idCVar			s_dotbias2;
	static idCVar			s_spatializationDecay;
	static idCVar			s_showStartSound;
	static idCVar			s_maxSoundsPerShader;
	static idCVar			s_reverse;
	static idCVar			s_showLevelMeter;
	static idCVar			s_meterTopTime;
	static idCVar			s_volume;
	static idCVar			s_constantAmplitude;
	static idCVar			s_playDefaultSound;
	static idCVar			s_useOcclusion;
	static idCVar			s_subFraction;
	static idCVar			s_globalFraction;
	static idCVar			s_doorDistanceAdd;
	static idCVar			s_singleEmitter;
	static idCVar			s_numberOfSpeakers;
	static idCVar			s_force22kHz;
	static idCVar			s_clipVolumes;
	static idCVar			s_realTimeDecoding;
	static idCVar			s_useEAXReverb;
	static idCVar			s_decompressionLimit;

	static idCVar			s_alReverbGain;

	static idCVar			s_scaleDownAndClamp;
	static idCVar			s_alOutputLimiter;
	static idCVar			s_alHRTF;

	static idCVar			s_slowAttenuate;

	static idCVar			s_enviroSuitCutoffFreq;
	static idCVar			s_enviroSuitCutoffQ;
	static idCVar			s_enviroSuitSkipLowpass;
	static idCVar			s_enviroSuitSkipReverb;

	static idCVar			s_reverbTime;
	static idCVar			s_reverbFeedback;
	static idCVar			s_enviroSuitVolumeScale;
	static idCVar			s_skipHelltimeFX;
};

extern	idSoundSystemLocal	soundSystemLocal;


/*
===================================================================================

  This class holds the actual wavefile bitmap, size, and info.

===================================================================================
*/

const int SCACHE_SIZE = MIXBUFFER_SAMPLES*20;	// 1/2 of a second (aroundabout)

class idSoundSample {
public:
							idSoundSample();
							~idSoundSample();

	idStr					name;						// name of the sample file
	ID_TIME_T					timestamp;					// the most recent of all images used in creation, for reloadImages command

	waveformatex_t			objectInfo;					// what are we caching
	int						objectSize;					// size of waveform in samples, excludes the header
	int						objectMemSize;				// object size in memory
	byte *					nonCacheData;				// if it's not cached
	byte *					amplitudeData;				// precomputed min,max amplitude pairs
	ALuint					openalBuffer;				// openal buffer
	bool					hardwareBuffer;
	bool					defaultSound;
	bool					onDemand;
	bool					purged;
	bool					levelLoadReferenced;		// so we can tell which samples aren't needed any more

	int						LengthIn44kHzSamples() const;
	ID_TIME_T					GetNewTimeStamp( void ) const;
	void					MakeDefault();				// turns it into a beep
	void					Load();						// loads the current sound based on name
	void					Reload( bool force );		// reloads if timestamp has changed, or always if force
	void					PurgeSoundSample();			// frees all data
	void					CheckForDownSample();		// down sample if required
	bool					FetchFromCache( int offset, const byte **output, int *position, int *size, const bool allowIO );
};


/*
===================================================================================

  Sound sample decoder.

===================================================================================
*/

class idSampleDecoder {
public:
	static void				Init( void );
	static void				Shutdown( void );
	static idSampleDecoder *Alloc( void );
	static void				Free( idSampleDecoder *decoder );
	static int				GetNumUsedBlocks( void );
	static int				GetUsedBlockMemory( void );

	virtual					~idSampleDecoder( void ) {}
	virtual void			Decode( idSoundSample *sample, int sampleOffset44k, int sampleCount44k, float *dest ) = 0;
	virtual void			ClearDecoder( void ) = 0;
	virtual idSoundSample *	GetSample( void ) const = 0;
	virtual int				GetLastDecodeTime( void ) const = 0;
};


/*
===================================================================================

  The actual sound cache.

===================================================================================
*/

class idSoundCache {
public:
							idSoundCache();
							~idSoundCache();

	idSoundSample *			FindSound( const idStr &fname, bool loadOnDemandOnly );

	const int				GetNumObjects( void ) { return listCache.Num(); }
	const idSoundSample *	GetObject( const int index ) const;

	void					ReloadSounds( bool force );

	void					BeginLevelLoad();
	void					EndLevelLoad();

	void					PrintMemInfo( MemInfo_t *mi );

private:
	bool					insideLevelLoad;
	idList<idSoundSample*>	listCache;
};

#endif /* !__SND_LOCAL_H__ */
