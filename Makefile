
linux:
	cc main.c -o sandy -Iinclude -lGL -ldl -lm -lX11 -lasound -lXi -lXcursor

mac-opengl:
	clang main.c sokol.m -o sandy -framework OpenGL -framework Cocoa -framework AudioToolbox

.PHONY: clean
clean:
	rm -f sandy
