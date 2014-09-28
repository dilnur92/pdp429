//  main.c
//  Program4 edited to Program7
//
//  Created by Dilnur Yuldashev on 12/5/13.
//  Copyright (c) 2013 Dilnur Yuldashevv. All rights reserved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *input;

typedef short Boolean;
typedef char* String;
typedef unsigned short Address;
typedef short Word;
#define OVERFLOW_LIMIT 0x7FFF
#define TRUE 1
#define FALSE 0
#define FACTS 3
#define MASK_W16 0xFFFF
#define A_REGISTER 0
#define B_REGISTER 1
#define C_REGISTER 2
#define D_REGISTER 3
#define PC_REGISTER 4
#define PSW_REGISTER 5
#define SP_REGISTER 6
#define SPL_REGISTER 7
#define LINK_BIT_INDEX 8
const int SIZE= 65536;
int MEMORY[65536];
int ENTRY_POINT;
//const int HALT=0xF02;
//Boolean RUN_PROGRAM=TRUE;
//12 bits
int registerAValue=0;
//either 0 or 1
int registerLinkValue;
int programCounter;
//should always point to an empty address
int stackPointer;
int stackPointerLimit;
int processorStatusWord;
int registers[8];
long long cycles=0;
String symbolicOpcodeOfInstruction;
String detailsOfInstruction;
Boolean SKIP_FLAG;
Boolean INDIRECT=FALSE;
Boolean VERBOSE_MODE=FALSE;
Address POP_STACK_VALUE;
//Boolean isInstructionNOP=FALSE;
char hexToString[12];
String registersToString[9]={"A", "B", "C", "D", "PC", "PSW", "SP", "SPL", "L"};


void hexToAscii(unsigned short a){
    //printf("hexToAscii: %s\n", hexToString);
    //hexToString[0]
    sprintf(hexToString, "0x%04X", a);
}

void instructionToStringRegisterToValue(int regSelector, int value ){
    //A -> 0xFFFF,
    strcat(detailsOfInstruction, registersToString[regSelector]);
    strcat(detailsOfInstruction, " -> ");
    hexToAscii(value);
    strcat(detailsOfInstruction, hexToString);
    strcat(detailsOfInstruction, ", ");
    
}

void instructionToStringValueToRegister(int regSelector, int value ){
    //0x0001 -> A,
    hexToAscii(value);
    //strcat(detailsOfInstruction, "0x");
    strcat(detailsOfInstruction, hexToString);
    strcat(detailsOfInstruction, " -> ");
    strcat(detailsOfInstruction, registersToString[regSelector]);
    strcat(detailsOfInstruction, ", ");
    
}

void instructionToStringMemoryToValue(Address address, int value ){
    //M[0x104F] -> 0x7FFF
    hexToAscii(address);
    strcat(detailsOfInstruction, "M[");
    strcat(detailsOfInstruction, hexToString);
    strcat(detailsOfInstruction, "] -> ");
    hexToAscii(value);
    strcat(detailsOfInstruction, hexToString);
    strcat(detailsOfInstruction, ", ");
}

void instructionToStringValueToMemory(Address address, int value){
    //0x101D -> M[0x103F]
    hexToAscii(value);
    //strcat(detailsOfInstruction, "0x");
    strcat(detailsOfInstruction, hexToString);
    strcat(detailsOfInstruction, " -> ");
    strcat(detailsOfInstruction, "M[");
    hexToAscii(address);
    strcat(detailsOfInstruction, hexToString);
    strcat(detailsOfInstruction, "]");
    strcat(detailsOfInstruction, ", ");
    
}

int addTwoTwelveBitInts(int a, int b){
    int result;
    result=a+b;
    result=result&0xFFF;
    return result;
}

void setTheLinkBit(){
    registerLinkValue=1;
    //print and check
    //printf("LINK BIT IS T CHANGE!!!!!!!!_--------->>>>!\n\n\n");
    instructionToStringValueToRegister(8, 1);
}


short addTwoSixteenBitInts(short a, short b, Boolean checkForOverflow){
    short result=a+b;
    if(checkForOverflow){
        if((a>=0 && b>=0) || (a<0 && b<0)){
            //overflow might occur
            if((a>=0 && b>=0)){
                if(result<0){
                    //overflow
                    setTheLinkBit();
                }
            }
            else{
                if(result>=0){
                    //overflow
                    setTheLinkBit();
                }
            }
        }
    }
    
    result=result&0xFFFF;
    return result;
}

short subtractTwoSixteenBitInts(short a, short b, Boolean checkForOverflow){
    short result=addTwoSixteenBitInts(a, (-b), checkForOverflow);
    return result;
}

short multiplyTwoSixteenBitInts(short a, short b, Boolean checkForOverflow){
    //need to make sure that it works correctly
    int result=a*b;
    result=result&0xFFFF;
    short shortResult=(short)result;
    if(checkForOverflow){
        if((a>=0 && b>=0) || (a<0 && b<0)){
            //overflow might occur
            if(result>0xFFFF){
                setTheLinkBit();
            }
            else if((a>=0 && b>=0)){
                if(shortResult<0){
                    //overflow
                    setTheLinkBit();
                }
            }
            else{
                if(shortResult>=0){
                    //overflow
                    setTheLinkBit();
                }
            }
        }
    }
    //(MULA): A -> 0x0002, M[0x1056] -> 0x7FFF, 0x0001 -> L, 0xFFFE -> A
    //two positive: result is negative
    return shortResult;
}

short divideTwoSixteenBitInts(short a, short b, Boolean checkForOverflow){
    
    if(checkForOverflow){
        //we must check whether we change the link bit
        if(b==0){
            setTheLinkBit();
        }
    }
    if(b==0) return 0;
    int result=a/b;
    result=result&0xFFFF;
    return result;
}

short modTwoSixteenBitInts(short a, short b, Boolean checkForOverflow){
    
    if(checkForOverflow){
        //we must check whether we change the link bit
        if(b==0){
            setTheLinkBit();
        }
    }
    if(b==0) return 0;
    int result=a%b;
    result=result&0xFFFF;
    return result;
}

String trimTrailingWhitespacesAndCommas(String fact){
    //3     =3
    //size=i+1, so fact[i]='\0'
    int i=(int)strlen(fact);
    i--;
    while(fact[i]==' ' || fact[i]==','){
        i--;
    }
    //abbnbnbn      '\0'
    i++;
    fact[i]='\0';
    return fact;
    
}

Boolean runTheProgram(){
    //check that PSW is one
    //check that RUN_PROGRAM is 1
    if(registers[PSW_REGISTER]==1){
        return TRUE;
    }
    else{
       
        return FALSE;
    }
}

void concatenateBooleanString(String instructions[], Boolean selected[], int size, int regSelector){
    symbolicOpcodeOfInstruction[0]='\0';
    String reg=registersToString[regSelector];
    int length=0;
    int i;
    for(i=0;i<size;i++){
        if(selected[i]==TRUE){
            //add String
            strcat(symbolicOpcodeOfInstruction, instructions[i]);
            if(strlen(instructions[i])==2){
                //strcpy(symbolicOpcodeOfInstruction, reg);
                strcat(symbolicOpcodeOfInstruction, reg);
            }
            strcat(symbolicOpcodeOfInstruction, " ");
            //result=strcat(result, instructions[i]);
            length=length+(int)strlen(instructions[i]);
        }
        
    }
    //symbolicOpcodeOfInstruction[length]='\0';
}

void Store_Memory(Address addr, Word value){
    //if (debug)
    //fprintf(stderr, "write memory: 0x%03X = 0x%03X\n", addr, value);
    //defined[addr] = TRUE;
    //printf("%04x: %04x\n", addr, (value & MASK_W16));
    MEMORY[addr] = value & MASK_W16;
}

int get2(void){
    //high-order 8 bits
    int c1 = getc(input);
    //low-order 8 bits
    int c2 = getc(input);
    //if (debug) fprintf(stderr, "read two bytes: 0x%X, 0x%X\n", c1, c2);
    if ((c1 == EOF) || (c2 == EOF))
    {
        fprintf(stderr, "Premature EOF\n");
        exit(1);
    }
    //if (c1 & (~0x3F)) fprintf(stderr, "Extra high order bits for 0x%X\n", c1);
    //if (c2 & (~0x3F)) fprintf(stderr, "Extra high order bits for 0x%X\n", c2);
    //be careful here
    //int n = ((c1 & 0x3F) << 6) | (c2 & 0x3F);
    //0x3f=0011 1111
    //now we get 16 bits=high-order & low-order
    int n = ((c1 & 0xFF) << 8) | (c2 & 0xFF);
    return(n);
}

Address Load_Binary_Object_File(){
    int c1 = getc(input);
    int c2 = getc(input);
    int c3 = getc(input);
    int c4 = getc(input);
    //if (debug) fprintf(stderr, "read four bytes: 0x%X, 0x%X, 0x%X, 0x%X\n", c1, c2, c3, c4);
    
    if ((c1 != 'O') || (c2 != 'B') || (c3 != 'J') || (c4 != 'G')){
        fprintf(stdout, "First four bytes are not OBJG: ");
        //fprintf(stdout, "%s", printrep(c1));
        //fprintf(stdout, "%s", printrep(c2));
        //fprintf(stdout, "%s", printrep(c3));
        //fprintf(stdout, "%s", printrep(c4));
        //fprintf(stdout, " (%02X %02X %02X %02X)\n", c1, c2, c3, c4);
        
        exit(1);
    }
    
    Address entry_point = get2();
    
    //so we got our entry point
    //time to read in the rest of input file
    int c;
    while ((c = getc(input)) != EOF){
        //if (debug) fprintf(stderr, "Read next block of %d bytes\n", n);
        c = c - 1;
        Address addr = get2(); c -= 2;
        //printf("Block address: %08x\n", addr);
        while (c > 0){
            Word data = get2(); c -= 2;
            
            Store_Memory(addr, data);
            addr += 1;
        }
    }
    
    return(entry_point);
}

Address getEffectiveAddressPDP429(int instruction){
    Address effectiveAddress;
    int highOrderEightBits;
    //just not to leave
    int address;
    int mask;
    //note that now we have 16 bits instead of 12 bits
    /*The memory reference instruction format is given in Figure 10.3. To interpret the instruction at location P, the Z/C bit is examined. If Z/C is zero, the high-order five bits of the memory address are zero (zero page); if Z/C is one, the high order five bits of the memory address are the same as the high order 5 bits of the address P (current page). The low-order seven bits are the address field of the instruction. This specifies a 12-bit memory address. Now if the D/I bit is zero, then this is the effective address (direct addressing); if the D/I bit is one, then the contents of the memory address are fetched, and these contents are the effective address (indirect addressing). The effective address is used in all memory reference instructions.
     
     
     */
    //mask for Z/C=0000 0001 0000 0000
    mask=0x0100;
    int pageBit=(instruction&mask)>>8;
    int addressingMode;
    if(pageBit==0){
        //PDP 8: 000(D/I) (Z/C)000 0000
        //PDP 429:0000 00(D/I)(Z/C) 0000 0000
        //PDP 8: pageBit=0---->high-order five bits are zero
        //PDP 429: pageBit=0---->high-order eight bits are zero
        //PDP 8: address=0000 0xxx xxxx, mask=0000 0111 1111=0x07F
        //PDP 429: address=0000 0000 xxxx xxxx, mask=0000 0000 1111 1111=0x00FF
        mask=0x00FF;
        address=instruction&mask;
        
    }
    else{
        //PDP 8: mask first five bits with p counter and concatenate with the seven
        //PDP 8: mask=1111 1000 0000=0xF80
        //PDP 429: mask first eight bits with p counter and concatenate with the eight
        //PDP 429: mask=1111 1111 0000 0000=0xFF00
        mask=0xFF00;
        //without shift
        highOrderEightBits=(mask&(registers[PC_REGISTER]));
        mask=0x00FF;
        address=(instruction&mask)+highOrderEightBits;
    }
    //now we have our instruction
    //it is time to find the addressing mode
    //PDP 8: 000(D/I) (Z/C)000 0000
    //PDP 429:0000 00(D/I)(Z/C) 0000 0000
    //PDP 8: mask=0001 0000 0000
    //PDP 429: mask=0000 0010 0000 0000=0x0200
    mask=0x0200;
    addressingMode=(instruction&mask)>>9;
    if(addressingMode==0){
        effectiveAddress=address;
    }
    else{
        //printf("Indirect addressing!\n");
        effectiveAddress=MEMORY[address];
        //strcpy(symbolicOpcodeOfInstruction, strcat(symbolicOpcodeOfInstruction, instructions[i]));
        //M[0x104C] -> 0x0010: memory to value
        instructionToStringMemoryToValue(address, effectiveAddress);
        //fetch, incr cycles
        //printf("indirect addressing\n");
        INDIRECT=TRUE;
        cycles++;
    }
    return effectiveAddress;
}

void pushToStack(Address a){
    //The stack grows down in memory; the stack pointer (SP) points to an empty location. Pushing to the
    //stack stores in the word that the stack pointer points to, and then the stack pointer is decremented
    //by one.
    //Errors include: (a) stack overflow, if SP < SPL when a stack push starts and (b) stack underflow, if
    //SP wraps around to zero (SP = 0xFFFF when a stack pop starts).
    
    if(registers[SP_REGISTER]<registers[SPL_REGISTER]){
        //fprintf(stderr, "Error in stack push:  SP < SPL\n");
        //Stack Pointer = 0xFFFB; Stack Limit = 0xFFFC
        fprintf(stderr, "Stack Pointer = 0x%04X; Stack Limit = 0x%04X\n", registers[SP_REGISTER], registers[SPL_REGISTER]);
        //exit(1);
        //set PSW to 0
        //Stack Pointer = 0xFFFB; Stack Limit = 0xFFFC
        //Time  21: PC=0x100B instruction = 0xC041 (PUSH Stack Overflow): M[0x0041] -> 0x0040, PSW -> 0x0001, 0x0000 -> PSW
        //PSW -> 0x0001, 0x0000 -> PSW
        //register->value, value->register
        strcat(symbolicOpcodeOfInstruction, " Stack Overflow");
        instructionToStringRegisterToValue(PSW_REGISTER, 1);
        instructionToStringValueToRegister(PSW_REGISTER, 0);
        registers[PSW_REGISTER]=0;
    }
    else{
        cycles++;
        MEMORY[registers[SP_REGISTER]]=a;
        //(CALL): 0x1024 -> M[0xFFFF], 0xFFFE -> SP----->Stack Part 0x1047 -> PC
        //value to memory
        instructionToStringValueToMemory(registers[SP_REGISTER],  MEMORY[registers[SP_REGISTER]]);
        //Sp=110(2)=6
        //stackPointer--;
        registers[SP_REGISTER]--;
        //0xFFFE -> SP
        //value to reg
        instructionToStringValueToRegister(SP_REGISTER, registers[SP_REGISTER]);
        
    }
}

Boolean popFromStack(){
    //Popping from the stack first increments the stack pointer by one, and then uses the word that the
    //stack pointer points at.
    //Errors include: (a) stack overflow, if SP < SPL when a stack push starts and (b) stack underflow, if
    //SP wraps around to zero (SP = 0xFFFF when a stack pop starts).
    if(registers[SP_REGISTER]==0xFFFF){
        //fprintf(stderr, "Error in stack pop:  SP = 0xFFFF\n");
        //Stack Pointer = 0xFFFF
        //Time 21: PC=0x100B instruction = 0xC041 (POP Stack Underflow): SP -> 0xFFFF, PSW -> 0x0001, 0x0000 -> PSW
        strcat(symbolicOpcodeOfInstruction, " Stack Underflow");
        //SP -> 0xFFFF
        //reg to value
        instructionToStringRegisterToValue(SP_REGISTER, registers[SP_REGISTER]);
        // PSW -> 0x0001, 0x0000 -> PSW
        instructionToStringRegisterToValue(PSW_REGISTER, 1);
        instructionToStringValueToRegister(PSW_REGISTER, 0);
        registers[PSW_REGISTER]=0;
        return FALSE;
    }
    else{
        instructionToStringRegisterToValue(SP_REGISTER, registers[SP_REGISTER]);
        registers[SP_REGISTER]++;
        instructionToStringValueToRegister(SP_REGISTER, registers[SP_REGISTER]);
        cycles++;
        //change the details string
        //SP -> 0xFFFE, 0xFFFF -> SP
        //reg->value, value->reg
        // M[0xFFFF] -> 0x1008
        instructionToStringMemoryToValue(registers[SP_REGISTER], MEMORY[registers[SP_REGISTER]]);
        POP_STACK_VALUE=MEMORY[registers[SP_REGISTER]];
        return TRUE;
    }
}

void runNonRegisterNonMemoryInstructions(int instruction){
    //0000 00xx xxxx xxxx=0x03FF
    int subOpcode=(instruction&0x03FF);
    switch (subOpcode) {
        case 0:
            //xisInstructionNOP=TRUE;
            //NOP
            //just increment the PC
            registers[PC_REGISTER]=addTwoSixteenBitInts(registers[PC_REGISTER], 1, FALSE);
            strcpy(symbolicOpcodeOfInstruction, "NOP");
            //strcat(symbolicOpcodeOfInstruction, regToString);
            //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
            symbolicOpcodeOfInstruction[3]='\0';
            break;
            
        case 1:
            //000000.0000000001 -- HLT. The low-order bit of the PSW is set to 0; the machine halts.
            registers[PSW_REGISTER]=((registers[PSW_REGISTER])&0xFF00);
            strcpy(symbolicOpcodeOfInstruction, "HLT");
            //strcat(symbolicOpcodeOfInstruction, regToString);
            //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
            symbolicOpcodeOfInstruction[3]='\0';
            //change the details string
            //PSW -> 0x0001, 0x0000 -> PSW
            //register->value, value->register
            instructionToStringRegisterToValue(PSW_REGISTER, 1);
            instructionToStringValueToRegister(PSW_REGISTER, 0);
            //set PSW to 0
            registers[PSW_REGISTER]=0;
            break;
        case 2:
            strcpy(symbolicOpcodeOfInstruction, "RET");
            //strcat(symbolicOpcodeOfInstruction, regToString);
            //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
            symbolicOpcodeOfInstruction[3]='\0';
            //000000.0000000010 -- RET. Pop the stack into the PC.
            
            //so our pop() function returns a Boolean value
            if(popFromStack()){
                registers[PC_REGISTER]=POP_STACK_VALUE;
                //details changed for popping from the stack
                ////SP -> 0xFFFE, 0xFFFF -> SP, M[0xFFFF] -> 0x1008
                //now, you change details for setting it to PC
                //0x1008 -> PC
                instructionToStringValueToRegister(PC_REGISTER, registers[PC_REGISTER]);
            }
            //instructionToStringMemoryToValue(address, memoryOp)
            break;
        default:
            break;
    }

}

void runRegisterMemoryReferenceInstructions(int instruction){
    //(ADDA): A -> 0x8000, M[0x104F] -> 0x7FFF, 0xFFFF -> A
    //xxxx 0000 0000 0000=0xF000
    int opcode=(instruction&0xF000)>>12;
    Address address=getEffectiveAddressPDP429(instruction);
    int memoryOperand=MEMORY[address];
    cycles++;
    //we also need to know with which register
    //we are dealing with
    //reg selector: 0000 xx00 0000 0000=0x0C00, shift>>10
    int regSelector=(instruction&0x0C00)>>10;
    //increment the program counter
    registers[PC_REGISTER]=addTwoSixteenBitInts(registers[PC_REGISTER], 1, FALSE);
    //B -> 0x0AA0: reg to value
    //A -> 0x8000, M[0x1056] -> 0x7FFF, 0xFFFF -> A
    if(opcode!=8 && opcode!=9){
        instructionToStringRegisterToValue(regSelector, registers[regSelector]);
        instructionToStringMemoryToValue(address, MEMORY[address]);
    }
    String regToString=registersToString[regSelector];
    switch (opcode) {
        case 1:
            //0001 -- ADD*: the register + memory-operand -> Reg
            registers[regSelector]=addTwoSixteenBitInts(registers[regSelector], memoryOperand, TRUE);
            //0xFFFF -> A: value to Reg
            
            ////--------------------------------------------      ///GERE WE DID, need to do it foe All cases
            //A -> 0x8000, M[0x1056] -> 0x7FFF, 0xFFFF -> A
            //First value of a
            instructionToStringValueToRegister(regSelector, registers[regSelector]);
            if (INDIRECT==FALSE) {
                strcpy(symbolicOpcodeOfInstruction, "ADD");
                strcat(symbolicOpcodeOfInstruction, regToString);
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[4]='\0';
            }
            else{
                strcpy(symbolicOpcodeOfInstruction, "I ADD");
                strcat(symbolicOpcodeOfInstruction, regToString);
                //strcpy(symbolicOpcodeOfInstruction, "");
                ///strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[6]='\0';
                INDIRECT=FALSE;
            }
            break;
        case 2:
            //0010 -- SUB*: the register - memory-operand -> Reg
            registers[regSelector]=subtractTwoSixteenBitInts(registers[regSelector], memoryOperand, TRUE);
            instructionToStringValueToRegister(regSelector, registers[regSelector]);
            if (INDIRECT==FALSE) {
                strcpy(symbolicOpcodeOfInstruction, "SUB");
                strcat(symbolicOpcodeOfInstruction, regToString);
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[4]='\0';
            }
            else{
                //strcpy(symbolicOpcodeOfInstruction, "I ");
                strcpy(symbolicOpcodeOfInstruction, "I SUB");
                strcat(symbolicOpcodeOfInstruction, regToString);
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[6]='\0';
                INDIRECT=FALSE;
            }
            break;
        case 3:
            //0011 -- MUL*: the register * memory-operand -> Reg
            registers[regSelector]=multiplyTwoSixteenBitInts(registers[regSelector], memoryOperand, TRUE);
            instructionToStringValueToRegister(regSelector, registers[regSelector]);
            if (INDIRECT==FALSE) {
                strcpy(symbolicOpcodeOfInstruction, "MUL");
                strcat(symbolicOpcodeOfInstruction, regToString);
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[4]='\0';
            }
            else{
                //strcpy(symbolicOpcodeOfInstruction, "I ");
                strcpy(symbolicOpcodeOfInstruction, "I MUL");
                strcat(symbolicOpcodeOfInstruction, regToString);
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[6]='\0';
                INDIRECT=FALSE;
            }
            break;
        case 4:
            //0100 -- DIV*: the register / memory-operand -> Reg
            registers[regSelector]=divideTwoSixteenBitInts(registers[regSelector], memoryOperand, TRUE);
            instructionToStringValueToRegister(regSelector, registers[regSelector]);
            if (INDIRECT==FALSE) {
                strcpy(symbolicOpcodeOfInstruction, "DIV");
                strcat(symbolicOpcodeOfInstruction, regToString);
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[4]='\0';
            }
            else{
                //strcpy(symbolicOpcodeOfInstruction, "I ");
                strcpy(symbolicOpcodeOfInstruction, "I DIV");
                strcat(symbolicOpcodeOfInstruction, regToString);
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[6]='\0';
                INDIRECT=FALSE;
            }
            break;
        case 5:
            //0101 -- AND*: the register & memory-operand -> Reg
            registers[regSelector]=(registers[regSelector])&memoryOperand;
            instructionToStringValueToRegister(regSelector, registers[regSelector]);
            if (INDIRECT==FALSE) {
                strcpy(symbolicOpcodeOfInstruction, "AND");
                strcat(symbolicOpcodeOfInstruction, regToString);
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[4]='\0';
            }
            else{
                //strcpy(symbolicOpcodeOfInstruction, "I ");
                strcpy(symbolicOpcodeOfInstruction, "I AND");
                strcat(symbolicOpcodeOfInstruction, regToString);
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[6]='\0';
                INDIRECT=FALSE;
            }
            break;
        case 6:
            //0110 -- OR*: the register | memory-operand -> Reg
            registers[regSelector]=(registers[regSelector])|memoryOperand;
            instructionToStringValueToRegister(regSelector, registers[regSelector]);
            if (INDIRECT==FALSE) {
                strcpy(symbolicOpcodeOfInstruction, "OR");
                strcat(symbolicOpcodeOfInstruction, regToString);
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[3]='\0';
            }
            else{
                //strcpy(symbolicOpcodeOfInstruction, "I ");
                strcpy(symbolicOpcodeOfInstruction, "I OR");
                strcat(symbolicOpcodeOfInstruction, regToString);
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[5]='\0';
                INDIRECT=FALSE;
            }
            break;
        case 7:
            //0111 -- XOR*: the register ^ memory-operand -> Reg
            registers[regSelector]=(registers[regSelector])^memoryOperand;
            instructionToStringValueToRegister(regSelector, registers[regSelector]);
            if (INDIRECT==FALSE) {
                strcpy(symbolicOpcodeOfInstruction, "XOR");
                strcat(symbolicOpcodeOfInstruction, regToString);
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[4]='\0';
            }
            else{
                //strcpy(symbolicOpcodeOfInstruction, "I ");
                strcpy(symbolicOpcodeOfInstruction, "I XOR");
                strcat(symbolicOpcodeOfInstruction, regToString);
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[6]='\0';
                INDIRECT=FALSE;
            }
            break;
        case 8:
            //1000 -- LD*: memory-operand -> Reg
            //(LDA): M[0x1058] -> 0x1059, 0x1059 -> A
            //memory to value
            instructionToStringMemoryToValue(address, memoryOperand);
            registers[regSelector]=memoryOperand;
            //value to register
            instructionToStringValueToRegister(regSelector, registers[regSelector]);
            if (INDIRECT==FALSE) {
                strcpy(symbolicOpcodeOfInstruction, "LD");
                strcat(symbolicOpcodeOfInstruction, regToString);
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[3]='\0';
            }
            else{
                //strcpy(symbolicOpcodeOfInstruction, "I ");
                strcpy(symbolicOpcodeOfInstruction, "I LD");
                strcat(symbolicOpcodeOfInstruction, regToString);
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[5]='\0';
                INDIRECT=FALSE;
            }
            break;
        case 9:
            //1001 -- ST*: the register -> memory-operand
            //STA: A -> 0xFFFE, 0xFFFE -> M[0x1039]
            //register to value
            instructionToStringRegisterToValue(regSelector, registers[regSelector]);
            
            Store_Memory(address, registers[regSelector]);
            //value to memory
            instructionToStringValueToMemory(address, registers[regSelector]);
            if (INDIRECT==FALSE) {
                strcpy(symbolicOpcodeOfInstruction, "ST");
                strcat(symbolicOpcodeOfInstruction, regToString);
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[3]='\0';
            }
            else{
                //strcpy(symbolicOpcodeOfInstruction, "I ");
                strcpy(symbolicOpcodeOfInstruction, "I ST");
                strcat(symbolicOpcodeOfInstruction, regToString);
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[5]='\0';
                INDIRECT=FALSE;
            }
            break;
        default:
            break;
    }
    
}

void runIOTransferInstructions(int instruction){
    /*The IOT instruction is handled specially. Ignore the function code.
     If the device is 3, do a getchar() to read a byte from standard input (stdin) 
     and put that byte in the register specified in the IOT instruction. 
     An EOF on stdin should cause the register to be set to 0xFFFF. 
     If the device is 4, take the low-order 8 bits from the A register 
     and output it as a byte to standard output (putchar()).
     */
    //mask for device: 0000 00xx xxxx x000=0x03F8, shift>>3
    int mask=0x03F8;
    int device=(instruction&mask)>>3;
    //mask for register: 0000 xx00 0000 0000=0x0C00, shift>>10
    mask=0x0C00;
    int regSelector=(instruction&mask)>>10;
    int lowOrderByteOfReg=(registers[regSelector])&0x00FF;
    
    //no memory access, so cycles stays the same
    //increment pc by 1
    registers[PC_REGISTER]=addTwoSixteenBitInts(registers[PC_REGISTER], 1, FALSE);
    if(device==3){
        int c=getc(stdin);
        if(c==EOF){
            registers[regSelector]=0xFFFF;
            //(IOT 3): 0x0031 -> C
            //value to reg
            instructionToStringValueToRegister(regSelector, 0xFFFF);
        }
        else{
            registers[regSelector]=c;
            //(IOT 3): 0x0031 -> C
            //value to reg
            instructionToStringValueToRegister(regSelector, c);
        }
        strcpy(symbolicOpcodeOfInstruction, "IOT 3");
        //strcat(symbolicOpcodeOfInstruction, regToString);
        //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
        symbolicOpcodeOfInstruction[5]='\0';
        
    }
    else if(device==4){
        putc(lowOrderByteOfReg, stdout);
        //(IOT 4): A -> 0x0020
        //reg to value
        instructionToStringRegisterToValue(regSelector, lowOrderByteOfReg);
        strcpy(symbolicOpcodeOfInstruction, "IOT 4");
        //strcat(symbolicOpcodeOfInstruction, regToString);
        //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
        symbolicOpcodeOfInstruction[5]='\0';
    }
    else{
        printf("Dumb, you got IOT device wrong!");
    }
    
}

void runNonRegisterMemoryReferenceInstructions(int instruction){
    //xxxx xx00 0000 0000=0xF000
    int opcode=(instruction&0xFC00)>>10;
    Address address=getEffectiveAddressPDP429(instruction);
    int memoryOperand=MEMORY[address];
    cycles++;
    /*
     The opcodes are:
     
     1011.00 -44- ISZ: memory-operand + 1 -> memory-operand; if memory-operand == 0, PC + 1 -> PC
     1011.01 -45- JMP: address of memory-operand -> PC
     1011.10 -46- CALL: push return address (PC + 1) on stack; address of memory-operand -> PC
     1100.00 -48- PUSH: push memory-operand to the stack
     1100.01 -49- POP: pop top of stack and store in memory-operand.
     */
    //no memory access, so cycles stays the same
    //increment pc by 1
    registers[PC_REGISTER]=addTwoSixteenBitInts(registers[PC_REGISTER], 1, FALSE);
    switch (opcode) {
        case 44:
            //1011.00 -44- ISZ: memory-operand + 1 -> memory-operand; if memory-operand == 0, PC + 1 -> PC
            //(ISZ): M[0x1039] -> 0xFFFE, 0xFFFF -> M[0x1039]
            //(ISZ): M[0x1039] -> 0xFFFF, 0x0000 -> M[0x1039], 0x103A -> PC-------if memoperand==0
            //memory to value
            instructionToStringMemoryToValue(address, memoryOperand);
            MEMORY[address]=addTwoSixteenBitInts(memoryOperand, 1, TRUE);
            //0xFFFF -> M[0x1039]
            //value to memory
            instructionToStringValueToMemory(address, MEMORY[address]);
            cycles++;
            if(MEMORY[address]==0){
                registers[PC_REGISTER]=addTwoSixteenBitInts(registers[PC_REGISTER], 1, FALSE);
                //0x103A -> PC-------if memoperand==0
                //value to register
                instructionToStringValueToRegister(PC_REGISTER, registers[PC_REGISTER]);
            }
            if (INDIRECT==FALSE) {
                strcpy(symbolicOpcodeOfInstruction, "ISZ");
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[4]='\0';
            }
            else{
                //strcpy(symbolicOpcodeOfInstruction, "I ");
                strcpy(symbolicOpcodeOfInstruction, "I ISZ");
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[6]='\0';
                INDIRECT=FALSE;
            }
            break;
        case 45:
            //1011.01 -45- JMP: address of memory-operand -> PC
            registers[PC_REGISTER]=address;
            //(JMP): 0x104D -> PC
            //value to reg
            instructionToStringValueToRegister(PC_REGISTER, registers[PC_REGISTER]);
            //it does not need memory operand, so cycles--
            cycles--;
            if (INDIRECT==FALSE) {
                strcpy(symbolicOpcodeOfInstruction, "JMP");
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[4]='\0';
            }
            else{
                //printf("Come on bro!\n");
                //strcpy(symbolicOpcodeOfInstruction, "I ");
                strcpy(symbolicOpcodeOfInstruction, "I JMP");
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[6]='\0';
                INDIRECT=FALSE;
            }
            break;
        case 46:
            if (INDIRECT==FALSE) {
                strcpy(symbolicOpcodeOfInstruction, "CALL");
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[5]='\0';
            }
            else{
                //strcpy(symbolicOpcodeOfInstruction, "I ");
                strcpy(symbolicOpcodeOfInstruction, "I CALL");
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[7]='\0';
                INDIRECT=FALSE;
            }
            //1011.10 -46- CALL: push return address (PC + 1) on stack; address of memory-operand -> PC
            //already incremented
            //it does not need memory operand, so cycles--
            cycles--;
            //(CALL): 0x1024 -> M[0xFFFF], 0xFFFE -> SP, 0x1047 -> PC
            //memory operand not needed
            pushToStack(registers[PC_REGISTER]);
            registers[PC_REGISTER]=address;
            //0x1047 -> PC
            //value to register
            instructionToStringValueToRegister(PC_REGISTER, registers[PC_REGISTER]);
            
           
            break;
        case 48:
            //1100.00 -48- PUSH: push memory-operand to the stack
            //(PUSH): M[0x1054] -> 0x00AA, 0x00AA -> M[0xFFFF], 0xFFFE -> SP
            //M[0x1054] -> 0x00AA
            //memory to value
            instructionToStringMemoryToValue(address, memoryOperand);
            
            
            
            if (INDIRECT==FALSE) {
                strcpy(symbolicOpcodeOfInstruction, "PUSH");
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[5]='\0';
            }
            else{
                //strcpy(symbolicOpcodeOfInstruction, "I ");
                strcpy(symbolicOpcodeOfInstruction, "I PUSH");
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[7]='\0';
                INDIRECT=FALSE;
            }
            pushToStack(memoryOperand);
            break;
        case 49:
            if (INDIRECT==FALSE) {
                strcpy(symbolicOpcodeOfInstruction, "POP");
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[4]='\0';
            }
            else{
                //strcpy(symbolicOpcodeOfInstruction, "I ");
                strcpy(symbolicOpcodeOfInstruction, "I POP");
                //strcpy(symbolicOpcodeOfInstruction, registersToString[regSelector]);
                symbolicOpcodeOfInstruction[6]='\0';
                INDIRECT=FALSE;
            }
            //1100.01 -49- POP: pop top of stack and store in memory-operand.
            //now, our function pop() returns a Booolean value
            if(popFromStack()){
                MEMORY[address]=POP_STACK_VALUE;
                // (POP): SP -> 0xFFFE, 0xFFFF -> SP, M[0xFFFF] -> 0x000B, 0x000B -> M[0x0033]
                //cycle???????????????????????????????????????????????????????????????????
                //POP): SP -> 0xFFFE, 0xFFFF -> SP, M[0xFFFF] -> 0x000B, 0x000B -> M[0x0033]
                //0x000B -> M[0x0033]
                //value to memory
                instructionToStringValueToMemory(address, MEMORY[address]);
            }
            
            break;
            
        default:
            printf("Dumb, there is no such case for Non REG Mem Inst\n");
            break;
    }
}

void runRegistertoRegisterInstructions(int instruction){
    //mask for subopcode: 0000 xxx0 0000 0000=0x0D00
    int subOpcode=(instruction&0x0E00)>>9;
    //mask for reg(i): 0000 000x xx00 0000=0x01C0
    int regI=(instruction&0x01C0)>>6;
    //mask for reg(j): 0000 0000 00xx x000=0x0038
    int regJ=(instruction&0x0038)>>3;
    //mask for reg(k): 0000 0000 000 0xxx=0x0007
    int regK=(instruction&0x0007);
    
    //(DIV): A -> 0x0071, B -> 0x000B, 0x000A -> C
    //(SUB): C -> 0x000A, B -> 0x000B, 0xFFFF -> C
    //first two: reg to value
    registers[PC_REGISTER]=addTwoSixteenBitInts(registers[PC_REGISTER], 1, FALSE);
    instructionToStringRegisterToValue(regJ, registers[regJ]);
    instructionToStringRegisterToValue(regK, registers[regK]);

    
    switch (subOpcode) {
        case 0:
            //1110.000 -- MOD: Reg[j] % Reg[k] -> Reg[i]
            //printf("mod: i: %s= %d, j: %s= %d, k: %s= %d\n", registersToString[regI], registers[regI], registersToString[regJ],registers[regJ], registersToString[regK], registers[regK]);
            registers[regI]=modTwoSixteenBitInts(registers[regJ], registers[regK], TRUE);
            strcpy(symbolicOpcodeOfInstruction, "MOD");
            symbolicOpcodeOfInstruction[3]='\0';
            break;
        case 1:
            //1110.001 -- ADD: Reg[j] + Reg[k] -> Reg[i]
            registers[regI]=addTwoSixteenBitInts(registers[regJ], registers[regK], TRUE);
            strcpy(symbolicOpcodeOfInstruction, "ADD");
            symbolicOpcodeOfInstruction[3]='\0';
            break;
        case 2:
            //1110.010 -- SUB: Reg[j] - Reg[k] -> Reg[i]
            registers[regI]=subtractTwoSixteenBitInts(registers[regJ], registers[regK], TRUE);
            strcpy(symbolicOpcodeOfInstruction, "SUB");
            symbolicOpcodeOfInstruction[3]='\0';
            break;
        case 3:
            //1110.011 -- MUL: Reg[j] * Reg[k] -> Reg[i]
            registers[regI]=multiplyTwoSixteenBitInts(registers[regJ], registers[regK], TRUE);
            strcpy(symbolicOpcodeOfInstruction, "MUL");
            symbolicOpcodeOfInstruction[3]='\0';
            break;
        case 4:
            //1110.100 -- DIV: Reg[j] / Reg[k] -> Reg[i]
            registers[regI]=divideTwoSixteenBitInts(registers[regJ], registers[regK], TRUE);
            strcpy(symbolicOpcodeOfInstruction, "DIV");
            symbolicOpcodeOfInstruction[3]='\0';
            break;
        case 5:
            //1110.101 -- AND: Reg[j] & Reg[k] -> Reg[i]
            registers[regI]=(registers[regJ])&(registers[regK]);
            strcpy(symbolicOpcodeOfInstruction, "AND");
            symbolicOpcodeOfInstruction[3]='\0';
            break;
        case 6:
            //1110.110 -- OR: Reg[j] | Reg[k] -> Reg[i]
            registers[regI]=(registers[regJ])|(registers[regK]);
            strcpy(symbolicOpcodeOfInstruction, "OR");
            symbolicOpcodeOfInstruction[2]='\0';
            break;
        case 7:
            //1110.111 -- XOR: Reg[j] ^ Reg[k] -> Reg[i]
            registers[regI]=(registers[regJ])^(registers[regK]);
            strcpy(symbolicOpcodeOfInstruction, "XOR");
            symbolicOpcodeOfInstruction[3]='\0';
            break;
            
        default:
            break;
    }
    ////(SUB): C -> 0x000A, B -> 0x000B, 0xFFFF -> C
    //last: value to register
    instructionToStringValueToRegister(regI, registers[regI]);
}

void runNonMemoryRegisterInstructions(int instruction){
    /*
     This is like the operate instruction for the PDP-8.
     
     The 2-bit register selector determines which general purpose register is used for this instruction. The encoding is the same as for the Register Memory Reference Instructions.
     
     The individual bits for the Non-memory Register Instruction are:
     
     SM* -- Skip if the register is negative (sign bit is 1)
     SZ* -- Skip if the register is zero
     SNL -- Skip if the Link bit is non-zero
     RSS -- Reverse the Skip Sense
     CL* -- Clear the register
     CLL -- Clear the Link bit
     CM* -- Complement the register
     CML -- Complement the Link bit
     DC* -- Decrement the register by one
     IN* -- Increment the register by one
     The "*" in the opcode indicates the register selected: SMA, SMB, SMC, SMD, and so on.
     The bits are evaluated in the order listed above.
     */
    registers[PC_REGISTER]=addTwoSixteenBitInts(registers[PC_REGISTER], 1, FALSE);
    //reg selector mask: 0000 xx00 0000 0000=0x0C00, shift>>10
    int regSelector=(instruction&0x0C00)>>10;
    Boolean registerIsNegative;
    //1000 0000 0000 0000, shift>>15
    if((registers[regSelector])>>15)
        registerIsNegative=TRUE;
    else
        registerIsNegative=FALSE;
    
    //editing from PDP-8 code---------------!!!!!!!!!!!!!!!!!!!!!------------------------!!!!!!!!!!!!!!
    String operateInstructions[]={"SM", "SZ", "SNL", "RSS", "CL", "CLL", "CM", "CML", "DC", "IN"};
    Boolean operateSelected[]={FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
    //0000 00(SM)(SZ) (SNL)(RSS)(CL)(CLL) (CM)(CML)(DC)(IN)
    //mask=0000 0010 0000 0000=0x0200
    int smBit=(instruction&0x0200)>>9;
    //mask=0000 0001 0000 0000=0x0100
    int szBit=(instruction&0x0100)>>8;
    //mask=0000 0000 1000 0000=0x0080
    int snlBit=(instruction&0x0080)>>7;
    //mask=0000 0000 0100 0000=0x0040
    int rssBit=(instruction&0x0040)>>6;
    //mask=0000 0000 0010 0000=0x0020
    int clBit=(instruction&0x0020)>>5;
    //mask=0000 0000 0001 0000=0x0010
    int cllBit=(instruction&0x0010)>>4;
    //mask=0000 0000 0000 1000=0x0008
    int cmBit=(instruction&0x0008)>>3;
    //mask=0000 0000 0000 0100=0x0004
    int cmlBit=(instruction&0x0004)>>2;
    //mask=0000 0000 0000 0010=0x0002
    int dcBit=(instruction&0x0002)>>1;
    //mask=0000 0000 0000 0001=0x0001
    int inBit=(instruction&0x0001);
    //int lastBit=(instruction&0x001);
    //so we are done evaluating the individual bits
    //now let's do the real job
    //error cases: lastBit=1
    //order
    //Test SMA, SZA, SNL. If any of these are selected and the condition is true, set the Skip
    //flag. If all selected conditions are false, clear the Skip flag. (If none are selected, the
    //Skip flag is cleared.)
    //SMA	 Skip on Minus A. If the A register is negative, skip the next instruction.
    //mask=1000 0000 0000
    
    //int bitEleven=(registerAValue&0x800)>>11;
    //int registerValue=(registers[regSelector]);
    if(smBit){
        //(SMA SZA): A -> 0xFFFE, 0x103C -> PC
        //(SMA SZA RSS): A -> 0xFFFE
        operateSelected[0]=TRUE;
        //instructionToStringRegisterToValue(regSelector, registers[regSelector]);
        instructionToStringRegisterToValue(regSelector, registers[regSelector]);
        if(registerIsNegative){
            SKIP_FLAG=TRUE;
            
        }
    }
    //SZ	 Skip on Zero A. If the A register is zero, skip the next instruction.
    if(szBit){
        operateSelected[1]=TRUE;
        //printf("regSelector: %s\n", registersToString[regSelector]);
        //printf("regValue: %d\n", (registers[regSelector]));
        if(SKIP_FLAG==FALSE){
            instructionToStringRegisterToValue(regSelector, registers[regSelector]);
            
        }
        if((registers[regSelector])==0){
            SKIP_FLAG=TRUE;
            //printf("Skip flag set\n");
           
        }
        //instructionToStringRegisterToValue(regSelector, registers[regSelector]);
    }
    //SNL	 Skip on Nonzero Link. If the Link bit is one, skip the next instruction.
    if(snlBit){
        operateSelected[2]=TRUE;
        instructionToStringRegisterToValue(LINK_BIT_INDEX, registerLinkValue);
        if(registerLinkValue!=0)
            SKIP_FLAG=TRUE;
    }
    //If RSS is selected, complement the Skip flag.
    //Reverse Skip Sense. If this bit is one, the SMA, SZA, and SNL subinstructions will skip
    //on the opposite condition. That is, SMA skips on positive or zero, SZA skips on nonzero
    // and SNL skips if the Link is zero.
    if(rssBit==1){
        operateSelected[3]=TRUE;
        if (SKIP_FLAG==TRUE) {
            SKIP_FLAG=FALSE;
        }
        else{
            SKIP_FLAG=TRUE;
        }
    }
    //printf("before clb B: %d\n", registers[B_REGISTER]);
    //(CLA CMA DCA): 0x0000 -> A, A -> 0x0000, 0xFFFF -> A, A -> 0xFFFF, 0xFFFE -> A
    //CL* -- Clear the register
    if(clBit==1){
        //registerAValue=0;
        registers[regSelector]=0;
        //0x0000 -> A
        //value to register
        instructionToStringValueToRegister(regSelector, 0);
        operateSelected[4]=TRUE;
    }
    //printf("after clb B: %d\n", registers[B_REGISTER]);
    //CLL -- Clear the Link bit
    if(cllBit==1){
        registerLinkValue=0;
        instructionToStringValueToRegister(LINK_BIT_INDEX, 0);
        operateSelected[5]=TRUE;
    }
    //CM* -- Complement the register (bit by bit, change 1 to 0 and 0 to 1).
    if (cmBit==1) {
        //mask=0000 0000 0000 0000 xxxx xxxx xxxx xxxx
        // A -> 0x0000, 0xFFFF -> A
        //register to value
        instructionToStringRegisterToValue(regSelector, registers[regSelector]);
        registers[regSelector]=(~(registers[regSelector]))&0xFFFF;
        //0xFFFF -> A
        //value to register
        instructionToStringValueToRegister(regSelector, registers[regSelector]);
        operateSelected[6]=TRUE;
    }
    //CML	 Complement the Link bit.
    if (cmlBit==1) {
        ////register to value
        instructionToStringRegisterToValue(LINK_BIT_INDEX, registerLinkValue);
        if (registerLinkValue==1) {
            registerLinkValue=0;
        }
        else{
            registerLinkValue=1;
        }
        //0xFFFF -> A
        //value to register
        instructionToStringValueToRegister(LINK_BIT_INDEX, registerLinkValue);
        operateSelected[7]=TRUE;
    }
    
    //DC* -- Decrement the register by one
    if (dcBit==1) {
        // A -> 0x0000, 0xFFFF -> A
        //register to value
        instructionToStringRegisterToValue(regSelector, registers[regSelector]);
        
        operateSelected[8]=TRUE;
        registers[regSelector]=subtractTwoSixteenBitInts((registers[regSelector]), 1, TRUE);
        //0xFFFF -> A
        //value to register
        instructionToStringValueToRegister(regSelector, registers[regSelector]);
    }
    //IN* -- Increment the register by one
    if (inBit==1) {
        // A -> 0x0000, 0xFFFF -> A
        //register to value
        instructionToStringRegisterToValue(regSelector, registers[regSelector]);
        
        operateSelected[9]=TRUE;
        registers[regSelector]=addTwoSixteenBitInts((registers[regSelector]), 1, TRUE);
        ///0xFFFF -> A
        //value to register
        instructionToStringValueToRegister(regSelector, registers[regSelector]);
    }
    //printf("after INb B: %d\n", registers[B_REGISTER]);
    //be careful in here!!!
    //concatenateBooleanString(String instructions[], Boolean selected[], int size, int regSelector)
    concatenateBooleanString(operateInstructions, operateSelected, 10, regSelector);
    
}

void process(){
    symbolicOpcodeOfInstruction=(String)malloc(100*sizeof(char));
    detailsOfInstruction=(String)malloc(200*sizeof(char));
    //first we have to load the file into memory
    
    //since load function returns an entry point
    ENTRY_POINT=Load_Binary_Object_File();
    fclose(input);
    //we have proccessed all the input info
    //now fetch decode and execute
    registers[PC_REGISTER]=ENTRY_POINT;
    programCounter=registers[PC_REGISTER];
    //printf("PC initial value hex: %03x\n", programCounter);
    //Boolean run=TRUE;
    int instruction;
    //fetch-decode-execute till HALT=0xFO2
    //int k=0;
    //for now 10 instructions
    //RUN_PROGRAM=FALSE;
    //VERBOSE_MODE=TRUE;
    //check psw before you run!
    //set PSW to 1 when we start the program
    registers[PSW_REGISTER]=0x0001;
    while(runTheProgram()){
        //printf("B value: %d\n", registers[B_REGISTER]);
        detailsOfInstruction[0]='\0';
        symbolicOpcodeOfInstruction[0]='\0';
        programCounter=registers[PC_REGISTER];
        instruction=MEMORY[programCounter];
        cycles++;
        //printf("intruction inside while: %03x\n", instruction);
        //numbers: xxxx xxxx xxxx xxxx
        //first four is an opcode, so mask=0xF000
        int opcode=(instruction&0xF000)>>12;
        //printf("opcode: %d\n", opcode);
        //there are 6 cases for opcodes
        //with subcases each
        //memory reference- 0 t0 5
        //operate instruction- 7
        //input-output instruction-6
        //increment thy program counter-
        //---jumps and that stuff watch out
        int pcBeforeExecution=programCounter;
        /*if(opcode<=5){
            runMemoryReferenceInstruction(instruction, opcode);
        }
        else if(opcode==7){
            SKIP_FLAG=FALSE;
            runOperateInstruction(instruction);
            //cycles++;-------------------------------check this--------------
            
            programCounter=addTwoTwelveBitInts(programCounter, 1);
            if (SKIP_FLAG==TRUE) {
                
                programCounter=addTwoTwelveBitInts(programCounter, 1);
            }
        }
        else{
            //iot instruction
            runInputOutputInstruction(instruction);
            programCounter=addTwoTwelveBitInts(programCounter, 1);
        }*/
        //write a function for each
        if(opcode==0){
            //Non-register, Non-memory Instructions
            runNonRegisterNonMemoryInstructions(instruction);
        }
        else if (opcode>=1 && opcode<=9){
            //Register Memory Reference Instructions
            runRegisterMemoryReferenceInstructions(instruction);
        }
        else if (opcode==10){
            //I/O Transfer Instructions
            runIOTransferInstructions(instruction);
        }
        else if (opcode==11 || opcode==12){
            //Non-register Memory Reference Instructions
            runNonRegisterMemoryReferenceInstructions(instruction);
        }
        else if (opcode==14){
            //Register-to-Register Instructions
            runRegistertoRegisterInstructions(instruction);
        }
        else if (opcode==15){
            //opcode=15
            //Non-memory Register Instructions
            SKIP_FLAG=FALSE;
            runNonMemoryRegisterInstructions(instruction);
            if(SKIP_FLAG==TRUE){
                //increment pc by one
                registers[PC_REGISTER]=addTwoSixteenBitInts(registers[PC_REGISTER], 1, FALSE);
                //change the details string
                //(SMA SZA): A -> 0xFFFE, 0x103C -> PC
                //value to register
                instructionToStringValueToRegister(PC_REGISTER, registers[PC_REGISTER]);
            }
        }
        else{
            printf("Dumb, there is no such opcode: 13\n");
        }
        if(VERBOSE_MODE==TRUE){
            /*
             fprintf(stderr, "Time %3lld: PC=0x%04X instruction = 0x%04X (%s)",
             time, old_PC, inst, get_opcode_names());
             char *regs = ...;
             if (regs != NULL)
             fprintf(stderr, ": %s", regs);
             fprintf(stderr, "\n");
             */
            fprintf(stderr, "Time %3lld: PC=0x%04X instruction = 0x%04X (%s)", cycles, pcBeforeExecution, instruction, trimTrailingWhitespacesAndCommas(symbolicOpcodeOfInstruction));
            //!isInstructionNOP &&
            if(strlen(detailsOfInstruction)!=0)
                fprintf(stderr, ": %s", trimTrailingWhitespacesAndCommas(detailsOfInstruction));
            fprintf(stderr, "\n");
        }
        //k++;
    }
    free(symbolicOpcodeOfInstruction);
    free(detailsOfInstruction);
}

void scanargs(String s){
    /* check each character of the option list for
     its meaning. */
    
    while (*(++s) != '\0')
        switch (*s){
                
            case 'v': /* debug option */
                VERBOSE_MODE = TRUE;
                break;
                
            default:
                fprintf (stderr,"PROGR: Bad option %c\n", *s);
                fprintf (stderr,"usage: PROGR [-d] file\n");
                exit(1);
        }
}

int main(int argc, char * argv[]){
    /*symbolicOpcodeOfInstruction=(String)malloc(100*sizeof(char));
     //input=fopen("/Users/dilnuryuldashevv/Desktop/UTexasAustin/FALL 2013/CS 429/Labs/XCodeLabs/Program7/spover.obj", "r");
     //process();*/
  
     while (argc > 1){
        argc--, argv++;
        if (**argv == '-'){
            //-d for encoding, so we look for 'd' in scanargs
            scanargs(*argv);
        }
        else{//------------------------------------------------------------------------
            //there should be a fileName given, so here we deal with encoding
            //or decoding from a file
            input = fopen(*argv,"r");
            if (input == NULL){
                //failed to open the file
                fprintf (stderr, "Can't open %s\n",*argv);
            }
            else{
                //file was opened successfully, so let's go on with encoding
                //or decoding, and we do it from the file, input!=stdin, but input=*argv
                process();
                //fclose(input);
            }
        }//------------------------------------------------------------------------------
    }
    return 0;
}