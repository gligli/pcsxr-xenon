#include <ppc/vm.h>
#include <assert.h>

#include "libxenon_vm.h"
#include "ppc.h"
#include "pR3000A.h"
#include "psxmem.h"

#define MEMORY_VM_BASE 0x40000000
#define MEMORY_VM_SIZE (1024*1024*1024)

#define PPC_OPCODE_B     18
#define PPC_OPCODE_SHIFT 26

#define CHECK_INVALID_CODE() assert(0);


int failsafeRec=0;

void recCallDynaMem(int addr, int data, int type)
{
//	printf("%d %d %d\n",addr,data,type);

	if (type<MEM_SW)
	{
		if(addr!=3)
			MR(3, addr);
		
		switch (type)
		{
			case MEM_LB:
				CALLFunc((u32) psxMemRead8);
				EXTSB(data, 3);
				break;
			case MEM_LBU:
				CALLFunc((u32) psxMemRead8);
				MR(data,3);
				break;
			case MEM_LH:
				CALLFunc((u32) psxMemRead16);
				EXTSH(data, 3);
				break;
			case MEM_LHU:
				CALLFunc((u32) psxMemRead16);
				MR(data,3);
				break;
			case MEM_LW:
				CALLFunc((u32) psxMemRead32);
				MR(data,3);
				break;
		}
	}
	else
	{
		assert(0);
	}
}

void recCallDynaMemVM(int rs_reg, int rt_reg, memType type, int immed)
{
//	do_disasm=1;
	
	u32 * preCall=NULL;
	u32 * old_ppcPtr=NULL;
	
	InvalidateCPURegs();

	int base = GetHWReg32( rs_reg );
	int data = -1;
	int addr_host = 7;
	int addr_emu = 3;
	
	if (type<MEM_SW)
		data = PutHWReg32( rt_reg );
	else
		data = GetHWReg32( rt_reg );
	
	ADDI(addr_emu, base, immed);
	
	if(!(failsafeRec&FAILSAFE_REC_NO_VM))
	{
		RLWINM(addr_host,addr_emu,0,3,31);
		ADDIS(addr_host,addr_host,MEMORY_VM_BASE>>16);

		// Perform the actual load
		switch (type)
		{
			case MEM_LB:
			{
//				LIS(addr_host,0x3040);
				
				LBZ(data, 0, addr_host);
				EXTSB(data, data);
				break;
			}
			case MEM_LBU:
			{
//				LIS(addr_host,0x3041);
				
				LBZ(data, 0, addr_host);
				break;
			}
			case MEM_LH:
			{
//				LIS(addr_host,0x3042);
				
				LHBRX(data, 0, addr_host);
				EXTSH(data, data);
				break;
			}
			case MEM_LHU:
			{
//				LIS(addr_host,0x3043);
				
				LHBRX(data, 0, addr_host);
				break;
			}
			case MEM_LW:
			{
//				LIS(addr_host,0x3044);
				
				LWBRX(data, 0, addr_host);
				break;
			}
/*			case MEM_LWL:
			{
				ADDI(rd, rd, immed);
				RLWINM(0, rd, 0, 30, 31);	// r0 = addr & 3
				CMPWI(0, 0);
				BNE(3); // /!\ branch to just after 'Skip over else'

				int r = PutHWReg32( rt_reg );
				LWZ(r, 0, rd);
				break;
			}
			case MEM_LWR:
			{
				ADDI(rd, rd, immed);
				RLWINM(0, rd, 0, 30, 31);	// r0 = addr & 3
				CMPWI(0, 3);
				BNE(4); // /!\ branch to just after 'Skip over else'

				RLWINM(rd, rd, 0, 0, 29);	// addr &= 0xFFFFFFFC

				int r = PutHWReg32( rt_reg );
				LWZ(r, 0, rd);
				break;
			}
			case MEM_SB:
			{
				int r = PutHWReg32( rt_reg );
				STB(r, immed, rd);
				CHECK_INVALID_CODE();
				break;
			}
			case MEM_SH:
			{
				int r = PutHWReg32( rt_reg );
				STH(r, immed, rd);
				CHECK_INVALID_CODE();
				break;
			}
			case MEM_SW:
			{
				int r = PutHWReg32( rt_reg );
				STW(r, immed, rd);
				CHECK_INVALID_CODE();
				break;
			}*/
			default:
				assert(0);
		}
		
		// Skip over else
		preCall = ppcPtr;
		B(0);
	}

	recCallDynaMem(addr_emu, data, type);
	
	if(!(failsafeRec&FAILSAFE_REC_NO_VM))
	{
		old_ppcPtr=ppcPtr;
		ppcPtr=preCall;
		B(old_ppcPtr-preCall-1);
		ppcPtr=old_ppcPtr;
	}
}

static void * rewriteDynaMemVM(void* fault_addr)
{
    // enabling slow access by adding a jump from the fault address to the slow mem access code

	u32 * old_ppcPtr=ppcPtr;

    u32 * fault_op=(u32 *)fault_addr;
	
	u32 * op=fault_op;
	
    while((*op>>PPC_OPCODE_SHIFT)!=PPC_OPCODE_B || (*op&1)!=0)
    {
        ++op;
    }

    // branch op
	++op;
	
	u32 * first_slow_op=op;

//	disassemble((u32)fault_op,*fault_op);
	
	ppcPtr=fault_op;
	B(first_slow_op-fault_op-1);
	
    memicbi(fault_op,4);
    
	ppcPtr=old_ppcPtr;
	
    return first_slow_op;
}

void * recDynaMemVMSegfaultHandler(int pir_,void * srr0,void * dar,int write)
{
    if((u32)srr0>=(u32)recMem && (u32)srr0<(u32)recMem+RECMEM_SIZE)
    {
        printf("Rewrite %d %p %p %d\n",pir_,srr0,dar,write);
        return rewriteDynaMemVM(srr0);
    }
    else
    {
        printf("VM GPF !!! %d %p %p %d\n",pir_,srr0,dar,write);
		
		// use the standard segfault handler
	    vm_set_user_mapping_segfault_handler(NULL);
        return NULL;
    }
}



void recInitDynaMemVM()
{
    vm_set_user_mapping_segfault_handler(recDynaMemVMSegfaultHandler);

    u32 base=MEMORY_VM_BASE;
    
    // map ram
    vm_create_user_mapping(base,((u32)&psxM[0])&0x7fffffff,2*1024*1024,VM_WIMG_CACHED);

    // map bios
    vm_create_user_mapping(base+0x1fc00000,((u32)&psxR[0])&0x7fffffff,512*1024,VM_WIMG_CACHED_READ_ONLY);
}

void recDestroyDynaMemVM()
{
    vm_set_user_mapping_segfault_handler(NULL);
    vm_destroy_user_mapping(MEMORY_VM_BASE,MEMORY_VM_SIZE);
}

