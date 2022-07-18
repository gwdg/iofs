# IOFS -- The I/O file system - A FUSE file system developed for I/O monitoring

The IOFS ...

The code also contains a Grafana reporting submodule that can be easily integrated into any FUSE module.
You may use the code in iofs-monitor.h and elasticsearch.c to integrate it into your own FUSE module.

# Dependencies
- `libcurl`
- `fuse3`
- `libfuse3-dev`

# Build
```
mkdir build && cd build
cmake ..
make
make install
```
You can set the install path with
```
cmake --install-prefix=/opt/iofs/
```
To set the default path for the iofs config file, use
```
cmake --DCONFIG_PATH=/opt/iofs/etc/iofs.conf ..
```

# Usage

Please use absolute paths for source and target directories:
```
iofs <absolute_path_target> <absolute_path_source>
```

# Configuration

All possible configuration settings can be seen via `iofs --help` or in the [example.conf](./example.conf).

Everything except the mount points can be both configured via CLI parameters or config file. Note that the CLI parameters
have a higher precedence.

There are 2 ways to use a configuration file:
1. Define the configuration file path at compile time, see above.
2. Set the `IOFS_CONFIG_PATH` environment variable to the proper path.

# Local Environment

See `./docker-devenv/README.md`.
