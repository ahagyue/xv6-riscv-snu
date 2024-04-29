#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

#ifdef SNU
/* Do not touch sys_time() */
uint64 
sys_time(void)
{
  uint64 x;

  asm volatile("rdtime %0" : "=r" (x));
  return x;
}
/* Do not touch sys_time() */

uint64
sys_sched_setattr(void)
{
  int pid, runtime, period;
  argint(0, &pid);
  argint(1, &runtime);
  argint(2, &period);

  if (!pid) pid = myproc()->pid;
  if (pid < 0 || runtime < 0 || period < 0 || runtime >= period) return -1;
  if (!runtime || !period) return 0;

  for (struct rt_proc *rtp = rt_proc; rtp < rt_proc + n_rt_proc; rtp++) {
    if (rtp->proc->pid == pid) return -1;
  }

  struct proc *p = 0;
  for(p = proc; p < &proc[NPROC]; p++) {
    if (p->pid == pid) {
      acquire(&p->lock);
      rt_proc[n_rt_proc++] = (struct rt_proc){p, runtime, period, ticks, 0};
      release(&p->lock);
      return 0;
    }
  }
  return -1;
}

uint64
sys_sched_yield(void)
{
  // printf("yield %d\n", myproc()->pid);
  for(struct rt_proc *rtp = rt_proc; rtp < rt_proc+n_rt_proc; rtp++) {
    if (rtp->proc == myproc()) {
      acquire(&myproc()->lock);
      rtp->finished = 1;
      release(&myproc()->lock);
    }
    // printf("finished: %d\n", rtp->finished);
  }
  yield();
  return 0;
}
#endif
