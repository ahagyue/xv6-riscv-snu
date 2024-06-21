//----------------------------------------------------------------
//
//  4190.307 Operating Systems (Spring 2024)
//
//  Project #4: KSM (Kernel Samepage Merging)
//
//  May 7, 2024
//
//  Dept. of Computer Science and Engineering
//  Seoul National University
//
//----------------------------------------------------------------

#ifdef SNU
#include "types.h"
#include "riscv.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "defs.h"
#include "proc.h"

#define PTE_COW (1 << 8)
#define IS_COW(pte) (pte & PTE_COW)
#define MRG_PTE(pageaddr, pte) (PA2PTE(pageaddr) | ((pte & 4) << 6) | (pte & 0x3fb))

extern int freemem;

pa_t zeropage;
uint64 zerohash;

uint8 merge_cnt[4096 * 4];

struct ksmmetadata {
    uint64 hash;
    pte_t *pte[16];
    int nshared;
} metadata [4096 * 10];

void init_val(uint64 va);
void update_val(uint64 va, int val);

#endif
