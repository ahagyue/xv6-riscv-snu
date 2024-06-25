#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"

#define BUFSZ  ((MAXOPBLOCKS+2)*BSIZE)

char buf[BUFSZ];

void
fourfiles(char *s)
{
  int fd, pid, i, j, n, total, pi;
  char *names[] = { "f0", "f1", "f2", "f3" };
  char *fname;
  enum { N=12, NCHILD=4, SZ=500 };
  
  for(pi = 0; pi < NCHILD; pi++){
    fname = names[pi];
    unlink(fname);

    pid = fork();
    if(pid < 0){
      printf("fork failed\n", s);
      exit(1);
    }

    if(pid == 0){
      fd = open(fname, O_CREATE | O_RDWR);
      printf("%d : %d\n", fd, pi);
      if(fd < 0){
        printf("create failed\n", s);
        exit(1);
      }

      memset(buf, '0'+pi, SZ);
      for(i = 0; i < N; i++){
        if((n = write(fd, buf, SZ)) != SZ){
          printf("%d write failed %d\n", fd, n);
          exit(1);
        }
      }
      exit(0);
    }
  }

  int xstatus;
  for(pi = 0; pi < NCHILD; pi++){
    wait(&xstatus);
    if(xstatus != 0)
      exit(xstatus);
  }

  for(i = 0; i < NCHILD; i++){
    fname = names[i];
    fd = open(fname, 0);
    total = 0;
    while((n = read(fd, buf, sizeof(buf))) > 0){
      for(j = 0; j < n; j++){
        if(buf[j] != '0'+i){
          printf("wrong char\n", s);
          exit(1);
        }
      }
      total += n;
    }
    close(fd);
    if(total != N*SZ){
      printf("wrong length %d\n", total);
      exit(1);
    }
    unlink(fname);
  }
}

int main() {
    fourfiles("fourfile");
    // sync();
    return 0;
}
