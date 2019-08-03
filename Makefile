engine: engine.c
	gcc -o engine -g map.c engine.c `sdl2-config --cflags --libs` -lm

clean:
	rm -f engine
