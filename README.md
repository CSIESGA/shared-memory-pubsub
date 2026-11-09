# Shared-Memory Publish/Subscribe in C

A publish/subscribe channel that passes messages between Unix processes through memory-mapped shared storage. Process-shared POSIX mutexes and condition variables coordinate publishers, subscribers, and the reuse of message slots.

The repository includes a file publisher that sends chunks through the channel and a multithreaded subscriber that computes a SHA-256 Merkle root from the received chunks.

## Build

From the repository root on Linux with GCC and Make installed:

```sh
make P1=src/part1 P2=src/part2
```

This builds `build/file_publisher` and `build/merkle_subscriber`.
