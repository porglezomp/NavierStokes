APP = NavierStokes
FLAGS = $(shell pkg-config graphicsmath0 --cflags) $(shell sdl2-config --cflags)
LIBS = $(shell pkg-config graphicsmath0 --libs) $(shell sdl2-config --libs)
OBJS = fluidmain.o
all: $(APP)

$(APP) : $(OBJS)
	g++ -o $@ $< $(FLAGS) $(LIBS) -g

%.o : %.cpp
	g++ $< -c $(FLAGS) -g

run:
	./NavierStokes

clean:
	rm $(APP) $(OBJS)
