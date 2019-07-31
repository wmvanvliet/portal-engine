engine: engine.c
	gcc -o engine map.c engine.c `sdl2-config --cflags --libs` -lm

clean:
	rm -f engine
