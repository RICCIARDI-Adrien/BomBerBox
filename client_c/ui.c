//--------------------------------------------------------------------
// INCLUDES
//--------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

#include "ui.h"

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>

//--------------------------------------------------------------------
// DEFINES
//--------------------------------------------------------------------
#define DISPLAY_MAX_TILES   256

//--------------------------------------------------------------------
// PRIVATE DEFINITIONS
//--------------------------------------------------------------------
struct _ui_context {
    SDL_Surface * screen;
    SDL_Surface * tiles[DISPLAY_MAX_TILES];
    TTF_Font    * text_font;
    SDL_Surface * text_surface;
};

//--------------------------------------------------------------------
// PRIVATE VARIABLES
//--------------------------------------------------------------------
static struct _ui_context * ctx = NULL;

//--------------------------------------------------------------------
// PRIVATE API
//--------------------------------------------------------------------
int _load_tiles(const char * path)
{
    DIR * dir;
    char absolute_path[256];
    struct dirent * file;

    // sanity check
    if ( ! path || ! ctx ) {
        return -1;
    }

    dir = opendir(path);
    if ( ! dir ) {
        return -1;
    }

    while( file = readdir(dir) )
    {
        if (!strcmp (file->d_name, "."))
            continue;
        if (!strcmp (file->d_name, ".."))    
            continue;
        if ( ! strstr(file->d_name, "png") ) {
            continue;
        }

        int tid = strtol(file->d_name, NULL, 0);
        if ( tid >= DISPLAY_MAX_TILES ) {
            fprintf(stderr, "load tiles: invalide ID\n");
            closedir(dir);
            return -1;
        }

        strncpy(absolute_path, path, 256);
        strncat(absolute_path, file->d_name, 256);
        printf("ADD %s to %d\n", absolute_path, tid);
        ctx->tiles[tid] = IMG_Load(absolute_path);
        if ( ! ctx->tiles[tid] ) {
            fprintf(stderr, "cannot load tile %s\n", absolute_path);
            continue;
        }
    }

    closedir(dir);
    return 0;
}

//--------------------------------------------------------------------
// PUBLIC API
//--------------------------------------------------------------------
int ui_init(void)
{
    int rc;
    SDL_Rect pos;
    
    // already initialized
    if ( ctx ) {
        return 0;
    }

    ctx = (struct _ui_context *) malloc(sizeof(struct _ui_context));
    if ( ! ctx ) {
        fprintf(stderr, "OOM condition\n");
        return -1;
    }

    TTF_Init();
    ctx->text_font = TTF_OpenFont("ressources/SIXTY.TTF", 22);
    if ( ! ctx->text_font ) {
        perror("TTF init");
        free(ctx);
        return -1;
    }
    ctx->text_surface = NULL;
    memset(&ctx->tiles[0], 0, DISPLAY_MAX_TILES * sizeof(SDL_Surface*));

    rc = SDL_Init(SDL_INIT_VIDEO);
    if ( rc < 0 ) {
        fprintf(stderr, "Failed to init SDL: %s\n", SDL_GetError());
        free(ctx);
        return -1;
    }

    ctx->screen = SDL_SetVideoMode(689, 650, 32, SDL_HWSURFACE);
    if ( ! ctx->screen ) {
        fprintf(stderr, "Failed to load video mode: %s\n", SDL_GetError());
        free(ctx);
        return -1;
    }

    rc = _load_tiles("ressources/");
    if ( rc ) {
        fprintf(stderr, "cannot load tiles\n");
        free(ctx);
    }

    pos.x=0;
    pos.y=0;
    SDL_BlitSurface(ctx->tiles[255], NULL, ctx->screen, &pos);
    SDL_Flip(ctx->screen);
    
    return rc;
}

//--------------------------------------------------------------------
int ui_text(char * str)
{
    SDL_Rect text_location;
    SDL_Color text_foreground = { 255, 255, 255 };
    SDL_Color text_background = { 0, 0, 255 };

    // sanity check
    if ( ! str ) {
        fprintf(stderr, "invalid arguments\n");
        return -1;
    }

    if ( ctx->text_surface ) {
        fprintf(stderr, "clean previous text\n");
        SDL_FreeSurface(ctx->text_surface);
    }

    text_location.x = 50;
    text_location.y = 600;
    ctx->text_surface = TTF_RenderText_Shaded(ctx->text_font, str,
                            text_foreground, text_background);

    SDL_BlitSurface(ctx->text_surface, NULL, ctx->screen, &text_location);
    SDL_Flip(ctx->screen);

    return 0;
}

//--------------------------------------------------------------------
int ui_tile(int tid, int x, int y)
{
    SDL_Rect pos;

    if ( ! ctx || ! ctx->screen ) {
        return -1;
    }

    if ( ! ctx->tiles[tid] ) {
        fprintf(stderr, "tile not found %d\n", tid);
        return -1;
    }

    pos.x = x;
    pos.y = y;

    SDL_BlitSurface(ctx->tiles[tid], NULL, ctx->screen, &pos);
    SDL_Flip(ctx->screen);
}

//--------------------------------------------------------------------
int ui_event(ui_event_t * event)
{
    int rc;
    SDL_Event sdl_event;

    // sanity check
    if ( ! event ) {
        return -1;
    }

    rc = SDL_PollEvent(&sdl_event);
    if ( ! rc ) {
        return 0;
    }

    if ( sdl_event.type == SDL_KEYDOWN ) {
        SDLKey keyPressed = sdl_event.key.keysym.sym;
        event->type = UI_EVENT_KEYBOARD;

        switch (keyPressed) {
            case SDLK_UP:
                event->value = UI_KEYBOARD_UP;
                break;
            case SDLK_DOWN:
                event->value = UI_KEYBOARD_DOWN;
                break;
            case SDLK_LEFT:
                event->value = UI_KEYBOARD_LEFT;
                break;
            case SDLK_RIGHT:
                event->value = UI_KEYBOARD_RIGHT;
                break;
            case SDLK_SPACE:
                event->value = UI_KEYBOARD_SPACE;
                break;
            case SDLK_ESCAPE:
                event->value = UI_KEYBOARD_ESCAPE;
                break;
        }
        
        return 1;
    }

    return 0;
}
