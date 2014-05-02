CFLAGS  = -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wno-unused-but-set-variable
LDFLAGS = -lglfw -lGL -lGLEW -lm

all: Lab4

Lab4: Lab4.o
	g++ -o $@ $< $(LDFLAGS)

Lab4.o: Lab4.cpp cube.h
	g++ -c project2.cpp $(CFLAGS)

clean:
	-rm *.o
	-rm Lab4
