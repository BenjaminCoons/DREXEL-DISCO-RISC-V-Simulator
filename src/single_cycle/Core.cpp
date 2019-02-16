#include "Core.h"

Core::Core(const string &fname, ofstream *out) : out(out), 
						clk(0), 
						PC(0),
						instr_mem(new Instruction_Memory(fname))
{

}

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

	// SB-Format
	} else if (instr == 0x0063){
		*branch = 1;
		*memread = 0;
		*aluop = 0x0001;
		*memwrite = 0;
		*alusource = 0;
		*regwrite = 0;
	}

}

void imem(uint32_t addr, uint8_t *control, uint8_t *read1, uint8_t *read2, uint8_t *write, uint32_t *immgen){

	*control = (addr & 0x0007);
	*write = (addr >> 7) & 0x0005; 
	*read1 = (addr >> 15) & 0x0005;
	*read2 = (addr >> 20) & 0x0005;
	*immgen = addr;

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
		// unsigned int n = 
		// while (n) {
	 //    	if (n & 1)
	 //        	printf("1");
	 //    	else
	 //        	printf("0");

	 //    	n >>= 1;
		// }
		// printf("\n");

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

// Student CPU Blocks

uint64_t Core::immgen(uint32_t instr) 
{
	if(instr >> 31 & 0b1)
		return (uint64_t)instr + 0xffffffff00000000;
	else
		return uint64_t(instr);
}


uint8_t Core::alu_control(uint8_t aluop, uint8_t func3, uint8_t func7)
{
	if((aluop & 0b11) == 0b00) return (uint8_t)0b0010;
	if(aluop & 0b1)            return (uint8_t)0b0110;

	if((aluop >> 1) & 0b1)
	{
		if(func7 == 0b0000000)
		{
			if(func3 == 0b000) return (uint8_t)0b0010;
			if(func3 == 0b111) return (uint8_t)0b0000;
			if(func3 == 0b110) return (uint8_t)0b0001;
		}

		if(func7 == 0b0100000 && func3 == 0b000)
			return (uint8_t)0b0110;
	}

	return (uint8_t)0b0000;

}

uint64_t Core::alu(uint64_t a, uint64_t b, uint8_t control, bool *zero)
{
	uint64_t temp = 0;
	switch(control & 0b1111)
	{
		case 0b0000: temp = a & b; break;
		case 0b0001: temp = a | b; break;
		case 0b0010: temp = a + b; break;
		case 0b0110: temp = a - b; break;
	}
	*zero = (temp == 0);
	return temp;
}

void Core::reg(uint8_t reg1, uint8_t reg2, uint8_t wreg, uint64_t wdata, 
		bool rwrite, uint64_t *data1, uint64_t *data2)
{
	*data1 = register_file[reg1];
	*data2 = register_file[reg2];
	if(rwrite)
		register_file[wreg] = wdata;
}

uint64_t Core::datmem(uint64_t addr, uint64_t wdata, bool read, bool write) 
{
	if(read)
		return *((uint64_t*)(data_memory+addr));
	if(write) {
		// This will store little-endian 64 bit value in 8 8-bit chunks
		uint8_t i;
	    for(i = 0; i < 8; i++) {
	        data_memory[i] = (wdata & (0xffULL << 8*i)) >> 8*i;
	    }
	}
}

