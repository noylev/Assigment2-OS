#include "types.h"
#include "stat.h"
#include "user.h"

void print_sent_signal(int);
void father_part(int);
void son_part();

int main(int argc, char **argv) {
  int pid;
  for (int index = 0; index < 3 ; index++) {
    if ((pid = fork()) != 0) {
      father_part(pid);
      // Wait for sons to die.
      wait();
    }
    else {
      son_part();
    }
  }
  exit();
}

void father_part(int son_pid) {
  int index = 0;
  int father_pid = getpid();
  printf(2, "Father is %d, son is %d.\n", father_pid, son_pid);
  int signals_to_test[5] = {2,17,3,4,19};
  for (index = 0; index < 5; index++) {
    printf(2, "Sending signal %d to son %d.", signals_to_test[index], son_pid);
    kill(son_pid, signals_to_test[index]);
    sleep(50);
  }

  // Let the things run for a bit.
  for (index = 0; index < 10; index++) {
    printf(2, "Father running.");
    sleep(50);
  }
}

void son_part() {
  int pid = getpid();
  printf(2, "Son %d setting signal hanlders.", pid);
  signal(2, (sighandler_t) print_sent_signal);
  signal(3, (sighandler_t) print_sent_signal);
  signal(4, (sighandler_t) print_sent_signal);
    // Let the things run for a bit.
    for (int index = 0; index < 10; index++) {
      printf(2, "Son %d running.", pid);
      sleep(50);
    }
}

void print_sent_signal(int signum) {
  printf(2, "pid: %d, got signal %d !\n", getpid(), signum);
}
