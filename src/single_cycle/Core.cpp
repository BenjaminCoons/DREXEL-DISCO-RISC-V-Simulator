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

// Student CPU Blocks

uint64_t Core::immgen(uint32_t instr) 
{
	if(inst >> 31 & 0b1)
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
		bool rwrite, uint64_t *data1, uint64_t *data2);
{
	*data1 = register_file[reg1];
	*data2 = register_file[reg2];
	if(rwrite)
		register_file[wreg] = wdata;
}

uint64_t Core::datmem(uint64_t addr, uint64_t wdata, bool read, bool write) 
{
	//HELPME how big is data mem? What is the initial register file? 
}

