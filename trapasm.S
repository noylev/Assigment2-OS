#include "mmu.h"
#include "syscall.h"
#include "traps.h"

.extern check_for_signal
.globl alltraps
.globl sigret_start
.globl sigret_end
.globl trapret

# vectors.S sends all traps here.
alltraps:
  # Build trap frame.
  pushl %ds
  pushl %es
  pushl %fs
  pushl %gs
  pushal

  # Set up data segments.
  movw $(SEG_KDATA<<3), %ax
  movw %ax, %ds
  movw %ax, %es

  # Call trap(tf), where tf=%esp
  pushl %esp
  call trap
  addl $4, %esp

  # Return falls through to trapret...
trapret:
  # Push stack pointer
  pushl %esp
  call check_for_signal
  # Fix stack pointer.
  addl $4, %esp

finish:
  popal
  popl %gs
  popl %fs
  popl %es
  popl %ds
  addl $0x8, %esp  # trapno and errcode
  iret

sigret_start:
  movl $SYS_sigret, %eax
  int $T_SYSCALL
sigret_end:
  movl $0, %eax
