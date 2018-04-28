#include "mem.h"
#include "bfd.h"
#include <cstring>
#include <algorithm>
#include <cstdio>
using std::min;

Memory::Memory()
{
	static_assert(sizeof(page_table_entry) == (1 << 3), "pte size error");
	pde = new page_table_entry[1 << 9];
	memset(pde, 0, 1 << 12);

	char buf[8];
	cacheI = cacheD = nullptr;
	for(int i = cache_level - 1; i > 0; i--)
	{
		snprintf(buf, 8, "L%d", i + 1);
		Cache *nc = new Cache(buf, cache_params[i][0], cache_params[i][1], cache_params[i][2]);
		nc->next = cacheD;
		cacheD = nc;
	}
	snprintf(buf, 8, "L1I");
	cacheI = new Cache(buf, cache_params[0][0], cache_params[0][1], cache_params[0][2]);
	cacheI->next = cacheD;
	snprintf(buf, 8, "L1D");
	Cache *nc = new Cache(buf, cache_params[0][0], cache_params[0][1], cache_params[0][2]);
	nc->next = cacheD;
	cacheD = nc;

	tlb = new tlb_entry*[1 << tlb_params[0]];
	for(int i = 0; i < (1 << tlb_params[0]); i++)
	{
		tlb[i] = new tlb_entry[tlb_params[1]];
		memset(tlb[i], 0, tlb_params[1] * sizeof(tlb_entry));
	}
}
Memory::~Memory()
{
	free_pages(pde);
	delete[] pde;
	delete cacheI;
	cacheI->next = nullptr;
	delete cacheD;
	for(int i = 0; i < tlb_params[1]; i++)
		delete[] tlb[i];
	delete[] tlb;
}

void Memory::assign_page(page_table_entry *pte)
{
	if(pte->avail) return;
	char *addr = new char[1 << 12];
	if(!addr) return;
	pte->ppn = reinterpret_cast<long long int>(addr);
	pte->avail = 1;
	memset(addr, 0, 1 << 12);
}
void Memory::free_one_page(page_table_entry *pte)
{
	if(!pte->avail) return;
	char *addr = reinterpret_cast<char*>(pte->ppn);
	if(pte->final)
	{
		delete[] addr;
	}
	else
	{
		free_pages((page_table_entry*)addr);
		delete[] addr;
	}
	pte->avail = 0;
	pte->ppn = 0;
}
void Memory::free_pages(page_table_entry *pt)
{
	for(int i = 0; i < (1 << 9); i++)
	{
		free_one_page(&pt[i]);
	}
	return;
}
void Memory::allocate_range(unsigned long long int addr, int len, int mod, int pl)
{
	for(ulli s = addr & (~0xfffull); s < addr + len; s += 0x1000)
	{
		for(int i = 1; i <= 4; i++)
		{
			page_table_entry *tmp = (page_table_entry*)translate(s, mod, 0, i);
			if(tmp->avail != 1)
			{
				assign_page(tmp);
				if(i == 4)
				{
					tmp->final = 1;
					tmp->PL = pl;
				}
			}
			if(i == 4)
			{
				if(mod & mod_r) tmp->R = 1;
				if(mod & mod_w) tmp->W = 1;
				if(mod & mod_x) tmp->X = tmp->R = 1;
			}
		}
	}
}

void* Memory::translate(unsigned long long int addr, int mod, int pl, int level)
{
	addr_split spliter;
	spliter.addr = addr;
	page_table_entry *ptr = nullptr;

	int swapout = 0;
	tlb_entry *line = tlb[spliter.set_idx()];
	bool hit = false;
	if((level <= 0 || level > 4) && pl > 0)
	{
		stats->summary("Memory: TLB access");
		int i;
		for(i = 0; i < tlb_params[1]; i++)
		{
			if(line[i].valid == 1 && line[i].tag == spliter.tag())
			{
				ptr = line[i].pte;
				swapout = i;
				hit = true;
				break;
			}
			if(line[i].valid == 0)
				swapout = i;
			else if(line[i].last_access_time < line[swapout].last_access_time)
				swapout = i;
		}
		if(!hit)
			stats->summary("Memory: TLB miss");
		else
			stats->summary("Memory: TLB hit");
	}

	if(!ptr)
	{
		ptr = pde;
		ptr = &ptr[spliter.vpn1()];
		if(level == 1) return ptr;
		if(ptr->avail != 1) return NULL;
		ptr = reinterpret_cast<page_table_entry*>(ptr->ppn);

		ptr = &ptr[spliter.vpn2()];
		if(level == 2) return ptr;
		if(ptr->avail != 1) return NULL;
		ptr = reinterpret_cast<page_table_entry*>(ptr->ppn);

		ptr = &ptr[spliter.vpn3()];
		if(level == 3) return ptr;
		if(ptr->avail != 1) return NULL;
		ptr = reinterpret_cast<page_table_entry*>(ptr->ppn);

		ptr = &ptr[spliter.vpn4()];
		if(level == 4) return ptr;	
		if(ptr->avail != 1) return NULL;
	}

	if(pl > ptr->PL) return NULL;
	if(pl == ptr->PL)
	{
		if(ptr->R != 1 && (mod & mod_r) != 0) return NULL;
		if(ptr->W != 1 && (mod & mod_w) != 0) return NULL;
		if(ptr->X != 1 && (mod & mod_x) != 0) return NULL;	
	}
	if(ptr->W == 1) ptr->dirty = 1;

	if(!hit && pl > 0)
	{
		line[swapout].valid = 1;
		line[swapout].pte = ptr;
		line[swapout].tag = spliter.tag();
	}
	if(pl > 0)
		line[swapout].last_access_time = stats->get("Machine: coarse cycle");

	ptr = reinterpret_cast<page_table_entry*>(ptr->ppn);

	return reinterpret_cast<char*>(ptr) + spliter.vpo();
}

int Memory::read(ulli addr, void *target, int len, int pl)
{
	Cache *c;
	int mod = mod_r;
	if(len < 0)
	{
		c = cacheI;
		len = -len;
		mod |= mod_x;
	}
	else
		c = cacheD;

	addr_split spliter;

	void* _target = malloc(len);
	int i = 0;

	while(i < len)
	{
		spliter.addr = addr;
		void* pa = translate(addr, mod, pl);
		if(!pa)
		{
			free(_target);
			return 1;
		}

		int _len = min((1 << 12) - spliter.vpo(), (uint)len - i);
		c->read(reinterpret_cast<ulli>(pa), (char*)_target + i, _len);
		addr += _len;
		i += _len;
	}

	memcpy(target, _target, len);
	free(_target);

	return 0;
}

int Memory::write(ulli addr, const void *src, int len, int pl)
{
	addr_split spliter;
	int i = 0;
	while(i < len)
	{
		spliter.addr = addr;
		void* pa = translate(addr + i, mod_w, pl);
		if(!pa) return 1;

		int _len = min((1 << 12) - spliter.vpo(), (uint)len);
		i += _len;
	}

	i = 0;

	while(i < len)
	{
		spliter.addr = addr;
		void* pa = translate(addr, mod_w, pl);
		if(!pa) return 1;

		int _len = min((1 << 12) - spliter.vpo(), (uint)len);
		cacheD->write(reinterpret_cast<ulli>(pa), (const char*)src + i, _len);
		addr += _len;
		i += _len;
	}
	return 0;
}

int Memory::direct_write(ulli addr, const void *src, int len)
{
	addr_split spliter;
	int i = 0;
	int pl = 0;
	while(i < len)
	{
		spliter.addr = addr;
		void* pa = translate(addr + i, mod_w, pl);
		if(!pa) return 1;

		int _len = min((1 << 12) - spliter.vpo(), (uint)len);
		i += _len;
	}

	i = 0;

	while(i < len)
	{
		spliter.addr = addr;
		void* pa = translate(addr, mod_w, pl);
		if(!pa) return 1;

		int _len = min((1 << 12) - spliter.vpo(), (uint)len);
		memcpy(pa, (const char*)src + i, _len);
		addr += _len;
		i += _len;
	}
	return 0;
}

bool Memory::load_file(int argc, char* argv[], char* envp[],
	long long int& entry,
	long long int& stack_ptr,
	long long int& global_pointer)
{
	if(argc < 2) return false;
	dbg_print("opening target file: %s\n", argv[1]);
	bfd* abfd = bfd_openr(argv[1], NULL);
	if(!abfd) return false;
	if(!bfd_check_format(abfd, bfd_object))
	{
		bfd_close(abfd);
		return false;
	}

	allocate_range(0xffff800000000000ull, 0x100000, mod_w | mod_r, 3);//argv and envp
	allocate_range(0x7fffff800000ull, 0x800000, mod_w | mod_r, 3);//stack
	{
		char zeros[]={0,0,0,0,0,0,0,0};
		ulli ptr = 0xffff800000000000ull;
		stack_ptr = 0x7ffffffff000ull;
		int i = 0;
		while(envp[i]) i++;
		for(i--; i >= 0; i--)
		{
			int len = strlen(envp[i]);
			direct_write(ptr, envp[i], len);
			stack_ptr -= 8;
			direct_write(stack_ptr, &ptr, 8);
			ptr += len;
		}
		stack_ptr -= 8;
		direct_write(stack_ptr, zeros, 8);
		for(i = argc - 1; i > 0; i--)
		{
			int len = strlen(argv[i]);
			direct_write(ptr, argv[i], len);
			stack_ptr -= 8;
			direct_write(stack_ptr, &ptr, 8);
			ptr += len;
		}
		stack_ptr -= 8;
		argc -= 1;
		direct_write(stack_ptr, &argc, 4);
		dbg_print("init stack pointer: %llx\n", stack_ptr);
	}

	for(asection *p = abfd->sections; p != NULL; p = p->next)
	{
		dbg_print("%s: %lx %lx\n", p->name, p->vma, p->vma + p->size);
		if(!strstr(p->name, "bss"))
		{
		if(!(p->flags & SEC_ALLOC)) continue;
		if(!(p->flags & SEC_HAS_CONTENTS)) continue;	
		}
		//if(strstr(p->name, ".debug")) continue;

		int mod = mod_r;
		if(!(p->flags & SEC_READONLY))
			mod |= mod_w;
		if(p->flags & SEC_CODE)
			mod |= mod_x;
		allocate_range(p->vma, p->size, mod, 3);
		dbg_print("allocated\n");

		if(!(p->flags & SEC_LOAD)) continue;
		bfd_byte *buf;
		bfd_malloc_and_get_section(abfd, p, &buf);
		for(unsigned long int i = 0; i < p->size; i++)
		{
			direct_write(p->vma + i, &buf[i], sizeof(bfd_byte));
		}
		free(buf);
	}

	asymbol **sym_tab = (asymbol**)malloc(bfd_get_symtab_upper_bound(abfd));
	int nos = bfd_canonicalize_symtab(abfd, sym_tab);
	for(int i = 0; i < nos; i++)
	{
		symbol_info info;
		bfd_symbol_info(sym_tab[i], &info);
		if(!strcmp(info.name, "main"))
		{
			entry = info.value;
		}
		if(!strcmp(info.name, "__global_pointer$"))
		{
			global_pointer = info.value;
		}
	}
	free(sym_tab);

	//entry = bfd_get_start_address(abfd);

	bfd_close(abfd);
	return true;
}

