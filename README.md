# OS-Scheduler

A CPU scheduling simulator built for the Operating Systems course (2020). A process
generator feeds a set of jobs to a scheduler over System V IPC, and the scheduler runs
them under one of three classic algorithms while an emulated clock drives the timing.

> **Phase 1 of a two-phase project.** This repository implements the CPU scheduler.
> Phase 2 adds a memory manager on top of the same simulation and is the more complete,
> later artifact — see
> [os-memory-management](https://github.com/MohammedEl-sayedAhmed/os-memory-management).

## Overview

The simulation is split into four cooperating programs that run as separate processes:

| Component | Source | Role |
|-----------|--------|------|
| Process generator | `src/process_generator.c` | Reads `processes.txt`, prompts for the algorithm (and quantum for RR), forks the clock and the scheduler, then releases each process to the scheduler at its arrival time. |
| Clock | `src/clk.c` | An emulated system clock. Holds a single integer in shared memory and increments it once per real second. |
| Scheduler | `src/scheduler.c` | Receives processes from the generator, runs the chosen algorithm, forks a worker per process, preempts/resumes them, logs events, and computes performance metrics. |
| Process | `src/process.c` | A workload process. Attaches to the clock and busy-waits until its assigned run time has elapsed, then signals the scheduler that it has finished. |

Supporting data structures live in headers: a generic linked-list queue
(`src/Queue.c` / `include/Queue.h`), a linked-list priority queue
(`include/PriorityQueue.h`), and the process control block (`include/PCB.h`).
`include/headers.h` holds the clock and message-queue helpers shared by every
component. `src/test_generator.c` is a small utility that writes a random
`processes.txt`.

## Scheduling algorithms

- **HPF — Highest Priority First (non-preemptive).** Jobs are ordered in a priority
  queue keyed by their priority value (lower number = higher priority) and each runs to
  completion before the next is dispatched.
- **SRTN — Shortest Remaining Time Next (preemptive).** Jobs are ordered by remaining
  run time; a newly arrived shorter job preempts the running one.
- **RR — Round Robin (preemptive).** A FIFO ready queue with a fixed time quantum; a job
  that does not finish within its quantum is stopped and re-enqueued at the tail.

## Inter-process communication

- **Shared memory** — the clock publishes the current time in a shared-memory segment
  (key `300`); every component attaches to it to read the time.
- **Message queue** — a one-way System V message queue (key `13245`) carries process
  control blocks from the generator to the scheduler. A sentinel PCB (`pid = -10`) marks
  the end of the input stream.
- **Signals** — the scheduler preempts and resumes workers with `SIGSTOP` / `SIGCONT`, a
  worker reports completion to the scheduler with `SIGUSR1`, and `SIGINT` drives the
  system-wide resource cleanup and teardown.

## Building

Requires `gcc` and a Linux system with System V IPC (shared memory, message queues).

```sh
make build   # compile all components (adds -Wall -Wextra)
make         # clean, then build
make clean   # remove binaries and generated output (keeps processes.txt)
```

## Running

1. Provide an input file. Either edit `processes.txt` by hand, copy one from
   `examples/`, or generate a random one:

   ```sh
   ./test_generator.out      # prompts for the number of processes, writes processes.txt
   ```

2. Start the simulation:

   ```sh
   make run                  # runs ./process_generator.out
   ```

3. When prompted, choose an algorithm (`HPF`, `SRTN`, or `RR`). For `RR`, also enter the
   time quantum.

The scheduler writes two output files:

- `SchedulerLog.txt` — a per-event trace (started / stopped / resumed / finished lines
  with arrival, total, remaining, and waiting times).
- `SchedulerCalc.txt` — aggregate metrics: CPU utilization, average weighted turnaround
  time, average waiting time, and the standard deviation of weighted turnaround time.

A captured sample run for each algorithm is kept as reference evidence in
[`examples/sample-output/`](examples/sample-output/) — the `hpf-*`, `srtn-*`, and `rr-*`
scheduler-log / metrics pairs.

## Input format (`processes.txt`)

A header line beginning with `#`, followed by one line per process with four
whitespace-separated fields:

```
#id arrival runtime priority
1	4	3	3
2	6	10	1
3	20	12	2
```

| Field | Meaning |
|-------|---------|
| `id` | Process identifier |
| `arrival` | Arrival time (in clock ticks) |
| `runtime` | Total CPU time the process needs |
| `priority` | Scheduling priority (lower value = higher priority; used by HPF) |

Additional sample inputs are kept in the `examples/` directory.

## Authors

- [Mohammed El-sayed Ahmed](https://github.com/MohammedEl-sayedAhmed)
- [Nadine Amin](https://github.com/nadine-amin)
- [Rahma](https://github.com/Rahma2015)
- [Tasneem Omara](https://github.com/TasneemOmara)
