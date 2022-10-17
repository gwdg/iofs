# Workflows

## Workflow Overview

Here are some Graph representations of the workflows.

Firstly, the overall overview:

![Overview on how the functions call each other](../assets/graphs/general_flow.png)

Secondly, the reporting thread internals:

![Overview on how the reporting thread works](../assets/graphs/reporthing_thread.png)

## More In-Depth Overview

This section will be split into three parts:

1. How does the program get initialized?
2. What happens if someone does a I/O request of any kind?
3. How is the data getting streamed into a TSDB?

### How does the program get initialized?

At first, the main parses all arguments [given](../setup/HowToUse.md) via cli arguments or configuration file, with the cli having higher precedence. Afterwards, it starts the [`fuse_main()`](https://libfuse.github.io/doxygen/fuse_8h.html).

The `fuse_main()` is configured by a so-called [`fuse_operations`](https://libfuse.github.io/doxygen/structfuse__operations.html) struct. This struct has a bunch of function pointers, which represent the functions required to be implemented by an FUSE filesystem (some are optional). It additionally defines some other functions, like a `init` and `destroy` function for starting and stopping the file system.

So, in order to initialize the FUSE file system, the `fuse_main()` calls our `cache_init()`, which is just a wrapper around our real `monitor_init()`.

The `monitor_init()` initializes all aggregation data structures as well as [cURL](https://libfuse.github.io/doxygen/structfuse__operations.html) for the REST data transfer. After that, it starts the `reporting_thread()` as a seperate `pthread`, which runs until the filesystem destroys itself via `cache_destroy()`.

### What happens if someone does a I/O request of any kind?

We already gave a higher level overview in the [introduction](../Introduction.md). But it works as follows:

At first, the user/caller calls any usual I/O function provided by the Linux kernel. This gets processed by the Linux VFS, which matches the proper kernel module for the file system. In our case, this is the FUSE kernel module.

The FUSE kernel module then calls the approprivate function given by our [`fuse_operations` struct](https://libfuse.github.io/doxygen/structfuse__operations.html). All our functions have the `cache_` prefix, thus calling the `X` function would end in `cache_X()`. This is only our personal convention.

After `cache_X` is called, we get the fake path prefix, which we have to map to where the file actually exists.

For example: We mounted `/data/real-data` to `/opt/fuse-playground`. Thus we get an I/O request for `/opt/fuse-playground/some-path/data.dat`, which we internally have to map to `/data/real-data/some-path/data.dat`.

After that is done, we can just call the same function `X` for our new path. This again gets passed through the VFS, but this time gets processed by the real file system kernel module.

When arriving the result, we track how long it took, and save the recorded data in our global structs. More on those in the next part. Lastly, we just return the same information we got back to the caller.

### How is the data getting streamed into a TSDB?

This is done by the aforementioned `reporting_thread()` method. This function runs in a endless loop until some state gets toggled by the `cache_destroy()` which is used to cleanup a filesystem before shutting down.

Every interval, which is chosen by the user, the function sends all aggregated data to the sources configured by the user.

This is pretty straightforward and the code is easily readable, as we just format and send our data in such a way to comply with Influx.

But one thing is still worth mentioning: How the data formatting and transfer is done non-blockingly without producing strongly inaccurate data.

This is done by having the global state two times. In essence, it works as follows:

- State 1 is ready to be sent
- Tell the program to send all data to state 2
- Process state 1, send it, wait until the interval is done
- Switch back to state 1 and process state 2
