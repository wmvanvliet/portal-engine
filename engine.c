#include "SDL.h"
#include <math.h>
#include <stdio.h>

typedef struct {
    float x;
    float y;
} vert;

typedef struct {
    vert v1;
    vert v2;
} wall;

typedef struct {
    vert loc;
    float angle;
    float fvel;
    float avel;
} player;

player p = {100, 100, 0, 0, 0};
SDL_bool done = SDL_FALSE;
unsigned int time_last = 0;
unsigned int time_current = 0;
unsigned int time_delta = 0;

vert vert_to_player_coords(vert v)
{
    vert v_trans = v;

    // Make player the center of the world
    /*
    v_trans.x -= (int) p.loc.x;
    v_trans.y += (int) p.loc.y;
    */

    // Apply player rotation in reverse to the vertex
    v_trans.x = v_trans.x * cos(p.angle) - v_trans.y * sin(p.angle);
    v_trans.y = v_trans.x * sin(p.angle) + v_trans.y * cos(p.angle);

    return v_trans;
}

wall wall_to_player_coords(wall w)
{
    wall w_trans = {vert_to_player_coords(w.v1), vert_to_player_coords(w.v2)};
    return w_trans;
}

void update()
{
    // Update time
    time_current = SDL_GetTicks();
    time_delta = time_current - time_last;
    //if (time_delta < 10) break; // limit time
    time_last = time_current;

    // Update player
    p.loc.x += time_delta * p.fvel * sin(p.angle);
    p.loc.y -= time_delta * p.fvel * cos(p.angle);
    p.angle += time_delta * p.avel;
}

void render(SDL_Renderer* renderer)
{
    wall w = {{50, 50}, {500, 50}};

    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    // Render wall
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    w = wall_to_player_coords(w);
    SDL_RenderDrawLine(renderer, w.v1.x, w.v1.y, w.v2.x, w.v2.y);

    // Render player
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(renderer, 320, 200, 320, 190);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawPoint(renderer, 320, 200);

    SDL_RenderPresent(renderer);
}

void handle_events(SDL_Renderer* renderer)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type)
        {
            case SDL_QUIT:
                done = SDL_TRUE;
                break; 
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_q:
                        done = SDL_TRUE;
                        break;
                    case SDLK_LEFT:
                        p.avel = -0.005;
                        break;
                    case SDLK_RIGHT:
                        p.avel = 0.005;
                        break;
                    case SDLK_UP:
                        p.fvel = 0.1;
                        break;
                    case SDLK_DOWN:
                        p.fvel = -0.1;
                        break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym)
                {
                    case SDLK_LEFT:
                        if (p.avel < 0) p.avel = 0;
                        break;
                    case SDLK_RIGHT:
                        if (p.avel > 0) p.avel = 0;
                        break;
                    case SDLK_UP:
                        if (p.fvel > 0) p.fvel = 0;
                        break;
                    case SDLK_DOWN:
                        if (p.fvel < 0) p.fvel = 0;
                        break;
                }
                break;
        }
    }
}

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) == 0) {
        SDL_Window* window = NULL;
        SDL_Renderer* renderer = NULL;

        if (SDL_CreateWindowAndRenderer(640, 480, 0, &window, &renderer) == 0) {
            while (!done) {
                handle_events(renderer);
                update();
				render(renderer);
            }
        }

        if (renderer) {
            SDL_DestroyRenderer(renderer);
        }
        if (window) {
            SDL_DestroyWindow(window);
        }
    }
    SDL_Quit();
    return 0;
}
