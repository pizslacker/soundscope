soundscope: soundscope.c
	gcc -Wall -Wextra -O3 -o soundscope soundscope.c -lSDL2
	strip soundscope

clean:
	rm -f soundscope
