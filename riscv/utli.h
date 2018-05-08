#ifndef UTLI_H
#define UTLI_H
typedef unsigned int uint;
typedef long long int lli;
typedef unsigned long long int ulli;
typedef unsigned short ushort;

#define num_registers 72//with hidden registers, like pc
#define num_usr_regs 64
#define num_int_regs 32
#define cache_level 3
#define ib_size 16
const int cache_params[cache_level][3] = {
	//s, L, b
	{6, 8, 6},//32KB * 2
	{9, 8, 6},//256KB
	{14, 8, 6}//8MB
};
const int tlb_params[2] = {
	//s, L
	10, 6//6K
};
#define _as(val, type) (*reinterpret_cast<type*>(&val))
template<typename T, typename N>
T treat_as(N& n)
{
	static_assert(sizeof(T) == sizeof(N), "cast between types with different sizes");
	return _as(n, T);
}
template<typename T, typename N>
const T treat_as(const N& n)
{
	static_assert(sizeof(T) == sizeof(N), "cast between types with different sizes");
	return _as(n, const T);
}

#ifdef DEBUG
#define dbg_print(info, ...) fprintf(stderr, info, ##__VA_ARGS__);
#else
#define dbg_print(...)
#endif

#endif
