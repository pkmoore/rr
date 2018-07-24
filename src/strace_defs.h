extern "C" {

#define umove(pid, addr, objp)	\
	umoven((pid), (addr), sizeof(*(objp)), (void *) (objp))

#ifndef MAX_ARGS
# ifdef LINUX_MIPSO32
#  define MAX_ARGS	7
# else
#  define MAX_ARGS	6
# endif
#endif

# include <asm/posix_types.h>

#define SUPPORTED_PERSONALITIES 1

#ifndef STRACE_SYSENT_H
#define STRACE_SYSENT_H

typedef struct sysent {
	unsigned nargs;
	int	sys_flags;
	int	sen;
	int	(*sys_func)();
	const char *sys_name;
} struct_sysent;

#define TRACE_FILE			00000001	/* Trace file-related syscalls. */
#define TRACE_IPC			00000002	/* Trace IPC-related syscalls. */
#define TRACE_NETWORK			00000004	/* Trace network-related syscalls. */
#define TRACE_PROCESS			00000010	/* Trace process-related syscalls. */
#define TRACE_SIGNAL			00000020	/* Trace signal-related syscalls. */
#define TRACE_DESC			00000040	/* Trace file descriptor-related syscalls. */
#define TRACE_MEMORY			00000100	/* Trace memory mapping-related syscalls. */
#define SYSCALL_NEVER_FAILS		00000200	/* Syscall is always successful. */
#define MEMORY_MAPPING_CHANGE		00000400	/* Trigger proc/maps cache updating */
#define STACKTRACE_CAPTURE_ON_ENTER	00001000	/* Capture stacktrace on "entering" stage */
#define TRACE_INDIRECT_SUBCALL		00002000	/* Syscall is an indirect socket/ipc subcall. */
#define COMPAT_SYSCALL_TYPES		00004000	/* A compat syscall that uses compat types. */
#define TRACE_STAT			00010000	/* Trace {,*_}{,old}{,x}stat{,64} syscalls. */
#define TRACE_LSTAT			00020000	/* Trace *lstat* syscalls. */
#define TRACE_STATFS			00040000	/* Trace statfs, statfs64, and statvfs syscalls. */
#define TRACE_FSTATFS			00100000	/* Trace fstatfs, fstatfs64 and fstatvfs syscalls. */
#define TRACE_STATFS_LIKE		00200000	/* Trace statfs-like, fstatfs-like and ustat syscalls. */
#define TRACE_FSTAT			00400000	/* Trace *fstat{,at}{,64} syscalls. */
#define TRACE_STAT_LIKE			01000000	/* Trace *{,l,f}stat{,x,at}{,64} syscalls. */
#define TRACE_PURE			02000000	/* Trace getter syscalls with no arguments. */

#endif /* !STRACE_SYSENT_H */
typedef __kernel_long_t kernel_long_t;
typedef __kernel_ulong_t kernel_ulong_t;

struct tcb {
    int flags;		/* See below for TCB_ values */
    int pid;		/* If 0, this tcb is free */
    int qual_flg;		/* qual_flags[scno] or DEFAULT_QUAL_FLAGS + RAW */
    unsigned long u_error;	/* Error code */
    kernel_ulong_t scno;	/* System call number */
    kernel_ulong_t u_arg[MAX_ARGS];	/* System call arguments */
    kernel_long_t u_rval;	/* Return value */
#if SUPPORTED_PERSONALITIES > 1
    unsigned int currpers;	/* Personality at the time of scno update */
#endif
    int sys_func_rval;	/* Syscall entry parser's return value */
    int curcol;		/* Output column for this process */
    FILE *outf;		/* Output file for this process */
    const char *auxstr;	/* Auxiliary info from syscall (see RVAL_STR) */
    void *_priv_data;	/* Private data for syscall decoding functions */
    void (*_free_priv_data)(void *); /* Callback for freeing priv_data */
    const struct_sysent *s_ent; /* sysent[scno] or dummy struct for bad scno */
    const struct_sysent *s_prev_ent; /* for "resuming interrupted SYSCALL" msg */
    struct inject_opts *inject_vec[SUPPORTED_PERSONALITIES];
    struct timespec stime;	/* System time usage as of last process wait */
    struct timespec dtime;	/* Delta for system time usage */
    struct timespec etime;	/* Syscall entry time */
    struct timespec delay_expiration_time; /* When does the delay end */

    struct mmap_cache_t *mmap_cache;
};

extern void qualify(const char *);
extern void clear_regs(struct tcb *tcp);
extern int syscall_entering_decode(struct tcb *);
extern int syscall_entering_trace(struct tcb *, unsigned int *);
extern void syscall_entering_finish(struct tcb *, int);

extern int syscall_exiting_decode(struct tcb *, struct timespec *);
extern int syscall_exiting_trace(struct tcb *, struct timespec *, int);
extern void syscall_exiting_finish(struct tcb *);

extern struct tcb *printing_tcp;

int umoven(struct tcb *, kernel_ulong_t addr, unsigned int len, void *laddr);
int umovestr(struct tcb *, kernel_ulong_t addr, unsigned int len, char *laddr);
void decode_socket_subcall(struct tcb *tcp);

}
