# Two-Level Cache Simulation

## Problem Statement

The problem involves simulating a **two-level cache system** with the following specifications:

- **Processor Address**: 16 bits
- **Main Memory**: 64K Words

### Level 1 Cache (L1)
- **Type**: Direct Mapped
- **Size**: 2K Words
- **Block Size**: 16 Words (64 bits per word)

### Level 2 Cache (L2)
- **Type**: 4-Way Set-Associative
- **Size**: 16K Words

### Additional Components:
- **Write Buffers**: 4 blocks
- **Victim Cache**: 4 blocks
- **Prefetch Cache (Instruction Stream Buffer)**: 4 blocks
- **Prefetch Cache (Data Stream Buffer)**: 4 blocks

## Project Overview

This project implements a two-level cache system consisting of:

1. **Level 1 Cache (L1)**: A direct-mapped cache with a size of 2K words and a block size of 16 words. 
2. **Level 2 Cache (L2)**: A 4-way set-associative cache with a size of 16K words.
3. **Additional Caches**: Write buffers, victim cache, and prefetch caches to improve system performance by optimizing memory access.

The simulator compares the current architecture with a fully associative cache design, implementing L1 and L2 cache behavior, as well as various buffer and prefetch strategies.

## Code Explanation

The code simulates the behavior of the two-level cache system, considering the following features:

- **Direct Mapped Cache (L1)**: The L1 cache is direct-mapped, meaning each memory block maps to exactly one cache line.
- **4-Way Set-Associative Cache (L2)**: The L2 cache uses 4-way set-associative mapping, where each block in memory can be placed in one of four possible slots.
- **Write Buffers**: These buffers hold data temporarily to avoid blocking the CPU during write operations.
- **Victim Cache**: A small cache that holds blocks evicted from the L1 cache before being written back to the main memory.
- **Prefetch Caches**: Separate instruction and data stream buffers to prefetch and store blocks likely to be accessed soon.

## Simulation Details

The simulation involves the following access patterns:
- **Spatial Access**: Simulates sequential memory access.
- **Temporal Access**: Simulates accessing the same memory locations multiple times.
- **Mixed Access**: A combination of spatial and temporal access.

The code includes the following components:

- **CacheBlock Class**: Defines the structure of a cache block, including whether the block is valid, dirty, the tag, and the data it holds.
- **Cache Class**: A base class for cache implementations, handling cache accesses, miss counts, and search statistics.
- **DirectMappedCache Class**: Inherits from `Cache` and implements direct-mapped cache access.
- **SetAssociativeCache Class**: Inherits from `Cache` and implements set-associative cache access.
- **TwoLevelCache Class**: Combines both L1 and L2 caches, implements write buffers, victim cache, and prefetch caches, and simulates the overall cache behavior.