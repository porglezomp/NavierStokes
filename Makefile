APP = NavierStokes
FLAGS = $(shell sdl2-config --cflags) -I$(HOME)/usr/include #$(shell Magick++-config --cxxflags)
LIBS = $(shell sdl2-config --libs) -lpng -L$(HOME)/usr/lib #$(shell Magick++-config --libs)
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
