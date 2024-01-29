CFLAGS = -g -Wall -Wextra -Wpedantic -std=c11

build:
	gcc ${CFLAGS} -c -o main.o `sdl2-config --cflags` main.c
	gcc ${CFLAGS} -c -o person.o `sdl2-config --cflags` person.c
	gcc ${CFLAGS} -o app main.o person.o -lm `sdl2-config --libs` -lSDL2_image -lSDL2_ttf

bug:
	gcc ${CFLAGS} -c -o bug.o `sdl2-config --cflags` bug.c
	gcc ${CFLAGS} -o app bug.o -lm -ldl `sdl2-config --libs` -lSDL2_image

demo:
	gcc ${CFLAGS} -c -o demo.o `sdl2-config --cflags` demo.c
	gcc ${CFLAGS} -o app demo.o -lm -ldl `sdl2-config --libs` -lSDL2_image

clean:
	rm app main.o a.out main.o person.o 2>/dev/null || true
