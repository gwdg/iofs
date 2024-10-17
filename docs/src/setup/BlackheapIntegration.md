# Classify I/O with blackheap

[blackheap](https://github.com/lquenti/blackheap) can be used together with iofs to automatically classify incoming I/O requests based on their predicted characteristics.

This allows for aggregate remote I/O analysis of multiple nodes. It currently supports any monitoring system that is based on Influxdb. In our internal use case, it is used in combinaton with Grafana.

Again, no changes to the recorded software binary is required, although some configuration may be needed in order to write to the correct mount point.

## Setup

Here we have a the chicken and the egg type of problem: On the one hand, we want to do access classification based on the expected values of the FUSE file system. On the other hand, we theoretically already need those classifications before starting iofs. [The the accompanying (open) issue if interested.](https://github.com/gwdg/iofs/issues/6).

As mentioned in that issue, there are basically two solutions. For now, this documentation will rely on the first idea since it cannot be assumed that one has that much computing time to create two seperate classification models.

### Step 0: If Influx is run locally, start Influx beforehand

This is done in the beginning to have an approximately same constant background load in order to have a more realistic classification.

We provide a full docker-compose setup for InfluxDB+Grafana [in the iofs repository](https://gwdg.github.io/iofs/book/setup/LocalGrafana.html).

### Step 1: Build iofs; mount it with fake classifications

Of course, iofs has to be built first. See [the related iofs documentation](https://gwdg.github.io/iofs/book/setup/Installation.html) for more.

After that, iofs should be mounted with a fake set of models in order to have the approximately correct overhead for I/O requests. Download them here related to their CLI parameter name:

- constant-linear (DEFAULT): [here](../assets/dummymodels/constant-linear.csv)
- linear: [here](../assets/dummymodels/linear.csv)

After that, mount iofs via

```
./iofs /new/mountpoint /where/real/data/is --classificationfile=/path/to/dummy.csv
```

or, if InfluxDB should be used:

```
iofs /new/fuse/mountpoint /where/real/data/is --classificationfile=/path/to/dummy.csv --in-server="http://influx_server:8086" --in-db=mydb
```

### Step 2: Create a performance model using the fake initial classifications

Note: This, of course, will stream wrong data to Influx. But that is okay. We just want to have the correct data produced by blackheap, which requires the same request time overhead.

So, now the correct performance model can be created using blackheap. This requires that the benchmark is done on the iofs FUSE file system, which can be controlled via the `--file` parameter. Therefore, the minimal call would be

```
blackheap <PATH_TO_MODEL> --file /path/to/somewhere/within/the/mountpoint
```

and afterwards run `<PATH_TO_MODEL>/build_models.py` to create all models:

### Step 3: Remount iofs with the real classifications

Afterwards, it should record and classify the I/O requests according to the performance model created.
