#include <iostream>
#include "SDL.h"
#include <thread>
#include <mutex>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define NUMBER_OF_THREADS 4

std::mutex buffer_mutex;

void drawAllPixels(SDL_Renderer* renderer, int thread_no) {
	int upper_limit = (SCREEN_WIDTH / NUMBER_OF_THREADS) * thread_no;
	int lower_limit = upper_limit - (SCREEN_WIDTH / NUMBER_OF_THREADS);

	std::cout << "Calling thread " << thread_no << std::endl;
	std::cout << "Lower " << lower_limit << std::endl;
	std::cout << "Upper " << upper_limit << std::endl;


	for (int i = lower_limit; i < upper_limit; i++) {
		for (int j = 0; j < SCREEN_HEIGHT; j++) {
			double x0 = i / (SCREEN_WIDTH / 3.5) - 2.5;
			double y0 = j / (SCREEN_HEIGHT / 2.0) - 1.0;
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
			
			std::lock_guard<std::mutex> guard(buffer_mutex);
			SDL_SetRenderDrawColor(renderer, 0, 0, iteration * 2.5, 255);
			SDL_RenderDrawPoint(renderer, i, j);
		}
	}
}

void spawnThreads(SDL_Renderer* renderer) {
	std::thread threads[NUMBER_OF_THREADS];

	for (int i = 0; i < NUMBER_OF_THREADS; i++) {
		threads[i] = std::thread(drawAllPixels, renderer, i + 1);
	}

	for (int i = 0; i < NUMBER_OF_THREADS; i++) {
		threads[i].join();
	}

	std::cout << "Threads are done." << std::endl;
}


bool OnEvent(SDL_Event* event) {
	if (event->type == SDL_QUIT) {
		return false;
	}
	else {
		return true;
	}
}

int main(int argc, char* args[]) {
	
	bool running = true;
	SDL_Window* window ;
	SDL_Renderer* renderer;
	SDL_Event Event;

	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(1024, 768, 0, &window, &renderer);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	while (running)
	{
		while (SDL_PollEvent(&Event)) {
			if (!OnEvent(&Event)) {
				running = false;
			}
		}

		spawnThreads(renderer);
		SDL_RenderPresent(renderer);

	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
