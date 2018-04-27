#include "types.h"
#include "stat.h"
#include "user.h"

void print_sent_signal(int);

int main(int argc, char **argv) {
  int pid;
  for (int j=0; j<3 ;j++) {
    if ((pid = fork()) != 0) {
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
    }
    else {
      printf(2, "SON_%d: setting signal handler %d\n", getpid(), signal(2, (sighandler_t)print_sent_signal));
      printf(2, "SON_%d: setting signal handler %d\n", getpid(), signal(3, (sighandler_t)print_sent_signal));
      printf(2, "SON_%d: setting signal handler %d\n", getpid(), signal(4, (sighandler_t)print_sent_signal));
    	printf(2, "SON_%d: MY ID IS %d\n", getpid(), getpid());

    	for (int i=0 ; i < 10 ; i++) {
    		sleep(50);
    		printf(2, "SON_%d: RUNNING....\n", getpid());
    	}
    }
  }
  exit();
}

void father_part(int son_pid) {
  int index = 0;
  int father_pid = getpid();
  printf(2, "Father is %d, son is %d.\n", father_pid, son_pid);
  int signals_to_test[5] = {2,17,3,4,19}
  for (index = 0; index < 5; index++) {
    printf(2, "Sending signal %d to son %d.", signals_to_test[index], son_pid);
    kill(son_pid, signals_to_test[index]);
    sleep(50);
  }

  // Let the things run for a bit.
  for (index = 0; index < 10; index++) {
    printf("Father running.");
    sleep(50);
  }
}

void son_part() {
  
}

void print_sent_signal(int signum) {
  printf(2, "pid: %d, got signal %d !\n", getpid(), signum);
}
