all: vectorKart

vectorKart: vectorKart.cpp
	g++ vectorKart.cpp -Wall -Wextra -lX11 -lGL -lGLU -lm \
	./libggfonts.a -o vectorKart

clean:
	rm -f vectorKart

