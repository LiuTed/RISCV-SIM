#ifndef MACHINE_H
#define MACHINE_H
#include "mem.h"
#include "stats.h"
#include "regfile.h"
#include "instr.h"
#include "utli.h"
#include "branch_pred.h"
#include <cstdio>
#include <set>

#define ra 1
#define sp 2
#define gp 3
#define a0 10
#define a1 11
#define f0 32
#define fa1 43
#define PCReg 64
#define EndPCReg 65

extern Statistics *stats;
#define icode 0
#define ifun 1
#define rA 2
#define rB 3
#define valA 2
#define valB 3
//rA and valA will not exist at the same time
#define valC 4
#define valP 5
//valP holds the pc of current instruction
#define valE 6
#define dstE 7
#define valM 8
#define dstM 9
#define pj 9
//if pred jump
#define rD 10
#define valD 10
#define newP 10
//rD is used in R4 type(in RV32F)
//newP is used in branch and c.j(al)r
#define NumofPipReg 11
//define pipeline registers
//assume that each stage has the same num of regs

#define Stall 1
#define Bubble 2
//define pipeline registers status

#define HALT 1

#define unknown_instruction 1
#define unsupported 2
#define illegal_instruction 3
#define unaligned_instruction 4
#define syscall 5
#define unknown_syscall 6
#define illegal_address 7
#define unknown_error -1
//define exceptions code

class Machine
{
private:
	RegFile* regfile;
	Memory* memory;

	struct
	{
		uint SS : 1;//single step
		uint BRK: 1;//break
		uint SC : 1;//facing syscall
		uint AC : 1;//instruction alignment check
		uint MS : 2;//machine status, like halt
		uint PL : 2;//privilage level, not used
	} CSR;
	struct
	{
		uint reserved : 24;
		uint frm : 3;
		#define RNE 0
		#define RTZ 1
		#define RDN 2
		#define RUP 3
		#define RMM 4
		#define RDY 7//invalid in FCSR, valid in rm's field
		uint NV : 1;
		uint DZ : 1;
		uint OF : 1;
		uint UF : 1;
		uint NX : 1;
	} FCSR;

	lli predPC, F_status;
	int incPC;//length of the instruction just fetched
	lli
		f_[NumofPipReg],
		D_[NumofPipReg], D_status,
		d_[NumofPipReg],
		E_[NumofPipReg], E_status,
		e_[NumofPipReg],
		M_[NumofPipReg],
		m_[NumofPipReg],
		W_[NumofPipReg];//pipeline registers
	void IF();
	void ID();
	void EX();
	void MEM();
	void WB();//5 stage pipeline
	void pipeline_forward();//data forwarding and pipe-reg propagation and PC update

	void pipeline_flush();
	//flush mem and wb stage
	//redo if and id stage
	//used when ex traped
	//ensure all instructions before has been done
	//and all instructions after get the correct data
	void breakpoint();
	BranchPredict predictor;
	std::set<lli> bps;
public:
	Machine();
	~Machine();
	void set_register(int id, lli val)
	{
		regfile->set(id, val);
	}
	void set_memory(Memory* mem)
	{
		memory = mem;
	}
	void run();
	void print_regfile(FILE* = stdout);
	void print_pipe_regs(FILE* = stdout);
	void raise_exception(int, lli);
	void single_step()
	{
		CSR.SS = 1;
	}
};
#endif
