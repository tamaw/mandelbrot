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
    if(myid == 0)
        start_time = MPI_Wtime();

    /*
    if(myid == 0) {
        single_proc(pixels);
        printf("time %f\n", MPI_Wtime() - start_time);
    }
    */
    //static_div(myid, num_procs, pixels);
    
    dynamic_div(myid, num_procs, pixels);
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
    const double ratioX = 4.0 / SCREEN_RES_X;
    const double ratioY = 4.0 / SCREEN_RES_Y;
    int end_row = -1;
    complex c;
    int index = 0;
    char *row;
    MPI_Status status;
    int current_row = 0;

    row = (char*) malloc(sizeof(char) * SCREEN_RES_X);

    if(myid != 0) 
    {
        MPI_Recv(&current_row, 1, MPI_INT, 0, ASSIGN_ROW_TAG,
                MPI_COMM_WORLD, &status);


        while(current_row >= 0) { // negitive row exits

            //printf("%d working on %d\n", myid, current_row);

            c.I = ((SCREEN_RES_Y / 2) - current_row) * ratioY;
            for(int x = 0; x < SCREEN_RES_X; x++) {
                c.R = (x - (SCREEN_RES_X / 2)) * ratioX;
                row[x] = cal_pixel(c);
            }

            // when finished send  row
            MPI_Send(&row, SCREEN_RES_X, MPI_INT, 0, ROW_TAG,
                    MPI_COMM_WORLD);

            printf("%d row %d sent\n", myid, current_row);
            MPI_Finalize(); exit(1);

            // get new row 
            MPI_Recv(&current_row, 1, MPI_INT, 0, ASSIGN_ROW_TAG,
                    MPI_COMM_WORLD, &status);
            //printf("%d got new row %d \n", myid, current_row);
        }

        printf("proc %d done\n", myid);

    }
        
    if(myid == 0) { // master creates threads to dispatches rows to slaves
        char *buffer[num_procs];
        MPI_Request requests[num_procs];
        // todo store what proc has what row here

        // give slaves there initial row
        for(int i = 1; i < num_procs; i++) {
            buffer[i] = (char*) malloc(sizeof(char) * SCREEN_RES_X);
            // set up to recieve from any slave
            MPI_Irecv(buffer[i], SCREEN_RES_X, MPI_INT,
                    MPI_ANY_SOURCE, ROW_TAG, MPI_COMM_WORLD, &requests[i]);

            MPI_Send(&current_row, 1, MPI_INT, i, ASSIGN_ROW_TAG,
                    MPI_COMM_WORLD);
            current_row += 1;
        }

        // receive the row and send a new row assignment
        int procs_done = 1; // proc 0 is master, not counted 
        while(procs_done < (num_procs)) {

            // wait for any response

            MPI_Waitany(num_procs-procs_done, &requests[procs_done], &index,
                   &status);

            //printf("got response from %d\n", status.MPI_SOURCE);
            printf("compelte %d of %d\n", procs_done, num_procs-1);
            procs_done++;
            continue; // todo remove me

            // todo add row to pixels maybe after sending new row

            // send new row
            current_row += 1;
            if(current_row < SCREEN_RES_Y) {
                printf("sending row %d to %d\n", current_row,
                        status.MPI_SOURCE);

                MPI_Irecv(buffer[status.MPI_SOURCE], SCREEN_RES_X, MPI_INT,
                        status.MPI_SOURCE, ROW_TAG, MPI_COMM_WORLD,
                        &requests[status.MPI_SOURCE]);

                MPI_Send(&current_row, 1, MPI_INT, status.MPI_SOURCE,
                        ASSIGN_ROW_TAG, MPI_COMM_WORLD);

            } else { // send -1 when finished
                printf("sending end row to %d\n", status.MPI_SOURCE);
                procs_done += 1;
                MPI_Send(&end_row, 1, MPI_INT, status.MPI_SOURCE,
                        ASSIGN_ROW_TAG, MPI_COMM_WORLD);
            }

        }
        printf("!!master done\n");
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

