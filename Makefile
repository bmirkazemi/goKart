all: model3d

model3d: model3d.cpp
	g++ model3d.cpp -Wall -Wextra -lX11 -lGL -lGLU -lm ./libggfonts.a -o model3d

clean:
	rm -f model3d

