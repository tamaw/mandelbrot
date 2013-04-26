#ifndef DISPLAY_TW
#define DISPLAY_TW

#include "mandelbrot.h"

Uint32 timeleft();
void DrawPixel(SDL_Surface *screen,
        int x, int y, Uint8 R, Uint8 G, Uint8 B);
int cal_pixel(complex c);

// main loop, updates bodies, draws them and handels events
void run(int myid, int num_procs)
{ 
    SDL_Surface *screen = SDL_GetVideoSurface();
    SDL_Event event;
    bool running = true;
    int z = 0;
    complex c;
    double ratioX = 4.0 / SCREEN_RES_X;
    double ratioY = 4.0 / SCREEN_RES_Y;

    while(running) {

        for(int x=0; x < SCREEN_RES_X; x++) {
            for(int y = 0; y < SCREEN_RES_Y && running; y++) {

                c.R = (x - (SCREEN_RES_X / 2)) * ratioX;
                c.I = ((SCREEN_RES_Y / 2) - y) * ratioY;

                int colour = cal_pixel(c);
                printf("(%.2f %.2f) ", c.R, c.I);
                fflush(stdout);
                DrawPixel(screen, x, y, colour, 0, 0);
            }

            if(myid == 0) {
                // process incoming events
                while(SDL_PollEvent(&event))
                    if(event.type == SDL_QUIT)
                        running = false;
            }
        }

        SDL_Flip(screen);

        //SDL_Delay(timeleft()); // sleep for framerate
        // sync is running
        //MPI_Bcast(&running, 1, MPI_C_BOOL, 0, MPI_COMM_WORLD);
    }
}

int cal_pixel(complex c) {
    int count = 0;
    complex z;
    float tmp, lengthsq;

    z.R = 0; z.I = 0;
    do {
        tmp = z.R * z.R - z.I * z.I + c.R;
        z.I = 2 * z.R * z.I + c.I;
        z.R = tmp;
        lengthsq = z.R * z.R + z.I * z.I;
        count++;
    } while(lengthsq < 4.0 && count < 256);

    return count;
}

Uint32 timeleft() 
{
    static Uint32 next_time = 0;
    Uint32 now = SDL_GetTicks();

    if(next_time <= now) {
        next_time = now + TICK_INTERVAL;
        return 0;
    }
    return next_time - now;
}

// draws a coloured pixel and the x,y coordinates 
void DrawPixel(SDL_Surface *screen,
        int x, int y, Uint8 R, Uint8 G, Uint8 B)
{
    Uint32 colour = SDL_MapRGB(screen->format, R, G, B);
    Uint8 *bufp = (Uint8*) screen->pixels + y * screen->pitch
        + x * screen->format->BytesPerPixel;

    switch (screen->format->BytesPerPixel)
    {
        case 4: // 32-bpp
            bufp[3] = colour >> 24;
        case 3: // 24-bpp 
            bufp[2] = colour >> 16;
        case 2: // 15 or 16-bpp
            bufp[1] = colour >> 8;
        case 1: // 8-bpp
            bufp[0] = colour;
    }

    //SDL_UpdateRect(screen, x, y, 1, 1);
}

#endif /* DISPLAY_TW */

