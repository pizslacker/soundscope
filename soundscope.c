#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define SCREEN_W 800
#define SCREEN_H 600

// We need a global struct to hold our WAV data so the audio thread can access it
typedef struct {
    Uint8* start;
    Uint32 length;
    Uint8* pos;
    Uint32 remaining;
} AudioContext;

AudioContext audio_ctx;

// Shared buffers for the visualizer (Left and Right channels)
// We use volatile because these are written by the audio thread and read by the main thread
volatile int16_t vis_buffer_L[SCREEN_W];
volatile int16_t vis_buffer_R[SCREEN_W];

// The Audio Callback: Runs on a separate background thread!
void audio_callback(void* userdata, Uint8* stream, int len) {
    AudioContext* ctx = (AudioContext*)userdata;

    if (ctx->remaining == 0) return;

    // Handle audio looping seamlessly
    int copied = 0;
    while (len > 0) {
        int to_copy = (len > ctx->remaining) ? ctx->remaining : len;
        SDL_memcpy(stream + copied, ctx->pos, to_copy);
        
        ctx->pos += to_copy;
        ctx->remaining -= to_copy;
        copied += to_copy;
        len -= to_copy;

        // Loop back to the start of the song
        if (ctx->remaining == 0) {
            ctx->pos = ctx->start;
            ctx->remaining = ctx->length;
        }
    }

    // --- INTERCEPT DATA FOR THE OSCILLOSCOPE ---
    // Cast the raw byte stream to 16-bit signed integers
    int16_t* samples = (int16_t*)stream;
    
    // Total 16-bit samples in this chunk (divide bytes by 2)
    int sample_count = copied / sizeof(int16_t); 

    // Assuming 2 channels (Stereo): Even indices are Left, Odd are Right
    int idx = 0;
    for (int i = 0; i < sample_count && idx < SCREEN_W; i += 2) {
        vis_buffer_L[idx] = samples[i];         // Left Channel
        vis_buffer_R[idx] = samples[i + 1];     // Right Channel
        idx++;
    }
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return 1;

    // 1. Load the WAV file into RAM
    SDL_AudioSpec wav_spec;
    Uint8* wav_buffer;
    Uint32 wav_length;

    if (SDL_LoadWAV("bitdream.wav", &wav_spec, &wav_buffer, &wav_length) == NULL) {
        printf("Failed to load bitdream.wav! Error: %s\n", SDL_GetError());
        return 1;
    }

    // 2. Setup our Audio Context for the callback
    audio_ctx.start = wav_buffer;
    audio_ctx.length = wav_length;
    audio_ctx.pos = wav_buffer;
    audio_ctx.remaining = wav_length;

    // 3. Configure the Audio Device
    wav_spec.callback = audio_callback;
    wav_spec.userdata = &audio_ctx;

    SDL_AudioDeviceID device = SDL_OpenAudioDevice(NULL, 0, &wav_spec, NULL, 0);
    if (device == 0) {
        printf("Failed to open audio device! Error: %s\n", SDL_GetError());
        return 1;
    }

    // 4. Setup Window and Renderer
    SDL_Window* window = SDL_CreateWindow("Cyberpunk Oscilloscope", 
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                          SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Enable Alpha Blending to create the CRT phosphor trail effect
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Unpause the audio device to start the callback loop
    SDL_PauseAudioDevice(device, 0);

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = false;
        }

        // --- DRAW CRT PHOSPHOR FADE ---
        // Instead of clearing the screen with opaque black, we draw a mostly-transparent 
        // black box over the screen. This leaves a fading trail of previous frames!
        SDL_SetRenderDrawColor(renderer, 5, 10, 5, 60); // Dark green-black with alpha=60
        SDL_RenderFillRect(renderer, NULL);

        // --- DRAW WAVEFORMS ---
        // 16-bit audio ranges from -32768 to 32767. 
        // We divide by 32768 to normalize it from -1.0 to 1.0, then multiply by a height scalar.
        float amplitude_scalar = (SCREEN_H / 2.0f) * 0.8f; 
        
        int center_y_L = SCREEN_H / 3;
        int center_y_R = (SCREEN_H / 3) * 2;

        // Draw Left Channel (Bright Green)
        SDL_SetRenderDrawColor(renderer, 50, 255, 100, 255);
        for (int x = 0; x < SCREEN_W - 1; x++) {
            int y1 = center_y_L - (int)((vis_buffer_L[x] / 32768.0f) * amplitude_scalar);
            int y2 = center_y_L - (int)((vis_buffer_L[x+1] / 32768.0f) * amplitude_scalar);
            SDL_RenderDrawLine(renderer, x, y1, x + 1, y2);
        }

        // Draw Right Channel (Bright Cyan)
        SDL_SetRenderDrawColor(renderer, 50, 200, 255, 255);
        for (int x = 0; x < SCREEN_W - 1; x++) {
            int y1 = center_y_R - (int)((vis_buffer_R[x] / 32768.0f) * amplitude_scalar);
            int y2 = center_y_R - (int)((vis_buffer_R[x+1] / 32768.0f) * amplitude_scalar);
            SDL_RenderDrawLine(renderer, x, y1, x + 1, y2);
        }

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    SDL_CloseAudioDevice(device);
    SDL_FreeWAV(wav_buffer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}