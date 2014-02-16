/*
 * Based on work in [JStam29]
 * Jos Stam, Real-Time Fluid Dynamics for Games 29
 */

#include <graphicsmath.h>
#include <SDL.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <utility>

#define SDLCRASH	1

void quit(int);
void quit(int, const char*);
void UploadAndRender();
void AddForces();
void Diffuse(float*, float*);
void Move();
void HandleEvents();
void PopulateGrids();
void UpdatePixels(float*);
void SetBoundaries(float*, int);

bool running = true;
const int width = 100, height = 100;
const int upscale = 5;
const int vieww = width * upscale, viewh = height * upscale;
const float dt = .01, diff = .01;
#define size		(width+2) * (height+2)
#define IX(i, j)	((i) + (j)*(width+2))
#define XY(i, j)	(((i) - 1) + ((j) - 1)*(width))

// Graphics variables
SDL_Window *window;
SDL_Renderer *renderer;
Uint32 pixels[width*height];
vec2 u[size], u_prev[size];
float dens[size], dens_prev[size];
SDL_Texture *texture;

int main() {
	// Initialize SDL stuff
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) quit(SDLCRASH, "Could not initialize SDL");
	SDL_CreateWindowAndRenderer(vieww, viewh, 0, &window, &renderer);
	if (SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1") < 0) quit(SDLCRASH, "Error setting scaling hint");
	if (window == NULL) quit(SDLCRASH, "Window was NULL");
	if (renderer == NULL) quit(SDLCRASH, "Renderer was NULL");
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);

	PopulateGrids();

	// Mainloop
	while (running) {
		HandleEvents();
		AddForces();
		std::swap(dens_prev, dens);
		Diffuse(dens_prev, dens);
		Move();
		UpdatePixels(dens);
		UploadAndRender();
		SDL_Delay(100);
		float a = 0;
		for (int i = 0; i < size; i++) {
			a += dens[i];
		}
		printf("%f\n", a);
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

void UpdatePixels(float *dens) {
	for (int y = 1; y <= height; y++) {
		for (int x = 1; x <= width; x++) {
			int val = dens[IX(x, y)] * 255;
			pixels[XY(x, y)] = val | val << 8 | val << 16;
		}
	}
}

void PopulateGrids() {
	for (int y = 1; y <= height; y++) {
		for (int x = 1; x <= width; x++) {
			float dx = width/2 - x, dy = height/2 - y;
			float rho = std::min(5 / (sqrt(dx * dx + dy * dy)+1), 1.0);
			dens[IX(x, y)] = rho;
		}
	}
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

// ************
//  Fluid code
// ************

void AddForces() {
}

void Diffuse(float *prev, float *cur) {
	float a = dt * diff * width * height;
	for (int i = 0; i < size; i++) { cur[i] = 0; }
	for (int k = 0; k < 20; k++) {
		for (int x = 1; x <= width; x++) {
			for (int y = 1; y <= height; y++) {
				cur[IX(x, y)] = (prev[IX(x, y)] + a*(cur[IX(x-1, y)] + cur[IX(x+1, y)] +
													cur[IX(x, y-1)] + cur[IX(x, y+1)]))/(4*a);
			}
		}
		SetBoundaries(cur, 0);
	}
}

void Move() {
}

void SetBoundaries(float *dens, int b) { 
	for (int y = 1; y <= height ; y++) { 
		dens[IX(0, y)] = b==1 ? -dens[IX(1, y)] : dens[IX(1, y)]; 
		dens[IX(width+1, y)] = b==1 ? -dens[IX(width, y)] : dens[IX(width, y)]; 
	}
	for (int x = 1; x <= width; x++) {
		dens[IX(x, 0)] = b==2 ? -dens[IX(x, 1)] : dens[IX(x, 1)];
		dens[IX(x, height+1)] = b==2 ? -dens[IX(x, height)] : dens[IX(x, height)];
	}
	/*dens[IX(0, 0)] = 0.5 * (dens[IX(1, 0)] + dens[IX(0, 1)]); 
	dens[IX(0, height+1)] = 0.5 * (dens[IX(1, height+1)] + dens[IX(0, height)]); 
	dens[IX(width+1, 0)] = 0.5 * (dens[IX(width, 0)] + dens[IX(width+1, 1)]); 
	dens[IX(width+1, height+1)] = 0.5 * (dens[IX(width, height+1)] + dens[IX(width+1, height)]); */
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