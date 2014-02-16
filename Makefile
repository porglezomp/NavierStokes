APP = NavierStokes
FLAGS = $(shell sdl2-config --cflags) -I$(HOME)/usr/include
LIBS = $(shell sdl2-config --libs) -lpng -L$(HOME)/usr/lib
OBJS = fluidmain.o
all: $(APP)

$(APP) : $(OBJS)
	g++ -o $@ $< $(FLAGS) $(LIBS) -fwhole-program -flto -Ofast

%.o : %.cpp
	g++ $< -c $(FLAGS) -flto -Ofast -O 

run:
	./NavierStokes

clean:
	rm $(APP) $(OBJS)
