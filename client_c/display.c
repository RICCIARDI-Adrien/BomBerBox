#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>

#define DISPLAY_MAX_TILES   256


typedef struct _display_tile display_tile_t;

struct _display_context {
    SDL_Surface * screen;
    SDL_Surface * tiles[DISPLAY_MAX_TILES];
    TTF_Font    * text_font;
    SDL_Surface * text_surface;
};

typedef struct _display_context display_context_t;

//--------------------------------------------------------------------
// PRIVATE VARIABLES
//--------------------------------------------------------------------
static display_context_t * ctx = NULL;

//--------------------------------------------------------------------
// PROTOTYPE
//--------------------------------------------------------------------
int _load_tiles(const char * path);

//--------------------------------------------------------------------
// PUBLIC API
//--------------------------------------------------------------------
int display_init(void)
{
    int rc;
    SDL_Rect pos;
    
    // already initialized
    if ( ctx ) {
        return 0;
    }

    ctx = (display_context_t *) malloc(sizeof(display_context_t));
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
int display_text(char * str)
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
int display_tile(int tid, int x, int y)
{
    SDL_Rect pos;

    if ( ! ctx || ! ctx->screen ) {
        return -1;
    }

    if ( ! ctx->tiles[tid] ) {
        fprintf(stderr, "tile not found\n");
        return -1;
    }

    pos.x = x;
    pos.y = y;

    SDL_BlitSurface(ctx->tiles[tid], NULL, ctx->screen, &pos);
    SDL_Flip(ctx->screen);
}


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
