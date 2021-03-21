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
#include <immintrin.h>
#include <atomic>


#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define NUMBER_OF_THREADS 1

std::mutex buffer_mutex;
std::mutex Queue_Mutex;
std::condition_variable condition;

std::queue<std::function<void()>> Queue;

Pixel image[SCREEN_WIDTH * SCREEN_HEIGHT];
std::atomic<Uint8> counter = 0;
int x_axis_offset = 0;
int y_axis_offset = 0;
float scale = 1;

Pixel palette[2048];

void drawAllPixels_SSE(SDL_Renderer* renderer, int thread_no) {
	int upper_limit = (SCREEN_WIDTH / NUMBER_OF_THREADS) * thread_no;
	int lower_limit = upper_limit - (SCREEN_WIDTH / NUMBER_OF_THREADS);

	for (int i = lower_limit; i < upper_limit; i++) {
		float _i[4] = { i };
		for (int j = 0; j < SCREEN_HEIGHT; j++) {
			__m128 x0 = _mm_load_ps(_i);
			float x_multipier = 3.5f / (float)SCREEN_WIDTH - 2.5f;
			float x_multipier_a[4] = { x_multipier };
			__m128 _x_multipier_a = _mm_load_ps(x_multipier_a);

			float x_additive = (float)x_axis_offset / 10.0f;
			float x_additive_a[4] = { x_additive };
			__m128 _x_additive_a = _mm_load_ps(x_additive_a);

			x0 = _mm_mul_ps(x0, _x_multipier_a);
			x0 = _mm_add_ps(x0, _x_additive_a);

			float x_divisor = (float)scale;
			float x_divisor_a[4] = { x_divisor };
			__m128 _x_divisor_a = _mm_load_ps(x_divisor_a);

			x0 = _mm_div_ps(x0, _x_divisor_a);

			float f_x0[4] = { 0 };
			_mm_store_ps(f_x0, x0);

			//float x0 = (float)i / ((float)SCREEN_WIDTH / 3.5f) - 2.5f + (float)x_axis_offset / 10.0f;
			//x0 /= scale;

			float y0[4] = { j / (SCREEN_HEIGHT / 2.0f) - 1.0f + (float)y_axis_offset / 10.0f };
			for (size_t k = 0; k < 4; k++)
			{
				y0[k] /= scale;
			}
			float x[4] = { 0.0f };
			float y[4] = { 0.0f };
			int iteration[4] = { 0 };
			int max_iteration = 100 ;

			for (size_t l = 0; l < 4; l++)
			{
				while (x[l] * x[l] + y[l] * y[l] <= 4.0f && iteration[l] < max_iteration) {
					float xtemp = x[l] * x[l] - y[l] * y[l] + f_x0[l];
					y[l] = 2.0f * x[l] * y[l] + y0[l];
					x[l] = xtemp;
					iteration[l] = iteration[l] + 1;
				}
			}

			int colorI = 0;
			for (size_t l = 0; l < 4; l++)
			{
				float smoothed = log2(log2(x[l] * x[l] + y[l] * y[l]) / 2.0f);  // log_2(log_2(|p|))
				colorI = (int)(sqrt((float)iteration[l] + 10.0f - smoothed) * 256.0f) % 2048;
			}


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

void drawAllPixels(SDL_Renderer* renderer, int thread_no) {
	int upper_limit = (SCREEN_WIDTH / NUMBER_OF_THREADS) * thread_no;
	int lower_limit = upper_limit - (SCREEN_WIDTH / NUMBER_OF_THREADS);
 
	for (int i = lower_limit; i < upper_limit; i++) {
		for (int j = 0; j < SCREEN_HEIGHT; j++) {
			float x0 = (float)i / ((float)SCREEN_WIDTH / 3.5f) - 2.5f + (float)x_axis_offset / 10.0f;
			float y0 = j / (SCREEN_HEIGHT / 2.0f) - 1.0f + (float)y_axis_offset / 10.0f;
			x0 /= scale;
			y0 /= scale;
			float x = 0.0f;
			float y = 0.0f;
			int iteration = 0;
			int max_iteration = 100;
			while (x * x + y * y <= 4.0f && iteration < max_iteration) {
				float xtemp = x * x - y * y + x0;
				y = 2.0f * x * y + y0;
				x = xtemp;
				iteration = iteration + 1;
			}

			float smoothed = log2(log2(x * x + y * y) / 2.0f);  // log_2(log_2(|p|))
			int colorI = (int)(sqrt((float)iteration + 10.0f - smoothed) * 256.0f) % 2048;

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
		else if (event->key.keysym.sym == SDLK_KP_PLUS)
		{
			scale += 1;
		}
		else if (event->key.keysym.sym == SDLK_KP_MINUS)
		{
			scale -= 1;
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
	std::cout << "in infinite loop function, " << (int)counter << "." << std::endl;
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
	std::cout << "spawning threads, " << (int)counter << std::endl;

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

	std::cout << "After clear" << std::endl;

	Uint64 NOW = SDL_GetPerformanceCounter();
	Uint64 LAST = 0;
	double deltaTime = 0;
	double avgFrameTime = 0;

	std::thread thread_pool[NUMBER_OF_THREADS];
	initThreads(thread_pool);

	std::cout << "After init threads" << std::endl;

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH, SCREEN_HEIGHT);

	while (running)
	{
		while (SDL_PollEvent(&Event)) {
			if (!OnEvent(&Event)) {
				return false;
			}
		}

		spawnThreads(renderer);
		while (counter > 0) {
			//std::cout << std::endl;
		}
		std::cout << "After init infinite loop" << std::endl;

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
