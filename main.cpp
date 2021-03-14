// * 2021 March
// * Dániel Békési
// *
// * Mandelbrot render
// * Draw, then incrementlly increase performance through different methods
// * and learn some c++ in the process.
// * Threadpool based on "PhD AP EcE"'s solution in
// * https://stackoverflow.com/questions/15752659/thread-pooling-in-c11 
// 
//


#include <iostream>
#include "SDL.h"
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <functional>
#include <cmath> 
#include "pixel.h"
#include "palette.h"
#include "main.h"


#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define NUMBER_OF_THREADS 16

std::mutex buffer_mutex;
std::mutex Queue_Mutex;
std::condition_variable condition;

std::queue<std::function<void()>> Queue;

Pixel image[SCREEN_WIDTH * SCREEN_HEIGHT];
Uint8 counter = 0;
int x_axis_offset = 0;
int y_axis_offset = 0;

Pixel palette[2048];

Pixel linear_interpolate(Pixel p1, Pixel p2) {
	Pixel newPixel;

	newPixel.red = std::min((p1.red + p2.red) / 2, 255);
	newPixel.green = std::min((p1.green + p2.green) / 2, 255);
	newPixel.blue = std::min((p1.blue + p2.blue) / 2, 255);

	return newPixel;
}

void drawAllPixels(SDL_Renderer* renderer, int thread_no) {
	int upper_limit = (SCREEN_WIDTH / NUMBER_OF_THREADS) * thread_no;
	int lower_limit = upper_limit - (SCREEN_WIDTH / NUMBER_OF_THREADS);
 
	for (int i = lower_limit; i < upper_limit; i++) {
		for (int j = 0; j < SCREEN_HEIGHT; j++) {
			double x0 = (double)i / ((double)SCREEN_WIDTH / 3.5) - 2.5 + (double)x_axis_offset / 100.0;
			double y0 = j / (SCREEN_HEIGHT / 2.0) - 1.0 + (double)y_axis_offset / 100.0;
			double x = 0.0;
			double y = 0.0;
			int iteration = 0;
			int max_iteration = 100;
			while (x * x + y * y <= 4.0 && iteration < max_iteration) {
				double xtemp = x * x - y * y + x0;
				y = 2.0 * x * y + y0;
				x = xtemp;
				iteration = iteration + 1;
			}

			double smoothed = log2(log2(x * x + y * y) / 2);  // log_2(log_2(|p|))
			int colorI = (int)(sqrt((double)iteration + 10.0 - smoothed) * 256.0) % 2048;

			Pixel color1;
			color1.red = red_palette[(int)floor(colorI)];
			color1.green = green_palette[(int)floor(colorI)];
			color1.blue = blue_palette[(int)floor(colorI)];

			image[SCREEN_WIDTH * j + i].red = color1.red;
			image[SCREEN_WIDTH * j + i].green = color1.green;
			image[SCREEN_WIDTH * j + i].blue = color1.blue;
		}
	}
}


bool OnEvent(SDL_Event* event) {
	if (event->type == SDL_QUIT) {
		return false;
	}
	else if (event->type == SDL_KEYDOWN) {
		std::cout << "Key down" << std::endl;
		if (event->key.keysym.sym == SDLK_DOWN) {
			y_axis_offset -= 1;
		}
		else if (event->key.keysym.sym == SDLK_UP) {
			y_axis_offset += 1;
		}
		else if (event->key.keysym.sym == SDLK_LEFT) {
			x_axis_offset += 1;
		}
		else if (event->key.keysym.sym == SDLK_RIGHT) {
			x_axis_offset -= 1;
		}
	}
	return true;
}

void add(std::function<void()> new_job) {
	{
		std::unique_lock<std::mutex> lock(Queue_Mutex);
		Queue.push(new_job);
	}
	condition.notify_one();
}

void Infinite_loop_function() {
	//std::cout << "in infinite loop function" << std::endl;
	std::function<void()>  Job;
	while (true)
	{
		{
			std::unique_lock<std::mutex> lock(Queue_Mutex);

			condition.wait(lock, [] { return !Queue.empty(); });
			Job = Queue.front();
			Queue.pop();
		}
		Job(); // function<void()> type
		std::unique_lock<std::mutex> lock(Queue_Mutex);
		counter -= 1;
	}
}

void initThreads(std::thread pool[NUMBER_OF_THREADS]) {
	for (int i = 0; i < NUMBER_OF_THREADS; i++) {
		pool[i] = std::thread(Infinite_loop_function);
	}

}

void spawnThreads(SDL_Renderer* renderer) {
	counter = NUMBER_OF_THREADS;

	for (int i = 0; i < NUMBER_OF_THREADS; i++) {
		add(std::bind(&drawAllPixels, renderer, i + 1));
	}
}

int main(int argc, char* args[]) {

	//initPalette(palette);
	
	bool running = true;
	SDL_Window* window ;
	SDL_Renderer* renderer;
	SDL_Texture* texture = NULL;
	SDL_Event Event;

	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	Uint64 NOW = SDL_GetPerformanceCounter();
	Uint64 LAST = 0;
	double deltaTime = 0;
	double avgFrameTime = 0;

	std::thread thread_pool[NUMBER_OF_THREADS];
	initThreads(thread_pool);

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH, SCREEN_HEIGHT);

	while (running)
	{
		while (SDL_PollEvent(&Event)) {
			if (!OnEvent(&Event)) {
				return false;
			}
		}

		spawnThreads(renderer);
		while (counter != 0) {}

		SDL_UpdateTexture(texture, NULL, image, SCREEN_WIDTH * (int)sizeof(Pixel));
		SDL_RenderCopy(renderer, texture, NULL, NULL);

		SDL_RenderPresent(renderer);

		//SDL_DestroyTexture(texture);

		LAST = NOW;
		NOW = SDL_GetPerformanceCounter();

		deltaTime = (double)((NOW - LAST) * 1000 / (double)SDL_GetPerformanceFrequency());
		avgFrameTime += deltaTime;
		avgFrameTime /= 2;

		std::cout << avgFrameTime << std::endl;
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
