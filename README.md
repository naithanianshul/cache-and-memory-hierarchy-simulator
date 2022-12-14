# cache-and-memory-hierarchy-simulator
Implemention of a flexible cache and memory hierarchy simulator.<br/>
The cache module also implements the functionality of stream buffers. The caches use Write-Back Write-Allocate (WBWA) write policy and Least-Recently-Used (LRU) replacement policy.

To run the simulator first compile the code by executing the command 'make'.<br/>
The format of the command to run the cache simulation is:

>./sim<br/>
>&emsp;&emsp;<block_size><br/>
>&emsp;&emsp;<L1_cache_size><br/>
>&emsp;&emsp;<L1_associativity><br/>
>&emsp;&emsp;<L2_cache_size><br/>
>&emsp;&emsp;<L2_associativity><br/>
>&emsp;&emsp;<no_of_stream_buffers><br/>
>&emsp;&emsp;<no_of_memory_blocks_in_each_stream_buffer><br/>
>&emsp;&emsp;<trace_file><br/>

Example - The cache specifications are:
* 32B block size
* 8KB 4-way set-associative L1 cache
* 256KB 8-way set-associative L2 cache
* L2 prefetch unit with 3 stream buffers with 10 blocks in each stream buffer
* Addresses to be accessed present in 'gcc_trace.txt' file 

```./sim 32 8192 4 262144 8 3 10 gcc_trace.txt```

The functionality of Prefetching can be disabled by setting **<no_of_stream_buffers>** and **<no_of_memory_blocks_in_each_stream_buffer>** as 0.
