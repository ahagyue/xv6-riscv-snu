#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

volatile static int started = 0;

// start() jumps here in supervisor mode on all CPUs.
void
main()
{
  if(cpuid() == 0){
    consoleinit();
    printfinit();
    printf("\n");
    printf("xv6 kernel is booting\n");
    printf("\n");

    // pa1 start
    printf("SNUOS2024\n");
    
    int i = 1;
    char student_id[11] = "2020-17164";
    char name[11] = "KangMingyu";
    for(; i <= 10; i++) {
      // write partial student id and name
      printf("%s", student_id + 10-i);
      printf("\n");
      printf("%s",  name + 10-i);
      printf("\n");
      
      // wait for a short time
      int k =0;
      while(k!=200000000) k++;
      
      // erase two line
      if (i != 10) {
        printf("\033[1A");
        printf("\033[K");
        printf("\033[1A");
        printf("\033[K");
        printf("\033[0G");
      }
    }
    printf("\n");
    // pa1 end

    kinit();         // physical page allocator
    kvminit();       // create kernel page table
    kvminithart();   // turn on paging
    procinit();      // process table
    trapinit();      // trap vectors
    trapinithart();  // install kernel trap vector
    plicinit();      // set up interrupt controller
    plicinithart();  // ask PLIC for device interrupts
    binit();         // buffer cache
    iinit();         // inode table
    fileinit();      // file table
    virtio_disk_init(); // emulated hard disk
    userinit();      // first user process
    __sync_synchronize();
    started = 1;
  } else {
    while(started == 0)
      ;
    __sync_synchronize();
    printf("hart %d starting\n", cpuid());
    kvminithart();    // turn on paging
    trapinithart();   // install kernel trap vector
    plicinithart();   // ask PLIC for device interrupts
  }

  scheduler();        
}
