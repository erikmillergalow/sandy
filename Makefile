
linux:
	../sokol-tools-bin/bin/linux/sokol-shdc -i shader/basic.glsl -o include/basic.glsl.h -l glsl430
	cc main.c -o sandy -Iinclude -lGL -ldl -lm -lX11 -lasound -lXi -lXcursor

mac-opengl:
	../sokol-tools-bin/bin/linux/sokol-shdc -i shader/basic.glsl -o include/basic.glsl.h -l glsl430
	clang main.c lib/sokol.m -o sandy -Iinclude -framework OpenGL -framework Cocoa -framework AudioToolbox

wasm:
	../sokol-tools-bin/bin/linux/sokol-shdc -i shader/basic.glsl -o include/basic.glsl.h -l glsl300es
	emcc main.c -o sandy.html -Iinclude -sUSE_WEBGL2 --shell-file=web/shell.html

.PHONY: clean
clean:
	rm -f sandy
