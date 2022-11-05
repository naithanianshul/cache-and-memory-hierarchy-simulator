# cache-memory-hierarchy-simulator
 Implemention of a flexible cache and memory hierarchy simulator.
 The cache module also implements the functionality of stream buffers. The caches use Write-Back Write-Allocate (WBWA) write policy and Least-Recently-Used (LRU) replacement policy.

To run the simulator first compile the code by executing the command 'make'.
The format of the command to run the cache simulation is:

```   ./sim 
           <block_size>
           <L1_cache_size>
           <L1_associativity>
           <L2_cache_size>
           <L2_associativity>
           <no_of_stream_buffers>
           <no_of_memory_blocks_in_each_stream_buffer>
           <trace_file>```
Example - The cache specifications are:
* 32B block size
* 8KB 4-way set-associative L1 cache
* 256KB 8-way set-associative L2 cache
* L2 prefetch unit with 3 stream buffers with 10 blocks in each stream buffer
* Addresses to be accesses present in 'gcc_trace.txt' file 

```./sim 32 8192 4 262144 8 3 10 gcc_trace.txt```

The functionality of Prefetching can be disabled by setting <no_of_stream_buffers> and <no_of_memory_blocks_in_each_stream_buffer> as 0.

