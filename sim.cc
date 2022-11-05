#include "sim.h"
#include <bitset>

class CacheHierarchy
{
	public:
		cache_params_t params;

	long long main_memory_accesses = 0;

	int printSB = 0;

	Cache L1;
	Cache L2;

	ADDRESS address_bits_L1;
	ADDRESS address_bits_L2;

	CacheHierarchy(cache_params_t params)
	{
		this->params = params;
		// Initialize L1 and L2 Cache
		if (params.L1_SIZE != 0)
		{
			L1.active = 1;
			if (params.L2_SIZE == 0)
			{
				//print("No L2 cache. Only L1 cache. Stream Buffers exist at L1.");
				this->L1.initializeCache(1,params.L1_SIZE, params.BLOCKSIZE, params.L1_ASSOC, 32, params.PREF_N, params.PREF_M);
				
				//std::cout << "L1: " << L1.no_tag_bits << " " << L1.no_index_bits << " " << L1.no_block_offset<< '\n';
			}
			else
			{
				//print("L1+L2 cache. Stream Buffers exist at L2.");
				this->L1.initializeCache(1,params.L1_SIZE, params.BLOCKSIZE, params.L1_ASSOC, 32, 0, 0);
				L2.active = 1;
				this->L2.initializeCache(2,params.L2_SIZE, params.BLOCKSIZE, params.L2_ASSOC, 32, params.PREF_N, params.PREF_M);

				//std::cout << "L1: " << L1.no_tag_bits << " " << L1.no_index_bits << " " << L1.no_block_offset<< '\n';
				//std::cout << "L2: " << L2.no_tag_bits << " " << L2.no_index_bits << " " << L2.no_block_offset<< '\n';
			}
		}
	}

	void printAddressSplit(ADDRESS address_bits, std::string caption = "Address Split")
	{
		std::cout << caption << ": " << address_bits.tag_bits << "(" << address_bits.tag_bits.length() << ") ";
		std::cout << address_bits.index_bits << "(" << address_bits.index_bits.length() << ")[" << std::stol(address_bits.index_bits, nullptr, 2) << "] ";
		std::cout << address_bits.block_offset_bits << "(" << address_bits.block_offset_bits.length() << ")" << '\n';
		std::cout << "Block Address: " << address_bits.block_address_bits << '\n';
	}

	void clearAddressSplit()
	{
		this->address_bits_L1.address_bits = "";
		this->address_bits_L1.tag_bits = "";
		this->address_bits_L1.index_bits = "";
		this->address_bits_L1.block_address_bits = "";
		this->address_bits_L1.block_offset_bits = "";

		if (L2.active == 1)
		{
			this->address_bits_L2.address_bits = "";
			this->address_bits_L2.tag_bits = "";
			this->address_bits_L2.index_bits = "";
			this->address_bits_L2.block_address_bits = "";
			this->address_bits_L2.block_offset_bits = "";
		}
	}

	void splitAdress(std::string addressBinary)
	{
		//print(addressBinary, "Origional Address: ");
		this->address_bits_L1.address_bits = addressBinary;
		for (long long i = 0; i < L1.no_tag_bits; i++)
		{
			this->address_bits_L1.tag_bits += addressBinary[i];
		}

		for (long long i = L1.no_tag_bits; i < L1.no_tag_bits + L1.no_index_bits; i++)
		{
			this->address_bits_L1.index_bits += addressBinary[i];
		}

		for (long long i = L1.no_tag_bits + L1.no_index_bits; i < L1.no_tag_bits + L1.no_index_bits + L1.no_block_offset; i++)
		{
			this->address_bits_L1.block_offset_bits += addressBinary[i];
		}

		this->address_bits_L1.block_address_bits = bin2hex(address_bits_L1.tag_bits + address_bits_L1.index_bits);

		//printAddressSplit(address_bits_L1, "L1 Address Split");

		if (L2.active == 1)
		{
			this->address_bits_L2.address_bits = addressBinary;
			for (long long i = 0; i < L2.no_tag_bits; i++)
			{
				this->address_bits_L2.tag_bits += addressBinary[i];
			}

			for (long long i = L2.no_tag_bits; i < L2.no_tag_bits + L2.no_index_bits; i++)
			{
				this->address_bits_L2.index_bits += addressBinary[i];
			}

			for (long long i = L2.no_tag_bits + L2.no_index_bits; i < L2.no_tag_bits + L2.no_index_bits + L2.no_block_offset; i++)
			{
				this->address_bits_L2.block_offset_bits += addressBinary[i];
			}

			this->address_bits_L2.block_address_bits = bin2hex(address_bits_L2.tag_bits + address_bits_L2.index_bits);

			//printAddressSplit(address_bits_L2, "L2 Address Split");
		}
	}

	bool searchStreamBuffer(Cache C, std::string block_address)
	{
		for (long long l = 0; l < C.pref_n; l++)
		{
			for (long long i = 0; i < C.pref_n; i++)
			{
				if ( (C.stream_buffer_state[i]["LRUC"] == int2str(l)) && (C.stream_buffer_state[i]["V"] == "1") )
				{
					for (long long j = 0; j < C.pref_m; j++)
					{
						if ( (C.stream_buffer_state[i]["V"] == "1") && (C.stream_buffer_state[i][int2str(j)] == block_address) )
						{
							return true;
						}
					}
				}
			}
		}

		return false;
	}

	bool searchCache(Cache C, ADDRESS address)
	{
		long long s_set = bin2dec(address.index_bits);
      	for (long long j = 0; j < C.assoc; j++)	// Way
		{
			if (C.cache_state[s_set][j]["V"] == "1")
			{
				if (C.cache_state[s_set][j]["TAG"] == address.tag_bits)
				{
					return true;
				}
			}
		}

		return false;
	}

   bool isDirty(Cache C, ADDRESS address) // This func chekcs if the block with given Index and Tag is dirty or not
   {
      	long long s_set = bin2dec(address.index_bits);
      	for (long long j = 0; j < C.assoc; j++)	// Way
		{
			if (C.cache_state[s_set][j]["V"] == "1")
			{
				if (C.cache_state[s_set][j]["TAG"] == address.tag_bits)
				{
					if (C.cache_state[s_set][j]["D"] == "1")
					{
						return true;
					}
				}
			}
		}
		return false;
   }

   	ADDRESS getHighestLRUC(Cache C, ADDRESS address) {
    	ADDRESS dirty_block;
		long long s_set = bin2dec(address.index_bits);
      	for (long long j = 0; j < C.assoc; j++)	// Way
		{
			if (C.cache_state[s_set][j]["LRUC"] == int2str(C.assoc-1))
			{
				dirty_block.tag_bits = C.cache_state[s_set][j]["TAG"];
				dirty_block.index_bits = address.index_bits;
				dirty_block.block_offset_bits = C.cache_state[s_set][j]["BLOCK"];
				dirty_block.address_bits = C.cache_state[s_set][j]["TAG"] + address.index_bits + C.cache_state[s_set][j]["BLOCK"];
			}
		}
		return dirty_block;
   	}
   	
	ADDRESS transformAddressAsPerCache(Cache C, ADDRESS address) {
    	ADDRESS new_address;
		
		new_address.address_bits = address.address_bits;
		for (long long i = 0; i < C.no_tag_bits; i++)
		{
			new_address.tag_bits += new_address.address_bits[i];
		}

		for (long long i = C.no_tag_bits; i < C.no_tag_bits + C.no_index_bits; i++)
		{
			new_address.index_bits += new_address.address_bits[i];
		}

		for (long long i = C.no_tag_bits + C.no_index_bits; i < C.no_tag_bits + C.no_index_bits + C.no_block_offset; i++)
		{
			new_address.block_offset_bits += new_address.address_bits[i];
		}
		
		new_address.block_address_bits = bin2hex(new_address.tag_bits + new_address.index_bits);

		return new_address;
   	}

	std::string addAddress(std::string address, long long delta)
	{
		long long dec_result = hex2dec(address) + delta;
		return dec2hex(dec_result);
	}

	void prefetchAddressesFromMainMemory(Cache &C, ADDRESS address, bool stream_buffer_hit)
	{
		//printAddressSplit(address);
		if (stream_buffer_hit == true)
		{
			//std::cout << "Prefetched in case of SB Hit \n";
			
			long long hit_n = 0;
			long long hit_lruc = 1000;
			for (long long l = 0; l < C.pref_n; l++)
			{
				for (long long i = 0; i < C.pref_n; i++)
				{
					if ( (C.stream_buffer_state[i]["LRUC"] == int2str(l)) && (C.stream_buffer_state[i]["V"] == "1") )
					{
						for (long long j = 0; j < C.pref_m; j++)
						{
							if (C.stream_buffer_state[i][int2str(j)] == address.block_address_bits)
							{
								hit_n = i;
								hit_lruc = std::stol(C.stream_buffer_state[i]["LRUC"]);
								break;
							}
						}
					}
					if (hit_lruc != 1000)
					{
						break;
					}
				}
				if (hit_lruc != 1000)
				{
					break;
				}
			}

			//std::cout << "Hit n = " << hit_n << '\n';

			for (long long n = 0; n < C.pref_n; n++)
			{
				if (C.stream_buffer_state[n]["LRUC"] < int2str(hit_lruc))
				{
					C.stream_buffer_state[n]["LRUC"] = int2str(std::stol(C.stream_buffer_state[n]["LRUC"]) + 1);
				}
			}
			
			long long x = 1;
			if (C.pref_m != 1)
			{
				long long min_x = hex2dec(address.block_address_bits) + x;
				long long max_x = min_x + C.pref_m - 1;
				for (long long m = 0; m < C.pref_m; m++)
				{
					//std::cout << std::hex << "Min X = " << min_x << ", Current = " << hex2dec(C.stream_buffer_state[hit_n][int2str(m)]) << ", Max X = " << max_x<< std::dec << '\n';
					if ( (hex2dec(C.stream_buffer_state[hit_n][int2str(m)]) < min_x) || (hex2dec(C.stream_buffer_state[hit_n][int2str(m)]) > max_x) )
					{
						C.prefetches++;
						readFromMemory(address);
						//std::cout << "Not in between " << C.prefetches << "\n";
					}
					else
					{
						//std::cout << "Exists in between\n";
					}
					C.stream_buffer_state[hit_n][int2str(m)] = addAddress(address.block_address_bits,x);
					x++;
				}
			}
			else
			{
				//std::cout << std::hex << "X+1 = " << hex2dec(address.block_address_bits)+x << ", Current = " << hex2dec(C.stream_buffer_state[hit_n]["0"]) << std::dec << '\n';
				C.stream_buffer_state[hit_n]["0"] = addAddress(address.block_address_bits,x);
				C.prefetches++;
				readFromMemory(address);
			}

			C.stream_buffer_state[hit_n]["LRUC"] = "0";
			C.stream_buffer_state[hit_n]["V"] = "1";
		}
		else
		{
			//std::cout << "Prefetched in case of SB Miss \n";
			
			long long n_max_lruc = 0;
			for (long long n = 0; n < C.pref_n; n++)
			{
				if (C.stream_buffer_state[n]["LRUC"] == int2str(C.pref_n-1))
				{
					n_max_lruc = n;
				}
				C.stream_buffer_state[n]["LRUC"] = int2str(std::stol(C.stream_buffer_state[n]["LRUC"]) + 1);
			}

			long long x = 1;
			for (long long m = 0; m < C.pref_m; m++)
			{
				C.stream_buffer_state[n_max_lruc][int2str(m)] = addAddress(address.block_address_bits,x);
				C.prefetches++;
				readFromMemory(address);
				x++;
			}

			C.stream_buffer_state[n_max_lruc]["LRUC"] = "0";
			C.stream_buffer_state[n_max_lruc]["V"] = "1";
		}
		//std::cout << "Prefetches = " << C.prefetches << "\n";
		//C.printStreamBufferState();
	}

	void read(Cache &C, ADDRESS address, bool traverse_back = false)
    {
        /*if (C.cache_level == 1)
			std::cout << "\tL" << C.cache_level << ": r " << bin2hex(address.address_bits) << "(tag=" << bin2hex(address.tag_bits) << " index=" << bin2dec(address.index_bits) << '\n' ;
		else
			std::cout << "\t\tL" << C.cache_level << ": r " << bin2hex(address.address_bits) << "(tag=" << bin2hex(address.tag_bits) << " index=" << bin2dec(address.index_bits) << '\n' ;
		*/
		long long s_lruc;
        long long s_set = bin2dec(address.index_bits);
        long long s_way = 0;

		//C.printCacheAtIndex(address.index_bits);
		
		if (searchCache(C, address) == true)									// Cache Read Hit
		{
			//std::cout << "Read Cache Hit\n";
			C.read_demand++;
			for (long long j = 0; j < C.assoc; j++)	// Way
			{
				if (C.cache_state[s_set][j]["TAG"] == address.tag_bits)
				{
					s_lruc = std::stol( C.cache_state[s_set][j]["LRUC"] );
					s_way = j;
					break;
				}
			}

			for (long long j = 0; j < C.assoc; j++)	// Way
			{
				if ( std::stol( C.cache_state[s_set][j]["LRUC"] ) <  s_lruc)
				{
					C.cache_state[s_set][j]["LRUC"] = int2str( std::stol(C.cache_state[s_set][j]["LRUC"]) + 1 );
				}
			}
	
			C.cache_state[s_set][s_way]["LRUC"] = "0";
	
			//std::cout << "R: Set=" << s_set << ", Way=" << s_way << ", Tag=" << std::hex << std::stol(address.tag_bits, nullptr, 2) << std::dec << '\n';

			// Prefetch
			if (traverse_back == false)
			{
				if (C.pref_n != 0)  // Stream Buffer exists
				{
					if (searchStreamBuffer(C, address.block_address_bits) == true)
					{
						//std::cout << "SB Hit\n";
						prefetchAddressesFromMainMemory(C, address, true);
						printSB++;
					}
					else
					{
						//std::cout << "SB Miss for "<< address.block_address_bits << " (Do Nothing)\n";
					}
				}
			}
		}
		else																	// Cache Read Miss
		{
			//std::cout << "Read Cache Miss\n";
			C.read_miss_stream_buffer_miss_demand++;
			ADDRESS victim_block = getHighestLRUC(C, address);
			if (isDirty(C, victim_block) == true)
			{
				C.write_backs++;
				if (L2.active == true)
				{
					if (C.cache_level == 1)
					{
						write(L2, transformAddressAsPerCache(L2, victim_block));
					}
					else if (C.cache_level == 2)
					{
						writeToMemory(victim_block);
					}
				}
				else
				{
					writeToMemory(victim_block);
				}
			}

			if (L2.active == true)
			{
				if (C.cache_level == 1)
				{
					read(L2, transformAddressAsPerCache(L2, address));
					writeAllocate(L1, address);
					read(L1, address, true);
				}
				else if (C.cache_level == 2)
				{
					// Prefetch
					if (C.pref_n != 0)  // Stream Buffer exists
					{
						if (searchStreamBuffer(C, address.block_address_bits) == true) 	// Miss in Cache but Hit in SB
						{
							//std::cout << "SB Hit\n";
							readFromStreamBuffer(address);
							C.read_miss_stream_buffer_miss_demand--;					// Not a Read Miss because fetched from SB
							prefetchAddressesFromMainMemory(C, address, true);
							printSB++;
							writeAllocate(L2, address);
							read(L2, address, true);
						}
						else
						{
							//std::cout << "SB Miss\n";
							prefetchAddressesFromMainMemory(C, address, false);
							printSB++;
							readFromMemory(address);
							writeAllocate(L2, address);
							read(L2, address, true);
						}
					}
					else
					{
						readFromMemory(address);
						writeAllocate(L2, address);
						read(L2, address, true);
					}
				}
			}
			else
			{
				// Prefetch
				if (C.pref_n != 0)  // Stream Buffer exists
				{
					if (searchStreamBuffer(C, address.block_address_bits) == true) 	// Miss in Cache but Hit in SB
					{
						//std::cout << "SB Hit\n";
						readFromStreamBuffer(address);
						C.read_miss_stream_buffer_miss_demand--;					// Not a Read Miss because fetched from SB
						prefetchAddressesFromMainMemory(C, address, true);
						printSB++;
						writeAllocate(L1, address);
						read(L1, address, true);
					}
					else
					{
						//std::cout << "SB Miss\n";
						prefetchAddressesFromMainMemory(C, address, false);
						printSB++;
						readFromMemory(address);
						writeAllocate(L1, address);
						read(L1, address, true);
					}
				}
				else
				{
					readFromMemory(address);
					writeAllocate(L1, address);
					read(L1, address, true);
				}
			}
		}
		//std::cout << "read_miss_stream_buffer_miss_demand = " << C.read_miss_stream_buffer_miss_demand << '\n';
		//C.printCacheAtIndex(address.index_bits);
    }

   void write(Cache &C, ADDRESS address, bool traverse_back = false)
   {
        /*if (C.cache_level == 1)
			std::cout << "\tL" << C.cache_level << ": w " << bin2hex(address.address_bits) << "(tag=" << bin2hex(address.tag_bits) << " index=" << bin2dec(address.index_bits) << '\n' ;
		else
			std::cout << "\t\tL" << C.cache_level << ": w " << bin2hex(address.address_bits) << "(tag=" << bin2hex(address.tag_bits) << " index=" << bin2dec(address.index_bits) << '\n' ;
		*/
		long long s_lruc;
        long long s_set = bin2dec(address.index_bits);
        long long s_way = 0;

		//C.printCacheAtIndex(address.index_bits);

		if (searchCache(C, address) == true)									// Cache Write Hit
		{
			//std::cout << "Write Cache Hit\n";
			C.write++;
			for (long long j = 0; j < C.assoc; j++)	// Way
			{
				if (C.cache_state[s_set][j]["TAG"] == address.tag_bits) // Replace the block with the matching TAG
				{
					s_lruc = std::stol( C.cache_state[s_set][j]["LRUC"] );
					s_way = j;
					C.cache_state[s_set][j]["V"] = "1";
					C.cache_state[s_set][j]["BLOCK"] = address.block_offset_bits;
					C.cache_state[s_set][j]["TAG"] = address.tag_bits;
					C.cache_state[s_set][j]["D"] = "1";
					break;
				}
			}
	
			for (long long j = 0; j < C.assoc; j++)	// Way
			{
				if ( std::stol( C.cache_state[s_set][j]["LRUC"] ) <  s_lruc)
				{
					C.cache_state[s_set][j]["LRUC"] = int2str( std::stol(C.cache_state[s_set][j]["LRUC"]) + 1 );
				}
			}
	
			C.cache_state[s_set][s_way]["LRUC"] = "0";
	
			//std::cout << "W: Set=" << s_set << ", Way=" << s_way << ", Tag=" << std::hex << std::stol(address.tag_bits, nullptr, 2) << std::dec << '\n';

			// Prefetch
			if (traverse_back == false)
			{
				if (C.pref_n != 0)  // Stream Buffer exists
				{
					if (searchStreamBuffer(C, address.block_address_bits) == true)
					{
						//std::cout << "SB Hit\n";
						prefetchAddressesFromMainMemory(C, address, true);
						printSB++;
					}
					else
					{
						//std::cout << "SB Miss for "<< address.block_address_bits << " (Do Nothing)\n";
					}
				}
			}
		}
		else																	// Cache Miss
		{
			//std::cout << "Write Cache Miss\n";
			C.write_miss_stream_buffer_miss++;
			ADDRESS victim_block = getHighestLRUC(C, address);
			if (isDirty(C, victim_block) == true)
			{
				C.write_backs++;
				if (L2.active == true)
				{
					if (C.cache_level == 1)
					{
						write(L2, transformAddressAsPerCache(L2, victim_block));
					}
					else if (C.cache_level == 2)
					{
						writeToMemory(victim_block);
					}
				}
				else
				{
					writeToMemory(victim_block);
				}
			}

			if (L2.active == true)
			{
				if (C.cache_level == 1)
				{
					read(L2, transformAddressAsPerCache(L2, address));
					writeAllocate(L1, address);
					write(L1, address, true);
				}
				else if (C.cache_level == 2)
				{
					// Prefetch
					if (C.pref_n != 0)  // Stream Buffer exists
					{
						if (searchStreamBuffer(C, address.block_address_bits) == true)	// Miss in Cache, Hit in SB
						{
							//std::cout << "SB Hit\n";
							readFromStreamBuffer(address);
							C.write_miss_stream_buffer_miss--;							// Not a Write Miss because fetched from SB
							prefetchAddressesFromMainMemory(C, address, true);
							printSB++;
							writeAllocate(L2, address);
							write(L2, address, true);
						}
						else
						{
							//std::cout << "SB Miss\n";
							prefetchAddressesFromMainMemory(C, address, false);
							printSB++;
							readFromMemory(address);
							writeAllocate(L2, address);
							write(L2, address, true);
						}
					}
					else
					{
						readFromMemory(address);
						writeAllocate(L2, address);
						write(L2, address, true);
					}
				}
			}
			else
			{
				// Prefetch
				if (C.pref_n != 0)  // Stream Buffer exists
				{
					if (searchStreamBuffer(C, address.block_address_bits) == true)	// Miss in Cache, Hit in SB
					{
						//std::cout << "SB Hit\n";
						readFromStreamBuffer(address);
						C.write_miss_stream_buffer_miss--;							// Not a Write Miss because fetched from SB
						prefetchAddressesFromMainMemory(C, address, true);
						printSB++;
						writeAllocate(L1, address);
						write(L1, address, true);
					}
					else
					{
						//std::cout << "SB Miss\n";
						prefetchAddressesFromMainMemory(C, address, false);
						printSB++;
						readFromMemory(address);
						writeAllocate(L1, address);
						write(L1, address, true);
					}
				}
				else
				{
					readFromMemory(address);
					writeAllocate(L1, address);
					write(L1, address, true);
				}
			}
		}
		//std::cout << "write_miss_stream_buffer_miss = " << C.write_miss_stream_buffer_miss << '\n';
		//C.printCacheAtIndex(address.index_bits);
   }
   
	void writeAllocate(Cache &C, ADDRESS address)
   	{
		long long s_lruc = C.assoc-1;
		long long s_set = bin2dec(address.index_bits);
		long long s_way = 0;
		for (long long j = 0; j < C.assoc; j++)	// Way
		{
         	if (C.cache_state[s_set][j]["LRUC"] == int2str(s_lruc)) // Replace the block with the max LRUC
         	{
				s_way = j;
				C.cache_state[s_set][j]["V"] = "1";
				C.cache_state[s_set][j]["BLOCK"] = address.block_offset_bits;
				C.cache_state[s_set][j]["TAG"] = address.tag_bits;
				C.cache_state[s_set][j]["D"] = "0";
				break;
         	}
		}

      	for (long long j = 0; j < C.assoc; j++)	// Way
		{
			if ( std::stol( C.cache_state[s_set][j]["LRUC"] ) <  s_lruc)
			{
				C.cache_state[s_set][j]["LRUC"] = int2str( std::stol(C.cache_state[s_set][j]["LRUC"]) + 1 );
			}
		}

      	C.cache_state[s_set][s_way]["LRUC"] = "0";

      	//std::cout << "WA from next level: Set=" << s_set << ", Way=" << s_way << ", Tag=" << std::stol(address.tag_bits, nullptr, 2) << std::dec << '\n';
   	}

   void readFromMemory(ADDRESS address) {
	this->main_memory_accesses++;
   }

   void writeToMemory(ADDRESS address) {
	this->main_memory_accesses++;
   }

   void readFromStreamBuffer(ADDRESS address) {}

	void addressRead(uint32_t addr)
	{
		std::bitset<32> bits;
		bits = addr;
		std::string addressBinary = bits.to_string();
		splitAdress(addressBinary);

		printSB = 0;

		//std::cout << "\tL" << L1.cache_level << ": r " << bin2hex(address_bits_L1.address_bits) << " (tag=" << bin2hex(address_bits_L1.tag_bits) << " index=" << bin2dec(address_bits_L1.index_bits) << ")\n" ;
		//L1.printCacheAtIndex(address_bits_L1.index_bits, "before");
        read(L1, address_bits_L1);
		//L1.printCacheAtIndex(address_bits_L1.index_bits, "after");

		/*if (printSB > 0)
		{
			L1.printSB();
		}*/

		clearAddressSplit();
	}

	void addressWrite(uint32_t addr)
	{
		std::bitset<32> bits;
		bits = addr;
		std::string addressBinary = bits.to_string();
		splitAdress(addressBinary);

		printSB = 0;

		//std::cout << "\tL" << L1.cache_level << ": w " << bin2hex(address_bits_L1.address_bits) << " (tag=" << bin2hex(address_bits_L1.tag_bits) << " index=" << bin2dec(address_bits_L1.index_bits) << ")\n" ;
		//L1.printCacheAtIndex(address_bits_L1.index_bits, "before");
        write(L1, address_bits_L1);
		//L1.printCacheAtIndex(address_bits_L1.index_bits, "after");

		/*if (printSB > 0)
		{
			L1.printSB();
		}*/

        clearAddressSplit();
   }

	void cacheStatistics()
	{
		std::cout << "\n===== L1 contents ====="  << '\n';
		this->L1.printCacheState();
		std::cout << '\n';
		if (L2.active == true)
		{
			std::cout << "===== L2 contents ====="  << '\n';
			this->L2.printCacheState();
			std::cout << '\n';
			
			if (L2.pref_n != 0)
			{
				std::cout << "===== Stream Buffer(s) contents =====" << '\n';
				this->L2.printStreamBufferState();
				std::cout << '\n';
			}
		}
		else
		{
			if (L1.pref_n != 0)
			{
				std::cout << "===== Stream Buffer(s) contents =====" << '\n';
				this->L1.printStreamBufferState();
				std::cout << '\n';
			}
		}

		std::cout << "===== Measurements =====" << '\n';
		std::cout << "a. L1 reads:" << std::string(19, ' ') << L1.read_demand << '\n';
		std::cout << "b. L1 read misses:" << std::string(13, ' ') << L1.read_miss_stream_buffer_miss_demand << '\n';
		//std::cout << "L1.read_miss_stream_buffer_hit=" << L1.read_miss_stream_buffer_hit << '\n';
		std::cout << "c. L1 writes:" << std::string(18, ' ') << L1.write << '\n';
		std::cout << "d. L1 write misses:" << std::string(12, ' ') << L1.write_miss_stream_buffer_miss << '\n';
		//std::cout << "L1.write_miss_stream_buffer_hit=" << L1.write_miss_stream_buffer_hit << '\n';
		float miss_rate = (L1.read_demand>0 || L1.write>0) ? ((float)L1.read_miss_stream_buffer_miss_demand + (float)L1.write_miss_stream_buffer_miss)/((float)L1.read_demand + (float)L1.write) : 0.0000;
		std::cout << "e. L1 miss rate:" << std::string(15, ' ') << std::fixed<< std::setprecision(4) << miss_rate << '\n';
		std::cout << "f. L1 writebacks:" << std::string(14, ' ') << L1.write_backs << '\n';
		std::cout << "g. L1 prefetches:" << std::string(14, ' ') << L1.prefetches << '\n';
		std::cout << "h. L2 reads (demand):" << std::string(10, ' ') << L2.read_demand << '\n';
		std::cout << "i. L2 read misses (demand):" << std::string(4, ' ') << L2.read_miss_stream_buffer_miss_demand << '\n';
		//std::cout << "L2.read_miss_stream_buffer_hit=" << L2.read_miss_stream_buffer_hit << '\n';
		std::cout << "j. L2 reads (prefetch):" << std::string(8, ' ') << L2.read_prefetch << '\n';
		std::cout << "k. L2 read misses (prefetch):" << std::string(2, ' ') << L2.read_miss_stream_buffer_miss_prefetch << '\n';
		std::cout << "l. L2 writes:" << std::string(18, ' ') << L2.write << '\n';
		std::cout << "m. L2 write misses:" << std::string(12, ' ') << L2.write_miss_stream_buffer_miss << '\n';
		//std::cout << "L2.write_miss_stream_buffer_hit=" << L2.write_miss_stream_buffer_hit << '\n';
		miss_rate = (L2.read_demand>0) ? (float)L2.read_miss_stream_buffer_miss_demand/(float)L2.read_demand : 0.0000;
		std::cout << "n. L2 miss rate:" << std::string(15, ' ') << std::fixed << std::setprecision(4) << miss_rate << '\n';
		std::cout << "o. L2 writebacks:" << std::string(14, ' ') << L2.write_backs << '\n';
		std::cout << "p. L2 prefetches:" << std::string(14, ' ') << L2.prefetches << '\n';
		std::cout << "q. memory traffic:" << std::string(13, ' ') << this->main_memory_accesses << '\n';
	}
};

/* "argc" holds the number of command-line arguments.
    "argv[]" holds the arguments themselves.

    Example:
    ./sim 32 8192 4 262144 8 3 10 gcc_trace.txt
    argc = 9
    argv[0] = "./sim"
    argv[1] = "32"
    argv[2] = "8192"
    ... and so on
*/
int main(int argc, char *argv[])
{	
	FILE * fp;	// File pointer.
	char *trace_file;	// This variable holds the trace file name.
	cache_params_t params;	// Look at the sim.h header file for the definition of struct cache_params_t.
	char rw;	// This variable holds the request's type (read or write) obtained from the trace.
	uint32_t addr;	// This variable holds the request's address obtained from the trace.
	// The header file < inttypes.h > above defines signed and unsigned integers of various sizes in a machine-agnostic way.  "uint32_t" is an unsigned integer of 32 bits.

	// Exit with an error if the number of command-line arguments is incorrect.
	if (argc != 9)
	{
		printf("Error: Expected 8 command-line arguments but was provided %d.\n", (argc - 1));
		exit(EXIT_FAILURE);
	}

	// "atoi()" (included by < stdlib.h>) converts a string (char*) to an integer (int).
	params.BLOCKSIZE = (uint32_t) atoi(argv[1]);
	params.L1_SIZE = (uint32_t) atoi(argv[2]);
	params.L1_ASSOC = (uint32_t) atoi(argv[3]);
	params.L2_SIZE = (uint32_t) atoi(argv[4]);
	params.L2_ASSOC = (uint32_t) atoi(argv[5]);
	params.PREF_N = (uint32_t) atoi(argv[6]);
	params.PREF_M = (uint32_t) atoi(argv[7]);
	trace_file = argv[8];

	// Open the trace file for reading.
	fp = fopen(trace_file, "r");
	if (fp == (FILE*) NULL)
	{
		// Exit with an error if file open failed.
		printf("Error: Unable to open file %s\n", trace_file);
		exit(EXIT_FAILURE);
	}

	// Prlong long simulator configuration.
	printf("===== Simulator configuration =====\n");
	printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
	printf("L1_SIZE:    %u\n", params.L1_SIZE);
	printf("L1_ASSOC:   %u\n", params.L1_ASSOC);
	printf("L2_SIZE:    %u\n", params.L2_SIZE);
	printf("L2_ASSOC:   %u\n", params.L2_ASSOC);
	printf("PREF_N:     %u\n", params.PREF_N);
	printf("PREF_M:     %u\n", params.PREF_M);
	printf("trace_file: %s\n", trace_file);
	//printf("===================================\n");
	//printf("\n");

	CacheHierarchy CH(params);

	// Read requests from the trace file and echo them back.
	//long long c = 1;
	while (fscanf(fp, "%c %x\n", &rw, &addr) == 2)
	{
		// Stay in the loop if fscanf() successfully parsed two tokens as specified.
		if (rw == 'r')
		{
			//printf("%llu=r %x\n", c, addr);
			CH.addressRead(addr);
			//c++;
		}
		else if (rw == 'w')
		{
         	//printf("%llu=w %x\n", c, addr);
			CH.addressWrite(addr);
			//c++;
      	}
		else
		{
			printf("Error: Unknown request type %c.\n", rw);
			exit(EXIT_FAILURE);
		}

		///////////////////////////////////////////////////////
		// Issue the request to the L1 cache instance here.
		///////////////////////////////////////////////////////
   	}

   CH.cacheStatistics();

	return (0);
}