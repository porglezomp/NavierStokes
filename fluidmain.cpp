/*
 * Based on work in [JStam29]
 * Jos Stam, Real-Time Fluid Dynamics for Games 29
 */

#include <SDL.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <utility>
#include "PngWriter.hpp"

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
const int width = 384, height = 216;
const int upscale = 3;
const int vieww = width * upscale, viewh = height * upscale;
const float diff = 0, visc = 0;;
float dt = .01;
#define size		(width+2) * (height+2)
#define IX(i, j)	((i) + (j)*(width+2))
#define XY(i, j)	(((i) - 1) + ((j) - 1)*(width))
PngWriter writer(width, height);

// Graphics variables
SDL_Window *window;
SDL_Renderer *renderer;
Uint32 pixels[width*height];
float u[size], u_prev[size];
float v[size], v_prev[size];
float dens[size], dens_prev[size];
SDL_Texture *texture;
long long a, b;

template <typename T>
T sgn(T t) { return (t < 0) ? T(-1) : T(1); }

// ******
//  Main
// ******

int main() {
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
		b = a;
		a = SDL_GetTicks();
		dt = (a - b) / 1000.0;
		HandleEvents();
		DensityStep();
		VelocityStep();
		UpdatePixels(dens);
		UploadAndRender();
		float mass = 0;
		for (int i = 1; i <= width; i++) {
			for (int j = 1; j <= height; j++) {
				mass += dens[IX(i, j)];
			}
		}
		for (int x = 1; x <= width; x++) {
			for (int y = 1; y <= height; y++) {
				int p = pixels[XY(x, y)];
				int r = (p & (255<<16))>>16;
				int g = (p & (255<<8))>>8;
				int b = (p * 255);
				writer.set(x-1, y-1, r, g, b);
			}
		}
		char name[32];
		sprintf(name, "frame%i.png", counter++);
		writer.write(name);
		printf("%s %f\n", name, mass);
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

// Thanks to Iñigo Quilez
float almostIdentity( float x, float m, float n ) {
    if( x>m ) return x;
    const float a = 2.0f*n - m;
    const float b = 2.0f*m - 3.0f*n;
    const float t = x/m;
    return (a*t + b)*t*t + n;
}

void PopulateGrids() {
	for (int y = 1; y <= height; y++) {
		for (int x = 1; x <= width; x++) {
			float dx = .5 - (float) x/width, dy = .5 - (float) y/width;
			float rho = std::min(5.0 / ((dx * dx + dy * dy)*100+1), 1.0);
			dens[IX(x, y)] = rho;
			u[IX(x, y)] = -cos(dx*10);//(1/almostIdentity(abs(dx), 2, 1)) * (1/almostIdentity(y, 2, 1)) * sgn(dx) * 10;
			v[IX(x, y)] = -cos(dx*10);
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
	std::swap(v, v_prev); Diffuse(1, v, v_prev, visc);
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