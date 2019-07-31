#include <stdio.h>
#include "SDL.h"
#include "map.h"

world* load_map(char* filename)
{
	world* w = malloc(sizeof(world));
 	FILE *f = fopen(filename, "rb");
	fread(w, 22, 1, f);
	printf("map_version: %d\n", w->map_version);
	printf("pos_x: %d\n", w->pos_x);
	printf("pos_y: %d\n", w->pos_y);
	printf("pos_z: %d\n", w->pos_z);
	printf("angle: %d\n", w->angle);
	printf("cur_sector: %d\n", w->cur_sector);
	printf("n_sectors: %d\n", w->n_sectors);

	w->sectors = (sector*) malloc(w->n_sectors * sizeof(sector));
	fread(w->sectors, sizeof(sector), w->n_sectors, f);

	fread(&w->n_walls, sizeof(Uint16), 1, f);
	printf("n_walls: %d\n", w->n_walls);
	w->walls = (wall*) malloc(w->n_walls * sizeof(wall));
	fread(w->walls, sizeof(wall), w->n_walls, f);

	printf("Read map.\n");

	for(int i=0; i<w->n_sectors; ++i) {
		printf("Sector %d has %d walls, first wall is at offset %d.\n", i, w->sectors[i].n_walls, w->sectors[i].first_wall);
	}
	for(int i=0; i<w->n_walls; ++i) {
		printf("Wall %d = (%d, %d), %d\n", i, w->walls[i].x, w->walls[i].y, w->walls[i].point2);
	}

	fclose(f);
	return w;
}

void free_map(world* w)
{
	free(w->sectors);
	free(w->walls);
	free(w->sprites);
	free(w);
}
