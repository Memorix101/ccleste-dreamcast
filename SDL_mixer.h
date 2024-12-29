
#ifndef _FAKE_MIXER_H
#define _FAKE_MIXER_H

#include <sound/sfxmgr.h>
#include <mp3/sndserver.h>
//#include <oggvorbis/sndoggvorbis.h>
#include <SDL2/SDL.h>
#include "mikmod.h"

#define MIX_DEFAULT_FORMAT	0
#define MIX_MAX_VOLUME		255
#define MIX_DEFAULT_CHANNELS 2

#define Mix_GetError		SDL_GetError
//#define Mix_Music			void
//#define Mix_Chunk			void

// Definition für Mix_Music
typedef struct {
    char file_path[256]; // Statische Speicherallokation für den Dateipfad
} Mix_Music;


#ifdef __cplusplus
extern "C" {
#endif

struct _Mix_Chunk
{
	sfxhnd_t handle;
	int volume;
};
typedef struct _Mix_Chunk Mix_Chunk;

int Mix_OpenAudio(int frequency, uint16 format, int channels, int chunksize);
void Mix_CloseAudio(void);

void Mix_FreeChunk(Mix_Chunk *chunk);
int Mix_PlayingMusic(void);
int Mix_Playing(int channel);

int Mix_PlayChannel(int channel, Mix_Chunk *chunk, int loops);
int Mix_PlayChannelTimed(int channel, Mix_Chunk *chunk, int loops, int ticks);
int Mix_HaltChannel(int channel);
int Mix_Volume(int channel, int volume);

//Mix_Chunk *Mix_LoadWAV(const char *file);
Mix_Music *Mix_LoadMUS(const char *file);
Mix_Chunk *Mix_LoadWAV_RW(SDL_RWops *src, int freesrc);

int Mix_VolumeChunk(Mix_Chunk *chunk, int volume);

Mix_Music *Mix_LoadMUS(const char *file);
Mix_Music *Mix_LoadMUS_RW(SDL_RWops *rw);

int Mix_PlayMusic(Mix_Music *music, int loops);
int Mix_HaltMusic(void);
void Mix_FreeMusic(Mix_Music *music);
int Mix_SetMusicPosition(double position);
int Mix_AllocateChannels(int numchans);
int Mix_ReserveChannels(int num);
int Mix_QuerySpec(int *frequency, Uint16 *format, int *channels);
int Mix_SetReverseStereo(int channel, int flip);
int Mix_SetDistance(int channel, Uint8 distance); 
int Mix_SetPosition(int channel, Sint16 angle, Uint8 distance);
int Mix_SetPanning(int channel, Uint8 left, Uint8 right);
int Mix_FadeOutMusic(int ms);
int Mix_FadeInMusic(Mix_Music *music, int loops, int ms);
void Mix_Resume(int channel);
void Mix_Pause(int channel);
void Mix_ResumeMusic(void);
void Mix_PauseMusic(void);
int Mix_VolumeMusic(int volume);
void Mix_HookMusicFinished(void (*music_finished)());
void Mix_HookMusic(void (*mix_func)(void *udata), void *arg);
int Mix_GetTempo(Mix_Music *music);
void Mix_SetTempo(Mix_Music *music, int tempo);
void Mix_ChannelFinished(void (*channel_finished)(int channel));
int Mix_SetMusicCMD(const char *command);

//This function is from SDL_mixer, to detect music formats from RW
typedef enum {
	MUS_NONE,
	MUS_CMD,
	MUS_WAV,
	MUS_MOD,
	MUS_MID,
	MUS_OGG,
	MUS_MP3,
	MUS_MP3_MAD,
	MUS_FLAC,
	MUS_MODPLUG
} Mix_MusicType;

Mix_MusicType detect_music_type(SDL_RWops *rw);
int detect_mp3(Uint8 *magic);

#ifdef __cplusplus
}
#endif

#endif
