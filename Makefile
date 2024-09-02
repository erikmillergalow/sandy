
linux:
	../sokol-tools-bin/bin/linux/sokol-shdc -i basic.glsl -o basic.glsl.h -l glsl430
	cc main.c -o sandy -Iinclude -lGL -ldl -lm -lX11 -lasound -lXi -lXcursor

mac-opengl:
	../sokol-tools-bin/bin/linux/sokol-shdc -i basic.glsl -o basic.glsl.h -l glsl430
	clang main.c sokol.m -o sandy -framework OpenGL -framework Cocoa -framework AudioToolbox

wasm:
	../sokol-tools-bin/bin/linux/sokol-shdc -i basic.glsl -o basic.glsl.h -l glsl300es
	emcc main.c -o sandy.html -sUSE_WEBGL2 --shell-file=shell.html

.PHONY: clean
clean:
	rm -f sandy
