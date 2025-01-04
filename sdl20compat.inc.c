#include <assert.h>
#include <SDL2/SDL.h>

//dummy values
enum
{
    SDL_PHYSPAL     = 1,
    SDL_LOGPAL      = 2,
    SDL_SRCCOLORKEY = 4,
    SDL_HWPALETTE   = 8,
    SDL_ANYFORMAT   = 16,
};

static SDL_Surface*  sdl2_screen     = NULL;
static SDL_Texture*  sdl2_screen_tex = NULL;
static SDL_Window*   sdl2_window     = NULL;
static SDL_Renderer* sdl2_rendr      = NULL;

#if defined (__NGAGE__) || defined (NGAGE_DEBUG) || defined(__3DS__) || defined(__PSP__) || defined(__XBOX__) || defined (DREAMCAST)
static SDL_Texture* frame = NULL;
#endif

static SDL_Surface *SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags)
{
    Uint32 format = SDL_PIXELFORMAT_UNKNOWN;
    SDL_RendererInfo info = { 0 };
    unsigned int i;

    if (!sdl2_window)
    {
#if defined (__NGAGE__) || defined (NGAGE_DEBUG)
        sdl2_window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, 0);
#elif defined (__XBOX__)
        sdl2_window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
#elif defined (DREAMCAST)
        sdl2_window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_FULLSCREEN_DESKTOP);
#else
        sdl2_window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_RESIZABLE);
#endif
        if (!sdl2_window)
        {
            goto die;
        }
#if defined (__NGAGE__) || defined (NGAGE_DEBUG) || defined (DREAMCAST)
        printf(">> SDL_RENDERER_SOFTWARE\n");
        SDL_SetHint(SDL_HINT_VIDEO_DOUBLE_BUFFER, "1");
            SDL_SetHint(SDL_HINT_DC_VIDEO_MODE, "SDL_DC_TEXTURED_VIDEO");  
        SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "software");  
        sdl2_rendr = SDL_CreateRenderer(sdl2_window, -1, SDL_RENDERER_SOFTWARE| SDL_RENDERER_PRESENTVSYNC);
#else
        //SDL_Log(">> SDL_RENDERER_SOFTWARE");
        sdl2_rendr = SDL_CreateRenderer(sdl2_window, -1, 0);
        SDL_RenderSetLogicalSize(sdl2_rendr, width, height);
#endif



        if (!sdl2_rendr)
        {
            goto die;
        }

        for (i = 0; i < info.num_texture_formats; i++) {
            int found_bpp = SDL_BYTESPERPIXEL(info.texture_formats[i]);
            if (found_bpp * 8 == bpp) {
                format = info.texture_formats[i];
                break;
            } else if ((flags & SDL_ANYFORMAT) && format == SDL_PIXELFORMAT_UNKNOWN) {
                if (found_bpp == 2 || found_bpp == 4) {
                    format = info.texture_formats[i];
                }
            }
        }
        if (format == SDL_PIXELFORMAT_UNKNOWN)
            format = SDL_PIXELFORMAT_RGBA8888; // SDL_PIXELFORMAT_RGBA8888 -> SDL_PIXELFORMAT_RGBA32;

       //SDL_Log(">> sdl2_screen_tex");
        sdl2_screen_tex = SDL_CreateTexture(sdl2_rendr, format, SDL_TEXTUREACCESS_STREAMING, width, height);

        if (0)
        {
        die:
#if defined (__NGAGE__) || defined (NGAGE_DEBUG) || defined(__3DS__) || defined(__PSP__) || defined(__XBOX__) //|| defined (DREAMCAST)
            if (frame)
            {
                SDL_DestroyTexture(frame);
            }
#endif
            if (sdl2_window)
            {
                SDL_DestroyWindow(sdl2_window);
            }
            if (sdl2_screen_tex)
            {
                SDL_DestroyTexture(sdl2_screen_tex);
            }
            if (sdl2_rendr)
            {
                SDL_DestroyRenderer(sdl2_rendr);
            }
            return NULL;
        }
    }
    #if defined(__3DS__)
    sdl2_screen = SDL_CreateRGBSurfaceWithFormat(0, 128, 128, SDL_BITSPERPIXEL(format), format);
	#elif defined(__PSP__)
    sdl2_screen = SDL_CreateRGBSurfaceWithFormat(0, 256, 256, SDL_BITSPERPIXEL(format), format);
    #else
    sdl2_screen = SDL_CreateRGBSurfaceWithFormat(0, width, height, SDL_BITSPERPIXEL(format), format);
    #endif
    assert(sdl2_screen && sdl2_screen->format->BitsPerPixel == bpp);

#if defined (__NGAGE__) || defined (NGAGE_DEBUG) || defined (__PSP__) || defined (__3DS__) || defined (__XBOX__) //|| defined (DREAMCAST)
    {
#if defined (__NGAGE__)
        SDL_Surface* frame_sf = SDL_LoadBMP("E:\\System\\Apps\\Celeste\\data\\frame_ngage.bmp");
#elif defined (NGAGE_DEBUG)
        SDL_Surface* frame_sf = SDL_LoadBMP("data\\frame_ngage.bmp");
#elif defined (__3DS__)
        SDL_Surface* frame_sf = SDL_LoadBMP("romfs:/data/frame_3ds.bmp");
#elif defined (__PSP__)
        SDL_Surface* frame_sf = SDL_LoadBMP("data/frame_psp.bmp");
#elif defined (__XBOX__)
        SDL_Surface* frame_sf = SDL_LoadBMP("D:\\data\\frame_xbox.bmp");
//#elif defined (DREAMCAST)
//        SDL_Surface* frame_sf = SDL_LoadBMP("/cd/data/frame_xbox.bmp");
#else
        SDL_Surface* frame_sf = SDL_LoadBMP("data/frame_ngage.bmp");
#endif

        if (! frame_sf)
        {
            SDL_Log("Failed to load image: %s", SDL_GetError());
        }
        else
        {
            if (0 != SDL_SetColorKey(frame_sf, SDL_TRUE, SDL_MapRGB(frame_sf->format, 0xff, 0x00, 0xff)))
            {
                SDL_Log("Failed to set color key for frame.bmp: %s", SDL_GetError());
            }
            if (0 != SDL_SetSurfaceRLE(frame_sf, 1))
            {
                SDL_Log("Could not enable RLE for surface frame.bmp: %s", SDL_GetError());
            }

            frame = SDL_CreateTextureFromSurface(sdl2_rendr, frame_sf);
            if (! frame)
            {
                SDL_Log("Could not create texture from surface: %s", SDL_GetError());
            }
        }
        SDL_FreeSurface(frame_sf);
        SDL_RenderCopy(sdl2_rendr, frame, NULL, NULL);
        SDL_RenderPresent(sdl2_rendr);

#if defined (__PSP__) || defined (__XBOX__) //|| defined (DREAMCAST) /* very hacky, draw the frame in both framebuffers */
        SDL_RenderClear(sdl2_rendr);
        SDL_RenderCopy(sdl2_rendr, frame, NULL, NULL);
        SDL_RenderPresent(sdl2_rendr);
#endif
    }
#endif

    return sdl2_screen;
}

static void SDL_SetPalette(SDL_Surface* surf, int flag, SDL_Color* pal, int begin, int count)
{
    (void)surf;
    (void)flag;
    (void)pal;
    (void)begin;
    (void)count;
}
static void SDL_WM_SetCaption(const char* title, const char* icon)
{
    assert(sdl2_window != NULL);
    SDL_SetWindowTitle(sdl2_window, title);
    (void)icon;
}

static int SDL_WM_ToggleFullScreen(SDL_Surface* screen)
{
    static int fullscreen = 0;
    assert(screen == sdl2_screen);
    assert(sdl2_window != NULL);
    fullscreen = !fullscreen;
    return SDL_SetWindowFullscreen(sdl2_window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) == 0;
}

static SDL_Surface* SDL_GetVideoSurface(void)
{
    return sdl2_screen;
}

static void SDL_Flip(SDL_Surface* screen)
{
#if defined(__NGAGE__)
    SDL_Rect source = { 0, 0, 128, 128 };
    SDL_Rect dest   = { 24, 40, 128, 128 };
#elif defined (NGAGE_DEBUG)
    SDL_Rect source = { 0, 0, 384, 384 };
    SDL_Rect dest   = { 72, 120, 384, 384 };
#elif defined (__3DS__)
    SDL_Rect source = { 0, 0, 128, 128 };
    SDL_Rect dest   = { 80, 0, 240, 240 };
#elif defined (__PSP__)
    SDL_Rect source = { 0, 0, 256, 256 };
    SDL_Rect dest   = { 112, 8, 256, 256 };
#elif defined (__XBOX__)
    SDL_Rect source = { 0, 0, 128, 128 };
    SDL_Rect dest   = { (SCREEN_WIDTH - SCREEN_HEIGHT) / 2, 0, SCREEN_HEIGHT, SCREEN_HEIGHT };
#elif defined (DREAMCAST)
    //SDL_Log(">> SDL_Flip");
    SDL_Rect source = { 0, 0, 512, 512 };
    SDL_Rect dest   = { (640 - 480) / 2, 10, 460, 460 };
#endif

/*if (sdl2_screen_tex != NULL) {
    Uint32 format;
    int access, width, height;

    if (SDL_QueryTexture(sdl2_screen_tex, &format, &access, &width, &height) == 0) {
        printf("Texture Info:\n");
        printf("  Format: 0x%x\n", format);
        printf("  Access: %d\n", access);
        printf("  Width: %d\n", width);
        printf("  Height: %d\n", height);
    } else {
        printf("Failed to query texture: %s\n", SDL_GetError());
    }
} else {
    printf("SDL_Texture sdl2_screen_tex is NULL. Cannot query info.\n");
}*/


    //SDL_Log("assert");
    assert(screen == sdl2_screen);
    assert(sdl2_window != NULL);
    //SDL_Log("SDL_UpdateTexture");
    SDL_UpdateTexture(sdl2_screen_tex, NULL, screen->pixels, screen->pitch);
    //SDL_Log("SDL_SetRenderDrawColor");
    SDL_SetRenderDrawColor(sdl2_rendr, 0, 0, 0, 255);
#if defined (__NGAGE__) || defined (NGAGE_DEBUG) || defined (__3DS__) || defined (__PSP__) || defined(__XBOX__) //|| defined(DREAMCAST)
    //SDL_Log("SDL_RenderCopy - source/dest");
    SDL_RenderCopy(sdl2_rendr, sdl2_screen_tex, &source, &dest);
#else
    //SDL_Log("SDL_RenderCopy");
    SDL_RenderCopy(sdl2_rendr, sdl2_screen_tex, NULL, NULL);
#endif
    //SDL_Log("SDL_RenderPresent");
    SDL_RenderPresent(sdl2_rendr);
    
}

#define SDL_GetKeyState SDL_GetKeyboardState
//the above function now returns array indexed by scancodes, so we need to use those constants
#if defined (__NGAGE__)
// Reset
#define SDLK_F9         SDL_SCANCODE_BACKSPACE
// Pause
#define SDLK_ESCAPE     SDL_SCANCODE_SOFTRIGHT
// Quit
#define SDLK_DELETE     SDL_SCANCODE_SOFTLEFT
// Save state
#define SDLK_s          SDL_SCANCODE_1
// Load state
#define SDLK_d          SDL_SCANCODE_2
// Toggle screenshake
#define SDLK_e          SDL_SCANCODE_3
// Jump
#define SDLK_z          SDL_SCANCODE_7
// Dash
#define SDLK_x          SDL_SCANCODE_5
// Directional controls
#define SDLK_LEFT       SDL_SCANCODE_LEFT
#define SDLK_RIGHT      SDL_SCANCODE_RIGHT
#define SDLK_UP         SDL_SCANCODE_UP
#define SDLK_DOWN       SDL_SCANCODE_DOWN
#else
#define SDLK_F9         SDL_SCANCODE_F9
#define SDLK_ESCAPE     SDL_SCANCODE_ESCAPE
#define SDLK_DELETE     SDL_SCANCODE_DELETE
#define SDLK_F11        SDL_SCANCODE_F11
#define SDLK_LSHIFT     SDL_SCANCODE_LSHIFT
#define SDLK_LEFT       SDL_SCANCODE_LEFT
#define SDLK_RIGHT      SDL_SCANCODE_RIGHT
#define SDLK_UP         SDL_SCANCODE_UP
#define SDLK_DOWN       SDL_SCANCODE_DOWN
#define SDLK_5          SDL_SCANCODE_5
#define SDLK_s          SDL_SCANCODE_S
#define SDLK_d          SDL_SCANCODE_D
#define SDLK_c          SDL_SCANCODE_C
#define SDLK_x          SDL_SCANCODE_X
#define SDLK_n          SDL_SCANCODE_N
#define SDLK_a          SDL_SCANCODE_A
#define SDLK_b          SDL_SCANCODE_B
#define SDLK_z          SDL_SCANCODE_Z
#define SDLK_e          SDL_SCANCODE_E
#define SDLK_v          SDL_SCANCODE_V
#endif

#define sym scancode // SDL_Keysym.sym -> SDL_Keysym.scancode
