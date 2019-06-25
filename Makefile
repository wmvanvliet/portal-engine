engine: engine.c
	gcc -o engine engine.c `sdl2-config --cflags --libs` -lm

clean:
	rm -f engine
