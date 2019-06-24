#include "SDL.h"
#include <math.h>
#include <stdio.h>

#define SCREEN_HEIGHT 700
#define SCREEN_WIDTH 1100
#define VIEW_HEIGHT 600
#define VIEW_WIDTH 300

#define FOV_H (0.5 * VIEW_WIDTH)
#define FOV_V (10 * VIEW_HEIGHT)

typedef struct {
    double x;
    double y;
} vert;

typedef struct {
    vert v1;
    vert v2;
} wall;

typedef struct {
    vert loc;
    double angle;
    double fvel;
    double avel;
} player;

// The world
player p = {150, 250, 0, 0, 0};
wall w = {{50, 450}, {250, 450}};

SDL_bool done = SDL_FALSE;
unsigned int time_last = 0;
unsigned int time_current = 0;
unsigned int time_delta = 0;

vert vert_to_screen_coords(vert v)
{
    // Rotate around the player
    vert v_trans = {
		(v.x - p.loc.x) * cos(p.angle) - (v.y - p.loc.y) * sin(p.angle),
		(v.x - p.loc.x) * sin(p.angle) + (v.y - p.loc.y) * cos(p.angle)
	};

	// Translate the vertex to screen coordinates
	v_trans.x += VIEW_WIDTH / 2;
	v_trans.y += VIEW_HEIGHT / 2;

    return v_trans;
}

wall wall_to_screen_coords(wall w)
{
    wall w_trans = {vert_to_screen_coords(w.v1), vert_to_screen_coords(w.v2)};
    return w_trans;
}

vert vert_to_player_coords(vert v)
{
    // Rotate around the player
    vert v_trans = {
		(v.x - p.loc.x) * cos(p.angle) - (v.y - p.loc.y) * sin(p.angle),
		(v.x - p.loc.x) * sin(p.angle) + (v.y - p.loc.y) * cos(p.angle)
	};

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
    p.loc.y += time_delta * p.fvel * cos(p.angle);
    p.angle -= time_delta * p.avel;

	if (p.angle < 0) p.angle += 2 * M_PI;
	if (p.angle > 2 * M_PI) p.angle -= 2 * M_PI;
}

void render_absolute(SDL_Renderer* renderer)
{
	// Render viewport border
	SDL_Rect border = {0, 0, VIEW_WIDTH, VIEW_HEIGHT};
	SDL_SetRenderDrawColor(renderer, 255, 255, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawRect(renderer, &border);

    // Render wall
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(renderer, w.v1.x, w.v1.y, w.v2.x, w.v2.y);

    // Render player
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(renderer, p.loc.x, p.loc.y, p.loc.x + 20 * sin(p.angle), p.loc.y + 20 * cos(p.angle));
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawPoint(renderer, p.loc.x, p.loc.y);
}

void render_relative(SDL_Renderer* renderer)
{
	// Render viewport border
	SDL_Rect border = {0, 0, VIEW_WIDTH, VIEW_HEIGHT};
	SDL_SetRenderDrawColor(renderer, 255, 0, 255, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawRect(renderer, &border);

    // Render wall
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    wall wt = wall_to_screen_coords(w);
    SDL_RenderDrawLine(renderer, wt.v1.x, wt.v1.y, wt.v2.x, wt.v2.y);

    // Render player
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(renderer, VIEW_WIDTH/2, VIEW_HEIGHT/2, VIEW_WIDTH/2, VIEW_HEIGHT/2 + 20);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawPoint(renderer, VIEW_WIDTH/2, VIEW_HEIGHT/2);
}

void render_perspective(SDL_Renderer* renderer)
{
	// Render viewport border
	SDL_Rect border = {0, 0, VIEW_WIDTH, VIEW_HEIGHT};
	SDL_SetRenderDrawColor(renderer, 0, 255, 255, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawRect(renderer, &border);

    // Render wall
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    wall wt = wall_to_player_coords(w);

    // If the wall is completely behind the player, don't draw it
	if(wt.v1.y <= 0 && wt.v2.y <= 0) return;

    SDL_RenderDrawLine(
		renderer,
		wt.v1.x * FOV_H / wt.v1.y + VIEW_WIDTH/2,
		-FOV_V / wt.v1.y + VIEW_HEIGHT/2,
		wt.v2.x * FOV_H / wt.v2.y + VIEW_WIDTH/2,
		-FOV_V / wt.v2.y + VIEW_HEIGHT/2
    );

    SDL_RenderDrawLine(
		renderer,
		wt.v1.x * FOV_H / wt.v1.y + VIEW_WIDTH/2,
		FOV_V / wt.v1.y + VIEW_HEIGHT/2,
		wt.v2.x * FOV_H / wt.v2.y + VIEW_WIDTH/2,
		FOV_V / wt.v2.y + VIEW_HEIGHT/2
    );
}

void handle_events()
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
                        p.avel = -0.002;
                        break;
                    case SDLK_RIGHT:
                        p.avel = 0.002;
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

        SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);

		SDL_Rect absolute_viewport = {50, 50, VIEW_WIDTH, VIEW_HEIGHT};
		SDL_Rect relative_viewport = {400, 50, VIEW_WIDTH, VIEW_HEIGHT};
		SDL_Rect perspective_viewport = {750, 50, VIEW_WIDTH, VIEW_HEIGHT};

		while (!done) {
			handle_events();
			update();

			// Clear screen
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
			SDL_RenderClear(renderer);

			// Render viewports
			SDL_RenderSetViewport(renderer, &absolute_viewport);
			render_absolute(renderer);
			SDL_RenderSetViewport(renderer, &relative_viewport);
			render_relative(renderer);
			SDL_RenderSetViewport(renderer, &perspective_viewport);
			render_perspective(renderer);

		    SDL_RenderPresent(renderer);
		}

        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
    }
    SDL_Quit();
    return 0;
}