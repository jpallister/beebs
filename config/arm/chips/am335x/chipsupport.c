/* Copyright (C) 2014 Embecosm Limited and University of Bristol

   Contributor James Pallister <james.pallister@bristol.ac.uk>

   This file is part of the Bristol/Embecosm Embedded Benchmark Suite.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>. */

#include <string.h>

// This file is needed to copy the initialised data from flash to RAM

extern unsigned int _start_data;
extern unsigned int _end_data;
extern unsigned int _start_datai;
extern unsigned int _end_datai;
extern unsigned int _start_bss;
extern unsigned int _end_bss;

#ifndef MMU_STATE
    #define MMU_STATE    1
#endif
#ifndef DCACHE_STATE
    #define DCACHE_STATE    1
#endif
#ifndef ICACHE_STATE
    #define ICACHE_STATE    1
#endif
#ifndef L2_STATE
    #define L2_STATE    1
#endif
#ifndef BP_STATE
    #define BP_STATE    1
#endif

void data_memory_barrier()
{
    asm("dmb");
    asm("dsb");
    asm("isb");
	asm("mcr p15, 0, %0, c7, c10, 5" :: "r"(0));
}

// Set the state of the mmu
void set_mmu(int enable)
{
	unsigned long ctl;

	asm("mrc p15, 0, %0, c1, c0, 0" : "=r"(ctl));
	if(enable)
		ctl |= 1;
	else
		ctl &= ~(1);
	asm("mcr p15, 0, %0, c1, c0, 0" ::"r"(ctl));
}

// Set the state of the data cache
void set_dcache(int enable)
{
	unsigned long ctl;

	asm("mrc p15, 0, %0, c1, c0, 0" : "=r"(ctl));
	if(enable)
		ctl |= 1 << 2;
	else
		ctl &= ~(1 << 2);
	asm("mcr p15, 0, %0, c1, c0, 0" ::"r"(ctl));
}

// Set the state of the instruction cache
void set_icache(int enable)
{
	unsigned long ctl;

	asm("mrc p15, 0, %0, c1, c0, 0" : "=r"(ctl));
	if(enable)
		ctl |= 1 << 12;
	else
		ctl &= ~(1 << 12);
	asm("mcr p15, 0, %0, c1, c0, 0" ::"r"(ctl));
}

void set_l2(int enable)
{
	unsigned long ctl;
	asm("mrc p15, 0, %0, c1, c0, 1" : "=r"(ctl));
	if(enable)
		ctl |= 1 << 1;
	else
		ctl &= ~(1 << 1);
	asm("mcr p15, 0, %0, c1, c0, 1" ::"r"(ctl));
}

void set_branch_predictor(int enable)
{
	unsigned long ctl;
	asm("mrc p15, 0, %0, c1, c0, 0" : "=r"(ctl));
	if(enable)
		ctl |= 1 << 11;
	else
		ctl &= ~(1 << 11);
	asm("mcr p15, 0, %0, c1, c0, 0" ::"r"(ctl));
}

void invalidate_l1()
{
	asm("mcr p15, 0, %0, c7, c5, 0" ::"r"(0));
}

void invalidate_branch_predictor()
{
	asm("mcr p15, 0, %0, c7, c5, 6" ::"r"(0));
}

void flush_prefetch_buffer()
{
	asm("mcr p15, 0, %0, c7, c5, 4" ::"r"(0));
}

void clean_invalidate_l1()
{
	int way, set;

	// set_mmu(0);
	// set_icache(0);
	// set_dcache(0);
	for(set = 0; set <= 0x7F; ++set)	// 32kB L2 cache
		for(way = 0; way < 4; ++way)	// 4-way associative
			asm("mcr p15, 0, %0, c7, c14, 2" ::"r"((way<<30) | (set<<6) ));
}

void clean_invalidate_l2()
{
	int way, set;

	// set_mmu(0);
	// set_l2(0);
	for(set = 0; set <= 0x1FF; ++set)	// 256kB L2 cache
		for(way = 0; way < 8; ++way)	// 8-way associative
			asm("mcr p15, 0, %0, c7, c14, 2" ::"r"((way<<29) | (set<<6) | (1<<1)));
}

// Clear I-cache, D-cache, branch predictor, prefetch buffer
void clear_caches()
{
	data_memory_barrier();

	set_branch_predictor(0);
	invalidate_l1();
	clean_invalidate_l1();
	clean_invalidate_l2();
	invalidate_branch_predictor();
	flush_prefetch_buffer();
    asm("mcr p15, 0, %0, c8, c7, 0" :: "r"(0));

	data_memory_barrier();
}

// Set up page tables
void set_up_mmu()
{
	unsigned long *base = 0x8F000000;
	unsigned long *ptr;
	int i;

	asm("mcr p15, 0, %0, c2, c0, 0" :: "r"(base));
	asm("mcr p15, 0, %0, c2, c0, 1" :: "r"(base));

	for(ptr = (unsigned long*)0x8F000000, i=0; ptr < 0x8F004000; ++ptr, ++i)
    {
        if(i >= 2048 && i < 2304)   // Only DRAM (0x80000000) should be cacheable
    		(*ptr) = (i << 20) | (3 << 10) | (0x3 << 3) | 0x2;
        else
    		(*ptr) = (i << 20) | (3 << 10) |   0x2;
    }


	// Set domain access control register
	asm("mcr p15, 0, %0, c3, c0, 0" : : "r" (~0));

    data_memory_barrier();

    // Invalidate both TLBs
    asm("mcr p15, 0, %0, c8, c7, 0" :: "r"(0));
}

void software_init_hook()
{
    unsigned long ctl;

    // Clean the caches
    clean_invalidate_l1(); 
    clean_invalidate_l2(); 
 
    // Turn off everything
    clear_caches(); 
    set_dcache(0); 
    set_icache(0); 
    set_l2(0); 
    set_branch_predictor(0); 
    set_mmu(0); 
    clear_caches(); 
    set_up_mmu(); 
    set_mmu(MMU_STATE); 
    clear_caches(); 

    // Have to call ROM, to set L2AUXCR 
    asm("push {r0-r12,r14}"); 
    asm("mrc p15, 0, r0, c1, c0, 1"); 
    asm("mov r0, #0x20"); 

    asm("mov r12, #0x100"); 
    asm("MCR p15,#0x0,r1,c7,c5,#6"); 
    asm("dsb"); 
    asm("isb"); 
    asm("dmb"); 
    asm("smc #1"); 
 
    asm("mrc p15, 1, r0, c9, c0, 2"); 
    asm("mvn r1, #0x0BC00000"); 
    asm("and r0, r0, r1"); 
    asm("ldr r12, =0x102"); 
    asm("MCR p15,#0x0,r1,c7,c5,#6"); 
    asm("dsb"); 
    asm("isb"); 
    asm("dmb"); 
    asm("smc #1"); 
    asm("pop {r0-r12,r14}"); 
    set_l2(L2_STATE); 
    set_icache(ICACHE_STATE); 
    set_branch_predictor(BP_STATE); 
    set_dcache(DCACHE_STATE); 

}

