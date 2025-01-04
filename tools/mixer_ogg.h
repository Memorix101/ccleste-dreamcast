#ifndef MIXER_OGG_H
#define MIXER_OGG_H

#include <stdint.h>
#include <oggvorbis/sndoggvorbis.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *file_path;
} Mix_Music;

typedef struct {
    int sound_id;
    int volume; 
} Mix_Chunk;

#define MIX_DEFAULT_FORMAT 0
#define MIX_MAX_VOLUME 255

int Mix_OpenAudio(int frequency, uint16_t format, int channels, int chunksize);
void Mix_CloseAudio(void);

Mix_Music *Mix_LoadMUS(const char *file);
Mix_Chunk *Mix_LoadWAV(const char *file);

int Mix_PlayMusic(Mix_Music *music, int loops);
int Mix_PlayChannel(int channel, Mix_Chunk *chunk, int loops);

void Mix_HaltMusic(void);
void Mix_FreeMusic(Mix_Music *music);
void Mix_FreeChunk(Mix_Chunk *chunk);
int Mix_FadeInMusic(Mix_Music *music, int loops, int ms);
int Mix_FadeOutMusic(int ms);
void Mix_PauseMusic(void);
void Mix_Resume(int channel);
void Mix_ResumeMusic(void);
void Mix_Pause(int channel);
int Mix_HaltChannel(int channel);

#define Mix_GetError		SDL_GetError

int Mix_SetMusicPosition(double position);
int Mix_VolumeChunk(Mix_Chunk *chunk, int volume);
int Mix_VolumeMusic(int volume);

#ifdef __cplusplus
}
#endif

#endif // MIXER_OGG_H
