#ifndef MAIN_TW
#define MAIN_TW
#include "mandelbrot.h"

int main(int argc, char *argv[]) 
{
    if(SDL_Init(SDL_INIT_VIDEO | SDL_HWSURFACE | SDL_DOUBLEBUF) == 0
            && SDL_SetVideoMode(
                SCREEN_RES_X, SCREEN_RES_Y, 16, SDL_SWSURFACE) != NULL)
        atexit(SDL_Quit);
    else { // on error return
            fprintf(stderr, "Unable to set video: %s\n", SDL_GetError());
            return EXIT_FAILURE;
    }

    run(0, 0);

    return EXIT_SUCCESS;
}

#endif /* MAIN_TW */

