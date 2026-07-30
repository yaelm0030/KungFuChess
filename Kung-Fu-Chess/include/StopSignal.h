#pragma once

// SIGINT/SIGTERM handling for the standalone --serve-shard/--serve-gateway processes
// (docker stop sends SIGTERM). POSIX consumes the signal synchronously via sigwait()
// instead of an async handler + poll, which avoids losing a signal delivered to an
// arbitrary thread in a multi-threaded process; Windows keeps the poll-based fallback.

// Blocks SIGINT/SIGTERM in the calling thread's mask (POSIX only; no-op on Windows).
// Must be called before spawning any child threads, since a thread's signal mask is
// inherited from its creator only at creation time.
void block_stop_signals();

// Blocks until SIGINT/SIGTERM (docker stop sends SIGTERM) instead of stdin: under
// Compose/`docker run -d` there's no TTY, so stdin closes immediately and a
// getline-based stop would exit the process right after it starts.
void wait_for_stop_signal();
