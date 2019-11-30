# IOFS -- The I/O file system - A FUSE file system developed for I/O monitoring

The IOFS ...

The code also contains a Grafana reporting submodule that can be easily integrated into any FUSE module.
You may use the code in iofs-monitor.h and elasticsearch.c to integrate it into your own FUSE module.

# Usage

Please use absolute paths for source and target directories:
```
iofs <absolute_path_target> <absolute_path_source>
```

# Preparation of the Grafana schema


