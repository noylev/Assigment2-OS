#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "signal.h"

extern void sigret_start(void);
extern void sigret_end(void);

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}
/***************************to do change name of function below************/
char* getState (enum procstate state){
	switch (state) 
   {
      case UNUSED: return "UNUSED";
      case EMBRYO: return "EMBRYO";
      case SLEEPING: return "SLEEPING";
      case RUNNABLE: return "RUNNABLE";
      case RUNNING: return "RUNNING";
      case ZOMBIE: return "ZOMBIE";
      case MINUS_UNUSED: return "MINUS_UNUSED";
      case MINUS_EMBRYO: return "MINUS_EMBRYO";
      case MINUS_SLEEPING: return "MINUS_SLEEPING";
      case MINUS_RUNNABLE: return "MINUS_RUNNABLE";
      case MINUS_RUNNING: return "MINUS_RUNNING";
      case MINUS_ZOMBIE: return "MINUS_ZOMBIE";
      default: return "";       
   }
}
/***************************to do change name of function below************/
// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;

  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli(); // disable interrupts to avoid deadlock.
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}


int
allocpid(void)
{
  int pid;
  pushcli();
  pid = nextpid;
  while (!cas(&nextpid, pid, pid + 1)){
     pid = nextpid;
};
  popcli();
  return pid;
}

struct proc* find_unused_entry(){
    struct proc *p = 0;
    for (p = ptable.proc; p<&ptable.proc[NPROC]; p++){
      if(p->state == UNUSED)
        return p;
    }
return 0;

}


//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;
 // pushcli();
 // p = find_unused_entry(); //trying to find an UNUSED entry in the process table
  //if(p == 0) //if it didn't find it returns NULL and we want to exit
   // return 0;
  //else
   // cas(&p->state, UNUSED, EMBRYO);// else, for p UNUSED perform cas form UNUSED to EMBRYO
  //popcli();
  /*********************need to wtich with code above*/////////////
  pushcli();
  do {
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
      if(p->state == UNUSED)
        break;
    if (p == &ptable.proc[NPROC]) {
      popcli();
      return 0; // ptable is full
    }
  } while (!cas(&p->state, UNUSED, EMBRYO));
  popcli();
/*********************need to wtich with code above*/////////////
  p->pid = allocpid();

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  //added here to 3.2 - for new process we add signal's handler
  for(int i = 0; i < 32; i++){
    p->signal_handler[i] = 0;
  }
  //added here to 3.2 - for new process we add signal's handler

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // Init signal/masking data.
  for(int index = 0; index < 32; ++index) {
    p->signal_handler[index] =(void*) SIGDFL;
  }
  p->pending_signals = 0;
  p->signal_mask = 0;

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  
  
  //acquire(&ptable.lock);
  pushcli();// task 4.1
  if(!cas (&p->state , EMBRYO , RUNNABLE))
      panic("cas from embryo to runnable");
  popcli(); // task 4.1
  //release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();
  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Copy signal masks and handers.
  // Reset signals.
  np->pending_signals = 0;
  np->signal_mask = curproc->signal_mask;
  for(i = 0; i < 32; i++){
    np->signal_handler[i] = curproc->signal_handler[i];
  }

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));



  pid = np->pid;

  //acquire(&ptable.lock);
  pushcli();// task 4.1
  if (!cas(&np->state, EMBRYO, RUNNABLE))
      panic("cas error in fok from embryo to runnable");
  popcli(); // task 4.1
  //release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  //acquire(&ptable.lock);
  pushcli();   
  if(!cas(&curproc->state, RUNNING, MINUS_ZOMBIE))
    panic("cas failed in exit function");
  
  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  //curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  pushcli();
  //acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(cas(&p->state, ZOMBIE, MINUS_UNUSED)){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
       // p->state = UNUSED;
       // cas(&p->state, MINUS_SLEEPING, RUNNING);
        //release(&ptable.lock);
        cas(&p->state, MINUS_UNUSED, UNUSED);
        popcli();
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      //release(&ptable.lock);
        if (!cas(&p->state, MINUS_SLEEPING, RUNNING))
            panic("MINUS_SLEEPING to RUNNING failed in wait function");
        popcli();
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    //acquire(&ptable.lock);
    pushcli(); //task 4 state RUNNABLE to RUNNING
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(!cas(&p->state, RUNNABLE, RUNNING)) {
            continue;
      }
        cas(&p->state, MINUS_RUNNABLE, RUNNABLE);
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
      cas(&p->state, MINUS_RUNNABLE, RUNNABLE);
      if (cas(&p->state, MINUS_ZOMBIE, ZOMBIE)){
           wakeup1(p->parent);
      }
      
      if (cas(&p->state, MINUS_SLEEPING, SLEEPING)) {
        if(p->killed == 1){
          p->state = RUNNABLE;
        }
      }
    

    }
    //release(&ptable.lock);
    popcli();

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

 /// if(!holding(&ptable.lock))
  //  panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  //acquire(&ptable.lock);  //DOC: yieldlock
  //myproc()->state = RUNNABLE;
  pushcli();
  if (!cas(&myproc()->state, RUNNING, MINUS_RUNNABLE))
    panic("yield- cas failed");
  sched();
  //release(&ptable.lock);
  popcli();
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  //release(&ptable.lock);
  popcli();
  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  if(p == 0)
    panic("sleep");
  if(lk == 0)
    panic("sleep without lk");
  
  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  
  p->chan = chan;
  // parent need to wait so enter to minus sleeping
   if(!cas(&p->state, RUNNING, MINUS_SLEEPING))
       panic("cas failed in sleep from running to minus sleeping");
    
  if(lk != &ptable.lock){  //DOC: sleeplock0
  //  acquire(&ptable.lock);  //DOC: sleeplock1
     pushcli();
     release(lk);
  }
  // Go to sleep.
  
  //p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
   // release(&ptable.lock);
      popcli();
    acquire(lk);
  }
  
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
	  /***************************change shid***************************************************************/
    if(p->chan == chan && (p->state == SLEEPING || p->state == MINUS_SLEEPING) ){
    	if (p->state == MINUS_SLEEPING){
    		while (p->state != SLEEPING); //busy-wait
    	} 
      if (cas(&p->state ,SLEEPING, MINUS_RUNNABLE)) {
      	p->chan = 0 ;
      	if(!cas(&p->state , MINUS_RUNNABLE , RUNNABLE))
            panic("faild cas in wakeup1");
    
    }  /***************************change shid***************************************************************/
  }
 }
   
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  //acquire(&ptable.lock);
  pushcli();// task 4.1
  wakeup1(chan);
  popcli();// task 4.1
  //release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid, int signum)
{
  struct proc *p;

  //acquire(&ptable.lock);
  //pushcli();// task 4.1
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->pid == pid){

      if (p->state == ZOMBIE || signum > 31 || signum < 0) {
        /// Process died or signum out of bounderies.
        //release(&ptable.lock);
        popcli();// task 4.1
        return -1;
      }

      // Set the correct signal bit.
      BIT_SET(p->pending_signals, signum);
      cas (&p->state , SLEEPING , RUNNABLE);
      //release(&ptable.lock);
     // popcli();// task 4.1
      return 0;
    }
  }

  // PID not found.
  //popcli();// task 4.1
  //release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
 static char *states[] = {
  [UNUSED]    "unused",
  [MINUS_UNUSED] "-unused", 
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [MINUS_SLEEPING] "-sleeping",
  [RUNNABLE]  "runble",
  [MINUS_RUNNABLE] "-runnable",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie",
  [MINUS_ZOMBIE] "-zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

// Changes signal mask.
uint sigprocmask (uint signal_mask) {
  struct proc * current_proc = myproc();
  uint old_signal_mask = current_proc->signal_mask;
  current_proc->signal_mask = signal_mask;
  return old_signal_mask;
}

// Updates a specific signal handler.
sighandler_t signal(int signal_number, sighandler_t handler) {
  if(0 > signal_number || signal_number > 31) {
    return (void*) -2;
  }
  struct proc *curproc = myproc();
  sighandler_t old_sigh = curproc->signal_handler[signal_number];
  curproc->signal_handler[signal_number] = handler;
  return old_sigh;
}

// =============== Kernel Signal handlers ====================
// SIGKILL - kill process.
void signal_handler_kill() {
  struct proc *current_process = myproc();
  current_process->killed = 1;
  return;
}

// SIGSTOP -- stop process.
void signal_handler_stop() {
  struct proc *current_process = myproc();

  while(BIT_READ (current_process->pending_signals, SIGCONT) == 0) {
    yield();
  }

  return;
}

// SIGCONT -- continue process.
void signal_handler_continue() {
  struct proc *current_process = myproc();

  if (BIT_READ(current_process->pending_signals, SIGSTOP) == 1) {
    BIT_SET(current_process->pending_signals, SIGCONT);
  }

  return;
}


//================================================
void check_for_signal(struct trapframe *tf) {
  struct proc *current_process = myproc();

  if(!((tf->cs&3) == DPL_USER)){
    return;
  }

  if(current_process->pending_signals == 0) {
    // No pending signals.
    return;
  }

  uint * masks = &current_process->signal_mask;
  uint * pending = &current_process->pending_signals;

  for (int bit_index = 0; bit_index < 32; bit_index++) {
    if (BIT_READ(*masks, bit_index) == 1) {
      // Masked bit. Skip.
      continue;
    }
    if(BIT_READ(*pending, bit_index) == 0) {
      continue;
    }
    if(current_process->signal_handler[bit_index] == (void*) SIGIGN){
      BIT_CLEAR(*pending, bit_index);
      return;
    }
    if (current_process->signal_handler[bit_index] == (void*) SIGDFL){
      if (bit_index == 17) {
        signal_handler_stop();
      }
      else if (bit_index == 19) {
        signal_handler_continue();
      }
      else {
        // Default action - kill.
        signal_handler_kill();
      }
      // Clear bit.
      BIT_CLEAR(*pending, bit_index);
      return;
    }
    else {
        uint stack_pointer = tf->esp;
        stack_pointer -= sizeof(struct trapframe);
        current_process->tf_backup = (struct trapframe *) stack_pointer;
        memmove(current_process->tf_backup, tf, sizeof(struct trapframe));
        uint function_size = sigret_start - sigret_end;
        stack_pointer -= function_size;
        uint sigret_address = stack_pointer;
        memmove((void*) stack_pointer, sigret_start, function_size);
        stack_pointer -= 4;
        *(int *) stack_pointer = bit_index;
        stack_pointer -= 4;
        *(uint *) stack_pointer = sigret_address;
        tf->esp = stack_pointer;
        tf->eip = (uint) current_process->signal_handler[bit_index];
        BIT_CLEAR(current_process->pending_signals, bit_index);
        return;
    }
  }
}

// restore frame or something?
void sigret(void) {
  tf_restore();
}

// Restores backed-up trapframe.
void tf_restore(void) {
  struct proc * current_proc = myproc();
  memmove(current_proc->tf, current_proc->tf_backup, sizeof(struct trapframe));
}
