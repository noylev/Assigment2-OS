// Default signal handling.
#define SIGDFL -1
// Ignore signal.
#define SIGIGN 1
#define SIGKILL 9
#define SIGSTOP 17
#define SIGCONT 19

#define BIT_READ(array, index) ((0u == (array & (1<<index))) ? 0u : 1u)
#define BIT_SET(array, index) (array |= 1UL << index)
#define BIT_CLEAR(array, index) (array &= ~(1UL << index))

typedef void (*sighandler_t)(int);
