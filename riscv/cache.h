#ifndef CACHE_H
#define CACHE_H
#include "utli.h"
#include "stats.h"
#include <string>

extern Statistics* stats;

class Cache
{
	int s, L, b;
	struct entry
	{
		bool valid;
		bool dirty;
		int last_access_time;
		void *contents;
		ulli tag;
	};
	ulli tag(ulli addr)
	{
		return addr >> (s + b);
	}
	uint set_idx(ulli addr)
	{
		return (addr >> b) & ((1ll << s) - 1);
	}
	uint block_idx(ulli addr)
	{
		return addr & ((1ll << b) - 1);
	}
	entry **cache;
	std::string name;
public:
	Cache *next;
	Cache(std::string, int, int, int);
	~Cache();
	bool read(ulli, void*, int);
	bool write(ulli, const void*, int);
};

#endif
