#include "Core.h"

#define DEBPRNT(x) printf("%s\n", x); fflush(stdout);

Core::Core(const string &fname, ofstream *out) : out(out), 
						clk(0), 
						PC(0),
						instr_mem(new Instruction_Memory(fname))
{
	int i;
	for(i = 0; i < 32*8; i++) register_file[i] = 0;
	for(i = 0; i < 32*8; i++) data_memory[i] = 0;
}

uint64_t Core::add64(uint64_t a, uint64_t b){
	uint64_t result;
	result = a + b;
	return result;
}

uint64_t Core::mux64(uint64_t a, uint64_t b, bool s){
	
	return s ? a : b;
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

void Core::imem(uint32_t instr, uint8_t *control, uint8_t *read1, uint8_t *read2, uint8_t *write, uint32_t *immgen){

	*control = (instr >> 0)     & 0b01111111; 
	*write   = (instr >> 7)     & 0b00011111; 
	*read1   = (instr >> 7+5)   & 0b00011111;
	*read2   = (instr >> 7+5+5) & 0b00011111;

	*immgen  = instr;

}

void Core::printbin(uint64_t n, int c) {
	int j = c;
	uint8_t temp[c];
	while (j-- > 0) {
    	if (n & 1) {
        	temp[j] = 1;
    	}
    	else{
        	temp[j] = 0;
    	}

    	n >>= 1;
	}
	uint8_t i;
	for(i = 0; i < c; i++) {
		printf("%d", temp[i]);
	}
	printf("\n");
}

bool Core::tick() {
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
		printf("instr:  "); printbin(instruction.instruction, 32);
		printf("\n");

		uint8_t ctrl, read1, read2, write;
		uint32_t ig;
		imem(instruction.instruction, &ctrl, &read1, &read2, &write, &ig);
		printf("ctrl:   "); printbin(ctrl, 7);
		printf("read1:  "); printbin(read1, 5);
		printf("read2:  "); printbin(read2, 5);
		printf("write:  "); printbin(write, 5);
		printf("immgen: "); printbin(ig, 32);
		printf("\n");

		uint64_t ig_out;
		ig_out = immgen(ig);
		printf("ig_out: ");
		printbin(ig_out, 64);
		printf("\n");

		bool branch, memread, memtoreg, memwrite, alusource, regwrite;
		uint8_t aluop;
		control(ctrl, &branch, &memread, &memtoreg, &aluop, &memwrite, 
			&alusource, &regwrite);
		printf("branch: "); printbin(branch, 1);
		printf("memrd:  "); printbin(memread, 1);
		printf("mem2rg: "); printbin(memtoreg, 1);
		printf("aluop:  "); printbin(aluop, 2);
		printf("memwt:  "); printbin(memwrite, 1);
		printf("alusrc: "); printbin(alusource, 1);
		printf("rgwt:   "); printbin(regwrite, 1);

		uint64_t data1, data2, wdata;
		reg(read1, read2, write, wdata, regwrite, &data1, &data1);
		printf("data1:  "); printbin(data1, 64);
		printf("data2:  "); printbin(data2, 64);


		// Single-cycle always takes one clock cycle to complete
		instruction.end_exe = clk + 1; 

		printf("===========================================================================\n");
	
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
	constexpr uint64_t one_extend = 0xfffffffffffff000ULL;
	uint64_t temp;
	switch(instr & 0b1111111) {
		case 0b0000011:
		case 0b0001111:
		case 0b0010011:
		case 0b0011011:
		case 0b1100111:
		case 0b1110011: // I-type
			temp = (uint64_t)instr >> 20; 
			if((uint8_t)(temp >> 11) & 0b1)
				temp |= one_extend;
			break;
		case 0b0010111:
		case 0b0110111: //U-type
			return 0;  //TODO implement u type
			break;
		case 0b1101111: //UJ-type
			return 0;  //TODO implement uj type
			break;
		case 0b0100011: //S-type
			temp = (((uint64_t)instr >> 7) & 0b11111) 
			     + (((uint64_t)instr >> 25) << 5); 
		    if((uint8_t)(temp >> 11) & 0b1)
				temp |= one_extend;
			break;
		case 0b1100011: //SB-type
			temp = ((((uint64_t)instr >> 8) & 0b1111) << 1)
			     + ((((uint64_t)instr >> 25) & 0b111111) << 5)
			     + ((((uint64_t)instr >> 7) & 0b1) << 11)
			     + (((uint64_t)instr >> 31) << 12); 
			if((uint8_t)(temp >> 11) & 0b1)
				temp |= one_extend;
			break;
		default:
			return 0; 
			break;
	}
	return temp;
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
	*data1 = *((uint64_t*)(register_file+reg1));
	*data2 = *((uint64_t*)(register_file+reg2));
	if(rwrite){
		// This will store little-endian 64 bit value in 8 8-bit chunks
		uint8_t i;
	    for(i = 0; i < 8; i++) {
	        register_file[i] = (wdata & (0xffULL << 8*i)) >> 8*i;
	    }
	}
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