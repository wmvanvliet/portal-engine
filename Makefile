engine: engine.c
	gcc `pkg-config --cflags --libs sdl2` -o engine engine.c
