# IOFS -- The I/O file system - A FUSE file system developed for I/O monitoring

## LINK TO DOCUMENTATION (FOLLOWING SOON)

The IOFS ...

The code also contains a Grafana reporting submodule that can be easily integrated into any FUSE module.
You may use the code in iofs-monitor.h and elasticsearch.c to integrate it into your own FUSE module.

# Configuration

All possible configuration settings can be seen via `iofs --help` or in the [example.conf](./example.conf).

Everything except the mount points can be both configured via CLI parameters or config file. Note that the CLI parameters
have a higher precedence.

There are 2 ways to use a configuration file:
1. Define the configuration file path at compile time, see above.
2. Set the `IOFS_CONFIG_PATH` environment variable to the proper path.

# Local Environment

See `./docker-devenv/README.md`.
