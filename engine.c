#include "SDL.h"
#include <math.h>
#include <stdio.h>

#include "map.h"

#define SCREEN_HEIGHT 700
#define SCREEN_WIDTH 1100
#define MAP_HEIGHT 600
#define MAP_WIDTH 300
#define VIEW_HEIGHT 600
#define VIEW_WIDTH 675
#define FOV_H (VIEW_WIDTH / 2)
#define FOV_V (VIEW_HEIGHT / 4)
#define PLAYER_EYE_LEVEL -10000
#define UNITS_TO_PIXELS 0.05

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define CLAMP(x, a, b) (MIN(MAX(x, a), b))
#define SIGN(x) ((x) < 0 ? (-1) : (1))
#define VXS(x0, y0, x1, y1) ((x0)*(y1) - (x1)*(y0)) // Vector cross product
// Determine whether the two number ranges overlap.
#define OVERLAP(a0, a1, b0, b1) (MIN(a0, a1) <= MAX(b0, b1) && MIN(b0, b1) <= MAX(a0,a1))
// Determine whether two 2D-boxes intersect.
#define BOXES_INTERSECT(x0, y0, x1, y1, x2, y2, x3, y3) (OVERLAP(x0, x1, x2, x3) && OVERLAP(y0, y1, y2, y3))
// Determine which side of a line the point is on. Return value: <0, =0 or >0.
#define POINT_SIDE(px, py, x0, y0, x1, y1) VXS((x1)-(x0), (y1)-(y0), (px)-(x0), (py)-(y0))

int render_order = 0;
int max_depth = 100;

// Field of view
float fov_h = 280;
float fov_v = 20;

typedef struct {
    double x;
    double y;
    double z;
    double angle;
    double fvel;
    double avel;
	int sector;
} player;

player p = {0, 0, 0, 0, 0, 0};

SDL_bool done = SDL_FALSE;
unsigned int time_last = 0;
unsigned int time_current = 0;
unsigned int time_delta = 0;

world* w;

int path_crosses_wall(double wall_x1, double wall_y1, double wall_x2, double wall_y2,
                      double path_x1, double path_y1, double path_x2, double path_y2)
{
	return (BOXES_INTERSECT(path_x1, path_y1, path_x2, path_y2, wall_x1, wall_y1, wall_x2, wall_y2) &&
            POINT_SIDE(path_x2, path_y2, wall_x1, wall_y1, wall_x2, wall_y2 < 0));
}

void update()
{
    // Update time
    time_current = SDL_GetTicks();
    time_delta = time_current - time_last;
    //if (time_delta < 10) break; // limit time
    time_last = time_current;

	// Player rotates to new angle
    p.angle -= time_delta * p.avel;
	if (p.angle < 0) p.angle += 2 * M_PI;
	if (p.angle > 2 * M_PI) p.angle -= 2 * M_PI;

	// Player is not moving
	if (p.fvel == 0) return;

    // Player wants to walk to this new location
	double dx = time_delta * p.fvel * sin(p.angle);
	double dy = time_delta * p.fvel * cos(p.angle);
    double new_x = p.x + dx;
    double new_y = p.y + dy;

	// Does our path cross a wall? Loop across all the walls in the current
	// sector to check.
	sector* current_sector = &w->sectors[p.sector];
	wall* w1 = &w->walls[current_sector->first_wall];
	for(int i=0; i<current_sector->n_walls; ++i) {
		wall* w2 = &w->walls[w1->point2];
		if (path_crosses_wall(w1->x, w1->y, w2->x, w2->y, p.x, p.y, new_x, new_y)) {
			if (w1->next_sector >= 0) {
				printf("Entering sector %d\n", w1->next_sector);
				// Crossed portal, move player into new sector
				p.sector = w1->next_sector;
				// The new sector may have a different floor height. Land the
				// player on the floor of the new sector.
				p.z = w->sectors[w1->next_sector].floor_z + PLAYER_EYE_LEVEL;
			} else {
				// Player bumps into wall. Move back using vector projection
				double xd = w2->x - w1->x;
				double yd = w2->y - w1->y;
				new_x = p.x + xd * (dx*xd + yd*dy) / (xd*xd + yd*yd);
				new_y = p.y + yd * (dx*xd + yd*dy) / (xd*xd + yd*yd);
			}
		}

		w1 = w2;
	}
	
	// Go ahead and update the players position now
	p.x = new_x;
	p.y = new_y;

}

void render_map(SDL_Renderer* renderer)
{
	// Render viewport border
	SDL_Rect border = {0, 0, MAP_WIDTH, MAP_HEIGHT};
	SDL_SetRenderDrawColor(renderer, 255, 255, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawRect(renderer, &border);

    // Render sectors
	for(int i=0; i<w->n_sectors; ++i) {
		sector* s = &w->sectors[i];

		// Render walls
		wall* w1 = &w->walls[s->first_wall];
		for(int j=0; j<s->n_walls; ++j) {
			wall* w2 = &w->walls[w1->point2];

			// Rotate around the player
			double x1 = (w1->x - p.x) * cos(p.angle) - (w1->y - p.y) * sin(p.angle);
			double y1 = (w1->x - p.x) * sin(p.angle) + (w1->y - p.y) * cos(p.angle);
			double x2 = (w2->x - p.x) * cos(p.angle) - (w2->y - p.y) * sin(p.angle);
			double y2 = (w2->x - p.x) * sin(p.angle) + (w2->y - p.y) * cos(p.angle);

			// Scale and translate the map to fit the viewport
			x1 = x1 * UNITS_TO_PIXELS + MAP_WIDTH/2;
			y1 = y1 * UNITS_TO_PIXELS + MAP_HEIGHT/2;
			x2 = x2 * UNITS_TO_PIXELS + MAP_WIDTH/2;
			y2 = y2 * UNITS_TO_PIXELS + MAP_HEIGHT/2;

			// Determine color of the wall
			if (w1->next_sector >= 0)
				// Wall is a portal
				SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
			else
				// Wall is solid
				SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
			SDL_RenderDrawLine(renderer, x1, y1, x2, y2);

			w1 = w2;
		}
	}

    // Render player
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(renderer, MAP_WIDTH/2, MAP_HEIGHT/2, MAP_WIDTH/2, MAP_HEIGHT/2 + 20);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawPoint(renderer, MAP_WIDTH/2, MAP_HEIGHT/2);
}


// These are global, so we only have to allocate the memory once
int* screen_y_min;
int* screen_y_max;

void render_sector(SDL_Renderer* renderer, int sector_num, int sector_x_min, int sector_x_max, int calling_sector_num, int depth)
{
	if(depth > max_depth) return;
	sector* s = &w->sectors[sector_num];

	// Render walls in the sector
	wall* w1 = &w->walls[s->first_wall];
	for(int j=0; j<s->n_walls; ++j) {
		wall* w2 = &w->walls[w1->point2];

		// Rotate around the player. Y-coordinate of the wall as drawn in
		// the 2D map, becomes the Z-coordinate in our perspective view.
		double x1 = (w1->x - p.x) * cos(p.angle) - (w1->y - p.y) * sin(p.angle);
		double z1 = (w1->x - p.x) * sin(p.angle) + (w1->y - p.y) * cos(p.angle);
		double x2 = (w2->x - p.x) * cos(p.angle) - (w2->y - p.y) * sin(p.angle);
		double z2 = (w2->x - p.x) * sin(p.angle) + (w2->y - p.y) * cos(p.angle);

		// If the wall is completely behind the player, don't draw it
		if (z1 <= 0 && z2 <= 0) {
			w1 = w2;
			continue;
		}

		// If only one vertex of the wall is behind the player, find the
		// intersection with z=0 and clip the wall.
		if (z1 <= 0 || z2 <= 0) {
			double x_i = x1 - (x2 - x1) / (z2 - z1) * z1;
			double z_i = 0.0001;

			if (z2 < z1) {
				x2 = x_i;
				z2 = z_i;
			} else {
				x1 = x_i;
				z1 = z_i;
			}
		}

		// Determine color of the wall
		if (w1->next_sector >= 0) {
			// Wall is a portal, make it red
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
		} else {
			// Wall is solid, make it white
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
		}

		double screen_x1 = x1 * fov_h / z1 + VIEW_WIDTH/2;
		double screen_x2 = x2 * fov_h / z2 + VIEW_WIDTH/2;
		double screen_x_min = CLAMP(MIN(screen_x1, screen_x2), sector_x_min, sector_x_max);
		double screen_x_max = CLAMP(MAX(screen_x1, screen_x2), sector_x_min, sector_x_max);

		// Wall is not visible
		if (screen_x_min == screen_x_max) {
			w1 = w2;
			continue;
		}

		// Draw the wall
		//x1 = x1 * fov_h / z1 + VIEW_WIDTH/2;
		//x2 = x2 * fov_h / z2 + VIEW_WIDTH/2;
		double screen_ceiling_y1 = (s->ceiling_z - p.z) * fov_v / z1 + VIEW_HEIGHT/2;
		double screen_ceiling_y2 = (s->ceiling_z - p.z) * fov_v / z2 + VIEW_HEIGHT/2;
		double screen_floor_y1 = (s->floor_z - p.z) * fov_v / z1 + VIEW_HEIGHT/2;
		double screen_floor_y2 = (s->floor_z - p.z) * fov_v / z2 + VIEW_HEIGHT/2;

		double next_screen_ceiling_y1;
		double next_screen_ceiling_y2;
		double next_screen_floor_y1;
		double next_screen_floor_y2;
		if(w1->next_sector >= 0) {
			sector* next_s = &w->sectors[w1->next_sector];
			next_screen_ceiling_y1 = (next_s->ceiling_z - p.z) * fov_v / z1 + VIEW_HEIGHT/2;
			next_screen_ceiling_y2 = (next_s->ceiling_z - p.z) * fov_v / z2 + VIEW_HEIGHT/2;
			next_screen_floor_y1 = (next_s->floor_z - p.z) * fov_v / z1 + VIEW_HEIGHT/2;
			next_screen_floor_y2 = (next_s->floor_z - p.z) * fov_v / z2 + VIEW_HEIGHT/2;
		}

		for (double x = screen_x_min; x < screen_x_max; ++x) {
			double ratio = (x - screen_x1) / (screen_x2 - screen_x1);

			// Depth controls the brightness of the color
			double z = z1 + (z2 - z1) * ratio;
			int brightness = CLAMP(255 - 255 * z / 20000, 0, 255);
			if (((int)x) == ((int)screen_x1) || ((int)x) == ((int)screen_x2))
				brightness = 0;
			//int brightness = CLAMP(255 - render_order * 10, 0, 255);
			SDL_SetRenderDrawColor(renderer, brightness, brightness, brightness, SDL_ALPHA_OPAQUE);

			/*
			double ceiling_y = (s->ceiling_z - p.z) * fov_v / z;
			double floor_y = (s->floor_z - p.z) * fov_v / z;
			*/
			double screen_ceiling_y = screen_ceiling_y1 + (screen_ceiling_y2 - screen_ceiling_y1) * ratio;
			double screen_floor_y = screen_floor_y1 + (screen_floor_y2 - screen_floor_y1) * ratio;
			screen_ceiling_y = CLAMP(screen_ceiling_y, screen_y_min[(int)x], screen_y_max[(int)x]);
			screen_floor_y = CLAMP(screen_floor_y, screen_y_min[(int)x], screen_y_max[(int)x]);
			if(w1->next_sector >= 0) {
				double next_screen_ceiling_y = next_screen_ceiling_y1 + (next_screen_ceiling_y2 - next_screen_ceiling_y1) * ratio;
				double next_screen_floor_y = next_screen_floor_y1 + (next_screen_floor_y2 - next_screen_floor_y1) * ratio;
				next_screen_ceiling_y = CLAMP(next_screen_ceiling_y, screen_ceiling_y, screen_floor_y);
				next_screen_floor_y = CLAMP(next_screen_floor_y, screen_ceiling_y, screen_floor_y);
				if (next_screen_ceiling_y > screen_ceiling_y)
					SDL_RenderDrawLine(renderer, x, screen_ceiling_y, x, next_screen_ceiling_y);
				if (next_screen_floor_y < screen_floor_y)
					SDL_RenderDrawLine(renderer, x, next_screen_floor_y, x, screen_floor_y);
			} else {
				SDL_RenderDrawLine(renderer, x, screen_ceiling_y, x, screen_floor_y);
			}

			// These will be the viewing window for the sector behind the portal
			screen_y_min[(int)x] = screen_ceiling_y;
			screen_y_max[(int)x] = screen_floor_y;
		}

		for(int x=0; x<VIEW_WIDTH; ++x) {
			SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);
			SDL_RenderDrawPoint(renderer, x, screen_y_min[x]);
			SDL_RenderDrawPoint(renderer, x, screen_y_max[x]);
		}

		render_order++;

		// To prevent infinite loops, we check whether the sector the portal
		// leads to was not the sector that performed the current render_sector
		// call.
		if(w1->next_sector >=0 && w1->next_sector != calling_sector_num && w->sectors[w1->next_sector].ceiling_z < s->floor_z && w->sectors[w1->next_sector].floor_z > s->ceiling_z)
			render_sector(renderer, w1->next_sector, screen_x_min, screen_x_max, sector_num, depth + 1);

		w1 = w2;
	}
}

void render_perspective(SDL_Renderer* renderer)
{
	// Render viewport border
	SDL_Rect border = {0, 0, VIEW_WIDTH, VIEW_HEIGHT};
	SDL_SetRenderDrawColor(renderer, 0, 255, 255, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawRect(renderer, &border);
	
	memset(screen_y_min, 0, VIEW_WIDTH * sizeof(int));
	memset(screen_y_max, VIEW_HEIGHT, VIEW_WIDTH * sizeof(int));
	render_order = 0;

    // Render sector player is currently in. View limits are the entire screen.
	render_sector(renderer, p.sector, 0, VIEW_WIDTH, p.sector, 0);

	// Render crosshair
	SDL_SetRenderDrawColor(renderer, 0, 255, 255, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawLine(renderer, VIEW_WIDTH/2-10, VIEW_HEIGHT/2, VIEW_WIDTH/2+10, VIEW_HEIGHT/2);
	SDL_RenderDrawLine(renderer, VIEW_WIDTH/2, VIEW_HEIGHT/2-10, VIEW_WIDTH/2, VIEW_HEIGHT/2+10);
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
                        p.avel = 0.002;
                        break;
                    case SDLK_RIGHT:
                        p.avel = -0.002;
                        break;
                    case SDLK_EQUALS:
						max_depth++;
						printf("Max_depth: %d\n", max_depth);
						break;
                    case SDLK_MINUS:
						max_depth--;
						printf("Max_depth: %d\n", max_depth);
						break;
                    case SDLK_UP:
                        p.fvel = 4;
                        break;
                    case SDLK_DOWN:
                        p.fvel = -4;
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
                        if (p.avel > 0) p.avel = 0;
                        break;
                    case SDLK_RIGHT:
                        if (p.avel < 0) p.avel = 0;
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
	if (argc < 2)
		w = load_map("test.map");
	else
		w = load_map(argv[1]);

	// Initialize some arrays who's length depends on things in the map
	screen_y_min = calloc(VIEW_WIDTH, sizeof(int));
	screen_y_max = calloc(VIEW_WIDTH, sizeof(int));

	// Set the player's location, based on the starting point stored in the map
	p.sector = w->cur_sector;
	p.x = w->pos_x;
	p.y = w->pos_y;
	p.z = w->sectors[p.sector].floor_z + PLAYER_EYE_LEVEL;
	printf("Player starts at (%f, %f, %f) in sector %d\n", p.x, p.y, p.z, p.sector);

	// Initialize graphics!
    if (SDL_Init(SDL_INIT_VIDEO) == 0) {
        SDL_Window* window = NULL;
        SDL_Renderer* renderer = NULL;

        SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);

		SDL_Rect map_viewport = {50, 50, MAP_WIDTH, MAP_HEIGHT};
		SDL_Rect perspective_viewport = {375, 50, VIEW_WIDTH, VIEW_HEIGHT};

		// Main loop of the engine
		while (!done) {
			handle_events();
			update();

			// Clear screen
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
			SDL_RenderClear(renderer);

			// Render viewports
			SDL_RenderSetViewport(renderer, &map_viewport);
			render_map(renderer);
			SDL_RenderSetViewport(renderer, &perspective_viewport);
			render_perspective(renderer);

			// Send everything to the video card
		    SDL_RenderPresent(renderer);
		}

        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
    }

	free_map(w);
    SDL_Quit();
    return 0;
}
