/*
 * This is the build editor map format. All of it.
 */

#ifndef MAP_H
#define MAP_H
#include "SDL.h"

typedef struct sector {
	Sint16 first_wall;
	Sint16 n_walls;
	Sint32 ceiling_z;
	Sint32 floor_z;
	Sint16 ceiling_stat;
	Sint16 floor_stat;
	Sint16 ceiling_texture;
	Sint16 ceiling_slopt;
	Sint8 ceiling_shade;
	Uint8 ceiling_palette;
	Uint8 ceiling_x_panning;
	Uint8 ceiling_y_panning;
	Sint16 floor_texture;
	Sint16 floor_slopt;
	Sint8 floor_shade;
	Uint8 floor_palette;
	Uint8 floor_x_panning;
	Uint8 floor_y_panning;
	Uint8 visibility;
	Sint16 low_tag;
	Sint16 high_tag;
	Sint16 extra;
} sector;

typedef struct wall {
	Sint32 x;
	Sint32 y;
	Sint16 point2;
	Sint16 next_wall;
	Sint16 next_sector;
	Sint16 flags;
	Sint16 texture;
	Sint16 overlay_texture;
	Sint8 shade;
	Uint8 palette;
	Uint8 x_repeat;
	Uint8 y_repeat;
	Uint8 x_panning;
	Uint8 y_panning;
	Sint16 low_tag;
	Sint16 high_tag;
	Sint16 extra;
} wall;

typedef struct sprite {
	Sint32 x;
	Sint32 y;
	Sint32 z;
	Sint16 flags;
	Sint16 texture;
	Sint8 shade;
	Uint8 palette;
	Uint8 clip_dist;
	Uint8 filler;
	Uint8 x_repeat;
	Uint8 y_repeat;
	Sint8 x_offset;
	Sint8 y_offset;
	Sint16 sector;
	Sint16 status;
	Sint16 angle;
	Sint16 owner;
	Sint16 vel_x;
	Sint16 vel_y;
	Sint16 vel_z;
	Sint16 low_tag;
	Sint16 high_tag;
	Sint16 extra;
} sprite;

typedef struct {
	Sint32 map_version;
	Sint32 pos_x;
	Sint32 pos_y;
	Sint32 pos_z;
	Sint16 angle;
	Sint16 cur_sector;
	Uint16 n_sectors;
	sector* sectors;
	Uint16 n_walls;
	wall* walls;
	Uint16 n_sprites;
	sprite* sprites;
} world;

world* load_map(char* filename);
void free_map(world* w);

#endif /* MAP_H */
