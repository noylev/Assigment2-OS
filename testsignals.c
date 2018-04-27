#include "types.h"
#include "stat.h"
#include "user.h"

void sig_handler(int);

int
main(int argc, char **argv)
{
  int pid;
 for(int j=0; j<3 ;j++){
  if ((pid=fork()) != 0) {
  	printf(2, "FATHER_%d: MY SON PID: %d\n",getpid(), pid);
  	sleep(200);
	  printf(2, "FATHER_%d: SENDING SIGNAL 2 TO MY SON %d\n",getpid(), kill(pid, 2));
    sleep(50);
    printf(2, "FATHER_%d: SENDING S-T-O-P SIGNAL TO MY SON %d\n",getpid(), kill(pid, 17));
    sleep(50);
    printf(2, "FATHER_%d: SENDING SIGNAL 3 TO MY SON %d\n",getpid(), kill(pid, 3));
    printf(2, "FATHER_%d: SENDING SIGNAL 4 TO MY SON %d\n",getpid(), kill(pid, 4));
    printf(2, "FATHER_%d: SENDING CONT SIGNAL TO MY SON- SON SHOULD CONTINUE %d\n",getpid(), kill(pid, 19));
  	//wait();
    for(int i=0 ; i < 10 ; i++) {
      sleep(50);
      printf(2, "F_%d: RUNNING....\n", getpid());
    }
    wait();
    //
  	//printf(2, "%d_DONE\n", getpid());
  }
  else {
    printf(2, "SON_%d: setting signal handler %d\n", getpid(), signal(2, (sighandler_t)sig_handler));
    printf(2, "SON_%d: setting signal handler %d\n", getpid(), signal(3, (sighandler_t)sig_handler));
    printf(2, "SON_%d: setting signal handler %d\n", getpid(), signal(4, (sighandler_t)sig_handler));
  	printf(2, "SON_%d: MY ID IS %d\n", getpid(), getpid());

  	for(int i=0 ; i < 10 ; i++) {
  		sleep(50);
  		printf(2, "SON_%d: RUNNING....\n", getpid());
  	}
  }
  }
  exit();
}

void sig_handler(int signum){
  printf(2, "PID: %d, I GOT SIGNAL %d \n", getpid(), signum);
  return;
}
