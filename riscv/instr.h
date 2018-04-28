#ifndef INSTR_H
#define INSTR_H
#include "utli.h"
union instruction//divide one instruction into different parts
{
	struct
	{
		uint val;
		uint funct7()
		{
			return val >> 25;
		}
		uint funct5()
		{
			return val >> 27;
		}
		uint fmt()
		{
			return (val >> 25) & 3;
		}
		uint rs2()
		{
			return (val << 7) >> 27;
		}
		uint rs1()
		{
			return (val << 12) >> 27;
		}
		uint funct3()
		{
			return (val << 17) >> 29;
		}
		uint rm()
		{
			return funct3();
		}
		uint rd()
		{
			return (val << 20) >> 27;
		}
		uint opcode()
		{
			return (val << 25) >> 25;
		}
	} R;
	struct
	{
		uint val;
		int imm()
		{
			return ((int)val) >> 20;
		}
		uint rs1()
		{
			return (val << 12) >> 27;
		}
		uint funct3()
		{
			return (val << 17) >> 29;
		}
		uint rd()
		{
			return (val << 20) >> 27;
		}
		uint opcode()
		{
			return (val << 25) >> 25;
		}
	} I;
	struct
	{
		uint val;
		int immh()
		{
			return ((int)val) >> 25;
		}
		uint rs2()
		{
			return (val << 7) >> 27;
		}
		uint rs1()
		{
			return (val << 12) >> 27;
		}
		uint funct3()
		{
			return (val << 17) >> 29;
		}
		uint imml()
		{
			return (val << 20) >> 27;
		}
		uint opcode()
		{
			return (val << 25) >> 25;
		}
	} S;
	struct
	{
		uint val;
		int imm1()
		{
			return ((int)val) >> 31;
		}
		uint imm3()
		{
			return (val << 1) >> 26;
		}
		uint rs2()
		{
			return (val << 7) >> 27;
		}
		uint rs1()
		{
			return (val << 12) >> 27;
		}
		uint funct3()
		{
			return (val << 17) >> 29;
		}
		uint imm4()
		{
			return (val << 20) >> 28;
		}
		uint imm2()
		{
			return (val << 24) >> 31;
		}
		uint opcode()
		{
			return (val << 25) >> 25;
		}
	} SB;
	struct
	{
		uint val;
		int imm()
		{
			return ((int)val) >> 12;
		}
		uint rd()
		{
			return (val << 20) >> 27;
		}
		uint opcode()
		{
			return (val << 25) >> 25;
		}
	} U;
	struct
	{
		uint val;
		int imm1()
		{
			return ((int)val) >> 31;
		}
		uint imm4()
		{
			return (val << 1) >> 22;
		}
		uint imm3()
		{
			return (val << 11) >> 31;
		}
		uint imm2()
		{
			return (val << 12) >> 24;
		}
		uint rd()
		{
			return (val << 20) >> 27;
		}
		uint opcode()
		{
			return (val << 25) >> 25;
		}
	} UJ;
	struct
	{
		uint val;
		uint rs3()
		{
			return val >> 27;
		}
		uint funct2()
		{
			return (val << 5) >> 30;
		}
		uint rs2()
		{
			return (val << 7) >> 27;
		}
		uint rs1()
		{
			return (val << 12) >> 27;
		}
		uint funct3()
		{
			return (val << 17) >> 29;
		}
		uint rd()
		{
			return (val << 20) >> 27;
		}
		uint opcode()
		{
			return (val << 25) >> 25;
		}
	} R4;
	struct
	{
		ushort val;
		ushort dummy;
		uint funct4()
		{
			return val >> 12;
		}
		uint r1()
		{
			return (ushort)(val << 4) >> 11;
		}
		uint r2()
		{
			return (ushort)(val << 9) >> 11;
		}
		uint op()
		{
			return val & 3;
		}
	} CR;
	struct
	{
		ushort val;
		ushort dummy;
		uint funct3()
		{
			return val >> 13;
		}
		uint imm1()
		{
			return (ushort)(val << 3) >> 15;
		}
		uint r1()
		{
			return (ushort)(val << 4) >> 11;
		}
		uint imm2()
		{
			return (ushort)(val << 9) >> 11;
		}
		uint op()
		{
			return val & 3;
		}
	} CI;
	struct
	{
		ushort val;
		ushort dummy;
		uint funct3()
		{
			return val >> 13;
		}
		uint imm()
		{
			return (ushort)(val << 3) >> 10;
		}
		uint r2()
		{
			return (ushort)(val << 9) >> 11;
		}
		uint op()
		{
			return val & 3;
		}
	} CSS;
	struct
	{
		ushort val;
		ushort dummy;
		uint funct3()
		{
			return val >> 13;
		}
		uint imm()
		{
			return (ushort)(val << 3) >> 8;
		}
		uint rd_()
		{
			return (ushort)(val << 11) >> 13;
		}
		uint op()
		{
			return val & 3;
		}
	} CIW;
	struct
	{
		ushort val;
		ushort dummy;
		uint funct3()
		{
			return val >> 13;
		}
		uint imm1()
		{
			return (ushort)(val << 3) >> 13;
		}
		uint r1_()
		{
			return (ushort)(val << 6) >> 13;
		}
		uint imm2()
		{
			return (ushort)(val << 9) >> 14;
		}
		uint rd_()
		{
			return (ushort)(val << 11) >> 13;
		}
		uint r2_()
		{
			return (ushort)(val << 11) >> 13;
		}
		uint op()
		{
			return val & 3;
		}
	} CL, CS;
	struct
	{
		ushort val;
		ushort dummy;
		uint funct3()
		{
			return val >> 13;
		}
		uint offset1()
		{
			return (ushort)(val << 3) >> 13;
		}
		uint funct2()
		{
			return (ushort)(val << 4) >> 14;
		}
		uint r1_()
		{
			return (ushort)(val << 6) >> 13;
		}
		uint offset2()
		{
			return (ushort)(val << 9) >> 11;
		}
		uint op()
		{
			return val & 3;
		}
	} CB;
	struct
	{
		ushort val;
		ushort dummy;
		uint funct3()
		{
			return val >> 13;
		}
		uint jt()
		{
			return (ushort)(val << 3) >> 5;
		}
		uint op()
		{
			return val & 3;
		}
	} CJ;
	uint wrap;
};
#endif