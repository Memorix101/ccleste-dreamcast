#include <SDL2/SDL.h>

#ifdef DREAMCAST
#include <kos.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#endif

#ifdef __XBOX__
#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>
#include <stdbool.h>
#include <hal/xbox.h>

static int SCREEN_WIDTH;
static int SCREEN_HEIGHT;
#endif

#if CELESTE_P8_ENABLE_AUDIO
#include "SDL_mixer.h" //fake_mixer
//#include "mixer_ogg.h"
#endif
#if SDL_MAJOR_VERSION >= 2
#include "sdl20compat.inc.c"
#else
#define SDL_Log printf
#endif
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include "celeste.h"

#ifdef N3DS_DEBUG
#include <3ds.h>
#endif

static void ErrLog(char* fmt, ...)
{
    FILE*   f = stderr;
    va_list ap;

    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);

    if (f != stderr && f != stdout)
    {
        fclose(f);
    }
}

SDL_Surface* screen  = NULL;
SDL_Surface* gfx     = NULL;
SDL_Surface* font    = NULL;
#if CELESTE_P8_ENABLE_AUDIO
Mix_Chunk*   snd[64] = {NULL};
Mix_Music*   mus[6]  = {NULL};
#endif

#define PICO8_W 128
#define PICO8_H 128

#if defined (__NGAGE__) || defined (__3DS__)
static int scale = 1;
#elif defined (__PSP__)
static int scale = 2;
#elif defined (NGAGE_DEBUG)
static int scale = 3;
#elif defined (__PSP__)
static int scale = 2;
#elif defined (__XBOX__)
static int scale = 1;
#elif defined (DREAMCAST)
static int scale = 1;
#else
static int scale = 4;
#endif

#if defined (__PSP__)
#define AUDIO_FREQ 44100
#else
#define AUDIO_FREQ 22050
#endif

static const SDL_Color base_palette[16] =
{
    { 0x00, 0x00, 0x00 },
    { 0x1d, 0x2b, 0x53 },
    { 0x7e, 0x25, 0x53 },
    { 0x00, 0x87, 0x51 },
    { 0xab, 0x52, 0x36 },
    { 0x5f, 0x57, 0x4f },
    { 0xc2, 0xc3, 0xc7 },
    { 0xff, 0xf1, 0xe8 },
    { 0xff, 0x00, 0x4d },
    { 0xff, 0xa3, 0x00 },
    { 0xff, 0xec, 0x27 },
    { 0x00, 0xe4, 0x36 },
    { 0x29, 0xad, 0xff },
    { 0x83, 0x76, 0x9c },
    { 0xff, 0x77, 0xa8 },
    { 0xff, 0xcc, 0xaa }
};

static SDL_Color palette[16];
static Uint32 map[16];

#define getcolor(col) map[col % 16]

static void SetPaletteEntry(char idx, char base_idx)
{
    palette[idx] = base_palette[base_idx];
    //map[idx] = SDL_MapRGB(screen->format, palette[idx].r, palette[idx].g, palette[idx].b);
    map[idx] = SDL_MapRGB(screen->format, palette[idx].b, palette[idx].g, palette[idx].r);
}

static void RefreshPalette(void)
{
    int i;

    for (i = 0; i < SDL_arraysize(map); i++)
    {
        //map[i] = SDL_MapRGB(screen->format, palette[i].r, palette[i].g, palette[i].b);
        map[i] = SDL_MapRGB(screen->format, palette[i].b, palette[i].g, palette[i].r);
    }
}

static void ResetPalette(void)
{
    SDL_memcpy(palette, base_palette, sizeof palette);
    RefreshPalette();
}

static char* GetDataPath(char* path, int n, const char* fname)
{
#ifdef _WIN32
    char pathsep = '\\';
#else
    char pathsep = '/';
#endif //_WIN32

#if defined (__NGAGE__)
    SDL_snprintf(path, n, "E:\\System\\Apps\\Celeste\\data\\%s", fname);
#elif defined(__XBOX__)
    SDL_snprintf(path, n, "D:\\data\\%s", fname);
#elif defined(DREAMCAST)
    SDL_snprintf(path, n, "/cd/data/%s", fname);
#else
    SDL_snprintf(path, n, "data%c%s", pathsep, fname);
#endif

    return path;
}

static Uint32 getpixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp)
    {
        case 1:
            return *p;
        case 2:
            return *(Uint16 *)p;
        case 3:
            if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            {
                return p[0] << 16 | p[1] << 8 | p[2];
            }
            else
            {
                return p[0] | p[1] << 8 | p[2] << 16;
            }
        case 4:
            return *(Uint32 *)p;
        default:
            return 0;
    }
}

static void loadbmpscale(char* filename, SDL_Surface** s)
{
    SDL_Surface *surf = *s;
    char           tmpath[4096];
    SDL_Surface*   bmp;
    int            w, h;
    unsigned char* data;
    int            y;

    if (surf)
    {
        SDL_FreeSurface(surf), surf = *s = NULL;
    }

    bmp = SDL_LoadBMP(GetDataPath(tmpath, sizeof tmpath, filename));
    if (! bmp)
    {
        ErrLog("error loading bmp '%s': %s\n", filename, SDL_GetError());
        return;
    }

    w = bmp->w;
    h = bmp->h;

    surf = SDL_CreateRGBSurface(SDL_SWSURFACE, w*scale, h*scale, 8, 0,0,0,0);
    assert(surf != NULL);
    data = surf->pixels;
    /*memcpy((_S)->format->palette->colors, base_palette, 16*sizeof(SDL_Color));*/
    for (y = 0; y < h; y++)
    {
        int x;
        for (x = 0; x < w; x++)
        {
            unsigned char pix = getpixel(bmp, x, y);
            int i;
            for (i = 0; i < scale; i++)
            {
                int j;
                for (j = 0; j < scale; j++)
                {
                    data[x*scale+i + (y*scale+j)*w*scale] = pix;
                }
            }
        }
    }
    SDL_FreeSurface(bmp);
    SDL_SetPalette(surf, SDL_PHYSPAL | SDL_LOGPAL, (SDL_Color*)base_palette, 0, 16);
    SDL_SetColorKey(surf, SDL_SRCCOLORKEY, 0);

    *s = surf;
}

#define LOGLOAD(w) SDL_Log("loading %s...", w)
#define LOGDONE() SDL_Log("done")

//setup vmu display image
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/vmu.h>
#include "vmu.xpm"
int vmu_task = 0; //bool 

/* LCD Test: this will do a grayscale seperation into several "frames" and
   flip through them quickly to give the illusion of grayscale on the LCD
   display. */

uint8 lcd_disp[8][48 * 32 / 8];
void lcd_gs_pixel(int x, int y, int amt) {
    int i;
    for(i = 0; i < amt; i++)
        lcd_disp[i][(y * 48 + x) / 8] |= 0x80 >> (x & 7);
}
void lcd_gs_setup() {
    char **xpm = graphic_xpm + 12;  // Skip header 
    int x, y;
    memset(lcd_disp, 0, sizeof(lcd_disp));
    for(y = 0; y < 32; y++) {
        for(x = 0; x < 48; x++) {
            // Note that LCD images must be flipped!
            int pixel = xpm[31 - y][47 - x];

            switch(pixel) {
                case ' ':   /* White */
                    lcd_gs_pixel(x, y, 1);
                    break;
                case '.':   /* #e3e3e3 */
                    //lcd_gs_pixel(x, y, 1);
                    break;
                case 'X':   /* #c7c7c7 */ 
                    //lcd_gs_pixel(x, y, 2);
                    break;
                case 'o':   /* #aaaaaa */
                    //lcd_gs_pixel(x, y, 3);
                    break;
                case 'O':   /* #8e8e8e */
                case '+':   /* #727272 */
                    //lcd_gs_pixel(x, y, 4);
                    break;
                case '$':   /* #555555 */
                    //lcd_gs_pixel(x, y, 5);
                    break;
                case '*':   /* #393939 */
                    //lcd_gs_pixel(x, y, 6);
                    break;
                case '&':   /* #1d1d1d */
                case '%':   /* #000000 */
                    //lcd_gs_pixel(x, y, 7);
                    break;
            }
        }
    }
}

// This performs the actual magic
void lcd_test() {
    lcd_gs_setup();
    while(vmu_task == 0) {
        maple_device_t *addr = maple_enum_type(0, MAPLE_FUNC_LCD);

        if(addr) {
            int rv = vmu_draw_lcd(addr, lcd_disp);

            if(rv < 0)
                printf("got error %d\n", rv);
            else {
             //printf("mounted vmu successfully %d\n", rv);
              vmu_task = 1;
              break;
            }
        }
    }
}

static void LoadData(void)
{
#if CELESTE_P8_ENABLE_AUDIO
    static const char sndids[] = {0,1,2,3,4,5,6,7,8,9,13,14,15,16,23,35,37,38,40,50,51,54,55};
    static const char musids[] = {0,10,20,30,40};
    int iid;
#endif
    LOGLOAD("gfx.bmp");
    loadbmpscale("gfx.bmp", &gfx);
    LOGDONE();

    LOGLOAD("font.bmp");
    loadbmpscale("font.bmp", &font);
    LOGDONE();

#if CELESTE_P8_ENABLE_AUDIO
    for (iid = 0; iid < sizeof sndids; iid++)
    {
        int  id = sndids[iid];
        char fname[20];
        char path[4096];

        SDL_snprintf(fname, 20, "snd%i.wav", id);
        LOGLOAD(fname);
        GetDataPath(path, sizeof path, fname);
        snd[id] = Mix_LoadWAV(path);
        if (! snd[id])
        {
            ErrLog("snd%i: Mix_LoadWAV: %s\n", id, Mix_GetError());
        }
        LOGDONE();
    }

    for (iid = 0; iid < sizeof musids; iid++)
    {
        int  id = musids[iid];
        char fname[20];
        char path[4096];

        SDL_snprintf(fname, 20, "mus%i.mp3", id);
        LOGLOAD(fname);
        GetDataPath(path, sizeof path, fname);
        mus[id/10] = Mix_LoadMUS(path);
        if (!mus[id/10])
        {
            ErrLog("mus%i: Mix_LoadMUS: %s\n", id, Mix_GetError());
        }
        LOGDONE();
    }
#endif
}

#include "tilemap.h"

static Uint16 buttons_state = 0;

#define SDL_CHECK(r) do {                                       \
        if (!(r)) {                                             \
            ErrLog("%s:%i, fatal error: `%s` -> %s\n",          \
                   __FILE__, __LINE__, #r, SDL_GetError());     \
            exit(2);                                            \
        }                                                       \
    } while(0)

static void p8_rectfill(int x0, int y0, int x1, int y1, int col);
static void p8_print(const char* str, int x, int y, int col);

//on-screen display (for info, such as loading a state, toggling screenshake, toggling fullscreen, etc)
static char osd_text[200] = "";
static int  osd_timer = 0;

static void OSDset(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    SDL_vsnprintf(osd_text, sizeof osd_text, fmt, ap);
    osd_text[sizeof osd_text - 1] = '\0'; //make sure to add NUL terminator in case of truncation
    SDL_Log("%s", osd_text);
    osd_timer = 30;
    va_end(ap);
}

static void OSDdraw(void)
{
    if (osd_timer > 0)
    {
        --osd_timer;
    }

    if (osd_timer > 0)
    {
        const int x = 4;
        const int y = 120 + (osd_timer < 10 ? 10-osd_timer : 0); //disappear by going below the screen
        p8_rectfill(x-2, y-2, x+4*(int)SDL_strlen(osd_text), y+6, 6); //outline
        p8_rectfill(x-1, y-1, x+4*(int)SDL_strlen(osd_text)-1, y+5, 0);
        p8_print(osd_text, x, y, 7);
    }
}

#if CELESTE_P8_ENABLE_AUDIO
static Mix_Music* current_music      = NULL;
#endif
static _Bool      enable_screenshake = 1;
static _Bool      paused             = 0;
static _Bool      running            = 1;
static void*      initial_game_state = NULL;
static void*      game_state         = NULL;
#if CELESTE_P8_ENABLE_AUDIO
static Mix_Music* game_state_music   = NULL;
#endif
static void       mainLoop(void);
static FILE*      TAS                = NULL;

int main(int argc, char** argv)
{
    int pico8emu(CELESTE_P8_CALLBACK_TYPE call, ...);
    int videoflag = SDL_SWSURFACE | SDL_ANYFORMAT;
    int initflag  = SDL_INIT_VIDEO;
#if CELESTE_P8_ENABLE_AUDIO
    //int mixflag = MIX_INIT_OGG;
    int i;
    initflag |= SDL_INIT_AUDIO;
#endif
    SDL_CHECK(SDL_Init(initflag) == 0);
#if SDL_MAJOR_VERSION >= 2 && ! defined (__NGAGE__) && ! defined (NGAGE_DEBUG)
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    SDL_GameControllerAddMappingsFromRW(SDL_RWFromFile("gamecontrollerdb.txt", "rb"), 1);
#endif
#ifdef N3DS_DEBUG
    consoleInit(GFX_BOTTOM, NULL);
#endif
#if defined (__NGAGE__) || defined (NGAGE_DEBUG)
    printf("__NGAGE__\n");
    SDL_CHECK(screen = SDL_SetVideoMode(176 * scale, 208 * scale, 32, videoflag));
#elif defined (__3DS__)
    printf("__3DS__\n");
    SDL_CHECK(screen = SDL_SetVideoMode(400, 240, 32, videoflag));
#elif defined (__PSP__)
    printf("__PSP__\n");
    SDL_CHECK(screen = SDL_SetVideoMode(480, 272, 32, videoflag));
#elif defined (__XBOX__)
    printf("__XBOX__\n");
    SDL_CHECK(screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, videoflag));
#elif defined (DREAMCAST)
    printf("DREAMCAST\n");
    SDL_CHECK(screen = SDL_SetVideoMode(PICO8_W*scale, PICO8_H*scale, 32, videoflag));
#else
    printf("ANY\n");
    SDL_CHECK(screen = SDL_SetVideoMode(PICO8_W*scale, PICO8_H*scale, 32, videoflag));
#endif
    SDL_WM_SetCaption("Celeste", NULL);
#if CELESTE_P8_ENABLE_AUDIO
    /*if (Mix_Init(mixflag) != mixflag)
    {
        ErrLog("Mix_Init: %s\n", Mix_GetError());
    }*/
    if (Mix_OpenAudio(AUDIO_FREQ, AUDIO_S16SYS, 1, 1024) < 0)
    {
        ErrLog("Mix_Init: %s\n", Mix_GetError());
    }
#endif
    ResetPalette();
    SDL_ShowCursor(0);

    if (argc > 1)
    {
        TAS = fopen(argv[1], "r");
        if (!TAS)
        {
            SDL_Log("couldn't open TAS file '%s': %s", argv[1], strerror(errno));
        }
    }

    SDL_Log("game state size %gkb", Celeste_P8_get_state_size()/1024.);

    //SDL_Log("Ported to Dreamcast Memorix101");
    //SDL_Delay(3000);

    SDL_Log("now loading...");

    {
        const unsigned char loading_bmp[] = {
            0x42, 0x4d, 0xca, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x82, 0x00,
            0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x09, 0x00,
            0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00,
            0x00, 0x00, 0x23, 0x2e, 0x00, 0x00, 0x23, 0x2e, 0x00, 0x00, 0x02, 0x00,
            0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x42, 0x47, 0x52, 0x73, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
            0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
            0x00, 0x00, 0x66, 0x3e, 0xf1, 0x24, 0xf0, 0x00, 0x00, 0x00, 0x49, 0x44,
            0x92, 0x24, 0x90, 0x00, 0x00, 0x00, 0x49, 0x3c, 0x92, 0x24, 0x90, 0x00,
            0x00, 0x00, 0x49, 0x04, 0x92, 0x24, 0x90, 0x00, 0x00, 0x00, 0x46, 0x38,
            0xf0, 0x3c, 0xf0, 0x00, 0x00, 0x00, 0x40, 0x00, 0x12, 0x00, 0x00, 0x00,
            0x00, 0x00, 0xc0, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00
        };

        SDL_RWops*   rw      = SDL_RWFromConstMem(loading_bmp, sizeof loading_bmp);
        SDL_Surface* loading = SDL_LoadBMP_RW(rw, 1);
        #if defined (__XBOX__)
        SDL_Rect     rc      = {45, 60};
        #elif defined (DREAMCAST)
        SDL_Rect     rc      = {50, 60};
        #else
        SDL_Rect     rc      = {60, 60};
        #endif

        if (!loading)
        {
            #if defined (__XBOX__)
            Sleep(1000);
            #elif defined (DREAMCAST)
            //SDL_Delay(1000);
            #endif
            goto skip_load;
        }

        SDL_BlitSurface(loading, NULL, screen, &rc);
        SDL_Flip(screen);
        SDL_FreeSurface(loading);
    }
skip_load:

    LoadData();

    lcd_test();

    Celeste_P8_set_call_func(pico8emu);

    //for reset
    initial_game_state = SDL_malloc(Celeste_P8_get_state_size());
    if (initial_game_state)
    {
        Celeste_P8_save_state(initial_game_state);
    }

    if (TAS)
    {
        // a consistent seed for tas playback
        Celeste_P8_set_rndseed(8);
    }
    else
    {
        Celeste_P8_set_rndseed((unsigned)(time(NULL) + SDL_GetTicks()));
    }

    Celeste_P8_init();

    SDL_Log("ready");
    {
        FILE*       start_fullscreen_f = fopen("ccleste-start-fullscreen.txt", "r");
        const char* start_fullscreen_v = SDL_getenv("CCLESTE_START_FULLSCREEN");
        if (start_fullscreen_f || (start_fullscreen_v && *start_fullscreen_v))
        {
            SDL_WM_ToggleFullScreen(screen);
        }
        if (start_fullscreen_f)
        {
            fclose(start_fullscreen_f);
        }
    }

#if !defined(EMSCRIPTEN)
    while (running) {
        mainLoop();
    }
#else
#include <emscripten.h>
    //FIXME: this assumes that the display refreshes at 60Hz
    emscripten_set_main_loop(mainLoop, 0, 0);
    emscripten_set_main_loop_timing(EM_TIMING_RAF, 2);
    return 0;
#endif

    if (game_state)
    {
        SDL_free(game_state);
    }
    if (initial_game_state)
    {
        SDL_free(initial_game_state);
    }

    SDL_FreeSurface(gfx);
    SDL_FreeSurface(font);
#if CELESTE_P8_ENABLE_AUDIO
    for (i = 0; i < (sizeof snd)/(sizeof *snd); i++)
    {
        if (snd[i])
        {
            Mix_FreeChunk(snd[i]);
        }
    }
    for (i = 0; i < (sizeof mus)/(sizeof *mus); i++)
    {
        if (mus[i])
        {
            Mix_FreeMusic(mus[i]);
        }
    }

    Mix_CloseAudio();
    //Mix_Quit();
#endif
    SDL_Quit();
    return 0;
}

#if SDL_MAJOR_VERSION >= 2
/* These inputs aren't sent to the game. */
enum {
    PSEUDO_BTN_SAVE_STATE = 6,
    PSEUDO_BTN_LOAD_STATE = 7,
    PSEUDO_BTN_EXIT = 8,
    PSEUDO_BTN_PAUSE = 9,
};
static void ReadGamepadInput(Uint16* out_buttons);
#endif

static void mainLoop(void)
{
    const Uint8* kbstate            = SDL_GetKeyState(NULL);
    static int   reset_input_timer  = 0;
    Uint16       prev_buttons_state = buttons_state;
    SDL_Event    ev;

    //hold F9 (select+start+y) to reset
    if (initial_game_state != NULL && kbstate[SDLK_F9])
    {
        reset_input_timer++;
        if (reset_input_timer >= 30)
        {
            reset_input_timer=0;
            //reset
            OSDset("reset");
            paused = 0;
            Celeste_P8_load_state(initial_game_state);
            Celeste_P8_set_rndseed((unsigned)(time(NULL) + SDL_GetTicks()));
#if CELESTE_P8_ENABLE_AUDIO
            Mix_HaltChannel(-1);
            Mix_HaltMusic();
#endif
            Celeste_P8_init();
        }
    }
    else
    {
        reset_input_timer = 0;
    }

    prev_buttons_state = buttons_state;
    buttons_state = 0;

#if SDL_MAJOR_VERSION >= 2 && ! defined (__NGAGE__) && ! defined (NGAGE_DEBUG)
    SDL_GameControllerUpdate();
    ReadGamepadInput(&buttons_state);

    if (!((prev_buttons_state >> PSEUDO_BTN_PAUSE) & 1) && (buttons_state >> PSEUDO_BTN_PAUSE) & 1)
    {
        goto toggle_pause;
    }

    if (!((prev_buttons_state >> PSEUDO_BTN_EXIT) & 1) && (buttons_state >> PSEUDO_BTN_EXIT) & 1)
    {
        goto press_exit;
    }

    if (!((prev_buttons_state >> PSEUDO_BTN_SAVE_STATE) & 1) && (buttons_state >> PSEUDO_BTN_SAVE_STATE) & 1)
    {
        goto save_state;
    }

    if (!((prev_buttons_state >> PSEUDO_BTN_LOAD_STATE) & 1) && (buttons_state >> PSEUDO_BTN_LOAD_STATE) & 1)
    {
        goto load_state;
    }
#endif

    while (SDL_PollEvent(&ev))
    {
        switch (ev.type)
        {
            case SDL_QUIT:
                running = 0;
                break;
            case SDL_KEYDOWN:
            {
#if SDL_MAJOR_VERSION >= 2
                if (ev.key.repeat) //no key repeat
                {
                    break;
                }
#endif
                if (ev.key.keysym.sym == SDLK_ESCAPE) //do pause
                {
                toggle_pause:
#if CELESTE_P8_ENABLE_AUDIO
                    if (paused)
                    {
                        Mix_Resume(-1);
                        Mix_ResumeMusic();
                    }
                    else
                    {
                        Mix_Pause(-1);
                        Mix_PauseMusic();
                    }
#endif
                    paused = !paused;
                    break;
                }
                else if (ev.key.keysym.sym == SDLK_DELETE) //exit
                {
                press_exit:
                    running = 0;
                    
#if defined (__XBOX__)
    XReboot();
#endif

                    break;
                }
                else if (ev.key.keysym.sym == SDLK_F11 && !(kbstate[SDLK_LSHIFT] || kbstate[SDLK_ESCAPE]))
                {
                    if (SDL_WM_ToggleFullScreen(screen)) //this doesn't work on windows..
                    {
                        OSDset("toggle fullscreen");
                    }
                    screen = SDL_GetVideoSurface();
                    RefreshPalette();
                    break;
                }
                else if (0 && ev.key.keysym.sym == SDLK_5)
                {
                    Celeste_P8__DEBUG();
                    break;
                }
#if defined (__NGAGE__)
                else if (ev.key.keysym.sym == SDLK_s) //save state
#else
                else if (ev.key.keysym.sym == SDLK_s && kbstate[SDLK_LSHIFT]) //save state
#endif
                {
                save_state:
                    game_state = game_state ? game_state : SDL_malloc(Celeste_P8_get_state_size());
                    if (game_state)
                    {
                        OSDset("save state");
                        Celeste_P8_save_state(game_state);
#if CELESTE_P8_ENABLE_AUDIO
                        game_state_music = current_music;
#endif
                    }
                    break;
                }
#if defined (__NGAGE__)
                else if (ev.key.keysym.sym == SDLK_d) //load state
#else
                else if (ev.key.keysym.sym == SDLK_d && kbstate[SDLK_LSHIFT]) //load state
#endif
                {
                load_state:
                    if (game_state)
                    {
                        OSDset("load state");
#if CELESTE_P8_ENABLE_AUDIO
                        if (paused)
                        {
                            paused = 0;
                            Mix_Resume(-1);
                            Mix_ResumeMusic();
                        }
#endif
                        Celeste_P8_load_state(game_state);
#if CELESTE_P8_ENABLE_AUDIO
                        if (current_music != game_state_music)
                        {
                            Mix_HaltMusic();
                            current_music = game_state_music;
                            if (game_state_music)
                            {
                                Mix_PlayMusic(game_state_music, -1);
                            }
                        }
#endif
                    }
                    break;
                }
                else if (ev.key.keysym.sym == SDLK_e) //toggle screenshake (e / L+R)
                {
                    enable_screenshake = !enable_screenshake;
                    OSDset("screenshake: %s", enable_screenshake ? "on" : "off");
                }
            }
            break;
        }
    }

    if (! TAS)
    {
        if (kbstate[SDLK_LEFT])  buttons_state |= (1<<0);
        if (kbstate[SDLK_RIGHT]) buttons_state |= (1<<1);
        if (kbstate[SDLK_UP])    buttons_state |= (1<<2);
        if (kbstate[SDLK_DOWN])  buttons_state |= (1<<3);
        if (kbstate[SDLK_z] || kbstate[SDLK_c] || kbstate[SDLK_n]) buttons_state |= (1<<4);
        if (kbstate[SDLK_x] || kbstate[SDLK_v] || kbstate[SDLK_m]) buttons_state |= (1<<5);
    }
    else if (TAS && !paused)
    {
        static int t = 0;
        t++;
        if (t==1)
        {
            buttons_state = 1 << 4;
        }
        else if (t > 80)
        {
            int btn;
            fscanf(TAS, "%d,", &btn);
            buttons_state = btn;
        }
        else
        {
            buttons_state = 0;
        }
    }

    if (paused)
    {
        const int x0 = PICO8_W/2-3*4, y0 = 8;

        p8_rectfill(x0-1,y0-1, 6*4+x0+1,6+y0+1, 6);
        p8_rectfill(x0,y0, 6*4+x0,6+y0, 0);
        p8_print("paused", x0+1, y0+1, 7);
    }
    else
    {
        Celeste_P8_update();
        Celeste_P8_draw();
    }
    OSDdraw();

    SDL_Flip(screen);

#ifdef EMSCRIPTEN //emscripten_set_main_loop already sets the fps
    SDL_Delay(1);
#else
    {
#if ! defined (__NGAGE__)
        static int      t           = 0;
        static unsigned frame_start = 0;
        unsigned        frame_end   = SDL_GetTicks();
        unsigned        frame_time  = frame_end-frame_start;
        unsigned        target_millis;
        // frame timing for 30fps is 33.333... ms, but we only have integer granularity
        // so alternate between 33 and 34 ms, like [33,33,34,33,33,34,...] which averages out to 33.333...
        if (t < 2)
        {
            target_millis = 33;
        }
        else
        {
            target_millis = 34;
        }

        if (++t == 3)
        {
            t = 0;
        }

        if (frame_time < target_millis)
        {
            SDL_Delay(target_millis - frame_time);
        }
        frame_start = SDL_GetTicks();
#endif
    }
#endif
}

static int  gettileflag(int, int);
static void p8_line(int,int,int,int,unsigned char);

//lots of code from https://github.com/SDL-mirror/SDL/blob/bc59d0d4a2c814900a506d097a381077b9310509/src/video/SDL_surface.c#L625
//coordinates should be scaled already
static inline void Xblit(SDL_Surface* src, SDL_Rect* srcrect, SDL_Surface* dst, SDL_Rect* dstrect, int color, int flipx, int flipy)
{
    SDL_Rect fulldst;
    int      srcx, srcy, w, h;

    assert(src && dst && !src->locked && !dst->locked);
    assert(src->format->BitsPerPixel == 8);
    assert(dst->format->BytesPerPixel == 2 || dst->format->BytesPerPixel == 4);
    /* If the destination rectangle is NULL, use the entire dest surface */
    if (!dstrect)
    {
        dstrect = (fulldst = (SDL_Rect){0,0,dst->w,dst->h}, &fulldst);
    }

    /* clip the source rectangle to the source surface */
    if (srcrect)
    {
        int maxw, maxh;

        srcx = srcrect->x;
        w = srcrect->w;
        if (srcx < 0)
        {
            w          += srcx;
            dstrect->x -= srcx;
            srcx        = 0;
        }
        maxw = src->w - srcx;
        if (maxw < w)
        {
            w = maxw;
        }

        srcy = srcrect->y;
        h = srcrect->h;
        if (srcy < 0)
        {
            h += srcy;
            dstrect->y -= srcy;
            srcy = 0;
        }
        maxh = src->h - srcy;
        if (maxh < h)
        {
            h = maxh;
        }

    }
    else
    {
        srcx = srcy = 0;
        w = src->w;
        h = src->h;
    }

    /* clip the destination rectangle against the clip rectangle */
    {
        SDL_Rect *clip = &dst->clip_rect;
        int       dx, dy;

        dx = clip->x - dstrect->x;
        if (dx > 0)
        {
            w          -= dx;
            dstrect->x += dx;
            srcx       += dx;
        }
        dx = dstrect->x + w - clip->x - clip->w;
        if (dx > 0)
        {
            w -= dx;
        }

        dy = clip->y - dstrect->y;
        if (dy > 0)
        {
            h -= dy;
            dstrect->y += dy;
            srcy += dy;
        }
        dy = dstrect->y + h - clip->y - clip->h;
        if (dy > 0)
        {
            h -= dy;
        }
    }

    if (w && h)
    {
        unsigned char* srcpix   = src->pixels;
        int            srcpitch = src->pitch;
        int            x, y;
#define _blitter(dsttype, dp, xflip) do { \
            dsttype* dstpix = dst->pixels; \
            for (y = 0; y < h; y++) \
            { \
                for (x = 0; x < w; x++) \
                { \
                    unsigned char p = srcpix[!xflip ? srcx+x+(srcy+y)*srcpitch : srcx+(w-x-1)+(srcy+y)*srcpitch]; \
                    if (p) dstpix[dstrect->x+x + (dstrect->y+y)*dst->w] = getcolor(dp); \
                } \
            } \
            } while(0)
        if (screen->format->BytesPerPixel == 2 && color && flipx)
        {
            _blitter(Uint16, color, 1);
        }
        else if (screen->format->BytesPerPixel == 2 && !color && flipx)
        {
            _blitter(Uint16, p, 1);
        }
        else if (screen->format->BytesPerPixel == 2 && color && !flipx)
        {
            _blitter(Uint16, color, 0);
        }
        else if (screen->format->BytesPerPixel == 2 && !color && !flipx)
        {
            _blitter(Uint16, p, 0);
        }
        else if (screen->format->BytesPerPixel == 4 && color && flipx)
        {
            _blitter(Uint32, color, 1);
        }
        else if (screen->format->BytesPerPixel == 4 && !color && flipx)
        {
            _blitter(Uint32, p, 1);
        }
        else if (screen->format->BytesPerPixel == 4 && color && !flipx)
        {
            _blitter(Uint32, color, 0);
        }
        else if (screen->format->BytesPerPixel == 4 && !color && !flipx)
        {
            _blitter(Uint32, p, 0);
        }
#undef _blitter
    }
}

static void p8_rectfill(int x0, int y0, int x1, int y1, int col)
{
    int w = (x1 - x0 + 1)*scale;
    int h = (y1 - y0 + 1)*scale;
    if (w > 0 && h > 0)
    {
        SDL_Rect rc = {x0*scale,y0*scale, w,h};
        SDL_FillRect(screen, &rc, getcolor(col));
    }
}

static void p8_print(const char* str, int x, int y, int col)
{
    char c;
    for (c = *str; c; c = *(++str))
    {
        SDL_Rect srcrc;
        SDL_Rect dstrc;
        c       &= 0x7F;
        srcrc.x  = 8*(c%16);
        srcrc.y  = 8*(c/16);
        srcrc.x *= scale;
        srcrc.y *= scale;
        srcrc.w  = srcrc.h = 8*scale;

        dstrc.x  = x*scale;
        dstrc.y  = y*scale;
        dstrc.w  = scale;
        dstrc.h  = scale;

        Xblit(font, &srcrc, screen, &dstrc, col, 0,0);
        x += 4;
    }
}

int pico8emu(CELESTE_P8_CALLBACK_TYPE call, ...) {
    static int camera_x = 0, camera_y = 0;
    va_list    args;
    int        ret      = 0;

    if (!enable_screenshake)
    {
        camera_x = camera_y = 0;
    }

    va_start(args, call);

#define   INT_ARG() va_arg(args, int)
#define  BOOL_ARG() (Celeste_P8_bool_t)va_arg(args, int)
#define RET_INT(_i)   do {ret = (_i); goto end;} while (0)
#define RET_BOOL(_b) RET_INT(!!(_b))

    switch (call)
    {
        case CELESTE_P8_MUSIC: //music(idx,fade,mask)
        {
#if CELESTE_P8_ENABLE_AUDIO
            int index = INT_ARG();
            int fade = INT_ARG();
            int mask = INT_ARG();

            (void)mask; //we do not care about this since sdl mixer keeps sounds and music separate

            if (index == -1) { //stop playing
                Mix_FadeOutMusic(fade);
                current_music = NULL;
            }
            else if (mus[index/10])
            {
                Mix_Music* musi = mus[index/10];
                current_music = musi;
                Mix_FadeInMusic(musi, -1, fade);
            }
#endif
            break;
        }
        case CELESTE_P8_SPR: //spr(sprite,x,y,cols,rows,flipx,flipy)
        {
            int sprite = INT_ARG();
            int x = INT_ARG();
            int y = INT_ARG();
            int cols = INT_ARG();
            int rows = INT_ARG();
            int flipx = BOOL_ARG();
            int flipy = BOOL_ARG();

            (void)cols;
            (void)rows;

            assert(rows == 1 && cols == 1);

            if (sprite >= 0)
            {
                SDL_Rect srcrc = {
                    8*(sprite % 16),
                    8*(sprite / 16)
                };
                SDL_Rect dstrc =
                    {
                    (x - camera_x)*scale, (y - camera_y)*scale,
                    scale, scale
                };
                srcrc.x *= scale;
                srcrc.y *= scale;
                srcrc.w = srcrc.h = scale*8;
                Xblit(gfx, &srcrc, screen, &dstrc, 0,flipx,flipy);
            }
            break;
        }
        case CELESTE_P8_BTN: //btn(b)
        {
            int b = INT_ARG();
            assert(b >= 0 && b <= 5);
            RET_BOOL(buttons_state & (1 << b));
            break;
        }
        case CELESTE_P8_SFX: //sfx(id)
        {
#if CELESTE_P8_ENABLE_AUDIO
            int id = INT_ARG();

            if (id < (sizeof snd) / (sizeof*snd) && snd[id])
            {
                Mix_PlayChannel(-1, snd[id], 0);
            }
#endif
            break;
        }
        case CELESTE_P8_PAL: //pal(a,b)
        {
            int a = INT_ARG();
            int b = INT_ARG();
            if (a >= 0 && a < 16 && b >= 0 && b < 16)
            {
                //swap palette colors
                SetPaletteEntry(a, b);
            }
            break;
        }
        case CELESTE_P8_PAL_RESET: //pal()
        {
            ResetPalette();
            break;
        }
        case CELESTE_P8_CIRCFILL: //circfill(x,y,r,col)
        {
            int cx        = INT_ARG() - camera_x;
            int cy        = INT_ARG() - camera_y;
            int r         = INT_ARG();
            int col       = INT_ARG();
            int realcolor = getcolor(col);

            if (r <= 1)
            {
                SDL_Rect rect_a = {scale*(cx-1), scale*cy, scale*3, scale};
                SDL_Rect rect_b = {scale*cx, scale*(cy-1), scale, scale*3};

                SDL_FillRect(screen, &rect_a, realcolor);
                SDL_FillRect(screen, &rect_b, realcolor);
            }
            else if (r <= 2)
            {
                SDL_Rect rect_a = {scale*(cx-2), scale*(cy-1), scale*5, scale*3};
                SDL_Rect rect_b = {scale*(cx-1), scale*(cy-2), scale*3, scale*5};

                SDL_FillRect(screen, &rect_a, realcolor);
                SDL_FillRect(screen, &rect_b, realcolor);
            }
            else if (r <= 3)
            {
                SDL_Rect rect_a = {scale*(cx-3), scale*(cy-1), scale*7, scale*3};
                SDL_Rect rect_b = {scale*(cx-1), scale*(cy-3), scale*3, scale*7};
                SDL_Rect rect_c = {scale*(cx-2), scale*(cy-2), scale*5, scale*5};

                SDL_FillRect(screen, &rect_a, realcolor);
                SDL_FillRect(screen, &rect_b, realcolor);
                SDL_FillRect(screen, &rect_c, realcolor);
            }
            else  //i dont think the game uses this
            {
                int f    = 1 - r; //used to track the progress of the drawn circle (since its semi-recursive)
                int ddFx = 1;   //step x
                int ddFy = -2 * r; //step y
                int x    = 0;
                int y    = r;

                //this algorithm doesn't account for the diameters
                //so we have to set them manually
                p8_line(cx,cy-y, cx,cy+r, col);
                p8_line(cx+r,cy, cx-r,cy, col);

                while (x < y)
                {
                    if (f >= 0)
                    {
                        y--;
                        ddFy += 2;
                        f += ddFy;
                    }
                    x++;
                    ddFx += 2;
                    f += ddFx;

                    //build our current arc
                    p8_line(cx+x,cy+y, cx-x,cy+y, col);
                    p8_line(cx+x,cy-y, cx-x,cy-y, col);
                    p8_line(cx+y,cy+x, cx-y,cy+x, col);
                    p8_line(cx+y,cy-x, cx-y,cy-x, col);
                }
            }
            break;
        }
        case CELESTE_P8_PRINT: //print(str,x,y,col)
        {
            const char* str = va_arg(args, const char*);
            int x = INT_ARG() - camera_x;
            int y = INT_ARG() - camera_y;
            int col = INT_ARG() % 16;

            p8_print(str,x,y,col);
            break;
        }
        case CELESTE_P8_RECTFILL: //rectfill(x0,y0,x1,y1,col)
        {
            int x0 = INT_ARG() - camera_x;
            int y0 = INT_ARG() - camera_y;
            int x1 = INT_ARG() - camera_x;
            int y1 = INT_ARG() - camera_y;
            int col = INT_ARG();

            p8_rectfill(x0,y0,x1,y1,col);
            break;
        }
        case CELESTE_P8_LINE: //line(x0,y0,x1,y1,col)
        {
            int x0 = INT_ARG() - camera_x;
            int y0 = INT_ARG() - camera_y;
            int x1 = INT_ARG() - camera_x;
            int y1 = INT_ARG() - camera_y;
            int col = INT_ARG();

            p8_line(x0,y0,x1,y1,col);
            break;
        }
        case CELESTE_P8_MGET: //mget(tx,ty)
        {
            int tx = INT_ARG();
            int ty = INT_ARG();

            RET_INT(tilemap_data[tx+ty*128]);
            break;
        }
        case CELESTE_P8_CAMERA: //camera(x,y)
        {
            if (enable_screenshake)
            {
                camera_x = INT_ARG();
                camera_y = INT_ARG();
            }
            break;
        }
        case CELESTE_P8_FGET: //fget(tile,flag)
        {
            int tile = INT_ARG();
            int flag = INT_ARG();

            RET_INT(gettileflag(tile, flag));
            break;
        }
        case CELESTE_P8_MAP: //map(mx,my,tx,ty,mw,mh,mask)
        {
            int mx   = INT_ARG(), my = INT_ARG();
            int tx   = INT_ARG(), ty = INT_ARG();
            int mw   = INT_ARG(), mh = INT_ARG();
            int mask = INT_ARG();
            int x, y;

            for (x = 0; x < mw; x++)
            {
                for (y = 0; y < mh; y++)
                {
                    int tile = tilemap_data[x + mx + (y + my)*128];
                    //hack
                    if (mask == 0 || (mask == 4 && tile_flags[tile] == 4) || gettileflag(tile, mask != 4 ? mask-1 : mask))
                    {
                        SDL_Rect srcrc =
                            {
                            8*(tile % 16),
                            8*(tile / 16)
                        };
                        SDL_Rect dstrc =
                            {
                            (tx+x*8 - camera_x)*scale, (ty+y*8 - camera_y)*scale,
                            scale*8, scale*8
                        };
                        srcrc.x *= scale;
                        srcrc.y *= scale;
                        srcrc.w = srcrc.h = scale*8;

                        /*
                        if (0)
                        {
                            srcrc.x = srcrc.y = 0;
                            srcrc.w = srcrc.h = 8;
                            dstrc.x = x*8, dstrc.y = y*8;
                            dstrc.w = dstrc.h = 8;
                        }
                        */

                        Xblit(gfx, &srcrc, screen, &dstrc, 0, 0, 0);
                    }
                }
            }
            break;
        }
    }

end:
    va_end(args);
    return ret;
}

static int gettileflag(int tile, int flag)
{
    return tile < sizeof(tile_flags)/sizeof(*tile_flags) && (tile_flags[tile] & (1 << flag)) != 0;
}

//coordinates should NOT be scaled before calling this
static void p8_line(int x0, int y0, int x1, int y1, unsigned char color)
{
    Uint32   realcolor = getcolor(color);
    int      sx, sy, dx, dy, err;
    SDL_Rect rect;

#define CLAMP(v,min,max) v = v < min ? min : v >= max ? max-1 : v;
    CLAMP(x0,0,screen->w);
    CLAMP(y0,0,screen->h);
    CLAMP(x1,0,screen->w);
    CLAMP(y1,0,screen->h);

#undef CLAMP
#define PLOT(xa, ya) \
    rect.x = xa * scale; \
    rect.y = ya * scale; \
    rect.w = scale; \
    rect.h = scale; \
    do { \
        SDL_FillRect(screen, &rect, realcolor); \
    } \
    while (0)
    dx = abs(x1 - x0);
    dy = abs(y1 - y0);
    if (!dx && !dy)
    {
        return;
    }

    if (x0 < x1)
    {
        sx = 1;
    }
    else
    {
        sx = -1;
    }
    if (y0 < y1)
    {
        sy = 1;
    }
    else
    {
        sy = -1;
    }
    err = dx - dy;
    if (!dy && !dx)
    {
        return;
    }
    else if (!dx) //vertical line
    {
        int y;
        for (y = y0; y != y1; y += sy)
        {
            PLOT(x0,y);
        }
    }
    else if (!dy) //horizontal line
    {
        int x;
        for (x = x0; x != x1; x += sx)
        {
            PLOT(x,y0);
        }
    }

    while (x0 != x1 || y0 != y1)
    {
        int e2;
        PLOT(x0, y0);
        e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
#undef PLOT
}

#if SDL_MAJOR_VERSION >= 2 && ! defined (__NGAGE__)
//SDL2: read input from connected gamepad

struct mapping
{
    SDL_GameControllerButton sdl_btn;
    Uint16 pico8_btn;
};

static const char* pico8_btn_names[] =
{
    "left", "right", "up", "down", "jump", "dash", "save", "load", "exit", "pause"
};

// initialized with default mapping
static struct mapping controller_mappings[30] =
{
    { SDL_CONTROLLER_BUTTON_DPAD_LEFT,  0 }, //left
    { SDL_CONTROLLER_BUTTON_DPAD_RIGHT, 1 }, //right
    { SDL_CONTROLLER_BUTTON_DPAD_UP,    2 }, //up
    { SDL_CONTROLLER_BUTTON_DPAD_DOWN,  3 }, //down
    { SDL_CONTROLLER_BUTTON_A,          2 }, //jump
    { SDL_CONTROLLER_BUTTON_B,          5 }, //dash

    { SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  PSEUDO_BTN_SAVE_STATE }, //save
    { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, PSEUDO_BTN_LOAD_STATE }, //load
    { SDL_CONTROLLER_BUTTON_GUIDE,         PSEUDO_BTN_EXIT }, //exit
    { SDL_CONTROLLER_BUTTON_BACK,         PSEUDO_BTN_EXIT }, //exit
    { SDL_CONTROLLER_BUTTON_START,         PSEUDO_BTN_PAUSE }, //pause
    { 0xff, 0xff }
};

static const Uint16 stick_deadzone = 32767 / 2; //about half

void print_joystick_input(SDL_Joystick *joystick) {
    if (!joystick) {
        printf("Joystick is not initialized.\n");
        return;
    }

    // Get the number of buttons and axes
    int num_buttons = SDL_JoystickNumButtons(joystick);
    int num_axes = SDL_JoystickNumAxes(joystick);
    int num_hats = SDL_JoystickNumHats(joystick);

    //printf("Number of buttons: %d\n", num_buttons);
    //printf("Number of axes: %d\n", num_axes);

    // Check button states
    //printf("Pressed buttons:\n");
    for (int i = 0; i < num_buttons; i++) {
        if (SDL_JoystickGetButton(joystick, i)) {
            printf("  Button %d: PRESSED\n", i);
        }
    }

    // Check axes states
    //printf("Active axes (outside deadzone):\n");
    Sint16 axis_value;

    for (int i = 0; i < num_axes; i++) {
        axis_value = SDL_JoystickGetAxis(joystick, i);
        if (axis_value < -stick_deadzone || axis_value > stick_deadzone) {
            printf("  Axis %d: %d\n", i, axis_value);
        }
    }

    /// Check hat states (D-Pad)
    //printf("Hat states:\n");
    for (int i = 0; i < num_hats; i++) {
        Uint8 hat_value = SDL_JoystickGetHat(joystick, i);

        printf("  Hat %d: ", i);
        switch (hat_value) {
            case SDL_HAT_CENTERED:
                //printf("CENTERED\n");
                break;
            case SDL_HAT_UP:
                printf("UP\n");
                break;
            case SDL_HAT_RIGHT:
                printf("RIGHT\n");
                break;
            case SDL_HAT_DOWN:
                printf("DOWN\n");
                break;
            case SDL_HAT_LEFT:
                printf("LEFT\n");
                break;
            case SDL_HAT_RIGHTUP:
                printf("RIGHT-UP\n");
                break;
            case SDL_HAT_RIGHTDOWN:
                printf("RIGHT-DOWN\n");
                break;
            case SDL_HAT_LEFTUP:
                printf("LEFT-UP\n");
                break;
            case SDL_HAT_LEFTDOWN:
                printf("LEFT-DOWN\n");
                break;
            default:
                printf("UNKNOWN\n");
        }
    }
}

int emulate_simple_controller(SDL_Joystick *joystick, Uint16 *out_buttons) {
    int pressed = 0; // 0: nothing pressed, 1: something pressed

/*
    { SDL_CONTROLLER_BUTTON_DPAD_LEFT,  0 }, //left
    { SDL_CONTROLLER_BUTTON_DPAD_RIGHT, 1 }, //right
    { SDL_CONTROLLER_BUTTON_DPAD_UP,    2 }, //up
    { SDL_CONTROLLER_BUTTON_DPAD_DOWN,  3 }, //down
    { SDL_CONTROLLER_BUTTON_A,          4 }, //jump
    { SDL_CONTROLLER_BUTTON_B,          5 }, //dash

    { SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  PSEUDO_BTN_SAVE_STATE }, //save
    { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, PSEUDO_BTN_LOAD_STATE }, //load
    { SDL_CONTROLLER_BUTTON_GUIDE,         PSEUDO_BTN_EXIT }, //exit
    { SDL_CONTROLLER_BUTTON_BACK,         PSEUDO_BTN_EXIT }, //exit
    { SDL_CONTROLLER_BUTTON_START,         PSEUDO_BTN_PAUSE }, //pause
    { 0xff, 0xff }
*/

    // Define the mappings for A, B, and Start buttons
    struct {
        int sdl_btn;
        int pico8_btn;
    } button_mappings[] = {
        { 2, 4 }, // A -> pico8_btn 4 (Jump)
        { 1, 5 }, // B -> pico8_btn 5 (Dash)
        { 6, 5 }, // B -> pico8_btn 5 (Dash)
        { 3, PSEUDO_BTN_PAUSE }, // B -> pico8_btn 5 (Dash)
    };

    // Check the buttons
    for (int i = 0; i < sizeof(button_mappings) / sizeof(button_mappings[0]); i++) {
        if (SDL_JoystickGetButton(joystick, button_mappings[i].sdl_btn)) {
            /*SDL_Log("Button PRESSED: SDL button %d -> pico8_btn %d",
                    button_mappings[i].sdl_btn, button_mappings[i].pico8_btn);*/
            *out_buttons |= (1 << button_mappings[i].pico8_btn);
            pressed = 1; // Mark as pressed
        }
    }

    // Check the Hats (D-Pad)
    int num_hats = SDL_JoystickNumHats(joystick);
    for (int i = 0; i < num_hats; i++) {
        Uint8 hat_value = SDL_JoystickGetHat(joystick, i);

        if (hat_value & SDL_HAT_UP) {
            //SDL_Log("Hat UP: pico8_btn 2");
            *out_buttons |= (1 << 2);
            pressed = 1; // Mark as pressed
        }
        if (hat_value & SDL_HAT_RIGHT) {
            //SDL_Log("Hat RIGHT: pico8_btn 1");
            *out_buttons |= (1 << 1);
            pressed = 1; // Mark as pressed
        }
        if (hat_value & SDL_HAT_DOWN) {
            //SDL_Log("Hat DOWN: pico8_btn 3");
            *out_buttons |= (1 << 3);
            pressed = 1; // Mark as pressed
        }
        if (hat_value & SDL_HAT_LEFT) {
            //SDL_Log("Hat LEFT: pico8_btn 0");
            *out_buttons |= (1 << 0);
            pressed = 1; // Mark as pressed
        }
    }

    return pressed;
}

//dc controller
maple_device_t *cont;
cont_state_t *state;

static void ReadGamepadInput(Uint16* out_buttons)
{
    static _Bool               read_config = 0;
    static SDL_GameController* controller  = NULL;
    Sint16                     x_axis;
    Sint16                     y_axis;
    int                        i;

    if (!read_config)
    {
        FILE*       cfg;
        const char* cfg_file_path = SDL_getenv("CCLESTE_INPUT_CFG_PATH");

        read_config = 1;

        if (!cfg_file_path)
        {
            cfg_file_path = "/cd/data/ccleste-input-cfg.txt";
            SDL_Log("input cfg: %s", cfg_file_path);
        }
        cfg = fopen(cfg_file_path, "r");
        if (cfg)
        {
            for (i = 0; i < sizeof controller_mappings / sizeof *controller_mappings - 1;)
            {
                char line[150], p8btn[31], sdlbtn[31];
                fgets(line, sizeof line - 1, cfg);
                if (feof(cfg) || ferror(cfg))
                {
                    break;
                }
                line[sizeof line - 1] = 0;
                if (*line == '#')
                {
                    //comment
                }
                else if (sscanf(line, "%30s %30s", p8btn, sdlbtn) == 2)
                {
                    int btn;
                    p8btn[sizeof p8btn - 1] = sdlbtn[sizeof sdlbtn - 1] = 0;
                    for (btn = 0; btn < sizeof pico8_btn_names / sizeof *pico8_btn_names; btn++)
                    {
                        if (!SDL_strcasecmp(pico8_btn_names[btn], p8btn))
                        {
                            SDL_Log("input cfg: %s -> %s", p8btn, sdlbtn);
                            controller_mappings[i].sdl_btn = SDL_GameControllerGetButtonFromString(sdlbtn);
                            controller_mappings[i].pico8_btn = btn;
                            i++;
                        }
                    }
                }
            }
            controller_mappings[i].pico8_btn = 0xFF;
            fclose(cfg);
        }
        else
        {
            cfg = fopen(cfg_file_path, "w");
            if (cfg)
            {
                struct mapping* mapping;
                SDL_Log("creating ccleste-input-cfg.txt with default button mappings");
                fprintf(cfg, "# in-game \tcontroller\n");
                for (mapping = controller_mappings; mapping->pico8_btn != 0xFF; mapping++)
                {
                    fprintf(cfg, "%s \t%s\n", pico8_btn_names[mapping->pico8_btn], SDL_GameControllerGetStringForButton(mapping->sdl_btn));
                }
                fclose(cfg);
            }
        }
    }

    if (!controller)
    {
        static int tries_left = 30;
        //use first available controller
        int count = SDL_NumJoysticks();

        if (! tries_left)
        {
            return;
        }
        tries_left--;

        //SDL_Log("sdl reports %i controllers", count);
        for (i = 0; i < count; i++)
        {
            
            //SDL_Log(">> controller found!");
            controller = SDL_JoystickOpen(0);
            if (!controller) {
                SDL_Log("Unable to open joystick: %s", SDL_GetError());
                return;
            }
            
            /*if (SDL_IsGameController(i))
            {
                SDL_Log(">> controller found!");
                controller = SDL_GameControllerOpen(i);
                if (!controller)
                {
                    fprintf(stderr, "error opening controller: %s\n", SDL_GetError());
                    return;
                }
                SDL_Log("picked controller: '%s'", SDL_GameControllerName(controller));
                break;
            }*/
        }
    }

    //pico 8 buttons and pseudo buttons
    //print_joystick_input(SDL_JoystickOpen(0));
    
for (i = 0; i < sizeof(controller_mappings) / sizeof(*controller_mappings); i++) 
{
    struct mapping mapping = controller_mappings[i];
    Uint16 mask;
    _Bool pressed;

    if (mapping.pico8_btn == 0xFF) {
        break; // End of mappings
    }

    // Check if the specific button in the mapping is pressed
    Uint16 xout_buttons = 0;
    emulate_simple_controller(controller, &xout_buttons);

    // Extract the pressed state for this specific pico8_btn
    pressed = (xout_buttons & (1 << mapping.pico8_btn)) != 0;

    // Log the button state
    if (pressed) {
        SDL_Log("Button mapped to pico8_btn %d (SDL button %d): PRESSED", 
                mapping.pico8_btn, 
                mapping.sdl_btn);
    }

    // Update the out_buttons bitmask
    mask = ~(1 << mapping.pico8_btn);
    *out_buttons = (*out_buttons & mask) | (pressed << mapping.pico8_btn);
}

// Joystick -> D-Pad
//x_axis = SDL_JoystickGetAxis(controller, 4);
//y_axis = SDL_JoystickGetAxis(controller, 5);

  //dc controls
  cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
  if (cont) 
  {
    //printf("axis %d", state->joyx);
    state = (cont_state_t *)maple_dev_status(cont);
    if (state->joyx <= -64) { // left
        //SDL_Log(">> joystick left!");
        //printf("axis %d", state->joyx);
        *out_buttons |= (1 << 0);
    }
    
    if (state->joyx >= 64) { // right
        //SDL_Log(">> joystick right!");
        *out_buttons |= (1 << 1);
    }

    if (state->joyy <= -64) { // up
        //SDL_Log(">> joystick up!");
        *out_buttons |= (1 << 2);
    }
    
    if (state->joyy >= 64) { // down
        //SDL_Log(">> joystick down!");
        *out_buttons |= (1 << 3);
    }
  }
}
#endif
