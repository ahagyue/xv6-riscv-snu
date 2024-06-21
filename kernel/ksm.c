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

#include "ksm.h"

extern struct proc proc[NPROC];
extern uint64 xxh64(void*, unsigned int); // hash function

uint64
sys_ksm(void)
{ 

  uint64 scanned, merged;
  argaddr(0, &scanned);
  argaddr(1, &merged);

  init_val(scanned);
  init_val(merged);

  intr_off();
  int nmetadata = 0;

  struct proc *p;
  uint64 va;
  pte_t *pte;

  // scan processes
  for(p = proc; p < &proc[NPROC]; p++) {
    if (p->pid == 1 || p->pid == 2 || p->pid == myproc()->pid) continue;
    if (p->state == UNUSED) continue;

    //printf("%d proccess\n", p->pid);
    
    // scan vm
    for(va = 0; va < p->sz; va += PGSIZE) {
      if ((pte = walk(p->pagetable, va, 0)) == 0) continue;
      update_val(scanned, 1);

      uint64 pa = PTE2PA(*pte);
      uint64 hash = xxh64((void*)pa, PGSIZE);

      // zeropage
      if (hash == zerohash) {
        if (zeropage == (pa_t)PTE2PA(*pte)) continue;
        pte_t tmp = MRG_PTE(zeropage, *pte);
        //printf("%d %p\n", (*pte)&4, tmp);
        uvmunmap(p->pagetable, va, 1, 1);
        *pte = tmp;
        update_val(merged, 1);
      } 
      
      // non zeropage
      else {
        int i = 0;
        for(; i < nmetadata; i++) {
          if (metadata[i].hash == hash) break;
        }

        if (i < nmetadata) {
          metadata[i].pte[metadata[i].nshared++] = pte;
        } else {
          metadata[i].hash = hash;
          metadata[i].nshared = 1;
          metadata[i].pte[0] = pte;
          nmetadata++;
        }
      }
    }
  }

  // merge
  //printf("%d nmetadata\n", nmetadata);
  while (nmetadata--) {
    struct ksmmetadata data = metadata[nmetadata-1];
    //printf("%d page merge\n", data.nshared);
    if (data.nshared < 2) continue;

    pa_t pas[16] = {0,};
    int cnt[16] = {0,};
    pa_t most = 0;

    int nscanned = 0;
    for(int i = 0; i < data.nshared; i++) {
      pa_t pa = (pa_t)PTE2PA(*data.pte[i]);
      int j=0;
      for(; j < nscanned; j++) {
        if (pa == pas[j]) {
          cnt[j]++;
          break;
        }
      }

      if (j >= nscanned) {
        pas[nscanned] = pa;
        cnt[nscanned] = 1;
        nscanned++;
      }
    }

    int max = -1;
    for(int i = 0; i < nscanned; i++) {
      if (max < cnt[i]) {
        most = pas[i];
        max = cnt[i];
      }
    }
    //printf("%p\n", most);
    
    for(int i = 0; i < data.nshared; i++) {
      pa_t pa = (pa_t)PTE2PA(*data.pte[i]);
      if (pa == most) continue;
      else {
        pte_t tmp = MRG_PTE(most, *data.pte[i]);

        int count;
        if ((count = get_count((pa_t)pa)) == 1)
          kfree((void*)pa);
        else if (count > 0) {
          set_count((pa_t)pa, count - 1);
        }
        set_count(most, get_count(most) + 1);

        *data.pte[i] = tmp;
      }
    }
    update_val(merged, data.nshared - max);
  }
  

  intr_on();

  return freemem;
}

int get_count(pa_t pa) {
  int idx = ((uint64)pa & 0x7fff000) >> 12;
  return idx & 1 ? merge_cnt[idx >> 1] & 0xf : (merge_cnt[idx >> 1] & 0xf0) >> 4;
}

void set_count(pa_t pa, int cnt) {
  if (cnt < 0 || cnt >= 16) panic("merge count");

  int idx = ((uint64)pa & 0x7fff000) >> 12;
  if (idx & 1) {
    merge_cnt[idx >> 1] &= 0xf0;
    merge_cnt[idx >> 1] += cnt;
  } else {
    merge_cnt[idx >> 1] &= 0x0f;
    merge_cnt[idx >> 1] += cnt << 4;
  }
}

void cow(pte_t *pte) {
  intr_on();
  pa_t pa = (pa_t)PA2PTE(*pte);
  int cnt;
  if ((cnt = get_count(pa)) == 1) {
    *pte = (*pte & (~PTE_COW)) | PTE_W;
    return;
  } else if (cnt > 1) {
    set_count(pa, get_count(pa)-1);
  }

  pa_t new_page = (pa_t)kalloc();
  memmove(new_page, pa, PGSIZE);
  *pte = (PA2PTE(new_page)) | PTE_FLAGS(*pte) | PTE_W;
  intr_off();
}

void init_val(uint64 va) {
  pte_t *pte;
  if ((pte = walk(myproc()->pagetable, va, 0)) == 0) panic("ksm arg");
  pa_t pa = (pa_t)(((char*)PTE2PA(*pte)) + (va & 0xfff));
  *(int*)pa = 0;
}

void update_val(uint64 va, int val) {
  pte_t *pte;
  if ((pte = walk(myproc()->pagetable, va, 0)) == 0) panic("ksm arg");
  pa_t pa = (pa_t)(((char*)PTE2PA(*pte)) + (va & 0xfff));
  *(int*)pa += val;
}