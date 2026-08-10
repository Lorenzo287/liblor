# Tracing

`lor/trace.h` records nested timing zones, instant events, and integer counters
into Chrome Trace Event JSON. The output opens directly in Perfetto and other
viewers that accept Chrome traces.

Tracing is instrumentation rather than statistical sampling. Sampling
profilers such as WPA remain separate tools and need no liblor runtime module.

## Manual Tracing

Create one process trace and one recorder for each participating thread:

```c
LorTrace trace = LOR_TRACE_INIT;
LorTraceConfig config = LOR_TRACE_CONFIG_INIT;
config.process_name = "example";

if (lor_trace_init_file(&trace, "trace.json", config) != LOR_STATUS_OK)
    return 1;

LorTraceThread thread = LOR_TRACE_THREAD_INIT;
if (lor_trace_thread_init(&thread, &trace, "main") != LOR_STATUS_OK) {
    lor_trace_deinit(&trace);
    return 1;
}

LOR_TRACE_BEGIN(&thread, "load");
LOR_TRACE_COUNTER(&thread, "items", 42);
LOR_TRACE_INSTANT(&thread, "ready");
LOR_TRACE_END(&thread);

LorStatus status = lor_trace_thread_finish(&thread);
if (status == LOR_STATUS_OK)
    status = lor_trace_finish(&trace);
```

Names are copied into the per-thread buffer, so literals are not required.
Event recording performs no allocation after thread initialization. Separate
thread objects may record concurrently, while one object is owned by one
operating-system thread.

`lor_trace_finish` requires every associated thread to be finished first.
Unclosed zones are closed by `lor_trace_thread_finish` to keep the JSON trace
valid, but it returns `LOR_STATUS_INVALID_ARGUMENT` to report the mismatch.

## Buffering

Each thread receives a fixed byte buffer. The default is 1 MiB.

- `LOR_TRACE_BUFFER_FLUSH` flushes to the shared file when full. Normal event
  recording is allocation-free and lock-free until a flush is needed.
- `LOR_TRACE_BUFFER_DROP` never performs file I/O from a full-buffer event.
  New events are omitted and counted in `dropped_events`.

The recorder reserves space for matching end events. A dropped begin suppresses
its nested events until the corresponding end, preventing malformed nesting.
Use `lor_trace_thread_stats` to inspect counts and current buffer use.

`lor_trace_thread_flush` serializes through the process trace and flushes the C
stream. Different thread buffers may be flushed concurrently.

## Scope Cleanup

Compilers for which `LOR_HAS_CLEANUP_ATTRIBUTE` is nonzero support a
cleanup-based scope form:

```c
#if LOR_TRACE_SCOPE_SUPPORTED
LOR_TRACE_SCOPE(&thread, "parse");
#endif
```

`LOR_TRACE_FUNCTION(&thread)` uses `__func__` as the name. These forms close
the zone on normal scope exit, including early `return`. Explicit begin/end
calls remain the portable interface.

Define `LOR_TRACE_DISABLED` before including the header to compile the
uppercase instrumentation macros to no-ops without evaluating their arguments.
The ordinary `lor_trace_*` functions remain available.

## Automatic Function Tracing

The same module can implement GCC and Clang's
`-finstrument-functions` callbacks. Define `LOR_TRACE_AUTO` while compiling
both `src/trace.c` and the instrumented program:

```powershell
clang -Iinclude -std=c11 -g -gcodeview -DLOR_TRACE_AUTO `
    -finstrument-functions -c src/trace.c -o trace_auto.o
clang -Iinclude -std=c11 -g -gcodeview -DLOR_TRACE_AUTO `
    -finstrument-functions app.c trace_auto.o -ldbghelp -o app.exe
```

Bind an initialized thread recorder before calling code to measure:

```c
lor_trace_auto_bind(&thread);
run_work();
lor_trace_auto_unbind();
```

Binding is thread-local. Every participating operating-system thread must own
and bind a separate `LorTraceThread`. `lor_trace_auto_disable` and
`lor_trace_auto_enable` temporarily pause and resume callbacks.

On Windows, automatic tracing uses DbgHelp and available PDB information to
resolve addresses when buffers are written. Link `dbghelp` as shown above.
Unresolved addresses and all current non-Windows automatic events use
hexadecimal names.

liblor currently implements the `__cyg_profile_func_enter` and
`__cyg_profile_func_exit` backend used by Clang and GCC. Native MSVC has
different `/Gh` and `/GH` hooks, for which liblor does not yet provide a
backend. Check `LOR_TRACE_AUTO_SUPPORTED`; automatic instrumentation can
produce large traces and changes program timing, so manual zones remain
preferable for long-running or high-frequency code.

See `examples/trace.c`.
