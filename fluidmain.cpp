/*
 * Based on work in [JStam29]
 * Jos Stam, Real-Time Fluid Dynamics for Games 29
 */

#include <SDL.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <utility>
#include <time.h>
#include "MathSupport.hpp"
extern "C" {
	#include "RGBE.h"
};

/******************************************* USER CHANGES GO HERE ***********************************************/

// ***************************************************
//  Modify these to customize the initial conditions!
// ***************************************************

// Constants
const int width = 384, height = 216, upscale = 3;
const float diff = 0.0, visc = 0.0;

// Initial density
float DensityFunc(float x, float y) {
	float dx = .5 - (float) x/width, dy = .5 - (float) y/height;
	return saturate(sin(x*.1)*tan(y*.05));
}

// Initial velocity
void VelocityFunc(float &u, float &v, float x, float y) {
	float dx = .5 - (float) x/height, dy = .5 - (float) y/height;
	u = rand()%11-5;
	v = rand()%11-5;
}

/****************************************************************************************************************/

// ***********
//  Body code
// ***********

#define SDLCRASH	1

void quit(int);
void quit(int, const char*);

void UploadAndRender();
void PopulateGrids();
void HandleEvents();
void UpdatePixels(float*);

void DensityStep();
void VelocityStep();

void Diffuse(int, float*, float*, float);
void Advect(int, float*, float*, float*, float*);
void Project(float*, float*, float*, float*);
void SetBoundaries(float*, int);

bool running = true;
const int vieww = width * upscale, viewh = height * upscale;
float dt = .01, initialmass = 0;
#define size		(width+2) * (height+2)
#define IX(i, j)	((i) + (j)*(width+2))
#define XY(i, j)	(((i) - 1) + ((j) - 1)*(width))

// Graphics variables
SDL_Window *window;
SDL_Renderer *renderer;
Uint32 pixels[width*height];
SDL_Texture *texture;

float u[size], u_prev[size];
float v[size], v_prev[size];
float dens[size], dens_prev[size];
float img[width*height][3];
long long a, b;

// ******
//  Main
// ******
const char* name1;

int main(int argc, char **argv) {
	if (argc > 1) {
		name1 = argv[1];
	}

	// Initialize SDL stuff
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) quit(SDLCRASH, "Could not initialize SDL");
	SDL_CreateWindowAndRenderer(vieww, viewh, 0, &window, &renderer);
	if (SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1") < 0) quit(SDLCRASH, "Error setting scaling hint");
	if (window == NULL) quit(SDLCRASH, "Window was NULL");
	if (renderer == NULL) quit(SDLCRASH, "Renderer was NULL");
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);

	a = SDL_GetTicks();
	PopulateGrids();

	int counter = 1;
	// Mainloop
	while (running) {
		// Deltatime
		b = a;
		a = SDL_GetTicks();
		dt = (a - b) / 10000.0; // Run at 1/10th speed

		// Simulate the smoke
		HandleEvents();
		DensityStep();
		VelocityStep();
		UpdatePixels(dens);
		UploadAndRender();
		
		// Prepare output
		float mass = 0;
		for (int i = 1; i <= width; i++) {
			for (int j = 1; j <= height; j++) {
				float p = dens[IX(i, j)];
				mass += p;
				img[XY(i, j)][0] = p;
				img[XY(i, j)][1] = p;
				img[XY(i, j)][2] = p;
			}
		}

		// Output
		printf("%f%% mass\n", mass/initialmass * 100);
		if (argc > 1) {
			char name[1024];
			sprintf(name, "%s%i.hdr", name1, counter++);
			FILE *f = fopen(name, "wb");
			RGBE_WriteHeader(f, width, height, NULL);
			RGBE_WritePixels_RLE(f, (float *) img, width, height);
			fclose(f);
		}
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
			#ifdef VECCOLS
			float rbase = u[IX(x, y)]*128, gbase = v[IX(x, y)]*128;
			int r = rbase >= 0 ? 128+rbase : 128-rbase;
			r = std::max(0,  std::min(r, 255));
			int g = gbase >= 0 ? 128+gbase : 128-gbase;
			g = std::max(0, std::min(g, 255));
			pixels[XY(x, y)] = r << 16 | g << 8 | val;
			#else
			pixels[XY(x, y)] = val << 16 | val << 8 | val;
			#endif
		}
	}
}

void PopulateGrids() {
	for (int y = 1; y <= height; y++) {
		for (int x = 1; x <= width; x++) {
			float rho = DensityFunc(x, y);
			dens[IX(x, y)] = rho;
			initialmass += rho;
			float uvec, vvec;
			VelocityFunc(uvec, vvec, x, y);
			u[IX(x, y)] = uvec;
			v[IX(x, y)] = vvec;
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

void DensityStep() {
	std::swap(dens, dens_prev);
	Diffuse(0, dens, dens_prev, diff);
	std::swap(dens, dens_prev);
	Advect(0, dens, dens_prev, u, v);
}

void VelocityStep() {
	std::swap(u, u_prev); Diffuse(1, u, u_prev, visc);
	std::swap(v, v_prev); Diffuse(2, v, v_prev, visc);
	Project(u, v, u_prev, v_prev);
	std::swap(u, u_prev); std::swap(v, v_prev);
	Advect(1, u, u_prev, u_prev, v_prev); Advect(2, v, v_prev, u_prev, v_prev);
	Project(u, v, u_prev, v_prev);
}


void Diffuse(int border, float *cur, float *prev, float diff) {
	float a = dt * diff * width * height;
	for (int k = 0; k < 20; k++) {
		for (int x = 1; x <= width; x++) {
			for (int y = 1; y <= height; y++) {
				cur[IX(x, y)] = (prev[IX(x, y)] + a*(cur[IX(x-1, y)] + cur[IX(x+1, y)] +
													 cur[IX(x, y-1)] + cur[IX(x, y+1)]))/(1+4*a);
			}
		}
		SetBoundaries(cur, border);
	}
}

void Advect(int border, float *cur, float *prev, float *u, float *v) {
	float dtx = dt * width, dty = dt * height;
	int i0, i1, j0, j1;
	float x, y, s0, s1, t0, t1;
	for (int i = 1; i <= width; i++) {
		for (int j = 1; j <= height; j++) {
			float x = i-dtx*u[IX(i, j)], y = j-dty*v[IX(i, j)];
			if (x < 0.5) x = 0.5; if (x > width + 0.5) x = width + 0.5;
			i0 = (int) x; i1 = i0 + 1;
			if (y < 0.5) y = 0.5; if (y > height + 0.5) y = height + 0.5;
			j0 = (int) y; j1 = j0 + 1;
			s1 = x - i0; s0 = 1 - s1; t1 = y - j0; t0 = 1 - t1;
			cur[IX(i, j)] = s0*(t0*prev[IX(i0, j0)] + t1*prev[IX(i0, j1)])+
							s1*(t0*prev[IX(i1, j0)] + t1*prev[IX(i1, j1)]);
		}
	}
	SetBoundaries(cur, border); 
}

void Project(float *u, float *v, float *p, float *div) {
	float x = 1.0/width, y = 1.0/height;

	for (int i = 1; i <= width ; i++) { 
		for (int j = 1; j <= height; j++) { 
			div[IX(i,j)] = -0.5*x*(u[IX(i+1,j)]-u[IX(i-1,j)]+ 
			v[IX(i,j+1)]-v[IX(i,j-1)]); 
			p[IX(i,j)] = 0; 
		} 
	} 
	SetBoundaries(div, 0); SetBoundaries(p, 0); 
	 
	for (int k = 0; k < 20; k++) { 
		for (int i = 1; i <= width; i++) { 
			for (int j = 1; j <= height; j++) { 
				p[IX(i,j)] = (div[IX(i,j)]+p[IX(i-1,j)]+p[IX(i+1,j)]+ 
				p[IX(i,j-1)]+p[IX(i,j+1)])/4; 
			} 
		} 
		SetBoundaries(p, 0); 
	} 
	 
	for (int i = 1; i <= width; i++) { 
		for (int j = 1; j <= height; j++) { 
			u[IX(i,j)] -= 0.5*(p[IX(i+1,j)]-p[IX(i-1,j)])/x; 
			v[IX(i,j)] -= 0.5*(p[IX(i,j+1)]-p[IX(i,j-1)])/y; 
		} 
	} 
	SetBoundaries(u, 1); SetBoundaries(v, 2); 
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