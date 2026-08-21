#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FFT_SIZE 512
#define NUM_BARS 32

// In-place Radix-2 Fast Fourier Transform (FFT)
void compute_fft(float* Re, float* Im, int N) {
    // 1. Bit-reversal sorting
    int j = 0;
    for (int i = 0; i < N; i++) {
        if (i < j) {
            float tr = Re[j], ti = Im[j];
            Re[j] = Re[i]; Im[j] = Im[i];
            Re[i] = tr; Im[i] = ti;
        }
        int k = N >> 1;
        while (k >= 1 && j >= k) { j -= k; k >>= 1; }
        j += k;
    }

    // 2. Cooley-Tukey Decimation-in-Time
    for (int step = 1; step < N; step <<= 1) {
        float theta = -M_PI / step;
        float wtemp, wr = 1.0f, wi = 0.0f, wpr = cosf(theta), wpi = sinf(theta);
        for (int m = 0; m < step; m++) {
            for (int i = m; i < N; i += step << 1) {
                j = i + step;
                float tmpr = wr * Re[j] - wi * Im[j];
                float tmpi = wr * Im[j] + wi * Re[j];
                Re[j] = Re[i] - tmpr;
                Im[j] = Im[i] - tmpi;
                Re[i] += tmpr;
                Im[i] += tmpi;
            }
            wtemp = wr;
            wr = wr * wpr - wi * wpi;
            wi = wi * wpr + wtemp * wpi;
        }
    }
}

#define SCREEN_W 1280
#define SCREEN_H 720

// Compact 8x8 ASCII Font (Characters 32 to 95: Space to '_')
const uint8_t font8x8[64][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, {0x18,0x3c,0x3c,0x18,0x18,0x00,0x18,0x00}, {0x6c,0x6c,0x6c,0x00,0x00,0x00,0x00,0x00}, {0x6c,0x6c,0xfe,0x6c,0xfe,0x6c,0x6c,0x00},
    {0x18,0x3e,0x60,0x3c,0x06,0x7c,0x18,0x00}, {0x00,0xc6,0xcc,0x18,0x30,0x66,0xc6,0x00}, {0x38,0x6c,0x6c,0x38,0x6d,0x66,0x3b,0x00}, {0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00},
    {0x0c,0x18,0x30,0x30,0x30,0x18,0x0c,0x00}, {0x30,0x18,0x0c,0x0c,0x0c,0x18,0x30,0x00}, {0x00,0x66,0x3c,0xff,0x3c,0x66,0x00,0x00}, {0x00,0x18,0x18,0x7e,0x18,0x18,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, {0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x00}, {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, {0x06,0x0c,0x18,0x30,0x60,0xc0,0x80,0x00},
    {0x3c,0x66,0x6e,0x76,0x66,0x66,0x3c,0x00}, {0x18,0x38,0x58,0x18,0x18,0x18,0x7e,0x00}, {0x3c,0x66,0x06,0x0c,0x30,0x60,0x7e,0x00}, {0x3c,0x66,0x06,0x1c,0x06,0x66,0x3c,0x00},
    {0x0c,0x1c,0x3c,0x6c,0x7e,0x0c,0x0c,0x00}, {0x7e,0x60,0x7c,0x06,0x06,0x66,0x3c,0x00}, {0x3c,0x66,0x60,0x7c,0x66,0x66,0x3c,0x00}, {0x7e,0x06,0x0c,0x18,0x30,0x30,0x30,0x00},
    {0x3c,0x66,0x66,0x3c,0x66,0x66,0x3c,0x00}, {0x3c,0x66,0x66,0x3e,0x06,0x66,0x3c,0x00}, {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00}, {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30},
    {0x06,0x0c,0x18,0x30,0x18,0x0c,0x06,0x00}, {0x00,0x00,0x7e,0x00,0x7e,0x00,0x00,0x00}, {0x60,0x30,0x18,0x0c,0x18,0x30,0x60,0x00}, {0x3c,0x66,0x06,0x0c,0x18,0x00,0x18,0x00},
    {0x3c,0x66,0x6e,0x6e,0x60,0x66,0x3c,0x00}, {0x18,0x3c,0x66,0x66,0x7e,0x66,0x66,0x00}, {0x7c,0x66,0x66,0x7c,0x66,0x66,0x7c,0x00}, {0x3c,0x66,0x60,0x60,0x60,0x66,0x3c,0x00},
    {0x78,0x6c,0x66,0x66,0x66,0x6c,0x78,0x00}, {0x7e,0x60,0x60,0x7c,0x60,0x60,0x7e,0x00}, {0x7e,0x60,0x60,0x7c,0x60,0x60,0x60,0x00}, {0x3c,0x66,0x60,0x6e,0x66,0x66,0x3e,0x00},
    {0x66,0x66,0x66,0x7e,0x66,0x66,0x66,0x00}, {0x3c,0x18,0x18,0x18,0x18,0x18,0x3c,0x00}, {0x06,0x06,0x06,0x06,0x06,0x66,0x3c,0x00}, {0x66,0x6c,0x78,0x70,0x78,0x6c,0x66,0x00},
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7e,0x00}, {0x63,0x77,0x7f,0x6b,0x63,0x63,0x63,0x00}, {0x66,0x76,0x7e,0x7e,0x6e,0x66,0x66,0x00}, {0x3c,0x66,0x66,0x66,0x66,0x66,0x3c,0x00},
    {0x7c,0x66,0x66,0x7c,0x60,0x60,0x60,0x00}, {0x3c,0x66,0x66,0x66,0x6a,0x6c,0x36,0x00}, {0x7c,0x66,0x66,0x7c,0x6c,0x66,0x66,0x00}, {0x3c,0x66,0x60,0x3c,0x06,0x66,0x3c,0x00},
    {0x7e,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, {0x66,0x66,0x66,0x66,0x66,0x66,0x3c,0x00}, {0x66,0x66,0x66,0x66,0x66,0x3c,0x18,0x00}, {0x63,0x63,0x63,0x6b,0x7f,0x77,0x63,0x00},
    {0x66,0x66,0x3c,0x18,0x3c,0x66,0x66,0x00}, {0x66,0x66,0x66,0x3c,0x18,0x18,0x18,0x00}, {0x7e,0x06,0x0c,0x18,0x30,0x60,0x7e,0x00}, {0x3c,0x30,0x30,0x30,0x30,0x30,0x3c,0x00},
    {0x60,0x30,0x18,0x0c,0x06,0x03,0x01,0x00}, {0x3c,0x0c,0x0c,0x0c,0x0c,0x0c,0x3c,0x00}, {0x00,0x00,0x3c,0x66,0x00,0x00,0x00,0x00}, {0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x00}
};

// Hardware accelerated text rendering
void draw_osd_text(SDL_Renderer* renderer, int x, int y, const char* str, int scale) {
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        char c = str[i];
        // --- NEW FIX: Convert lowercase to uppercase ---
        if (c >= 'a' && c <= 'z') {
            c -= 32; 
        }
        
        // Fallback to space for unsupported chars
        if (c < 32 || c > 95) c = 32; 
        
        for (int row = 0; row < 8; row++) {
            uint8_t glyph_row = font8x8[c - 32][row];
            for (int col = 0; col < 8; col++) {
                if (glyph_row & (1 << (7 - col))) {
                    SDL_Rect r = { x + (i * 8 * scale) + (col * scale), y + (row * scale), scale, scale };
                    SDL_RenderFillRect(renderer, &r);
                }
            }
        }
    }
}

// Print the Command Line Help Screen
void print_help_screen(const char* prog_name) {
    printf("SoundScope - Real-Time Audio Visualizer\n");
    printf("\n");
    printf("USAGE:\n");
    printf("  %s [options] [audio_file.wav]\n\n", prog_name);
    printf("ARGUMENTS:\n");
    printf("  audio_file.wav     Path to a 16-bit PCM WAV file. If omitted,\n");
    printf("                     the program defaults to 'bitdream.wav'.\n\n");
    printf("OPTIONS:\n");
    printf("  -h, --help         Display this help menu and exit.\n\n");
    printf("CONTROLS:\n");
    printf("  [ESC]              Quit the visualizer.\n\n");
    printf("EXAMPLE:\n");
    printf("  %s my_music.wav\n", prog_name);
    printf("\n");
}

// Global Audio Context
typedef struct {
    Uint8* start;
    Uint32 length;
    Uint8* pos;
    Uint32 remaining;
} AudioContext;

AudioContext audio_ctx;

// Shared buffers for the visualizer
volatile int16_t vis_buffer_L[SCREEN_W];
volatile int16_t vis_buffer_R[SCREEN_W];

void audio_callback(void* userdata, Uint8* stream, int len) {
    AudioContext* ctx = (AudioContext*)userdata;

    if (ctx->remaining == 0) return;

    int copied = 0;
    while (len > 0) {
       int to_copy = (int)(((Uint32)len > ctx->remaining) ? ctx->remaining : (Uint32)len);
        SDL_memcpy(stream + copied, ctx->pos, to_copy);
        
        ctx->pos += to_copy;
        ctx->remaining -= to_copy;
        copied += to_copy;
        len -= to_copy;

        if (ctx->remaining == 0) {
            ctx->pos = ctx->start;
            ctx->remaining = ctx->length;
        }
    }

    int16_t* samples = (int16_t*)stream;
    
    // We divide by 2 because it's stereo (2 channels per frame)
    int sample_count = (copied / sizeof(int16_t)) / 2; 

    // --- NEW FIX: Sliding Window / Scrolling Oscilloscope ---
    // 1. Shift the old waveform to the left
    int shift = sample_count;
    if (shift > SCREEN_W) shift = SCREEN_W;
    int keep = SCREEN_W - shift;

    if (keep > 0) {
        memmove((void*)vis_buffer_L, (void*)(vis_buffer_L + shift), keep * sizeof(int16_t));
        memmove((void*)vis_buffer_R, (void*)(vis_buffer_R + shift), keep * sizeof(int16_t));
    }

    // 2. Append the newest audio samples to the right side
    int idx = keep;
    for (int i = 0; i < sample_count * 2 && idx < SCREEN_W; i += 2) {
        vis_buffer_L[idx] = samples[i];
        vis_buffer_R[idx] = samples[i + 1];
        idx++;
    }
}

// Track the height of the EQ bars for smooth gravity falloff
float eq_peaks[NUM_BARS] = {0};

int main(int argc, char* argv[]) {
    const char* filename = "bitdream.wav"; // Default filename

    // Parse Command Line Arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help_screen(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            // Treat the first non-flag argument as the filename
            filename = argv[i];
        } else {
            printf("Error: Unknown option '%s'\n", argv[i]);
            print_help_screen(argv[0]);
            return 1;
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return 1;

    SDL_AudioSpec wav_spec;
    Uint8* wav_buffer;
    Uint32 wav_length;

    // Load the parsed filename
    if (SDL_LoadWAV(filename, &wav_spec, &wav_buffer, &wav_length) == NULL) {
        printf("Failed to load '%s'!\nError: %s\n", filename, SDL_GetError());
        return 1;
    }

    // --- NEW FIX: Force a low-latency audio buffer ---
    wav_spec.samples = 512; // Lower buffer = faster visualizer updates

    audio_ctx.start = wav_buffer;

    audio_ctx.start = wav_buffer;
    audio_ctx.length = wav_length;
    audio_ctx.pos = wav_buffer;
    audio_ctx.remaining = wav_length;

    wav_spec.callback = audio_callback;
    wav_spec.userdata = &audio_ctx;

    SDL_AudioDeviceID device = SDL_OpenAudioDevice(NULL, 0, &wav_spec, NULL, 0);
    if (device == 0) return 1;

    Uint32 bytes_per_second = wav_spec.freq * wav_spec.channels * 2;

    SDL_Window* window = SDL_CreateWindow("SoundScope", 
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                          SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_PauseAudioDevice(device, 0);

    bool running = true;
    SDL_Event event;

    // Tracks the sliding scale of the EQ for Auto-Gain Control
    float global_peak = 0.1f; 

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = false;
        }

        // --- 1. DRAW CRT PHOSPHOR FADE ---
        SDL_SetRenderDrawColor(renderer, 5, 10, 5, 60); 
        SDL_RenderFillRect(renderer, NULL);

        // --- 2. DRAW WAVEFORMS (Upper Left and Upper Right) ---
        int half_w = SCREEN_W / 2;
        float scope_amp = (SCREEN_H * 0.15f); 
        int scope_y = SCREEN_H * 0.175f;      

        // Draw Left Channel
        SDL_SetRenderDrawColor(renderer, 50, 255, 100, 255);
        for (int x = 0; x < half_w - 1; x++) {
            int idx1 = x * 2;
            int idx2 = idx1 + 2;
            int y1 = scope_y - (int)((vis_buffer_L[idx1] / 32768.0f) * scope_amp);
            int y2 = scope_y - (int)((vis_buffer_L[idx2] / 32768.0f) * scope_amp);
            SDL_RenderDrawLine(renderer, x, y1, x + 1, y2);
        }

        // Draw Right Channel
        SDL_SetRenderDrawColor(renderer, 50, 200, 255, 255);
        for (int x = 0; x < half_w - 1; x++) {
            int idx1 = x * 2;
            int idx2 = idx1 + 2;
            int y1 = scope_y - (int)((vis_buffer_R[idx1] / 32768.0f) * scope_amp);
            int y2 = scope_y - (int)((vis_buffer_R[idx2] / 32768.0f) * scope_amp);
            SDL_RenderDrawLine(renderer, half_w + x, y1, half_w + x + 1, y2);
        }

        // Draw Dashboard Borders
        SDL_SetRenderDrawColor(renderer, 20, 80, 40, 150); 
        SDL_RenderDrawLine(renderer, half_w, 0, half_w, SCREEN_H * 0.35f);            
        SDL_RenderDrawLine(renderer, 0, SCREEN_H * 0.35f, SCREEN_W, SCREEN_H * 0.35f); 

        // --- 3. FFT FREQUENCY EQUALIZER (With Auto-Gain Control) ---
        float re[FFT_SIZE] = {0};
        float im[FFT_SIZE] = {0};
        
        int start_idx = SCREEN_W - FFT_SIZE;
        for (int i = 0; i < FFT_SIZE; i++) {
            float sample = (vis_buffer_L[start_idx + i] + vis_buffer_R[start_idx + i]) / 2.0f;
            sample /= 32768.0f;
            float window_func = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FFT_SIZE - 1)));
            re[i] = sample * window_func;
            im[i] = 0.0f;
        }

        compute_fft(re, im, FFT_SIZE);

        int bar_width = SCREEN_W / NUM_BARS;
        float eq_max_h = SCREEN_H * 0.65f - 50.0f; 
        
        // PASS 1: Analyze raw amplitudes
        float raw_amps[NUM_BARS] = {0};
        float current_frame_peak = 0.01f; 

        for (int b = 0; b < NUM_BARS; b++) {
            float freq_start = powf((float)b / NUM_BARS, 2.0f) * (FFT_SIZE / 2);
            float freq_end   = powf((float)(b + 1) / NUM_BARS, 2.0f) * (FFT_SIZE / 2);
            if ((int)freq_end <= (int)freq_start) freq_end = freq_start + 1.0f;

            float sum = 0.0f;
            for (int i = (int)freq_start; i < (int)freq_end && i < FFT_SIZE / 2; i++) {
                sum += sqrtf(re[i] * re[i] + im[i] * im[i]);
            }
            
            raw_amps[b] = sum / (freq_end - freq_start);
            if (raw_amps[b] > current_frame_peak) current_frame_peak = raw_amps[b];
        }

        // Apply Auto-Gain Control
        if (current_frame_peak > global_peak) {
            global_peak = current_frame_peak; 
        } else {
            global_peak *= 0.995f; 
        }
        if (global_peak < 0.05f) global_peak = 0.05f; 

        // PASS 2: Apply gravity and draw the bars
        for (int b = 0; b < NUM_BARS; b++) {
            float amplitude = (raw_amps[b] / global_peak) * (eq_max_h * 0.9f);

            if (amplitude > eq_peaks[b]) {
                eq_peaks[b] = amplitude; 
            } else {
                eq_peaks[b] -= 4.0f;     
            }
            if (eq_peaks[b] < 0) eq_peaks[b] = 0;

            int bar_h = (int)eq_peaks[b];
            if (bar_h > eq_max_h) bar_h = eq_max_h; 

            SDL_Rect bar_rect = {
                b * bar_width + 4,              
                SCREEN_H - 45 - bar_h,  
                bar_width - 8,                  
                bar_h                           
            };
            
            SDL_SetRenderDrawColor(renderer, 50, 255, 100, 200);
            SDL_RenderFillRect(renderer, &bar_rect);
        }

        // --- 4. CALCULATE AND DRAW TIMECODE (Bottom Left) ---
        Uint8* current_pos = audio_ctx.pos;
        Uint32 bytes_played = (Uint32)(current_pos - audio_ctx.start);
        long total_ms = (long)(((double)bytes_played / bytes_per_second) * 1000.0);
        
        int minutes = (total_ms / 60000);
        int seconds = (total_ms / 1000) % 60;
        int centi = (total_ms % 1000) / 10; 

        char time_str[32];
        snprintf(time_str, sizeof(time_str), "[ %02d:%02d.%02d ]", minutes, seconds, centi);

        int text_scale = 2;
        int text_x = 20;
        int text_y = SCREEN_H - 36;
        
        SDL_Rect bg_rect = { text_x - 5, text_y - 5, (strlen(time_str) * 8 * text_scale) + 10, (8 * text_scale) + 10 };
        SDL_SetRenderDrawColor(renderer, 5, 10, 5, 255); 
        SDL_RenderFillRect(renderer, &bg_rect);

        SDL_SetRenderDrawColor(renderer, 50, 255, 100, 255); 
        draw_osd_text(renderer, text_x, text_y, time_str, text_scale);

        // --- 5. CALCULATE AND DRAW FILENAME (Bottom Right) ---
        int fn_len = strlen(filename);
        int fn_text_x = SCREEN_W - (fn_len * 8 * text_scale) - 20; 
        
        SDL_Rect fn_bg_rect = { fn_text_x - 5, text_y - 5, (fn_len * 8 * text_scale) + 10, (8 * text_scale) + 10 };
        SDL_SetRenderDrawColor(renderer, 5, 10, 5, 255); 
        SDL_RenderFillRect(renderer, &fn_bg_rect);

        SDL_SetRenderDrawColor(renderer, 50, 255, 100, 255); 
        draw_osd_text(renderer, fn_text_x, text_y, filename, text_scale);

        SDL_RenderPresent(renderer);
    }

    SDL_CloseAudioDevice(device);
    SDL_FreeWAV(wav_buffer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
