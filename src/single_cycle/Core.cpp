#include "Core.h"

Core::Core(const string &fname, ofstream *out) : out(out), 
						clk(0), 
						PC(0),
						instr_mem(new Instruction_Memory(fname))
{

}

/*
	TODO - Add more functions and modify tick() to simulate single-cycle RISC-V architecture
*/

uint64_t Core::add64(uint64_t a, uint64_t b){
	uint64_t result;
	result = a + b;

	return result;
}

uint64_t Core::mux64(uint64_t a, uint64_t b, bool s){
	if(s){
		return a;
	} else {
		return b;
	}
}

uint64_t Core::lshift64(uint64_t a, uint8_t s = 1){
	uint64_t shifted_result;

	shifted_result = a << s;

	return shifted_result;	
}

bool Core::and_gate(bool a, bool b){
	bool and_result;

	and_result = a & b;

	return and_result;
}

void Core::control(uint8_t instr, bool *branch, bool *memread, bool *memtoreg,
		uint8_t *aluop, bool *memwrite, bool *alusource, bool *regwrite){


	// R-Format
	if(instr == 0x0033 || instr == 0x003B){
		*branch = 0;
		*memread = 0;
		*memtoreg = 0;
		*aluop = 0x0002;
		*memwrite = 0;
		*alusource = 0;
		*regwrite = 1;

	// I-Format
	} else if (instr == 0x0002 || instr == 0x000F || instr == 0x0013 || instr == 0x0033){
		*branch = 0;
		*memread = 1;
		*memtoreg = 1;
		*aluop = 0x0000;
		*memwrite = 0;
		*alusource = 1;
		*regwrite = 1;

	// S-Format
	} else if (instr == 0x0023){
		*branch = 0;
		*memread = 0;
		*aluop = 0x0000;
		*memwrite = 1;
		*alusource = 1;
		*regwrite = 0;

	// U-Format
	} else if (instr == 0x0063){
		*branch = 1;
		*memread = 0;
		*aluop = 0x0001;
		*memwrite = 0;
		*alusource = 0;
		*regwrite = 0;
	}

}

bool Core::tick()
{
	/*
		Step One: Serving pending instructions
	*/
	if (pending_queue.size() > 0)
	{
		serve_pending_instrs();
	}
	
	/*
		Step Two: Where simulation happens
	*/
	if (PC <= instr_mem->last_addr())
	{
		// Get Instruction
		Instruction &instruction = instr_mem->get_instruction(PC);

		// Increment PC
		// TODO, PC should be incremented or decremented based on instruction
		PC += 4;

		/*
			Step Three: Simulator related
		*/
		instruction.begin_exe = clk;
		
		// Single-cycle always takes one clock cycle to complete
		instruction.end_exe = clk + 1; 
	
		pending_queue.push_back(instruction);
	}

	clk++;

	/*
		Step Four: Should we shut down simulator
	*/
	if (pending_queue.size() == 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

void Core::serve_pending_instrs()
{
	list<Instruction>::iterator instr = pending_queue.begin();

	if (instr->end_exe <= clk)
	{
		printStats(instr);
		
		pending_queue.erase(instr);	
	}
}

void Core::printStats(list<Instruction>::iterator &ite)
{
	*out << ite->raw_instr << " => ";
	*out << "Core ID: " << id << "; ";
	*out << "Begin Exe: " << ite->begin_exe << "; ";
	*out << "End Exe: " << ite->end_exe << endl;
}

