# How to Use (In standalone mode)

[Once compiled](./Installation.md), `iofs` can be executed via

```
iofs <absolute_path_target> <absolute_path_source>
```

Example: If you have your real data on `/data/my-data`, and want to measue it by accessing `/opt/iofs-data`, you'd use
```
iofs /opt/iofs-data /data/my-data
```

## Further Configuration

All possible configuration settings can be seen via `iofs --help` or in the [example configuration](../assets/example.conf).

Everything except the mount points can be both configured via CLI parameters or config file. Note that the CLI parameters have a higher precedence.

There are 2 ways to use a configuration file:
1. Define the configuration file path [at compile time](./Installation.md).
2. Set the `IOFS_CONFIG_PATH` environment variable to the proper path, i.e.

```
IOFS_CONFIG_PATH=/home/user/example.conf iofs /opt/iofs-data /data/my-data
```

See [here](../assets/example.conf) for a documented example.
