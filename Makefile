APP = NavierStokes
FLAGS = $(shell sdl2-config --cflags)
LIBS = $(shell sdl2-config --libs)
OBJS = fluidmain.o RGBE.o
all: $(APP)

$(APP) : $(OBJS)
	g++ -o $@ $(OBJS) $(FLAGS) $(LIBS) -fwhole-program -flto -Ofast

%.o : %.cpp
	g++ $< -c $(FLAGS) -flto -Ofast 
%.o : %.cpp
	gcc $< -c $(FLAGS) -flto -Ofast

run:
	./NavierStokes

clean:
	rm $(APP) $(OBJS)
