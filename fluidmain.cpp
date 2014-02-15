/*
 * Based on work in [JStam29]
 * Jos Stam, Real-Time Fluid Dynamics for Games 29
 */

#include <graphicsmath.h>
#include <SDL.h>
#include <stdio.h>

#define SDLCRASH	1

void quit(int);
void quit(int, const char*);
void UploadAndRender();
void UpdateFluid();
void HandleEvents();

bool running = true;
const int width = 64, height = 64;
const int upscale = 6;
const int vieww = width * upscale, viewh = height * upscale;
#define IX(i, j)	(i + j*(width+2))

// Graphics variables
SDL_Window *window;
SDL_Renderer *renderer;
Uint32 pixels[(width+2) * (height+2)];
SDL_Texture *texture;

int main() {
	// Initialize SDL stuff
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) quit(SDLCRASH, "Could not initialize SDL");
	SDL_CreateWindowAndRenderer(vieww, viewh, 0, &window, &renderer);
	if (window == NULL) quit(SDLCRASH, "Window was NULL");
	if (renderer == NULL) quit(SDLCRASH, "Renderer was NULL");
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
	
	// Mainloop
	while (running) {
		HandleEvents();
		UpdateFluid();
		UploadAndRender();
	}
	
	// Cleanup and quit
	quit(0);
}


// ********************
//  Mainloop functions
// ********************

void UploadAndRender() {
	SDL_UpdateTexture(texture, NULL, pixels, width * sizeof(Uint32));
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void HandleEvents() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT:
				running = false;
				break;
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym) {
					case SDLK_ESCAPE:
						running = false;
						break;
				}
				break;
		}
	}
}

void UpdateFluid() {
	// NOP
}

// *******************
//  Quit handler code
// *******************

void quit(int rc) { quit(rc, ""); }

void quit(int rc, const char* message) {
	if (rc != 0) {
		fprintf(stderr, "Error: %s\n", message);
		if (rc == SDLCRASH) {
			fprintf(stderr, "%s\n", SDL_GetError());
		}
	}
	SDL_Quit();
	exit(rc);
}