# Command benchmark

Build and run the Linux Vulkan benchmark from the repository root:

```sh
mise exec -- cmake --build build/linux-gnu-x64-vulkan --target mln_command_bench
mise exec -- build/linux-gnu-x64-vulkan/tests/native/bench/mln_command_bench 200
```

The target is excluded from the default build and from CTest. The optional
argument sets the measured repetitions after five warmup repetitions per case.
The output is CSV; native debug logs go to stderr.

The benchmark attaches a 64 × 64 owned texture target to a continuous map. A
separate graphics thread services a caller-driver session, acquires and releases
each rendered frame, and re-arms a demand with `IF_NEEDED | WAIT_FOR_UPDATE`
after each result. The style has one background layer, zero transition duration,
and no sources or network resources.

A gate holds the runtime worker while each burst is submitted. This measures the
transport of a queued burst without worker scheduling races between its
commands. Submission latency covers each C call on the submitting thread.
Completion latency starts at that call and ends at its callback, including the
time behind the gate and earlier commands. A discarded command has no callback;
its completion column ends at the later map query that fences the burst.

The image case replaces 64 warm image IDs with 8 × 8 RGBA images. The image-256
case replaces one warm ID with a 256 × 256 RGBA image. These cases measure
submission copies and command transport, without tile layout or image loading.

The executable replaces `operator new` and `operator new[]`. Submission counts
cover the submitting thread during the C calls. Total counts cover that thread
and the runtime worker until the burst's fencing query completes, including the
query's allocation cost. The graphics host, GPU driver, aligned allocation, and
direct C or Rust allocator calls are outside these counts. Frame counts cover
each burst through a driver fence after completion. Compare runs built with the
same toolchain; allocator and scheduler differences affect the results.
