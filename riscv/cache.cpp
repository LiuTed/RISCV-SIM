#include "cache.h"
#include <string>
#include <cstring>
#include <algorithm>
using namespace std;

Cache::Cache(string _name, int _s, int _L, int _b):
name(_name),s(_s),L(_L),b(_b),next(nullptr)
{
	dbg_print("creating cache %s with s = %d, L = %d, b = %d\n", name.c_str(), s, L, b);
	cache = new entry*[1 << _s];
	for(int i = 0; i < (1 << _s); i++)
	{
		cache[i] = new entry[_L];
		for(int j = 0; j < _L; j++)
		{
			cache[i][j].valid = cache[i][j].dirty = false;
			cache[i][j].last_access_time = cache[i][j].tag = 0;
			cache[i][j].contents = malloc(1 << _b);
		}
	}
}
Cache::~Cache()
{
	for(int i = 0; i < (1 << s); i++)
		delete[] cache[i];
	delete[] cache;
	delete next;
}

static int cnt = 0;
bool Cache::read(ulli addr, void* target, int len)
{
	stats->summary(string("Cache: ") + name + " cache read");
	stats->summary(string("Cache: ") + name + " cache read bytes", len);

	while(len > 0)
	{
		stats->summary(string("Cache: ") + name + " cache fine-grained read");
		entry *line = cache[set_idx(addr)];
		bool flag = false;
		int swapout = 0;
		for(int i = 0; i < L; i++)
		{
			if(!line[i].valid)
			{
				if(line[swapout].valid) swapout = i;
				continue;
			}
			if(line[i].tag == tag(addr))
			{
				swapout = i;
				flag = true;
				break;
			}
			if(line[swapout].last_access_time > line[i].last_access_time)
				swapout = i;
		}
		uint _len = min((uint)len, (1 << b) - block_idx(addr));
		if(!flag)
		{
			stats->summary(string("Cache: ") + name + " cache read miss");
			void *swapin = malloc(1 << b);
			if(next)
			{
				if(!next->read(addr - block_idx(addr), swapin, 1 << b))
					return false;
				if(line[swapout].valid && line[swapout].dirty)
					if(!next->write((line[swapout].tag << (s + b)) | (set_idx(addr) << b), line[swapout].contents, 1 << b))
						return false;
			}
			else
			{
				memcpy(swapin, reinterpret_cast<void*>(addr - block_idx(addr)), 1 << b);
				if(line[swapout].valid && line[swapout].dirty)
					memcpy(reinterpret_cast<void*>((line[swapout].tag << (s + b)) | (set_idx(addr) << b)),
						line[swapout].contents, 1 << b);
			}
			memcpy(line[swapout].contents, swapin, 1 << b);
			line[swapout].tag = tag(addr);
			line[swapout].valid = 1;
			line[swapout].dirty = false;
			free(swapin);
		}
		memcpy(target, (char*)line[swapout].contents + block_idx(addr), _len);
		line[swapout].last_access_time = cnt++;

		target = (char*)target + _len;
		len -= _len;
		addr += _len;
	}
	return true;
}
bool Cache::write(ulli addr, const void* src, int len)//write-back and write-allocate
{
	stats->summary(string("Cache: ") + name + " cache write");
	stats->summary(string("Cache: ") + name + " cache write bytes", len);

	while(len > 0)
	{
		stats->summary(string("Cache: ") + name + " cache fine-grained write");
		entry *line = cache[set_idx(addr)];
		bool flag = false;
		int swapout = 0;
		for(int i = 0; i < L; i++)
		{
			if(!line[i].valid)
			{
				if(line[swapout].valid) swapout = i;
				continue;
			}
			if(line[i].tag == tag(addr))
			{
				swapout = i;
				flag = true;
				break;
			}
			if(line[swapout].last_access_time > line[i].last_access_time)
				swapout = i;
		}
		uint _len = min((uint)len, (1 << b) - block_idx(addr));
		if(!flag)//write-allocate
		{
			stats->summary(string("Cache: ") + name + " cache write miss");
			void *swapin = malloc(1 << b);
			if(next)
			{
				if(!next->read(addr - block_idx(addr), swapin, 1 << b))
					return false;
				if(line[swapout].valid && line[swapout].dirty)
					if(!next->write((line[swapout].tag << (s + b)) | (set_idx(addr) << b), line[swapout].contents, 1 << b))
						return false;
			}
			else
			{
				memcpy(swapin, reinterpret_cast<void*>(addr - block_idx(addr)), 1 << b);
				if(line[swapout].valid && line[swapout].dirty)
					memcpy(reinterpret_cast<void*>((line[swapout].tag << (s + b)) | (set_idx(addr) << b)),
						line[swapout].contents, 1 << b);
			}
			memcpy(line[swapout].contents, swapin, 1 << b);
			line[swapout].tag = tag(addr);
			line[swapout].valid = 1;
			line[swapout].dirty = false;
			free(swapin);
		}
		memcpy((char*)line[swapout].contents + block_idx(addr), src, _len);
		line[swapout].last_access_time = cnt++;
		line[swapout].dirty = true;//write-back

		src = (char*)src + _len;
		len -= _len;
		addr += _len;
	}
	return true;
}

