#include "mixer_ogg.h"
#include <stdarg.h>
#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include <kos.h>
#include <oggvorbis/sndoggvorbis.h>

char *strdup(const char *src) {
    char *dst = (char *)malloc(strlen(src) + 1);
    if (dst) {
        strcpy(dst, src);
    }
    return dst;
}

static char mix_error[256] = "";

static int playing = 0;
static int looping = 0;
static Mix_Music *current_music = NULL;

static void Mix_SetError(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(mix_error, sizeof(mix_error), fmt, args);
    va_end(args);
}

int Mix_OpenAudio(int frequency, uint16_t format, int channels, int chunksize) {
    printf("SDL_Mixer mixer_ogg\n");
    if (snd_stream_init() < 0) {
        Mix_SetError("Mix_OpenAudio: Failed to initialize sound system.");
        return -1;
    }

    if (sndoggvorbis_init() < 0) {
        Mix_SetError("Mix_OpenAudio: Failed to initialize sndoggvorbis.");
        return -1;
    }

    return 0;
}

Mix_Music *Mix_LoadMUS(const char *file) {
    Mix_Music *music = (Mix_Music *)malloc(sizeof(Mix_Music));
    if (!music) {
        Mix_SetError("Mix_LoadMUS: Memory allocation failed.");
        return NULL;
    }

    music->file_path = strdup(file);
    if (!music->file_path) {
        Mix_SetError("Mix_LoadMUS: Failed to allocate memory for file path.");
        free(music);
        return NULL;
    }

    return music;
}

Mix_Chunk *Mix_LoadWAV(const char *file) {
    int sound_id = snd_sfx_load(file);
    if (sound_id < 0) {
        Mix_SetError("Mix_LoadWAV: Failed to load WAV file %s.", file);
        return NULL;
    }

    Mix_Chunk *chunk = (Mix_Chunk *)malloc(sizeof(Mix_Chunk));
    if (!chunk) {
        Mix_SetError("Mix_LoadWAV: Memory allocation failed for Mix_Chunk.");
        return NULL;
    }

    chunk->sound_id = sound_id;
    return chunk;
}

int Mix_PlayMusic(Mix_Music *music, int loops) {
    if (!music) {
        Mix_SetError("Mix_PlayMusic: No music to play.");
        return -1;
    }

    int loop_flag = 0; 
    if (loops == -1) {
        loop_flag = 1; 
        printf("Looping: %s\n", (char *)music);
    } else {
        printf("Playing once: %s\n", (char *)music);
    }

    if (sndoggvorbis_start(music->file_path, loop_flag) < 0) {
        Mix_SetError("Mix_PlayMusic: Failed to start playback for file %s.", music->file_path);
        return -1;
    }

    current_music = music;
    playing = 1;
    looping = loops;
    return 0;
}

int Mix_PlayChannel(int channel, Mix_Chunk *chunk, int loops) {
    if (!chunk) {
        Mix_SetError("Mix_PlayChannel: Invalid Mix_Chunk.");
        return -1;
    }

    int result = snd_sfx_play(chunk->sound_id, loops, channel);
    if (result < 0) {
        Mix_SetError("Mix_PlayChannel: Failed to play sound ID %d.", chunk->sound_id);
        return -1;
    }

    return 0;
}

void Mix_HaltMusic() {
    if (playing) {
        sndoggvorbis_stop();
        playing = 0;
    }
}

static int music_paused = 0;  // 0 = playback , 1 = paused

void Mix_Pause(int channel) {
    if (music_paused) {
        Mix_SetError("Mix_Pause: Music is already paused.");
        return;
    }

    sndoggvorbis_stop();

    music_paused = 1;
    printf("Music paused.\n");
}

int Mix_HaltChannel(int channel) {
    if (channel < 0) {
        Mix_SetError("Mix_HaltChannel: Invalid channel %d.", channel);
        return -1;
    }

    snd_sfx_stop(channel);

    printf("Channel %d halted.", channel);
    return 0;
}


int Mix_FadeInMusic(Mix_Music *music, int loops, int ms) {
    if (!music) {
        Mix_SetError("Mix_FadeInMusic: No music provided.");
        return -1;
    }

    sndoggvorbis_start(music->file_path, loops);
    return 0;
}


int Mix_FadeOutMusic(int ms) {
    if (!playing) {
        Mix_SetError("Mix_FadeOutMusic: No music is playing.");
        return -1;
    }

    sndoggvorbis_stop();
    playing = 0;
    return 0;
}

void Mix_PauseMusic(void) {
    Mix_SetError("Mix_PauseMusic: Not implemented.");
}

void Mix_Resume(int channel) {
    Mix_SetError("Mix_Resume: Not implemented for channel %d.", channel);
}

void Mix_ResumeMusic(void) {
    Mix_SetError("Mix_ResumeMusic: Not implemented.");
}

void Mix_FreeMusic(Mix_Music *music) {
    if (music) {
        if (music->file_path) {
            free(music->file_path);
        }
        free(music);
    }
}

void Mix_FreeChunk(Mix_Chunk *chunk) {
    if (chunk) {
        snd_sfx_unload(chunk->sound_id);
        free(chunk);
    }
}

void Mix_CloseAudio() {
    Mix_HaltMusic();
    snd_sfx_unload_all();
    sndoggvorbis_stop();
    sndoggvorbis_shutdown();
    snd_stream_shutdown();
}