# kb-networking-cpp

## Build from source

Add an env variable for the `kbnetworkingcpp` vcpkg overlay:
```bash
export VCPKG_OVERLAY_PORTS=PATH/TO/kb-networking-cpp/vcpkg-overlays
```

Set up the build files:
`cmake --preset default -B ./build`

After build CMake files:
`cmake --build build --config Release -j --clean-first --target kb-networking kb-networking-cli`