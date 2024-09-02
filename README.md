# sandy


### Compile shader and build
```shell
// linux
make linux

// mac
make mac-opengl

// wasm
make wasm

```

### Profile
```shell
// run relevant build command from makefile with -g -O0 flags
perf record -g ./sandy
perf report

```

### Adding new materials

- Add material name to material enum
- Define material color in `draw_material`
- Create `process_<material_name>(int index)` function to define behavior
- Call process function in `process_world()`
- Add button to select material in `draw_ui`
