all: vectorKart

vectorKart: vectorKart.cpp timers.cpp
	g++ vectorKart.cpp timers.cpp joystick.c -Wall -Wextra -lX11 -lGL -lGLU -lm \
	./libggfonts.a /usr/lib/x86_64-linux-gnu/libopenal.so /usr/lib/x86_64-linux-gnu/libalut.so -o vectorKart

clean:
	rm -f vectorKart

