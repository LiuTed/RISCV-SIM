#include <cstdio>
#include "machine.h"
#include "stats.h"
#include "bfd.h"

Statistics *stats;

int main(int argc, char* argv[], char* envp[])
{
	bfd_init();
	stats = new Statistics;
	Memory* mem = new Memory;
	lli entry, stackpointer, globalpointer;
	if(!mem->load_file(argc, argv, envp, entry, stackpointer, globalpointer))
		exit(1);
	Machine* machine = new Machine;
	machine->set_memory(mem);
	machine->set_register(PCReg, entry);
	machine->set_register(EndPCReg, 0);
	machine->set_register(ra, 0);
	machine->set_register(sp, stackpointer);
	machine->set_register(gp, globalpointer);
	machine->run();
	stats->propagate();
	stats->print(stderr);
}