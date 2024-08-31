# sandy

### build
```shell
../sokol-tools-bin/bin/linux/sokol-shdc -i basic.glsl -o basic.glsl.h -l glsl430

gcc main.c -o sandy -Iinclude -lGL -ldl -lm -lX11 -lasound -lXi -lXcursor
```
