# Installation

## Dependencies

The following dependencies are required:

- `libcurl`
- `fuse3`
- `libfuse3-dev`

## Build Process

The build process is pretty straight forward

```
mkdir build && cd build
cmake ..
make
```

The binary can now be found at `./build/src/iofs`. In order to automatically install it, use
```
make install
```

### Further options

You can set the install path with
```
cmake --install-prefix=/opt/iofs/
```

To set the default path for the iofs config file, use
```
cmake --DCONFIG_PATH=/opt/iofs/etc/iofs.conf ..
```
