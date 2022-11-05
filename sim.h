#ifndef SIM_CACHE_H
#define SIM_CACHE_H
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include <sstream>
#include <cstring>

typedef 
struct {
   uint32_t BLOCKSIZE;
   uint32_t L1_SIZE;
   uint32_t L1_ASSOC;
   uint32_t L2_SIZE;
   uint32_t L2_ASSOC;
   uint32_t PREF_N;
   uint32_t PREF_M;
} cache_params_t;

// Put additional data structures here as per your requirement.
void print(std::string text = "", std::string caption = ":")
{
	if (true)
	{
		if (caption == ":")
		{
			std::cout << text << '\n';
		}
		else
		{
			std::cout << caption << ": " << text << '\n';
		}
	}
}

std::string int2str(long long numbers)
{
	std::string text = std::to_string(numbers);
	return text;
}

std::string dec2hex(long long dec_num)
{
	std::stringstream s;
	s << std::hex << dec_num;
	std::string hex_num ( s.str() );
	return hex_num;
}

long long hex2dec(std::string hex_num)
{
	long long dec_num;
	std::stringstream s;
    s << hex_num;
    s >> std::hex >> dec_num;
	return dec_num;
}

long long bin2dec(std::string bin_num)
{
	int n = bin_num.length();
    char c[n + 1];
    std::strcpy(c, bin_num.c_str());
	long long dec_num = std::strtoll(c, nullptr, 2);
	return dec_num;
}

std::string bin2hex(std::string bin_num)
{
	long long dec_num = bin2dec(bin_num);
	return dec2hex(dec_num);
}

typedef struct addressType
{
   std::string address_bits = "";
   std::string tag_bits = "";
   std::string index_bits = "";
   std::string block_address_bits = ""; // Equal to Tag + Index
   std::string block_offset_bits = "";
}
ADDRESS;

class Cache
{
	public:
	long long cache_level;
	long long no_tag_bits;
	long long no_index_bits;
	long long no_block_offset;
	long long cachesize;
	long long blocksize;
	long long assoc;
	long long pref_n;
	long long pref_m;

	// Declare Cache as a map with depth = 3
	// Key (1) Set i
	// Key (2) Way j
	// Key (3) Properties[TAG, V, D and LRUC]
	std::map<int, std::map<int, std::map< std::string, std::string> > > cache_state;

	//  Declare Stream Buffer as a map with depth = 3
	//  Key (1) N (which buffer)
	//  Key (2)["Valid", "LRUC", < Block Indexes 0 to M-1>]
	//  Key (3) Values based on value of 2nd key
	//      "VALID": 0 or 1
	//      "LRUC":[0 ... N-1]
	//     <Block Indexes>: TAG values of prefetched blocks
	std::map<int, std::map<std::string, std::string> > stream_buffer_state;
	bool stream_buffer_exists = true;

	bool active = false;
	long long  read_demand = 0;
	long long  read_prefetch = 0;
	long long  read_miss_stream_buffer_miss_demand = 0;
	long long  read_miss_stream_buffer_hit_demand = 0;
	long long  read_miss_stream_buffer_miss_prefetch = 0;
	long long  read_miss_stream_buffer_hit_prefetch = 0;
	long long  write = 0;
	long long  write_miss_stream_buffer_miss = 0;
	long long  write_miss_stream_buffer_hit = 0;
	long long  prefetches = 0;
	long long  write_backs = 0;

	void initializeCache(long long cache_level, long long cachesize, long long blocksize, long long assoc, long long address_bits, long long pref_n, long long pref_m)
	{
		this->cache_level = cache_level;
		this->cachesize = cachesize;
		this->blocksize = blocksize;
		this->assoc = assoc;
		this->pref_n = pref_n;
		this->pref_m = pref_m;
		// Calculate the no_block_offset, no_index_bits, no_tag_bits
		// BLOCK OFFSET
		this->no_block_offset = log2(blocksize);

		long long no_blocks = cachesize / blocksize;
		long long no_sets = no_blocks / assoc;
		// INDEX BITS
		this->no_index_bits = log2(no_sets);

		// TAG BITS
		this->no_tag_bits = address_bits - no_block_offset - no_index_bits;

		// Initialize Cache with default values for (1) Set i, (2) Way j, (3) Properties[TAG, V, D and LRUC]
		for (long long i = 0; i < no_sets; i++)	// Set
		{
			for (long long j = 0; j < assoc; j++)	// Way
			{
				this->cache_state[i][j]["BLOCK"] = "-";
				this->cache_state[i][j]["TAG"] = "-";
				this->cache_state[i][j]["V"] = "0";
				this->cache_state[i][j]["D"] = "0";
				this->cache_state[i][j]["LRUC"] = std::to_string(j);
			}
		}

		// Initialize Stream Buffer with default values for (1) N, (2) Properties[V, LRUC]
		if (pref_n == 0)
		{
			//print("No Stream Buffers at this Cache");
			this->stream_buffer_exists = false;
		}

		for (long long i = 0; i < pref_n; i++)
		{
			this->stream_buffer_state[i]["V"] = "0";
			this->stream_buffer_state[i]["LRUC"] = std::to_string(i);
			for (long long j = 0; j < pref_m; j++)
			{
				this->stream_buffer_state[i][std::to_string(j)] = "-";
			}
		}
	}

	/*void printCacheState()
	{
		for (auto itr1: this->cache_state)
		{
			std::cout << "Set " << itr1.first << '\n';
			for (auto itr2: itr1.second)
			{
				std::cout << "\tWay " << itr2.first << '\n';
				for (auto itr3: itr2.second)
				{
					std::cout << "\t\t" << itr3.first << " = " << itr3.second << '\n';
				}
			}

			std::cout << '\n';
		}
	}*/

	void printCacheState()
	{
		for (auto itr1: this->cache_state)
		{
			long long spaces = 7 - std::to_string(itr1.first).length();
			std::cout << "set" << std::string(spaces, ' ') << itr1.first << ": ";
			for (long long i = 0; i<assoc; i++)
			{
				for (auto itr2: itr1.second)
				{
					if (cache_state[itr1.first][itr2.first]["LRUC"] == std::to_string(i))
					{
						if ( cache_state[itr1.first][itr2.first]["TAG"] != "-" )
						{
							std::cout << "  " << std::hex << std::stol(cache_state[itr1.first][itr2.first]["TAG"], nullptr, 2) << std::dec << " ";
						}
						else
						{
							std::cout << "  ";
						}
						
						if ( cache_state[itr1.first][itr2.first]["D"] == "1" )
						{
							std::cout << "D";
						}
						else
						{
							std::cout << " ";
						}
					}
				}
			}
			std::cout << '\n';
		}
	}

	/*void printStreamBufferState()
	{
		std::cout << "===== Stream Buffer(s) contents =====\n";
		for (auto itr1: this->stream_buffer_state)
		{
			std::cout << "N " << itr1.first << '\n';
			for (auto itr2: itr1.second)
			{
				std::cout << "\t" << itr2.first << " = " << itr2.second << '\n';
			}

			std::cout << '\n';
		}
	}*/
	void printStreamBufferState()
	{
		long long lruc = 0;
		while (lruc != pref_n)
		{
			for (auto itr1: this->stream_buffer_state)
			{
				if (stream_buffer_state[itr1.first]["LRUC"] == std::to_string(lruc))
				{
					for (long long i = 0; i < pref_m; i++)
					{
						std::cout << " " << std::hex << stream_buffer_state[itr1.first][std::to_string(i)] << std::dec << " ";
					}
					lruc++;
					std::cout << '\n';
				}
			}
		}
	}
	
	void printCacheAtIndex(std::string index_bits, std::string when)
	{
		long long s_set = bin2dec(index_bits);
		if (when == "after")
		{
			if (cache_level == 1)
			{	
				std::cout << "\tL" << cache_level << ":  " << when<< ": set     " << s_set << ":";
			}
			else
			{
				std::cout << "\tL" << cache_level << ":  " << when<< ": set     " << s_set << ":";
			}
		}
		else
		{
			if (cache_level == 1)
				std::cout << "\tL" << cache_level << ": " << when<< ": set     " << s_set << ":";
			else
				std::cout << "\tL" << cache_level << ": " << when<< ": set     " << s_set << ":";
		}

		for (long long i = 0; i<assoc; i++)
		{
			for (long long j = 0; j < assoc; j++)	// Way
			{
				if (cache_state[s_set][j]["LRUC"] == int2str(i))
				{
					if (cache_state[s_set][j]["TAG"] == "-")
					{	
						std::cout << "   ";
					}
					else
					{
						std::cout << "   " << std::hex << std::stol(cache_state[s_set][j]["TAG"], nullptr, 2) << std::dec << " ";
					}

					if ( (cache_state[s_set][j]["D"] == "0") || (cache_state[s_set][j]["D"] == "-") )
					{	
						std::cout << " ";
					}
					else
					{
						std::cout << "D";
					}
				}
			}
		}
		std::cout << "\n";
	}

	void printSB()
	{
		long long lruc = 0;
		while (lruc != pref_n)
		{
			for (auto itr1: this->stream_buffer_state)
			{
				if (stream_buffer_state[itr1.first]["LRUC"] == std::to_string(lruc))
				{
					for (long long i = 0; i < pref_m; i++)
					{
						if (stream_buffer_state[itr1.first][std::to_string(i)] != "-")
						{
							std::cout << "\t\t\tSB: ";
							std::cout << " " << std::hex << stream_buffer_state[itr1.first][std::to_string(i)] << std::dec << " ";
							std::cout << '\n';
						}
					}
					lruc++;
				}
			}
		}
	}
};

#endif
