#ifndef DISPLAY_TW
#define DISPLAY_TW

#include "mandelbrot.h"

Uint32 timeleft();
void DrawPixel(SDL_Surface *screen,
        int x, int y, Uint8 R, Uint8 G, Uint8 B);
int cal_pixel(complex c);
void single_proc(char **pixels);
void static_div(int myid, int num_procs, char **pixels);
void dynamic_div(int myid, int num_procs, char **pixels);

// main loop, updates bodies, draws them and handels events
void run(int myid, int num_procs)
{ 
    SDL_Surface *screen = SDL_GetVideoSurface();
    SDL_Event event;
    bool running = true;
    double start_time;
    char colour;
    int z = 0;
    char **pixels = (char**) malloc(sizeof(char*) * SCREEN_RES_Y);

    for(int i = 0; i < SCREEN_RES_X; i++)
        pixels[i] = (char*) malloc(sizeof(char) * SCREEN_RES_X);

    // todo barrier to wait for proc 0 with sdl

    // calculate each pixel
    if(myid == 0) start_time = MPI_Wtime();

    /*
    if(myid == 0) {
        single_proc(pixels);
        printf("time %f\n", MPI_Wtime() - start_time);
    }
    */
    static_div(myid, num_procs, pixels);
    if(myid == 0)
        printf("time %f\n", MPI_Wtime() - start_time);


    if(myid == 0) 
    {
        while(running) {
            // display the pixels
            for(int y = 0; y < SCREEN_RES_Y; y++) {
                for(int x = 0; x < SCREEN_RES_X && running; x++) {
                    colour = pixels[y][x];
                    DrawPixel(screen, x, y, colour, colour, colour);
                }

                // process incoming events
                while(SDL_PollEvent(&event))
                    if(event.type == SDL_QUIT)
                        running = false;
            }

            SDL_Flip(screen);
        }

        //SDL_Delay(timeleft()); // sleep for framerate
    }
}

// dynamically assign rows to each processor
void dynamic_div(int myid, int num_procs, char **pixels)
{
    complex c;
    double ratioX = 4.0 / SCREEN_RES_X;
    double ratioY = 4.0 / SCREEN_RES_Y;
    int rows_low, rows_high, rows_div;
    int index = 0;
    char *row, *buffer;
    MPI_Status status;
        
    if(myid == 0) // master creates threads to dispatches rows to slaves
        int row = 0;
        int control[num_procs];
        MPI_Request request[num_procs];
        buffer = (char*) malloc(sizeof(char) * SCREEN_RES_X);

        // async recieve from all slaves
        for(int i = 1; i < num_procs; i++) {
            MPI_Irecv(&control[1], 1, MPI_INT, i, CONTROL_TAG, 
                    MPI_COMM_WORLD, &request[i-1]);
        }

        // on any recieve send next row 
        while(row != SCREEN_RES_Y) {
            MPI_Waitany(num_procs-1, &request[1], &index, &status);

            MPI_Send(&row, 1, MPI_INT, status.MPI_SOURCE,
                    ROW_TAG, MPI_COMM_WORLD);
            MPI_Irecv(buffer, 1, MPI_INT, i, CONTROL_TAG, 
                    MPI_COMM_WORLD, &request[i-1]);
        }

        // tell all slaves to exit
        for(int i = 1; i < num_procs i++) {
            MPI_Request_free(request[i]); 
            // todo bound here :(
            MPI_Send(EXIT_SIG, 1, MPI_INT, i, ROW_TAG, MPI_COMM_WORLD);
        }


        
        // wait for rows to complete
    }
}

// assign each process a set number of tasks
void static_div(int myid, int num_procs, char **pixels) 
{
    complex c;
    double ratioX = 4.0 / SCREEN_RES_X;
    double ratioY = 4.0 / SCREEN_RES_Y;
    int rows_low, rows_high, rows_div, pos = 0;
    char *row, *buffer;
    MPI_Status status;
    
    // divide up the rows
    rows_div = SCREEN_RES_Y / num_procs;
    buffer = (char*) malloc(sizeof(char) * (rows_div * SCREEN_RES_X));
    rows_high = rows_div * (myid +1);
    rows_low = rows_high - rows_div;

    //printf("id:%d low:%d high:%d\n", myid, rows_low, rows_high);

    // each proc calculates the rows assigned
    for(int y = rows_low; y < rows_high; y++) {
        row = (char*) malloc(sizeof(char) * SCREEN_RES_X);
        c.I = ((SCREEN_RES_Y / 2) - y) * ratioY;

        for(int x = 0; x < SCREEN_RES_X; x++) {
            c.R = (x - (SCREEN_RES_X / 2)) * ratioX;
            if(myid == 0) // master just adds the rows to the pixels
                pixels[y][x] = cal_pixel(c);
            else
                row[x] = cal_pixel(c);
        }

        if(myid != 0) // pack row into a buffer
            MPI_Pack(row, SCREEN_RES_X, MPI_CHAR, 
                buffer, rows_div * SCREEN_RES_X, &pos, MPI_COMM_WORLD);
    }

    if(myid != 0) { // send the buffer to the master
        //printf("sending from %d\n", myid);
        MPI_Send(buffer, pos, MPI_PACKED, 0, PIXEL_TAG, MPI_COMM_WORLD);
    } else { // recieve buffer and add to pixels
        for(int i = 1; i < num_procs; i++) {
            rows_high = rows_div * (i+1);
            rows_low = rows_high - rows_div;

            //printf("waiting from %d\n", i);
            MPI_Recv(buffer, rows_div * SCREEN_RES_X, MPI_CHAR, i, 
                    PIXEL_TAG, MPI_COMM_WORLD, &status);

            for(int y = rows_low, k = 0; y < rows_high; y++) {
                for(int x = 0; x < SCREEN_RES_X; x++, k++) {
                    pixels[y][x] = buffer[k];
                }
            }
        }
    }
}


void single_proc(char **pixels)
{
    complex c;
    double ratioX = 4.0 / SCREEN_RES_X;
    double ratioY = 4.0 / SCREEN_RES_Y;

    for(int y = 0; y < SCREEN_RES_Y; y++) {
        c.I = ((SCREEN_RES_Y / 2) - y) * ratioY;
        for(int x = 0; x < SCREEN_RES_X; x++) {
            c.R = (x - (SCREEN_RES_X / 2)) * ratioX;
            pixels[y][x] = cal_pixel(c);
        }
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

