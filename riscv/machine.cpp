#include "machine.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cfenv>
#include <cmath>
#include <set>
using namespace std;
Machine::Machine():predictor(),bps()
{
	regfile = new RegFile(num_registers);
	CSR.MS = 0;
	CSR.AC = 1;
	CSR.BRK = 0;
	CSR.SS = 0;
	CSR.SC = 0;
	CSR.PL = 3;
}

Machine::~Machine()
{
	delete regfile;
	delete memory;
}

void Machine::breakpoint()
{
#ifndef SDB
	return;
#endif
	static int cnt = 0;
	cnt--;
	if(cnt > 0) return;
	char cmd;
	fprintf(stderr, "break at: %llx\n", E_[valP]);
	do
	{
		fprintf(stderr, "\r>");
		fflush(stderr);
		cmd = getchar();
		switch(cmd)
		{
			case 'p':
				print_pipe_regs(stderr);
				break;
			case 'r':
				print_regfile(stderr);
				break;
			case 'S':
				scanf("%d", &cnt);
			case 's':
				CSR.SS = 1;
				break;
			case 'C':
				scanf("%d", &cnt);
			case 'c':
				CSR.SS = 0;
				break;
			case 'm':
			{
				ulli addr;
				int len;
				scanf("%llx%d", &addr, &len);
				for(int i = 0; i < len; i++)
				{
					char buf;
					memory->read(addr, &buf, 1, 0);
					fprintf(stderr, "%02x", (int)(unsigned char)buf);
				}
				fprintf(stderr, "\n");
				break;
			}
			case 'j':
			{
				ulli addr;
				scanf("%llx", &addr);
				predPC = addr;
				memset(E_, -1, sizeof(E_));
				memset(D_, -1, sizeof(D_));
				break;
			}
			case 'b':
			{
				ulli addr;
				scanf("%llx", &addr);
				bps.insert(addr);
				break;
			}
			default:
				break;
		}
	}while(tolower(cmd) != 'c' && tolower(cmd) != 's');
	CSR.BRK = 0;
}
void Machine::run()
{
	predPC = regfile->get(PCReg);
	memset(D_, -1, sizeof(D_));
	memset(E_, -1, sizeof(E_));
	memset(M_, -1, sizeof(M_));
	memset(W_, -1, sizeof(W_));
	memset(f_, -1, sizeof(f_));
	memset(d_, -1, sizeof(d_));
	memset(e_, -1, sizeof(e_));
	memset(m_, -1, sizeof(m_));//clear pipe-regs
	F_status = D_status = E_status = 0;
#ifdef DEBUG
	print_pipe_regs();
#endif
	breakpoint();
	while(CSR.MS != HALT)
	{
		IF();
		ID();
		EX();
		MEM();
		WB();
		pipeline_forward();
		if(CSR.SC == 1)
			pipeline_flush();
		if(CSR.SS == 1 || CSR.BRK == 1 || bps.count(E_[valP]))//breakpoint
		{
			breakpoint();
		}
		stats->summary("Machine: coarse cycle");
	}
#ifdef DEBUG
	fprintf(stderr, "halt!\n");
	print_regfile();
#endif
}

static const int
LOAD = 0,
LOAD_FP = 1,
MISC_MEM = 3,
OP_IMM = 4,
AUIPC = 5,
OP_IMM_32 = 6,
STORE = 8,
STORE_FP = 9,
AMO = 11,
OP = 12,
LUI = 13,
OP_32 = 14,
MADD = 16,
MSUB = 17,
NMSUB = 18,
NMADD = 19,
OP_FP = 20,
BRANCH = 24,
JALR = 25,
JAL = 27,
SYSTEM = 28,
NOP = -1,
ILLEGAL = -2;

static const int
R = 0,
I = 1,
S = 2,
SB = 3,
U = 4,
UJ = 5,
R4 = 6;

static const int code_type_4[] = {
	 I, I,-1, I, I, U, I,-1,
	 S, S,-1,R4, R, U, R,-1,
	R4,R4,R4,R4, R,-1,-1,-1,
	SB, I,-1,UJ, I,-1,-1,-1
};

static const int
CADDI4SPN = 0,
CFLD = 1,
CLW = 2,
CLD = 3,
CFSD = 5,
CSW = 6,
CSD = 7,
CADDI = 8,
CADDIW = 9,
CLI = 10,
CLUI_CADDI16SP = 11,
CMISC_ALU = 12,
CJ = 13,
CBEQZ = 14,
CBNEZ = 15,
CSLLI = 16,
CFLDSP = 17,
CLWSP = 18,
CLDSP = 19,
CJALR_CADD = 20,
CFSDSP = 21,
CSWSP = 22,
CSDSP = 23;


static const int
OP_ADD = 0,
OP_SUB = 1 << 8,
OP_SLL = 1,
OP_SLT = 2,
OP_SLTU = 3,
OP_XOR = 4,
OP_SRL = 5,
OP_SRA = (1 << 8) | 5,
OP_OR = 6,
OP_AND = 7,
OP_MUL = 8,
OP_MULH = 9,
OP_MULHSU = 10,
OP_MULHU = 11,
OP_DIV = 12,
OP_DIVU = 13,
OP_REM = 14,
OP_REMU = 15;

static const int
OP_ADDI = 0,
OP_SLLI = 1,
OP_SLTI = 2,
OP_SLTIU = 3,
OP_XORI = 4,
OP_SRLI = 5,
OP_SRAI = (1 << 8) | 5,
//srl and sra have the same funct3
OP_ORI = 6,
OP_ANDI = 7;

static const int
BEQ = 0,
BNE = 1,
BLT = 4,
BGE = 5,
BLTU = 6,
BGEU = 7;

static const int
ECALL = 0,
EBREAK = 1;

static const int
OP_FADD = 0,
OP_FSUB = 1,
OP_FMUL = 2,
OP_FDIV = 3,
OP_FSQRT = 11,
OP_FSGNJ = 4,
OP_FMM = 5,
OP_FCVTFW = 24,
OP_FMVFX_FCLASS = 28,
OP_FCMP = 20,
OP_FCVTWF = 26,
OP_FMVXF = 30,
OP_FCVTFF = 8,

OP_FMVFX = 28,
OP_FCLASS = 32 | 28;



void Machine::IF()
{
	if(CSR.AC && ((predPC & 1) == 1))//check alignment
	{
		raise_exception(unaligned_instruction, predPC);
	}
	f_[valP] = predPC;
	f_[newP] = predPC;
	if(predPC == regfile->get(EndPCReg))//end pc
	{
		f_[icode] = NOP;//do nothing
		incPC = 0;//and stall here until a jump instruction executed
		//or the pipeline clear, then halt
		return;
	}
	instruction instr;
	if(memory->read(predPC, &instr.wrap, -4, CSR.PL))
		raise_exception(unknown_error, predPC);//fetch one instruction
	int type;

	dbg_print("%llx: %08x\n", predPC, instr.wrap);

	do
	{
		incPC = 4;//assert it's a 4-byte instr
		//then check the instruction, if illegal, try compressed instr
		if((instr.R.opcode() & 3) != 3)
			break;//no 4-byte instr fit, try compressed.

		dbg_print("%08x\n", instr.wrap);
		f_[icode] = instr.R.opcode() >> 2;
		type = code_type_4[instr.R.opcode() >> 2];//get instr type
		// if(instr.wrap == 0b10011)//addi x0, x0, 0, a.k.a. nop
		// {
		// 	f_[icode] = NOP;
		// 	return;
		// }
		if(f_[icode] == MISC_MEM)//fence, do nothing actually
		{
			if(instr.I.rs1() != 0 || instr.I.rd() != 0)
				break;
			if((instr.I.imm() >> 8) != 0)
				break;
			if(instr.I.imm() == 0)
			{
				if(instr.I.funct3() != 1)
					break;
			}
			else if(instr.I.funct3() != 0)
			{
				break;
			}//check the instruction
		}
		if(f_[icode] == SYSTEM &&
			(instr.wrap != 0x73 && instr.wrap != 0x100073))
			break;//system instruction only support ecall and ebreak
		if(f_[icode] == LOAD && instr.I.rd() == 0)
			break;//load x0 is illegal

		switch(type)
		{
			case R:
				if((instr.R.funct7() & 0x2) != 0)
					break;
				f_[ifun] = (instr.R.funct7() << 3)
						|(instr.R.funct3())
						;
				f_[rA] = (instr.R.rs1());
				f_[rB] = (instr.R.rs2());
				f_[dstE] = (instr.R.rd());
				if(f_[icode] == OP_FP)
				{
					f_[ifun] = instr.R.funct5();
					f_[valC] = (instr.R.fmt() << 3) | instr.R.rm();
					if(f_[ifun] == OP_FSQRT)
					{
						if(f_[rB] != 0)
							break;
						f_[rA] += num_int_regs;
						f_[dstE] += num_int_regs;
					}
					else if(f_[ifun] == OP_FMVFX_FCLASS)
					{
						if(f_[rB] != 0)
							break;
						if(instr.R.rm() == 0)
						{
							f_[ifun] = OP_FMVFX;
							f_[rA] += num_int_regs;
						}
						else if(instr.R.rm() == 1)
						{
							f_[ifun] = OP_FCLASS;
							f_[rA] += num_int_regs;
						}
						else
							break;
					}
					else if(f_[ifun] == OP_FMVXF)
					{
						if(f_[rB] != 0 || instr.R.rm() != 0)
							break;
						f_[dstE] += num_int_regs;
					}
					else if(f_[ifun] == OP_FCVTFW)
					{
						if(f_[rB] > 3)
							break;
						f_[rA] += num_int_regs;
					}
					else if(f_[ifun] == OP_FCVTWF)
					{
						if(f_[rB] > 3)
							break;
						f_[dstE] += num_int_regs;
					}
					else if(f_[ifun] == OP_FCVTFF)
					{
						if(f_[rB] > 1)
							break;
						if(f_[rB] == ((f_[valC] >> 3 & 1)))
							break;
						f_[dstE] += num_int_regs;
						f_[rA] += num_int_regs;
					}
					else if(f_[ifun] == OP_FSGNJ)
					{
						if(instr.R.rm() > 2)
							break;
						f_[rA] += num_int_regs;
						f_[rB] += num_int_regs;
						f_[dstE] += num_int_regs;
					}
					else if(f_[ifun] == OP_FMM)
					{
						if(instr.R.rm() > 1)
							break;
						f_[rA] += num_int_regs;
						f_[rB] += num_int_regs;
						f_[dstE] += num_int_regs;
					}
					else
					{
						f_[rA] += num_int_regs;
						f_[rB] += num_int_regs;
						f_[dstE] += num_int_regs;
					}
				}
				return;
			case I:
				f_[ifun] = (instr.I.funct3());
				f_[rA] = (instr.I.rs1());
				f_[valC] = (instr.I.imm());
				f_[dstE] = (instr.I.rd());
				if(f_[icode] == OP_IMM)
				{
					if(f_[ifun] == OP_SLLI)
					{
						if((f_[valC] >> 6) != 0)
							break;
					}
					else if(f_[ifun] == OP_SRLI)
					{
						if((f_[valC] >> 6) != 0
							&&(f_[valC] >> 6) != 0x10)
							break;
					}
				}
				else if(f_[icode] == OP_IMM_32)
				{
					if(f_[ifun] == OP_SLLI)
					{
						if((f_[valC] >> 5) != 0)
							break;
					}
					else if(f_[ifun] == OP_SRLI)
					{
						if((f_[valC] >> 5) != 0
							&&(f_[valC] >> 5) != 0x20)
							break;
					}
				}
				if(f_[icode] == LOAD_FP)
				{
					if(f_[ifun] > 3 || f_[ifun] < 2) break;
					f_[dstE] += num_int_regs;
				}
				return;
			case S:
				f_[ifun] = (instr.S.funct3());
				f_[rA] = (instr.S.rs1());
				f_[rB] = (instr.S.rs2());
				f_[valC] = (instr.S.immh() << 5)
						|((int)instr.S.imml());
				if(f_[icode] == STORE_FP)
				{
					if(f_[ifun] > 3 || f_[ifun] < 2) break;
					f_[rB] += num_int_regs;
				}
				return;
			case SB:
				f_[ifun] = (instr.SB.funct3());
				f_[rA] = (instr.SB.rs1());
				f_[rB] = (instr.SB.rs2());
				f_[valC] = (instr.SB.imm1() << 12)
						|((int)instr.SB.imm2() << 11)
						|((int)instr.SB.imm3() << 5)
						|((int)instr.SB.imm4() << 1)
						;
				return;
			case U:
				f_[dstE] = (instr.U.rd());
				f_[valC] = (instr.U.imm() << 12);
				return;
			case UJ:
				f_[dstE] = (instr.UJ.rd());
				f_[valC] = (instr.UJ.imm1() << 20)
						|((int)instr.UJ.imm2() << 12)
						|((int)instr.UJ.imm3() << 11)
						|((int)instr.UJ.imm4() << 1)
						;
				return;
			case R4:
				f_[valC] = (instr.R4.funct2() << 3)
						|(instr.R4.funct3())
						;
				f_[rA] = (instr.R4.rs1()) + num_int_regs;
				f_[rB] = (instr.R4.rs2()) + num_int_regs;
				f_[rD] = (instr.R4.rs3()) + num_int_regs;
				f_[dstE] = (instr.R4.rd()) + num_int_regs;
				return;
			default:
				break;
		}
	}while(false);
	//could not be interpreted as a 4-byte instruction
	//try to interpreted as a 2-byte compressed instruction
	do
	{
		incPC = 2;
		dbg_print("%04hx\n", instr.CR.val);
		if(instr.CR.op() == 3)//reserved
		{
			break;
		}
		if(instr.CR.val == 0)//illegal
		{
			break;
		}

		lli tmp = (instr.CI.op() << 3) | (instr.CI.funct3());

		switch(tmp)//try to reinterpret to a 4-byte instrction
		{
			case CADDI4SPN:
			{
				f_[icode] = OP_IMM;
				f_[ifun] = OP_ADDI;
				ulli imm = instr.CIW.imm();
				imm = ((imm & 1) << 3)
					| ((imm & 2) >> 1 << 2)
					| ((imm & 0x3c) >> 2 << 6)
					| ((imm & 0xc0) >> 6 << 4)
					;
				f_[valC] = imm;
				f_[rA] = 2;
				f_[dstE] = instr.CIW.rd_() + 8;
				return;
			}
			case CLW:
			{
				f_[icode] = LOAD;
				f_[ifun] = 2;
				ulli imm1 = instr.CL.imm1(),
				imm2 = instr.CL.imm2();
				imm1 = (imm1 << 3)
					 | ((imm2 & 2) >> 1 << 2)
					 | ((imm2 & 1) << 6)
					 ;
				f_[rA] = instr.CL.r1_() + 8;
				f_[valC] = imm1;
				f_[dstE] = instr.CL.rd_() + 8;
				return;
			}
			case CLD:
			{
				f_[icode] = LOAD;
				f_[ifun] = 3;
				ulli imm1 = instr.CL.imm1(),
				imm2 = instr.CL.imm2();
				imm1 = (imm1 << 3)
					 | ((imm2 & 2) >> 1 << 7)
					 | ((imm2 & 1) << 6)
					 ;
				f_[rA] = instr.CL.r1_() + 8;
				f_[valC] = imm1;
				f_[dstE] = instr.CL.rd_() + 8;
				return;
			}
			case CFLD:
			{
				f_[icode] = LOAD_FP;
				f_[ifun] = 3;
				f_[rA] = instr.CL.r1_() + 8;
				f_[dstE] = instr.CL.rd_() + 8 +num_int_regs;
				f_[valC] = (instr.CL.imm1() << 3)
						 | (instr.CL.imm2() << 6)
						 ;
				return;
			}
			case CSW:
			{
				f_[icode] = STORE;
				f_[ifun] = 2;
				ulli imm1 = instr.CS.imm1(),
				imm2 = instr.CS.imm2();
				imm1 = (imm1 << 3)
					 | ((imm2 & 2) >> 1 << 2)
					 | ((imm2 & 1) << 6)
					 ;
				f_[rA] = instr.CS.r1_() + 8;
				f_[rB] = instr.CS.r2_() + 8;
				f_[valC] = imm1;
				return;
			}
			case CSD:
			{
				f_[icode] = STORE;
				f_[ifun] = 3;
				ulli imm1 = instr.CS.imm1(),
				imm2 = instr.CS.imm2();
				imm1 = (imm1 << 3)
					 | ((imm2 & 2) >> 1 << 7)
					 | ((imm2 & 1) << 6)
					 ;
				f_[rA] = instr.CS.r1_() + 8;
				f_[rB] = instr.CS.r2_() + 8;
				f_[valC] = imm1;
				return;
			}
			case CFSD:
			{
				f_[icode] = STORE_FP;
				f_[ifun] = 3;
				f_[rA] = instr.CS.r1_() + 8;
				f_[rB] = instr.CS.r2_() + 8 + num_int_regs;
				f_[valC] = (instr.CS.imm1() << 3)
						 | (instr.CS.imm2() << 6)
						 ;
				return;
			}
			case CADDI:
			{
				f_[icode] = OP_IMM;
				f_[ifun] = OP_ADDI;
				f_[rA] = f_[dstE] = instr.CI.r1();
				f_[valC] = (instr.CI.imm1() << 5)
						 | (instr.CI.imm2());
				f_[valC] = (f_[valC] << 58) >> 58;
				// if(f_[valC] == 0)
				// {
				// 	f_[icode] = NOP;
				// }
				return;
			}
			case CADDIW:
			{
				f_[icode] = OP_IMM_32;
				f_[ifun] = OP_ADDI;
				f_[rA] = instr.CI.r1();
				if(f_[rA] == 0)
					break;
				f_[dstE] = f_[rA];
				f_[valC] = (instr.CI.imm1() << 5)
						 | (instr.CI.imm2());
				f_[valC] = (f_[valC] << 58) >> 58;
				return;
			}
			case CLI:
			{
				f_[icode] = OP_IMM;
				f_[ifun] = OP_ADDI;
				f_[rA] = 0;
				f_[dstE] = instr.CI.r1();
				if(f_[dstE] == 0)
					break;
				f_[valC] = (instr.CI.imm1() << 5)
						 | (instr.CI.imm2());
				f_[valC] = (f_[valC] << 58) >> 58;
				return;
			}
			case CLUI_CADDI16SP:
			{
				
				f_[valC] = (instr.CI.imm1())
						 | (instr.CI.imm2());
				if(f_[valC] == 0)
					break;
				f_[dstE] = instr.CI.r1();
				if(f_[dstE] == 0)
					break;
				if(f_[dstE] == 2)
				{
					f_[icode] = OP_IMM;
					f_[ifun] = OP_ADDI;
					ulli imm1 = instr.CI.imm1(),
					imm2 = instr.CI.imm2();
					imm1 = (imm1 << 9)
						 | ((imm2 & 1) << 5)
						 | ((imm2 & 6) >> 1 << 7)
						 | ((imm2 & 8) >> 3 << 6)
						 | ((imm2 & 16) >> 4 << 4)
						 ;
					f_[valC] = imm1;
					f_[valC] = (f_[valC] << 54) >> 54;
					f_[rA] = f_[dstE];
				}
				else
				{
					f_[icode] = LUI;
					f_[valC] = (instr.CI.imm1() << 17)
							 | (instr.CI.imm2() << 12);
				}
				return;
			}
			case CMISC_ALU:
			{
				switch(instr.CB.funct2())
				{
					case 0:
					{
						f_[icode] = OP_IMM;
						f_[ifun] = OP_SRLI;
						f_[valC] = ((instr.CB.offset1() & 4) >> 2 << 5)
								 | instr.CB.offset2();
						if(f_[valC] == 0)
							break;
						f_[rA] = f_[dstE] = instr.CB.r1_() + 8;
						return;
					}
					case 1:
					{
						f_[icode] = OP_IMM;
						f_[ifun] = OP_SRAI;
						f_[valC] = ((instr.CB.offset1() & 4) >> 2 << 5)
								 | instr.CB.offset2();
						if(f_[valC] == 0)
							break;
						f_[rA] = f_[dstE] = instr.CB.r1_() + 8;
						return;
					}
					case 2:
					{
						f_[icode] = OP_IMM;
						f_[ifun] = OP_ANDI;
						f_[valC] = ((instr.CB.offset1() & 4) >> 2 << 5)
								 | instr.CB.offset2();
						f_[valC] = (f_[valC] << 58) >> 58;
						f_[rA] = f_[dstE] = instr.CB.r1_() + 8;
						return;
					}
					case 3:
					{
						if(instr.CS.imm1() & 4)
						{
							f_[icode] = OP_32;
							if(instr.CS.imm2() & 2)
								break;
							if(instr.CS.imm2() == 0)
								f_[ifun] = OP_SUB;
							else
								f_[ifun] = OP_ADD;
						}
						else
						{
							f_[icode] = OP;
							switch(instr.CS.imm2())
							{
								case 0:
									f_[ifun] = OP_SUB;
									break;
								case 1:
									f_[ifun] = OP_XOR;
									break;
								case 2:
									f_[ifun] = OP_OR;
									break;
								case 3:
									f_[ifun] = OP_AND;
									break;
								default:
									assert(false);
							}
						}
						f_[rA] = f_[dstE] = instr.CS.r1_() + 8;
						f_[rB] = instr.CS.r2_() + 8;
						return;
					}
					default:
						assert(false);
				}
				break;
			}
			case CJ:
			{
				f_[icode] = JAL;
				f_[dstE] = 0;
				ulli imm = instr.CJ.jt();
				imm = ((imm & 1) << 5)
					| ((imm & 0xe) >> 1 << 1)
					| ((imm & 0x10) >> 4 << 7)
					| ((imm & 0x20) >> 5 << 6)
					| ((imm & 0x40) >> 6 << 10)
					| ((imm & 0x180) >> 7 << 8)
					| ((imm & 0x200) >> 9 << 4)
					| ((imm & 0x400) >> 10 << 11)
					;
				f_[valC] = (lli)(imm << 52) >> 52;
				f_[newP] = f_[valP] - 2;
				return;
			}
			case CBEQZ:
			{
				f_[icode] = BRANCH;
				f_[ifun] = BEQ;
				f_[rA] = instr.CB.r1_() + 8;
				f_[rB] = 0;
				ulli imm1 = instr.CB.offset1(),
				imm2 = instr.CB.offset2();
				imm1 = ((imm1 & 4) >> 2 << 8)
					 | ((imm1 & 3) << 3)
					 | ((imm2 & 0x18) >> 3 << 6)
					 | ((imm2 & 6) >> 1 << 1)
					 | ((imm2 & 1) << 5)
					 ;
				f_[valC] = (lli)(imm1 << 55) >> 55;
				f_[newP] = f_[valP] - 2;
				return;
			}
			case CBNEZ:
			{
				f_[icode] = BRANCH;
				f_[ifun] = BNE;
				f_[rA] = instr.CB.r1_() + 8;
				f_[rB] = 0;
				ulli imm1 = instr.CB.offset1(),
				imm2 = instr.CB.offset2();
				imm1 = ((imm1 & 4) >> 2 << 8)
					 | ((imm1 & 3) << 3)
					 | ((imm2 & 0x18) >> 3 << 6)
					 | ((imm2 & 6) >> 1 << 1)
					 | ((imm2 & 1) << 5)
					 ;
				f_[valC] = (lli)(imm1 << 55) >> 55;
				f_[newP] = f_[valP] - 2;
				return;
			}
			case CSLLI:
			{
				f_[icode] = OP_IMM;
				f_[ifun] = OP_SLLI;
				f_[rA] = f_[dstE] = instr.CI.r1();
				if(f_[rA] == 0)
					break;
				f_[valC] = (instr.CI.imm1() << 5)
						 | (instr.CI.imm2());
				if(f_[valC] == 0)
					break;
				return;
			}
			case CJALR_CADD:
			{
				if(instr.CR.r2() == 0)
				{
					f_[rA] = instr.CR.r1();
					if(f_[rA] == 0)
					{
						f_[icode] = SYSTEM;
						f_[valC] = EBREAK;
						f_[ifun] = 0;
						f_[dstE] = 0;
					}
					else
					{
						f_[icode] = JALR;
						f_[newP] = f_[valP] - 2;
						f_[valC] = 0;
						f_[dstE] = instr.CR.funct4() & 1;	
					}
				}
				else
				{
					f_[icode] = OP;
					f_[ifun] = OP_ADD;
					f_[dstE] = instr.CR.r1();
					if(f_[dstE] == 0) break;
					f_[rB] = instr.CR.r2();
					f_[rA] = ((instr.CR.funct4() & 1)? f_[dstE]: 0);
				}
				return;
			}
			case CLDSP:
			{
				f_[icode] = LOAD;
				f_[ifun] = 3;
				f_[rA] = sp;
				f_[dstE] = instr.CI.r1();
				if(f_[dstE] == 0) break;
				ulli imm1 = instr.CI.imm1(),
				imm2 = instr.CI.imm2();
				f_[valC] = (imm1 << 5)
						 | ((imm2 & 7) << 6)
						 | ((imm2 & 0x18) >> 3 << 3)
						 ;
				return;
			}
			case CLWSP:
			{
				f_[icode] = LOAD;
				f_[ifun] = 2;
				f_[rA] = sp;
				f_[dstE] = instr.CI.r1();
				if(f_[dstE] == 0) break;
				ulli imm1 = instr.CI.imm1(),
				imm2 = instr.CI.imm2();
				f_[valC] = (imm1 << 5)
						 | ((imm2 & 3) << 6)
						 | ((imm2 & 0x1c) >> 2 << 2)
						 ;
				return;
			}
			case CFLDSP:
			{
				f_[icode] = LOAD_FP;
				f_[ifun] = 3;
				f_[rA] = sp;
				f_[dstE] = instr.CI.r1() + num_int_regs;
				ulli imm1 = instr.CI.imm1(),
				imm2 = instr.CI.imm2();
				f_[valC] = (imm1 << 5)
						 | ((imm2 & 7) << 6)
						 | ((imm2 & 0x18) >> 3 << 3)
						 ;
				return;
			}
			case CSDSP:
			{
				f_[icode] = STORE;
				f_[ifun] = 3;
				f_[rA] = sp;
				f_[rB] = instr.CSS.r2();
				ulli imm = instr.CSS.imm();
				f_[valC] = ((imm & 7) << 6)
						 | ((imm & 0x38) >> 3 << 3)
						 ;
				return;
			}
			case CSWSP:
			{
				f_[icode] = STORE;
				f_[ifun] = 2;
				f_[rA] = sp;
				f_[rB] = instr.CSS.r2();
				ulli imm = instr.CSS.imm();
				f_[valC] = ((imm & 3) << 6)
						 | ((imm & 0x3c) >> 2 << 2)
						 ;
				return;
			}
			case CFSDSP:
			{
				f_[icode] = STORE_FP;
				f_[ifun] = 3;
				f_[rA] = sp;
				f_[rB] = instr.CSS.r2() + num_int_regs;
				ulli imm = instr.CSS.imm();
				f_[valC] = ((imm & 7) << 6)
						 | ((imm & 0x38) >> 3 << 3)
						 ;
				return;
			}
			default:
				break;
		}
	}while(false);
	//could not be interpreted as 2-byte comp. instr. either
	//this is not an error if a jalr instruction has been fetched
	//but not executed yet
	f_[icode] = ILLEGAL;
	//thus do not raise exception immediately
	
}
void Machine::ID()
{
	d_[icode] = D_[icode];
	d_[ifun] = D_[ifun];
	d_[valP] = D_[valP];//icode, ifun, valP will always be passed
	switch(D_[icode])//decode, do not consider the data forwarding
	{
		case MISC_MEM:
		case NOP:
		case ILLEGAL:
			break;
		case MADD:
		case MSUB:
		case NMADD:
		case NMSUB:
			d_[valC] = D_[valC];
			d_[valA] = regfile->get(D_[rA]);
			d_[valB] = regfile->get(D_[rB]);
			d_[valD] = regfile->get(D_[rD]);
			d_[dstE] = D_[dstE];
			break;
		case OP_FP:
			d_[valC] = D_[valC];
			switch(D_[ifun])
			{
				case OP_FMVXF:
				case OP_FMVFX:
				case OP_FSQRT:
				case OP_FCLASS:
				case OP_FCVTFF:
				case OP_FCVTWF:
				case OP_FCVTFW:
					d_[valB] = D_[rB];
					d_[valA] = regfile->get(D_[rA]);
					d_[dstE] = D_[dstE];
					break;
				default:
					d_[valB] = regfile->get(D_[rB]);
					d_[valA] = regfile->get(D_[rA]);
					d_[dstE] = D_[dstE];
					break;
			}
			break;
		case OP:
			d_[valA] = regfile->get(D_[rA]);
			d_[valB] = regfile->get(D_[rB]);
			d_[dstE] = D_[dstE];
			break;
		case OP_32:
			d_[valA] = (int)regfile->get(D_[rA]);
			d_[valB] = (int)regfile->get(D_[rB]);
			d_[dstE] = D_[dstE];
			break;
		case OP_IMM:
			d_[valA] = regfile->get(D_[rA]);
			d_[dstE] = D_[dstE];
			switch(D_[ifun])
			{
				case OP_SRLI:
					if(D_[valC] & (1 << 10) != 0)
						d_[ifun] = OP_SRAI;
				case OP_SLLI:
					d_[valC] = D_[valC] & ((1 << 6) - 1);
					break;
				default:
					d_[valC] = D_[valC];
					break;
			}
			break;
		case OP_IMM_32:
			d_[valA] = (int)regfile->get(D_[rA]);
			d_[dstE] = D_[dstE];
			switch(D_[ifun])
			{
				case OP_SRLI:
					if(D_[valC] & (1 << 10) != 0)
						d_[ifun] = OP_SRAI;
				case OP_SLLI:
					d_[valC] = D_[valC] & ((1 << 5) - 1);
					break;
				default:
					d_[valC] = D_[valC];
					break;
			}
			break;
		case BRANCH:
			d_[valA] = regfile->get(D_[rA]);
			d_[valB] = regfile->get(D_[rB]);
			d_[valC] = D_[valC];
			d_[pj] = D_[pj];
			d_[newP] = D_[newP];
			break;
		case JAL:
			d_[newP] = D_[newP];
		case LUI:
		case AUIPC:
			d_[dstE] = D_[dstE];
			d_[valC] = D_[valC];
			break;
		case LOAD:
		case LOAD_FP:
			d_[dstE] = D_[dstE];
			d_[valC] = D_[valC];
			d_[valA] = regfile->get(D_[rA]);
			break;
		case STORE:
		case STORE_FP:
			d_[valA] = regfile->get(D_[rA]);
			d_[valB] = regfile->get(D_[rB]);
			d_[valC] = D_[valC];
			break;
		case JALR:
			d_[dstE] = D_[dstE];
			d_[valA] = regfile->get(D_[rA]);
			d_[valC] = D_[valC];
			d_[newP] = D_[newP];
			break;
		case SYSTEM:
			if(D_[rA] == 0 && D_[dstE] == 0 && D_[ifun] == 0)
			{
				d_[ifun] = D_[valC];
				break;
			}

		default:
			raise_exception(unsupported, D_[valP]);
	}
}
void Machine::EX()
{
	e_[icode] = E_[icode];
	e_[ifun] = E_[ifun];
	e_[valP] = E_[valP];
	stats->summary("Machine: total instr execute");
	if(E_[icode] != NOP)
		stats->summary("Machine: Nnop instr execute");
	switch(E_[icode])//execute the instrucion
	{
		case ILLEGAL:
			raise_exception(illegal_instruction, E_[valP]);
			break;
		case NOP:
			break;
		case OP:
		case OP_32:
		e_[dstE] = E_[dstE];
		switch(E_[ifun])
		{
			case OP_ADD:
				e_[valE] = E_[valA] + E_[valB];
				break;
			case OP_SUB:
				e_[valE] = E_[valA] - E_[valB];
				break;
			case OP_SLL:
				e_[valE] = E_[valA] << E_[valB];
				break;
			case OP_SLT:
				e_[valE] = (E_[valA] < E_[valB] ? 1 : 0);
				break;
			case OP_SLTU:
				e_[valE] = (
					(ulli)E_[valA]
					<(ulli)E_[valB]
					? 1 : 0);
				break;
			case OP_XOR:
				e_[valE] = E_[valA] ^ E_[valB];
				break;
			case OP_SRL:
				e_[valE] = (ulli)E_[valA]
					>> E_[valB];
				break;
			case OP_SRA:
				e_[valE] = E_[valA] >> E_[valB];
				break;
			case OP_OR:
				e_[valE] = E_[valA] | E_[valB];
				break;
			case OP_AND:
				e_[valE] = E_[valA] & E_[valB];
				break;
			case OP_MUL:
				e_[valE] = E_[valA] * E_[valB];
				stats->summary("OP: multiplication");
				break;
			case OP_MULH:
				__asm__(
					"mov %1, %%rax\n\t"
					"imulq %2\n\t"
					"mov %%rdx, %0"
					:"=g"(e_[valE])
					:"g"(E_[valA]),"g"(E_[valB])
					:"rax","rdx"
				);
				//use inline assembly to get the high 64-bit result
				//of multiplication
				//rdx:rax <-imul- rax r/m64
				//imul is signed multiplication
				stats->summary("OP: multiplication");
				break;
			case OP_MULHU:
				__asm__(
					"mov %1, %%rax\n\t"
					"mulq %2\n\t"
					"mov %%rdx, %0"
					:"=g"(e_[valE])
					:"g"(E_[valA]),"g"(E_[valB])
					:"rax","rdx"
				);//mul is unsigned multiplication
				stats->summary("OP: multiplication");
				break;
			case OP_MULHSU:
			{
				lli tmp = E_[valA];
				if(tmp < 0)
					__asm__(
						"mov %1, %%rax\n\t"
						"mulq %2\n\t"
						"test %%rax, %%rax\n\t"
						"setz %%al\n\t"
						"movzbq %%al, %%rax\n\t"
						"neg %%rdx\n\t"
						"add %%rax, %%rdx\n\t"
						"mov %%rdx, %0"
						:"=g"(e_[valE])
						:"g"(tmp),"g"(E_[valB])
						:"rax","rdx"
					);
				else
					__asm__(
						"mov %1, %%rax\n\t"
						"mulq %2\n\t"
						"mov %%rdx, %0"
						:"=g"(e_[valE])
						:"g"(tmp),"g"(E_[valB])
						:"rax","rdx"
					);
				stats->summary("OP: multiplication");
				break;
			}
			case OP_DIV:
				e_[valE] = E_[valA] / E_[valB];
				stats->summary("OP: division");
				break;
			case OP_DIVU:
				e_[valE] = ((ulli)E_[valA])
						/ ((ulli)E_[valB]);
				stats->summary("OP: division");
				break;
			case OP_REM:
				e_[valE] = E_[valA] % E_[valB];
				stats->summary("OP: division");
				break;
			case OP_REMU:
				e_[valE] = ((ulli)E_[valA])
						% ((ulli)E_[valB]);
				stats->summary("OP: division");
				break;
			default:
				assert(false);
		}
		break;

		case OP_IMM_32:
		case OP_IMM:
		e_[dstE] = E_[dstE];
		switch(E_[ifun])
		{
			case OP_ADDI:
				e_[valE] = E_[valA] + E_[valC];
				break;
			case OP_SLTI:
				e_[valE] = (E_[valA] < E_[valC] ? 1 : 0);
				break;
			case OP_SLTIU:
				e_[valE] = (
					(ulli)E_[valA]
					< E_[valC]
					? 1 : 0);
				break;
			case OP_XORI:
				e_[valE] = E_[valA] ^ E_[valC];
				break;
			case OP_ORI:
				e_[valE] = E_[valA] | E_[valC];
				break;
			case OP_ANDI:
				e_[valE] = E_[valA] & E_[valC];
				break;
			case OP_SLLI:
				e_[valE] = E_[valA] << E_[valC];
				break;
			case OP_SRLI:
				e_[valE] = (ulli)E_[valA]
					>> E_[valC];
				break;
			case OP_SRAI:
				e_[valE] = E_[valA] >> E_[valC];
				break;
			default:
				assert(false);
		}
		break;

		case MADD:
		case MSUB:
		case NMADD:
		case NMSUB:
		{
			auto oldround = fegetround();
			switch(E_[valC] & 7)
			{
				case 0:
				case 4:
					fesetround(FE_TONEAREST);
					break;
				case 1:
					fesetround(FE_TOWARDZERO);
					break;
				case 2:
					fesetround(FE_DOWNWARD);
					break;
				case 3:
					fesetround(FE_UPWARD);
					break;
				case 7:
					break;
				default:
					raise_exception(illegal_instruction, E_[valP]);
					break;
			}
			e_[dstE] = E_[dstE];
			switch(E_[icode])
			{
				case MADD:
					if(E_[valC] & 8)
						e_[valE] = treat_as<lli>(
							treat_as<double>(E_[valA])
							*treat_as<double>(E_[valB])
							+treat_as<double>(E_[valD]));
					else
						e_[valE] = treat_as<int>(
							treat_as<float>((int)E_[valA])
							*treat_as<float>((int)E_[valB])
							+treat_as<float>((int)E_[valD]));
					break;
				case MSUB:
					if(E_[valC] & 8)
						e_[valE] = treat_as<lli>(
							treat_as<double>(E_[valA])
							*treat_as<double>(E_[valB])
							-treat_as<double>(E_[valD]));
					else
						e_[valE] = treat_as<int>(
							treat_as<float>((int)E_[valA])
							*treat_as<float>((int)E_[valB])
							-treat_as<float>((int)E_[valD]));
					break;
				case NMSUB:
					if(E_[valC] & 8)
						e_[valE] = treat_as<lli>(
							-treat_as<double>(E_[valA])
							*treat_as<double>(E_[valB])
							+treat_as<double>(E_[valD]));
					else
						e_[valE] = treat_as<int>(
							-treat_as<float>((int)E_[valA])
							*treat_as<float>((int)E_[valB])
							+treat_as<float>((int)E_[valD]));
					break;
				case NMADD:
					if(E_[valC] & 8)
						e_[valE] = treat_as<lli>(
							-treat_as<double>(E_[valA])
							*treat_as<double>(E_[valB])
							-treat_as<double>(E_[valD]));
					else
						e_[valE] = treat_as<int>(
							-treat_as<float>((int)E_[valA])
							*treat_as<float>((int)E_[valB])
							-treat_as<float>((int)E_[valD]));
					break;
				default:
					assert(false);
			}
			fesetround(oldround);
			if(E_[valC] & 8)
				stats->summary("OP: double precision floating mul-add");
			else
				stats->summary("OP: single precision floating mul-add");

			break;
		}

		case OP_FP:
		{
			dbg_print("%llx: float op %lld with vA = %lf, vB = %lf(%016llx), vC = %05llx\n",
				E_[valP], E_[ifun],
				treat_as<double>(E_[valA]),
				treat_as<double>(E_[valB]),
				E_[valB], E_[valC]);
			e_[dstE] = E_[dstE];
			auto oldround = fegetround();
			switch(E_[valC] & 7)
			{
				case 0:
				case 4:
					fesetround(FE_TONEAREST);
					break;
				case 1:
					fesetround(FE_TOWARDZERO);
					break;
				case 2:
					fesetround(FE_DOWNWARD);
					break;
				case 3:
					fesetround(FE_UPWARD);
					break;
				case 7:
					break;
				default:
					raise_exception(illegal_instruction, E_[valP]);
					break;
			}
			switch(E_[ifun])
			{
				case OP_FADD:
					if(E_[valC] & 8)
					{
						e_[valE] = 
						treat_as<lli>(
							treat_as<double>(E_[valA])
							+
							treat_as<double>(E_[valB]));
						stats->summary("OP: double precision floating add/sub");
					}
					else
					{
						e_[valE] = 
						treat_as<int>(
							treat_as<float>((int)E_[valA])
							+
							treat_as<float>((int)E_[valB]));
						stats->summary("OP: single precision floating add/sub");
					}
					break;
				case OP_FSUB:
					if(E_[valC] & 8)
					{
						e_[valE] = 
						treat_as<lli>(
							treat_as<double>(E_[valA])
							-
							treat_as<double>(E_[valB]));
						stats->summary("OP: double precision floating add/sub");
					}
					else
					{
						e_[valE] = 
						treat_as<int>(
							treat_as<float>((int)E_[valA])
							-
							treat_as<float>((int)E_[valB]));
						stats->summary("OP: single precision floating add/sub");
					}
					break;
				case OP_FMUL:
					if(E_[valC] & 8)
					{
						e_[valE] = 
						treat_as<lli>(
							treat_as<double>(E_[valA])
							*
							treat_as<double>(E_[valB]));
						stats->summary("OP: double precision floating mul");
					}
					else
					{
						e_[valE] = 
						treat_as<int>(
							treat_as<float>((int)E_[valA])
							*
							treat_as<float>((int)E_[valB]));
						stats->summary("OP: single precision floating mul");
					}
					break;
				case OP_FDIV:
					if(E_[valB] == 0)
					{
						FCSR.DZ = 1;
					}
					if(E_[valC] & 8)
					{
						e_[valE] = 
						treat_as<lli>(
							treat_as<double>(E_[valA])
							/
							treat_as<double>(E_[valB]));
						stats->summary("OP: double precision floating div");
					}
					else
					{
						e_[valE] = 
						treat_as<int>(
							treat_as<float>((int)E_[valA])
							/
							treat_as<float>((int)E_[valB]));
						stats->summary("OP: single precision floating div");
					}
					break;
				case OP_FSQRT:
					if(E_[valC] & 8)
					{
						e_[valE] = 
						treat_as<lli>(
							sqrt(treat_as<double>(E_[valA])));
						stats->summary("OP: double precision sqrt");
					}
					else
					{
						e_[valE] = 
						treat_as<int>(
							sqrt(treat_as<float>((int)E_[valA])));
						stats->summary("OP: single precision sqrt");
					}
					break;
				case OP_FMM:
					if(E_[valC] & 8)
					{
						if(E_[valC] & 1)
							e_[valE] = 
							treat_as<lli>(max(
								treat_as<double>(E_[valA]),
								treat_as<double>(E_[valB])));
						else
							e_[valE] = 
							treat_as<lli>(min(
								treat_as<double>(E_[valA]),
								treat_as<double>(E_[valB])));
					}
					else
					{
						if(E_[valC] & 1)
							e_[valE] = 
							treat_as<int>(max(
								treat_as<float>((int)E_[valA]),
								treat_as<float>((int)E_[valB])));
						else
							e_[valE] = 
							treat_as<int>(min(
								treat_as<float>((int)E_[valA]),
								treat_as<float>((int)E_[valB])));
					}
					break;
				case OP_FSGNJ:
				switch(E_[valC] & 3)
				{
					case 0://SGNJ
					if(E_[valC] & 8)
					{
						e_[valE] = 
							(E_[valA] & ((1ull << 63) - 1))
							|(E_[valB] & (1ull << 63));
					}
					else
					{
						e_[valE] = 
							(E_[valA] & ((1ull << 31) - 1))
							|(E_[valB] & (1ull << 31));
					}
					break;
					case 1://SGNJN
					if(E_[valC] & 8)
					{
						e_[valE] = 
							(E_[valA] & ((1ull << 63) - 1))
							|(~E_[valB] & (1ull << 63));
					}
					else
					{
						e_[valE] = 
							(E_[valA] & ((1ull << 31) - 1))
							|(~E_[valB] & (1ull << 31));
					}
					break;
					case 2:
					if(E_[valC] & 8)
					{
						e_[valE] = 
							E_[valA] ^ (E_[valB] & (1ull << 63));
					}
					else
					{
						e_[valE] = 
							E_[valA] ^ (E_[valB] & (1ull << 31));
					}
					break;
					default:
					raise_exception(illegal_instruction, E_[valP]);
				}
				break;

				case OP_FMVFX:
				case OP_FMVXF:
					e_[valE] = E_[valA];
					break;

				case OP_FCMP:
				switch(E_[valC] & 3)
				{
					case 2://FEQ
					if(E_[valC] & 8)
						e_[valE] = 
							treat_as<double>(E_[valA])
							==
							treat_as<double>(E_[valB]);
					else
						e_[valE] = 
							treat_as<float>((int)E_[valA])
							==
							treat_as<float>((int)E_[valB]);
					break;
					case 1://FLT
					if(E_[valC] & 8)
						e_[valE] = 
							treat_as<double>(E_[valA])
							<
							treat_as<double>(E_[valB]);
					else
						e_[valE] = 
							treat_as<float>((int)E_[valA])
							<
							treat_as<float>((int)E_[valB]);
					break;
					case 0://FLE
					if(E_[valC] & 8)
						e_[valE] = 
							treat_as<double>(E_[valA])
							<=
							treat_as<double>(E_[valB]);
					else
						e_[valE] = 
							treat_as<float>((int)E_[valA])
							<=
							treat_as<float>((int)E_[valB]);
					break;
					default:
					raise_exception(illegal_instruction, E_[valP]);
				}
				break;

				case OP_FCVTFF:
					stats->summary("OP: floating-floating convert");
					if(E_[valB] == 1)
						e_[valE] = treat_as<lli>(
							(double)treat_as<float>(
								(int)E_[valA]));
					else
						e_[valE] = treat_as<int>(
							(float)treat_as<double>(
								E_[valA]));
					break;

				case OP_FCVTFW:
					stats->summary("OP: floating-integer convert");
					if(E_[valC] & 8)
						e_[valE] =
							(lli)treat_as<double>(E_[valA]);
					else
						e_[valE] =
							(lli)treat_as<float>((int)E_[valA]);
					if(!(E_[valB] & 2))
						e_[valE] = (int)e_[valE];
					if(E_[valB] == 1)
						e_[valE] = (uint)e_[valE];
					break;
				case OP_FCVTWF:
					stats->summary("OP: integer-floating convert");
					switch(E_[valB] & 3)
					{
						case 0://W
							e_[valE] = (int)E_[valA];
							break;
						case 1://WU
							e_[valE] = (uint)E_[valA];
							break;
						case 2://L
						case 3://LU
							e_[valE] = E_[valA];
							break;
					}
					if(E_[valC] & 8)
					{
						if((E_[valB] & 3) == 3)
							e_[valE] = treat_as<lli>(
								(double)(ulli)e_[valE]);
						else
							e_[valE] = treat_as<lli>(
								(double)e_[valE]);
					}
					else
					{
						if((E_[valB] & 3) == 3)
							e_[valE] = treat_as<int>(
								(float)(ulli)e_[valE]);
						else
							e_[valE] = treat_as<int>(
								(float)e_[valE]);
					}
					break;
				case OP_FCLASS:
					raise_exception(unsupported, E_[valP]);
			}
			fesetround(oldround);
		}
		break;

		case LUI:
			e_[valE] = E_[valC];
			e_[dstE] = E_[dstE];
			break;

		case AUIPC:
			e_[valE] = E_[valC] + E_[valP];
			e_[dstE] = E_[dstE];
			break;

		case LOAD:
		case LOAD_FP:
			if(E_[dstE] == 0)
				raise_exception(illegal_instruction, E_[valP]);
			e_[dstE] = E_[dstE];
			e_[dstM] = E_[valA] + E_[valC];
			break;

		case STORE:
		case STORE_FP:
			e_[dstM] = E_[valA] + E_[valC];
			e_[valM] = E_[valB];
			break;

		case JAL:
			e_[dstE] = E_[dstE];
			e_[valE] = E_[newP] + 4;
			e_[newP] = E_[valP] + E_[valC];
			break;

		case JALR:
			e_[dstE] = E_[dstE];
			e_[valE] = E_[newP] + 4;
			e_[newP] = (E_[valA] + E_[valC]) & (~1ll);
			break;

		case BRANCH:
		switch(E_[ifun])
		{
			case BEQ:
				e_[newP] = (
					E_[valA] == E_[valB]
					? E_[valP] + E_[valC]
					: E_[valP]);
				break;
			case BNE:
				e_[newP] = (
					E_[valA] != E_[valB]
					? E_[valP] + E_[valC]
					: E_[valP]);
				break;
			case BLT:
				e_[newP] = (
					E_[valA] < E_[valB]
					? E_[valP] + E_[valC]
					: E_[valP]);
				break;
			case BGE:
				e_[newP] = (
					E_[valA] >= E_[valB]
					? E_[valP] + E_[valC]
					: E_[valP]);
				break;
			case BLTU:
				e_[newP] = (
					(ulli)E_[valA]
					< (ulli)E_[valB]
					? E_[valP] + E_[valC]
					: E_[valP]);
				break;
			case BGEU:
				e_[newP] = (
					(ulli)E_[valA]
					>= (ulli)E_[valB]
					? E_[valP] + E_[valC]
					: E_[valP]);
				break;
			default:
				raise_exception(illegal_instruction, E_[valP]);
		}
		break;
		case SYSTEM:
			switch(E_[ifun])
			{
				case ECALL:
				{
					raise_exception(syscall, E_[valP]);
					break;
				}
				case EBREAK:
				{
					CSR.BRK = 1;
					break;
				}
				default:
					raise_exception(unsupported, E_[valP]);
			}
			break;
		default:
			raise_exception(unsupported, E_[valP]);
	}
}
void Machine::MEM()
{
	memcpy(m_, M_, sizeof(M_));
	switch(M_[icode])
	{
		case LOAD:
		case LOAD_FP:
		{
			int len = M_[ifun];
			long long int uext = 0;
			if (len & 4)
			{
				len -= 4;
				uext = 1;
			}
			if(memory->read(M_[dstM], &m_[valE], 1 << len, CSR.PL))
				raise_exception(illegal_address, M_[valP]);
			m_[dstE] = M_[dstE];
			if(!uext)
				m_[valE] = m_[valE] << (64 - (1 << len) * 8) >> (64 - (1 << len) * 8);
			stats->summary("Machine: load");
			break;
		}
		case STORE:
		case STORE_FP:
		{
			int len = M_[ifun];
			if(len & 4)
				raise_exception(illegal_instruction, M_[valP]);
			if(memory->write(M_[dstM], &M_[valM], 1 << len, CSR.PL))
				raise_exception(illegal_address, M_[valP]);
			stats->summary("Machine: store");
			break;
		}
		default://only load and store instruction could access memory
			break;
	}
}
void Machine::WB()
{
	switch(W_[icode])
	{
		case OP:
		case OP_IMM:
		case OP_32:
		case OP_IMM_32:
		case LUI:
		case AUIPC:
		case JAL:
		case JALR:
		case LOAD:
		case OP_FP:
		case LOAD_FP:
		case MADD:
		case MSUB:
		case NMADD:
		case NMSUB:
			if(W_[dstE] != 0)
				regfile->set(W_[dstE], W_[valE]);
			break;
		default:
			break;
	}
}

void Machine::pipeline_flush()
{
	MEM();
	WB();
	memcpy(W_, m_, sizeof(m_));
	memset(M_, -1, sizeof(M_));
	memset(m_, -1, sizeof(m_));
	WB();
	memset(W_, -1, sizeof(W_));
	predPC = D_[valP];
	memset(D_, -1, sizeof(D_));
	stats->summary("Machine: coarse cycle", 2);
	stats->summary("Machine: pipeline flush");
}

void Machine::pipeline_forward()
{
	F_status = D_status = E_status = 0;

	predPC = f_[valP] + incPC;

	bool datarace = false;

	if(D_[rA] > 0)
	{
		if(D_[rA] == W_[dstE])
			d_[valA] = W_[valE];
		if(D_[rA] == m_[dstE])
			d_[valA] = m_[valE];
		if(D_[rA] == e_[dstE])
		{
			if(E_[icode] == LOAD || E_[icode] == LOAD_FP)
			{
				F_status |= Stall;
				D_status |= Stall;
				E_status |= Bubble;
				if(!datarace)
				{
					datarace = true;
					stats->summary("Machine: data race");
				}
			}
			else
				d_[valA] = e_[valE];
		}
	}
	if(D_[rB] > 0)
	{
		if(D_[icode] == OP_FP && (
			D_[ifun] == OP_FMVXF ||
			D_[ifun] == OP_FMVFX ||
			D_[ifun] == OP_FSQRT ||
			D_[ifun] == OP_FCLASS||
			D_[ifun] == OP_FCVTFF||
			D_[ifun] == OP_FCVTWF||
			D_[ifun] == OP_FCVTFW))
		{;}
		else
		{
			if(D_[rB] == W_[dstE])
				d_[valB] = W_[valE];
			if(D_[rB] == m_[dstE])
				d_[valB] = m_[valE];
			if(D_[rB] == e_[dstE])
			{
				if(E_[icode] == LOAD || E_[icode] == LOAD_FP)
				{
					F_status |= Stall;
					D_status |= Stall;
					E_status |= Bubble;
					if(!datarace)
					{
						datarace = true;
						stats->summary("Machine: data race");
					}
				}
				else
					d_[valB] = e_[valE];
			}
		}
	}
	if(D_[icode] == MADD || D_[icode] == MSUB || D_[icode] == NMSUB || D_[icode] == NMADD)
	{
		if(D_[rD] == W_[dstE])
			d_[valD] = W_[valE];
		if(D_[rD] == m_[dstE])
			d_[valD] = m_[valE];
		if(D_[rD] == e_[dstE])
		{
			if(E_[icode] == LOAD || E_[icode] == LOAD_FP)
			{
				F_status |= Stall;
				D_status |= Stall;
				E_status |= Bubble;
				if(!datarace)
				{
					datarace = true;
					stats->summary("Machine: data race");
				}
			}
			else
				d_[valD] = e_[valE];
		}
	}
	//data forwarding

	if(f_[icode] == JAL)
	{
		predPC = f_[valP] + f_[valC];
	}
	//unconditional jump, just update the pc

	if(f_[icode] == BRANCH)
	{
		if(predictor.predict(f_[valP], f_[valC]))//backward taken
		{
			predPC = f_[valP] + f_[valC];
			f_[pj] = 1;
		}
		else
		{
			f_[pj] = 0;
		}
	}

	if(E_[icode] == JALR)
	{
		predPC = e_[newP];
		D_status |= Bubble;
		E_status |= Bubble;
		stats->summary("Machine: jr stall");
	}

	if(E_[icode] == BRANCH)
	{
		if((e_[newP] != E_[valP]) && !E_[pj])//should jump but not taken
		{
			predPC = e_[newP];
			D_status |= Bubble;
			E_status |= Bubble;
			predictor.feedback(E_[valP], E_[valC], false, false);
		}
		else if((e_[newP] == E_[valP]) && E_[pj])//should not jump but taken
		{
			predPC = E_[newP] + 4;
			D_status |= Bubble;
			E_status |= Bubble;
			predictor.feedback(E_[valP], E_[valC], true, false);
		}
		else
		{
			predictor.feedback(E_[valP], E_[valC], E_[pj], true);
		}
	}
	//update pc if mispredicted

	memcpy(W_, m_, sizeof(m_));
	memcpy(M_, e_, sizeof(e_));
	//pipe-regs propagate
#define pass
	if(E_status & Bubble)
	{
		memset(E_, -1, sizeof(E_));
	}
	else if(E_status & Stall)
	{
		pass;
	}
	else
	{
		memcpy(E_, d_, sizeof(d_));
	}

	if(D_status & Bubble)
	{
		memset(D_, -1, sizeof(D_));
	}
	else if(D_status & Stall)
	{
		pass;
	}
	else
	{
		memcpy(D_, f_, sizeof(f_));
	}

	if(F_status & Stall)
	{
		predPC = f_[valP];
	}
	else
	{
		pass;
	}
#undef pass

	if(W_[valP] == regfile->get(EndPCReg))
	{
		CSR.MS = HALT;
	}
	//pipeline is clear now, halt

	memset(f_, -1, sizeof(f_));
	memset(d_, -1, sizeof(d_));
	memset(e_, -1, sizeof(e_));
	memset(m_, -1, sizeof(m_));
	//clear the pipe-regs
	regfile->set(PCReg, predPC);
	//write the pc back to regfile

	if(E_[icode] == SYSTEM && E_[ifun] == ECALL)
		CSR.SC = 1;//facing syscall, pipeline_flush is needed
	else
		CSR.SC = 0;
}

void Machine::raise_exception(int which, lli badVAddr)
{
	switch(which)
	{
		case syscall:
		{
			stats->summary("Machine: syscall");
			int idx = regfile->get(a0);
			switch(idx)
			{
				case 0:
					putchar(regfile->get(a1));
					break;
				case 1:
					printf("%lld", regfile->get(a1));
					break;
				case 2:
					printf("%llu", regfile->get(a1));
					break;
				case 3:
				{
					ulli idx = regfile->get(a1);
					char c;
					if(memory->read(idx++, &c, 1, CSR.PL))
						raise_exception(illegal_address, E_[valP]);
					while(c != 0)
					{
						putchar(c);
						if(memory->read(idx++, &c, 1, CSR.PL))
							raise_exception(illegal_address, E_[valP]);
					}
					break;
				}
				case 4:
					printf("%lf", treat_as<double>(regfile->get(a1)));
					break;
				case 10:
					CSR.MS = HALT;
					break;
				default:
					raise_exception(unknown_syscall, badVAddr);
			}
			break;
		}
		default:
			int instr;
			memory->read(badVAddr, &instr, -4);
			fprintf(stderr, "except %d at %llx, instr: %08x\n", which, badVAddr, instr);
			print_pipe_regs(stderr);
			print_regfile(stderr);
			exit(1);
	}
}

void Machine::print_pipe_regs(FILE* out)
{
	fprintf(out,
		"%18s%18s%18s%18s\n%18s%18s%18s%18s\n%18s%18s%18s%18s\n",
		"stage/status","icode","ifun","rA/valA",
		"rB/valB","valC/[fmt,rm]","valP/predPC","valE",
		"dstE","valM","dstM","rD/valD/newP");
	for(int i = 0; i < 72; i++)
		fprintf(out, "-");
	fprintf(out,
		"\n%16s%2c%18x%18x%18x\n%18x%18x%18llx%18x\n%18x%18x%18x%18x\n",
		"Fetch",(F_status & Stall)?'S':'N',0,0,0,
		0,0,predPC,0,
		0,0,0,0);
	for(int i = 0; i < 72; i++)
		fprintf(out, "-");
	fprintf(out,
		"\n%16s%2c%18llx%18llx%18llx\n%18llx%18llx%18llx%18llx\n%18llx%18llx%18llx%18llx\n",
		"Decode",(D_status & Bubble)?'B':((D_status & Stall)?'S':'N'),D_[icode],D_[ifun],D_[rA],
		D_[rB],D_[valC],D_[valP],D_[valE],
		D_[dstE],D_[valM],D_[dstM],D_[rD]);
	for(int i = 0; i < 72; i++)
		fprintf(out, "-");
	fprintf(out,
		"\n%16s%2c%18llx%18llx%18llx\n%18llx%18llx%18llx%18llx\n%18llx%18llx%18llx%18llx\n",
		"Execute",(E_status & Bubble)?'B':((E_status & Stall)?'S':'N'),E_[icode],E_[ifun],E_[rA],
		E_[rB],E_[valC],E_[valP],E_[valE],
		E_[dstE],E_[valM],E_[dstM],E_[rD]);
	for(int i = 0; i < 72; i++)
		fprintf(out, "-");
	fprintf(out,
		"\n%16s%2c%18llx%18llx%18llx\n%18llx%18llx%18llx%18llx\n%18llx%18llx%18llx%18llx\n",
		"Memory",'N',M_[icode],M_[ifun],M_[rA],
		M_[rB],M_[valC],M_[valP],M_[valE],
		M_[dstE],M_[valM],M_[dstM],M_[rD]);
	for(int i = 0; i < 72; i++)
		fprintf(out, "-");
	fprintf(out,
		"\n%16s%2c%18llx%18llx%18llx\n%18llx%18llx%18llx%18llx\n%18llx%18llx%18llx%18llx\n",
		"Writeback",'N',W_[icode],W_[ifun],W_[rA],
		W_[rB],W_[valC],W_[valP],W_[valE],
		W_[dstE],W_[valM],W_[dstM],W_[rD]);
	for(int i = 0; i < 72; i++)
		fprintf(out, "-");
	fprintf(out,"\n");
}
void Machine::print_regfile(FILE* out)
{
	regfile->print(out);
}