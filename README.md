# soundscope

A real-time oscilloscope, we need to strip away SDL_mixer and use SDL2's raw audio callback.

Instead of just telling SDL to play a file and forgetting about it, the callback forces SDL to "ask" our program for audio data a fraction of a second before it hits the speakers. We can intercept this 16-bit PCM waveform data, copy it to a visualizer buffer, and draw it to the screen exactly as it plays.

For this demo, we will use a .wav file (since SDL2 has a built-in WAV loader). We'll also use an alpha-blended background clear to simulate the glowing phosphor fade of a classic CRT monitor.
