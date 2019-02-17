#ifndef __CORE_H__
#define __CORE_H__

#include <fstream>
#include <iostream>
#include <string>
#include <list>

#include "Instruction_Memory.h"
#include "Instruction.h"

using namespace std;

class Core
{
public:
	Core(const string &fname, ofstream *out);

	bool tick(); // FALSE means all the instructions are exhausted

	int id; // Each core has its own ID

	void printInstrs()
	{
		cout << "Core " << id << " : " << endl;

		instr_mem->printInstr();
	}

private:

	ofstream *out; // Output file

	unsigned long long int clk;

	long PC;

	Instruction_Memory *instr_mem;

	list<Instruction> pending_queue;

	void serve_pending_instrs();

	void printStats(list<Instruction>::iterator &ite);

	// Student CPU Blocks

	uint8_t register_file[32*8];

	uint8_t data_memory[32*8];

	uint64_t add64(uint64_t a, uint64_t b);

	uint64_t mux64(uint64_t a, uint64_t b, bool s);

	uint64_t lshift64(uint64_t a, uint8_t s);

	void printbin(uint64_t n, int c);

	bool and_gate(bool a, bool b);

	void control(uint8_t instr, bool *branch, bool *memread, bool *memtoreg,
		uint8_t *aluop, bool *memwrite, bool *alusource, bool *regwrite);

	uint64_t immgen(uint32_t instr);

	void imem(uint32_t instr, uint8_t *control, uint8_t *read1, uint8_t *read2, 
		uint8_t *write, uint32_t *immgen);

	uint8_t alu_control(uint8_t aluop, uint8_t func3, uint8_t func7);

	uint64_t alu(uint64_t a, uint64_t b, uint8_t control, bool *zero);

	void reg(uint8_t reg1, uint8_t reg2, uint8_t wreg, uint64_t wdata,
		bool rwrite, uint64_t *data1, uint64_t *data2);

	uint64_t datmem(uint64_t addr, uint64_t wdata, bool read, bool write);
};

#endif
