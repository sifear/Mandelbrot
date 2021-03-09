#include <iostream>
#include "SDL.h"
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <functional>


#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define NUMBER_OF_THREADS 4

std::mutex buffer_mutex;
std::mutex Queue_Mutex;
std::condition_variable condition;

std::queue<std::function<void()>> Queue;

Uint8 image[SCREEN_WIDTH * SCREEN_HEIGHT * 4];


void drawAllPixels(SDL_Renderer* renderer, int thread_no) {
	int upper_limit = (SCREEN_WIDTH / NUMBER_OF_THREADS) * thread_no;
	int lower_limit = upper_limit - (SCREEN_WIDTH / NUMBER_OF_THREADS);
 
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
			image[800 * 4 * j + i * 4 + 2] = iteration * 2.5;
		}
	}
}


bool OnEvent(SDL_Event* event) {
	if (event->type == SDL_QUIT) {
		return false;
	}
	else {
		return true;
	}
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
	}
}

void initThreads(std::thread pool[NUMBER_OF_THREADS]) {
	for (int i = 0; i < NUMBER_OF_THREADS; i++) {
		pool[i] = std::thread(Infinite_loop_function);
	}

}

void spawnThreads(SDL_Renderer* renderer) {
	std::thread threads[NUMBER_OF_THREADS];

	for (int i = 0; i < NUMBER_OF_THREADS; i++) {
		add(std::bind(&drawAllPixels, renderer, i + 1));
	}
}



int main(int argc, char* args[]) {
	
	bool running = true;
	SDL_Window* window ;
	SDL_Renderer* renderer;
	SDL_Event Event;
	SDL_Texture* texture = NULL;

	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	Uint64 NOW = SDL_GetPerformanceCounter();
	Uint64 LAST = 0;
	double deltaTime = 0;

	std::thread thread_pool[NUMBER_OF_THREADS];
	initThreads(thread_pool);


	while (running)
	{
		while (SDL_PollEvent(&Event)) {
			if (!OnEvent(&Event)) {
				running = false;
			}
		}

		spawnThreads(renderer);

		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, 800, 600);
		SDL_UpdateTexture(texture, NULL, image, 800 * 4);
		SDL_RenderCopy(renderer, texture, NULL, NULL);

		SDL_RenderPresent(renderer);

		SDL_DestroyTexture(texture);

		LAST = NOW;
		NOW = SDL_GetPerformanceCounter();

		deltaTime = (double)((NOW - LAST) * 1000 / (double)SDL_GetPerformanceFrequency());

		std::cout << deltaTime << std::endl;
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
