#ifndef MEM_H
#define MEM_H
#include "stats.h"
#include "utli.h"
#include "cache.h"
#include <cstddef>
#include <cstdio>

extern Statistics* stats;

struct page_table_entry
{
	long long int ppn : 48;
	unsigned int
		X : 1,
		R : 1,
		W : 1,
		dirty : 1,
		avail : 1,
		final : 1,
		PL : 2,
		_place_holder : 8;
};

struct tlb_entry
{
	page_table_entry *pte;
	ulli last_access_time;
	ulli tag;
	int valid;
};

class Memory
{
	struct addr_split
	{
		unsigned long long int addr : 48;
		unsigned int vpn1()
		{
			return (addr >> 39) & 0x1ff;
		}
		unsigned int vpn2()
		{
			return (addr >> 30) & 0x1ff;
		}
		unsigned int vpn3()
		{
			return (addr >> 21) & 0x1ff;
		}
		unsigned int vpn4()
		{
			return (addr >> 12) & 0x1ff;
		}
		unsigned int vpo()
		{
			return addr & 0xfff;
		}
		ulli tag()
		{
			return addr >> (12 + tlb_params[0]);
		}
		uint set_idx()
		{
			return (addr >> 12) & ((1 << tlb_params[0]) - 1);
		}
	};
	page_table_entry *pde;
	tlb_entry **tlb;
	Cache *cacheI, *cacheD;
	void assign_page(page_table_entry*);
	void free_one_page(page_table_entry*);
	void free_pages(page_table_entry*);
	void allocate_range(unsigned long long int addr, int len, int mod, int pl);
	void* translate(unsigned long long int addr, int mod, int pl, int level = 0);
	int direct_write(ulli addr, const void* src, int len);
	#define mod_r 1
	#define mod_w 2
	#define mod_x 4
public:
	Memory();
	~Memory();
	int read(ulli addr, void* target, int len, int pl = 3);
	int write(ulli addr, const void* src, int len, int pl = 3);
	bool load_file(int argc, char* argv[], char* envp[],
		long long int&, long long int&, long long int&);
};

#endif
