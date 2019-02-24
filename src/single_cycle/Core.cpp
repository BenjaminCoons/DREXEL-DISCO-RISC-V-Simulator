#include "Core.h"

#define DEBPRNT(x) printf("%s\n", x); fflush(stdout);

Core::Core(const string &fname, ofstream *out) : out(out), 
						clk(0), 
						PC(0),
						instr_mem(new Instruction_Memory(fname))
{	
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
		serve_pending_instrs();
	
	/*
		Step Two: Where simulation happens
	*/
	if (PC <= instr_mem->last_addr())
	{
		// Get Instruction
		Instruction &instruction = instr_mem->get_instruction(PC);


		/*
			Step Three: Simulator related
		*/

		instruction.begin_exe = clk;
		// initial addr and instruction
		printf("addr:   %d\n", instruction.addr);
		printf("instr:  "); printbin(instruction.instruction, 32);
		printf("\n");

		// split instruction into parts
		uint8_t ctrl, read1, read2, write, func3, func7;
		uint32_t ig;
		imem(instruction.instruction, &ctrl, &read1, &read2, &write, &ig, &func3, &func7);
		printf("ctrl:   "); printbin(ctrl, 7);
		printf("read1:  "); printbin(read1, 5);
		printf("read2:  "); printbin(read2, 5);
		printf("write:  "); printbin(write, 5);
		printf("immgen: "); printbin(ig, 32);
		printf("func3:  "); printbin(func3, 3);
		printf("func7:  "); printbin(func7, 7);
		printf("\n");

		// do any immediate processing
		uint64_t ig_out;
		ig_out = immgen(ig);
		printf("ig_out: "); printbin(ig_out, 64);
		printf("\n");

		// left shift immediate and sum it with PC
		// just in case we decide to branch later
		uint64_t ls_ig, pcig_summed;
		ls_ig = lshift64(ig_out, 1);
		pcig_summed = add64(PC, ls_ig);
		printf("ls_ig:  "); printbin(ls_ig, 64);
		printf("pcigsm: "); printbin(pcig_summed, 64);
		printf("\n");

		// figure out what parts of the processor need to be active
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
		printf("\n");

		// get the relevant data from src registers
		uint64_t data1, data2, wdata;
		reg(read1, read2, write, wdata, regwrite, &data1, &data2);
		printf("data1:  "); printbin(data1, 64);
		printf("data2:  "); printbin(data2, 64);
		printf("\n");

		// mux the immediate value and second register output
		// depending on the opcode
		uint64_t alu_muxin;
		alu_muxin = mux64(data2, ig_out, alusource);
		printf("aluin2: "); printbin(alu_muxin, 64);
		printf("\n");

		// figure out how to trigger the ALU to do what we need
		uint8_t alu_ctrl;
		alu_ctrl = alu_control(aluop, func3, func7);
		printf("aluctr: "); printbin(alu_ctrl, 4);
		printf("\n");

		// call the alu and check for zero result
		bool zero;
		uint64_t alu_res;
		alu_res = alu(data1, alu_muxin, alu_ctrl, &zero);
		printf("alu_rs: "); printbin(alu_res, 64);
		printf("zero:   "); printbin(zero, 1); 
		printf("\n");

		// figure out whether or not we're going to branch
		uint64_t pc_nxt;
		pc_nxt = add64(PC, 4);
		printf("pc_nxt: "); printbin(pc_nxt, 64);
		printf("\n");

		// Do the actual PC muxing
		uint64_t pcmux_s, tmppc;
		pcmux_s = and_gate(branch, zero);
		tmppc = mux64(pc_nxt, pcig_summed, pcmux_s);
		printf("pcmux: "); printbin(pcmux_s, 1);
		printf("pcupdt: %d", tmppc);
		printf("\n");

		// retrieve any relevant data from the rest of the 
		// system's memory
		uint64_t memval;
		memval = datmem(alu_res, data2, memread, memwrite);
		printf("datmem: "); printbin(memval, 64);

		// mux out the data we need to store in a register
		// if the opcode requires it
		wdata = mux64(alu_res, memval, memtoreg);

        // if jal(r) instruction, mux in the program counter
        // to write to the appropriate register. 
        wdata = mux64(wdata, pc_nxt, ctrl == 0b1101111 || ctrl == 0b1100111);

		printf("wdata:  "); printbin(wdata, 64);

        // check in on our register block one more time
        // and see if we need to write any data
        reg(read1, read2, write, wdata, regwrite, &data1, &data2);
        if(regwrite) {
            printf("writing wdata to reg "); printbin(write, 5);
        }
        else
            printf("no data to be written.\n");
        printf("\n");

        // calculate sum of immediate value and value stored
        // in rs1, then mux into PC for jalr command
        uint64_t igoffst;
        igoffst = add64(ig_out, data1);
		PC = mux64(tmppc, igoffst, ctrl == 0b1100111);
        printf("igofst: "); printbin(igoffst, 64);
        printf("\n");

		// Single-cycle always takes one clock cycle to complete
		instruction.end_exe = clk + 1; 
		printf("===========================================================================\n");
	
		pending_queue.push_back(instruction);
	}

	clk++;

	/*
		Step Four: Should we shut down simulator
	*/
	return (pending_queue.size() != 0);
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

uint64_t Core::add64(uint64_t a, uint64_t b){
	uint64_t result;
	result = a + b;
	return result;
}

uint64_t Core::mux64(uint64_t a, uint64_t b, bool s){
	
	return s ? b : a;
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

	switch(instr) {
        case 0b0000011: 
            // ld
            *branch = 0;
            *memread = 1;
            *memtoreg = 1;
            *aluop = 0b00;
            *memwrite = 0;
            *alusource = 1;
            *regwrite = 1;
            break;
        case 0b0010011: 
            // addi, slli, xori, srli, ori, andi
            *branch = 0;
            *memread = 0;
            *memtoreg = 0;
            *aluop = 0b10;
            *memwrite = 0;
            *alusource = 1;
            *regwrite = 1;
            break;
        case 0b0100011: 
            // sd
            *branch = 0;
            *memread = 0;
            *memtoreg = 0;
            *aluop = 0b00;
            *memwrite = 1;
            *alusource = 1;
            *regwrite = 0;
            break;
        case 0b0110011: 
            // add, sub, sll, xor, srl, or, and
            *branch = 0;
            *memread = 0;
            *memtoreg = 0;
            *aluop = 0b10;
            *memwrite = 0;
            *alusource = 0;
            *regwrite = 1;
            break;
        case 0b1100011: 
            // beq, bne, blt, bge
            *branch = 1;
            *memread = 0;
            *memtoreg = 0;
            *aluop = 0b01;
            *memwrite = 0;
            *alusource = 0;
            *regwrite = 0;
            break;
        case 0b1101111: 
        case 0b1100111:
            // jal, jalr
            *branch = 1;
            *memread = 0;
            *memtoreg = 0;
            *aluop = 0b00;
            *memwrite = 0;
            *alusource = 1;
            *regwrite = 1;
            break;
		default:
            // bad instr
			*branch = 0;
			*memread = 0;
			*memtoreg = 0;
			*aluop = 0b00;
			*memwrite = 0;
			*alusource = 0;
			*regwrite = 0;
			break;
	}
}

void Core::imem(uint32_t instr, uint8_t *control, uint8_t *read1, uint8_t *read2, uint8_t *write, uint32_t *immgen,
	uint8_t* func3, uint8_t* func7){

	*control = (instr >> 0)         & 0b01111111; 
	*write   = (instr >> 7)         & 0b00011111;
	*func3   = (instr >> 7+5)       & 0b00000111; 
	*read1   = (instr >> 7+5+3)     & 0b00011111;
	*read2   = (instr >> 7+5+3+5)   & 0b00011111;
	*func7   = (instr >> 7+5+3+5+5) & 0b01111111;

	*immgen  = instr;

}

uint64_t Core::immgen(uint32_t instr) 
{
	constexpr uint64_t one_extend = 0xfffffffffffff000ULL;
	uint64_t temp;
	switch(instr & 0b1111111) {
		case 0b0000011:
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
			temp = (((uint64_t)instr >> 21) & 0b1111111111)
                 + ((((uint64_t)instr >> 20) & 0b1) << 10)
                 + ((((uint64_t)instr >> 12) & 0b11111111) << 11)
                 + ((((uint64_t)instr >> 31) & 0b1) << 19); 
            if((uint8_t)(temp >> 19) & 0b1)
                temp |= (one_extend - 0xff000);
			break;
		case 0b0100011: //S-type
			temp = (((uint64_t)instr >> 7) & 0b11111) 
			     + (((uint64_t)instr >> 25) << 5); 
		    if((uint8_t)(temp >> 11) & 0b1)
				temp |= one_extend;
			break;
		case 0b1100011: //SB-type
			temp = (((uint64_t)instr >> 8) & 0b1111)
			     + ((((uint64_t)instr >> 25) & 0b111111) << 4)
			     + ((((uint64_t)instr >> 7) & 0b1) << 10)
			     + (((uint64_t)instr >> 31) << 11); 
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
    // aluop 
    // 00 = addi, slli, srli
    // 01 = beq, bne, bge, blt 
    // 10 = add, sub, subi, mul, muli, div, divi, sll, srl

    if(aluop & 0b00) return 0b0010;
    if(aluop & 0b01)  
    {
        if(func3 == 0b000) return 0b0110; // beq
        if(func3 == 0b001) return 0b1110; // bne
        if(func3 == 0b010) return 0b1011; // blt
        if(func3 == 0b011) return 0b0011; // bge
    }
    if(aluop & 0b10)  
    {
        if(func7 == 0b0100000) return 0b0110; // sub

        if(func3 == 0b000) return 0b0010; // add
        if(func3 == 0b001) return 0b0111; // sll
        if(func3 == 0b100) return 0b0100; // xor
        if(func3 == 0b101) return 0b0101; // srl
        if(func3 == 0b110) return 0b0001; // or
        if(func3 == 0b111) return 0b0000; // and
    }
    if(aluop & 0b11) return 0b0000;

	return 0b0000;

}

uint64_t Core::alu(uint64_t a, uint64_t b, uint8_t control, bool *zero)
{
	uint64_t temp = 0;
	switch(control & 0b111)
	{
		case 0b000: temp = a & b; break;
		case 0b001: temp = a | b; break;
		case 0b010: temp = a + b; break;
        case 0b011: temp = a < b; break;
        case 0b100: temp = a ^ b;  break;
        case 0b101: temp = a >> b; break;
		case 0b110: temp = a - b;  break;
        case 0b111: temp = a << b; break;
    }
    
    *zero = (temp == 0); // beq, bgt

    if(control & 0b1000) // bne, blt
        *zero = !*zero;

	return temp;
}

void Core::reg(uint8_t reg1, uint8_t reg2, uint8_t wreg, uint64_t wdata, 
    bool rwrite, uint64_t *data1, uint64_t *data2)
{
  	*data1 = register_file[reg1];
  	*data2 = register_file[reg2];

  	if(rwrite){
    	if(wreg == 0) return;
		register_file[wreg] = wdata;   		
 	}
}

uint64_t Core::datmem(uint64_t addr, uint64_t wdata, bool read, bool write) 
{
	uint64_t rtn = 0;
	if(read)
		rtn = *(uint64_t*)(data_memory+addr);
	if(write) {
		// This will store little-endian 64 bit value in 8 8-bit chunks
		uint8_t* bytes = (uint8_t*)&wdata;
		uint8_t i;
	    for(i = 0; i < 8; i++) {
	        data_memory[addr + i] = bytes[i];
	    }
	}
	return rtn;
}