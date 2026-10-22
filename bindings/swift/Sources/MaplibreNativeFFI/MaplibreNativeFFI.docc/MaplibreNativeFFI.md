# `MaplibreNativeFFI`

The low-level Swift binding for the MapLibre Native C API.

## Overview

An `async` operation submits its work to native before it first suspends, so
operations that one actor awaits in order reach native in that order. Tasks
reach native in the order that their executor runs them: `Task.immediate`
submits before it returns, and `Task {}` enqueues its operation, which another
actor than the main actor can run after a task of higher priority.

Start a command group's begin, its commands, and its end as tasks on one actor,
then await their completions. The group holds the map's render updates until its
end, so awaiting each command before starting the next stalls rendering.

Cancelling a task that awaits an operation ends the wait with
`CancellationError`, but the native work still finishes. The binding disposes a
value that arrives afterwards, such as a created map.
