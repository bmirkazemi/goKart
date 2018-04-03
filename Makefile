all: vectorKart

vectorKart: vectorKart.cpp timers.cpp
	g++ vectorKart.cpp timers.cpp -Wall -Wextra -lX11 -lGL -lGLU -lm \
	./libggfonts.a -o vectorKart

clean:
	rm -f vectorKart

