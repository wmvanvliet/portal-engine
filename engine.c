#include "SDL.h"
#include <math.h>
#include <stdio.h>

#define SCREEN_HEIGHT 700
#define SCREEN_WIDTH 1100
#define VIEW_HEIGHT 600
#define VIEW_WIDTH 300
#define FOV_H (VIEW_WIDTH / 2)
#define FOV_V (VIEW_HEIGHT / 4)
#define PLAYER_EYE_LEVEL 1.6

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define CLAMP(x, a, b) (MIN(MAX(x, a), b))
#define SIGN(x) ((x) < 0 ? (-1) : (1))

float fov_h = VIEW_WIDTH / 2;
float fov_v = VIEW_HEIGHT / 4;

#define METERS_TO_PIXELS 10 // One meter is 10 pixels

typedef struct {
    double x;
    double y;
} point;

typedef struct {
    point v1;
    point v2;
	double h;
	SDL_Color c;
} wall;

typedef struct {
    point loc;
    double angle;
    double fvel;
    double avel;
} player;

// The world
player p = {{0, 0}, 0, 0, 0};
wall world[] = {
	{{-10, 10}, {10, 10}, 4, {255, 0, 0, 255}},
	{{-10, -10}, {10, -10}, 4, {0, 255, 0, 255}},
	{{-10, 10}, {-10, -10}, 4, {0, 0, 255, 255}},
	{{10, -10}, {10, 10}, 4, {255, 255, 0, 255}},
};
unsigned int n_walls = 4;

SDL_bool done = SDL_FALSE;
unsigned int time_last = 0;
unsigned int time_current = 0;
unsigned int time_delta = 0;

point point_to_screen_coords(point v)
{
    // Rotate around the player
    point v_trans = {v.x * METERS_TO_PIXELS + VIEW_WIDTH/2,
		             v.y * METERS_TO_PIXELS + VIEW_HEIGHT/2};
    return v_trans;
}

wall wall_to_screen_coords(wall w)
{
    wall w_trans = {point_to_screen_coords(w.v1), point_to_screen_coords(w.v2), w.h, w.c};
    return w_trans;
}

point point_to_player_coords(point v)
{
    // Rotate around the player
    point v_trans = {
		(v.x - p.loc.x) * cos(p.angle) - (v.y - p.loc.y) * sin(p.angle),
		(v.x - p.loc.x) * sin(p.angle) + (v.y - p.loc.y) * cos(p.angle)
	};

    return v_trans;
}

wall wall_to_player_coords(wall w)
{
    wall w_trans = {point_to_player_coords(w.v1), point_to_player_coords(w.v2), w.h, w.c};
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

    // Render walls
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
	for(int i=0; i<n_walls; i++) {
		wall w = wall_to_screen_coords(world[i]);
		SDL_RenderDrawLine(renderer, w.v1.x, w.v1.y, w.v2.x, w.v2.y);
	}

    // Render player
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, SDL_ALPHA_OPAQUE);
	point loc = point_to_screen_coords(p.loc);
    SDL_RenderDrawLine(renderer, loc.x, loc.y, loc.x + 20 * sin(p.angle), loc.y + 20 * cos(p.angle));
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawPoint(renderer, loc.x, loc.y);
}

void render_relative(SDL_Renderer* renderer)
{
	// Render viewport border
	SDL_Rect border = {0, 0, VIEW_WIDTH, VIEW_HEIGHT};
	SDL_SetRenderDrawColor(renderer, 255, 0, 255, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawRect(renderer, &border);

    // Render walls
	for(int i=0; i<n_walls; i++) {
		wall w = world[i];
		wall wt = wall_to_screen_coords(wall_to_player_coords(w));
		
		// Determine wall color based on whether the player can see the wall
		if (wt.v1.y <= VIEW_HEIGHT/2 && wt.v2.y <= VIEW_HEIGHT/2)
			SDL_SetRenderDrawColor(renderer, 255, 255, 0, SDL_ALPHA_OPAQUE);
		else
   	    	SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);

		// Draw the wall
		SDL_RenderDrawLine(renderer, wt.v1.x, wt.v1.y, wt.v2.x, wt.v2.y);

		// If only one vertex of the wall is behind the player, find the
		// intersection with y=0 and mark it with a single red pixel.
		if (wt.v1.y <= VIEW_HEIGHT/2 || wt.v2.y <= VIEW_HEIGHT/2) {
			point intersect = {
				wt.v1.x - (wt.v2.x - wt.v1.x) / (wt.v2.y - wt.v1.y) * (wt.v1.y - VIEW_HEIGHT/2), // x-coord
				VIEW_HEIGHT/2 // y-coord
			};
			SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);
			SDL_RenderDrawPoint(renderer, wt.v1.x, wt.v1.y);
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
			SDL_RenderDrawPoint(renderer, intersect.x, intersect.y);
		}
	}

    // Render player
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(renderer, VIEW_WIDTH/2, VIEW_HEIGHT/2, VIEW_WIDTH/2, VIEW_HEIGHT/2 + 20);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawPoint(renderer, VIEW_WIDTH/2, VIEW_HEIGHT/2);
}

void draw_wall(SDL_Renderer* renderer, wall w)
{
    SDL_SetRenderDrawColor(renderer, w.c.r, w.c.g, w.c.b, w.c.a);
	int x1 = w.v1.x * fov_h / w.v1.y + VIEW_WIDTH/2;
	int x2 = w.v2.x * fov_h / w.v2.y + VIEW_WIDTH/2;
	double y1 = w.h * fov_v / w.v1.y;
	double y2 = w.h * fov_v / w.v2.y;

	for (int x = CLAMP(MIN(x1, x2), 0, VIEW_WIDTH); x < CLAMP(MAX(x1, x2), 0, VIEW_WIDTH); ++x) {
		double y = y1 + (y2 - y1) * (x - x1) / (x2 - x1);
		SDL_RenderDrawLine(renderer, x, -y + VIEW_HEIGHT/2, x, y + VIEW_HEIGHT/2);
	}
}

void render_perspective(SDL_Renderer* renderer)
{
	// Render viewport border
	SDL_Rect border = {0, 0, VIEW_WIDTH, VIEW_HEIGHT};
	SDL_SetRenderDrawColor(renderer, 0, 255, 255, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawRect(renderer, &border);

    // Render walls
	for (int i=0; i<n_walls; i++) {
		wall w = world[i];
		wall wt = wall_to_player_coords(w);

		// If the wall is completely behind the player, don't draw it
		if (wt.v1.y <= 0 && wt.v2.y <= 0) continue;

		// If only one vertex of the wall is behind the player, find the
		// intersection with y=0 and clip the wall.
		if (wt.v1.y <= 0 || wt.v2.y <= 0) {
			point intersect = {
				wt.v1.x - (wt.v2.x - wt.v1.x) / (wt.v2.y - wt.v1.y) * wt.v1.y, // x-coord
				0.0001 // y-coord
			};

			if (wt.v2.y < wt.v1.y) wt.v2 = intersect;
			else wt.v1 = intersect;
		}

		draw_wall(renderer, wt);
	}
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
                        p.fvel = 0.01;
                        break;
                    case SDLK_DOWN:
                        p.fvel = -0.01;
                        break;
                    case SDLK_a:
						fov_h -= 10;
						printf("fov_h=%f\n", fov_h);
						break;
                    case SDLK_d:
						fov_h += 10;
						printf("fov_h=%f\n", fov_h);
						break;
                    case SDLK_w:
						fov_v += 10;
						printf("fov_v=%f\n", fov_v);
						break;
                    case SDLK_s:
						fov_v -= 10;
						printf("fov_v=%f\n", fov_v);
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
