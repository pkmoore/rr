#include <stdio.h>
#include "defs.h"

extern struct tcb *current_tcp;
extern bool print_pid_pfx;
extern int sys_open();
extern int acolumn;
extern char *acolumn_spaces;

int main () {
    // This is also set up by strace.  Needs to be run once
    acolumn_spaces = xmalloc(acolumn + 1);
    memset(acolumn_spaces, ' ', acolumn);
    acolumn_spaces[acolumn] = '\0';
    // Tell strace to be super verbose and filter nothing
    qualify("trace=all");
    qualify("abbrev=none");
    qualify("verbose=all");
    qualify("signal=all");
    print_pid_pfx = true; // Tell strace to print out the PID information
    max_strlen = 65535;

    // Data that we need to decode.  We can set this stuff each time
    struct tcb tcp;
    current_tcp = &tcp;
    clear_regs(&tcp); // Unset regs error
    tcp._priv_data = NULL; // Must be initialized to null
    tcp.curcol = 1; // reset to 1 each time?
    tcp.outf = stdout;
    tcp.pid = 555;
    char* path = "test";
    tcp.scno = 5;
    // These values go into registers
    tcp.u_rval = 5;
    tcp.u_arg[0] = (long)path;
    tcp.u_arg[1] = 1;
    tcp.u_arg[2] = 0;
    tcp.u_arg[3] = 0;
    tcp.u_arg[4] = 0;
    tcp.u_arg[5] = 0;

    int res = syscall_entering_decode(&tcp);
    // We must save the return value from here
    res = syscall_entering_trace(&tcp, 0);
    // And pass it in here
    syscall_entering_finish(&tcp, res);

    // Must have a struct ts allocated even if we don't use it
    struct timespec ts = {};
    res = syscall_exiting_decode(&tcp, &ts);
    // We must also save the return value here
    syscall_exiting_trace(&tcp, NULL, res);
    // And pass it in here
    syscall_exiting_finish(&tcp);

    fflush(stdout);

    // Data that we need to decode.  We can set this stuff each time
    current_tcp = &tcp;
    clear_regs(&tcp); // Unset regs error
    tcp._priv_data = NULL; // Must be initialized to null
    tcp.curcol = 1; // reset to 1 each time?
    tcp.outf = stdout;
    tcp.pid = 555;
    tcp.scno = 183;
    // These values go into registers
    tcp.u_rval = 15;
    char* anotherstr = "/home/preston";
    tcp.u_arg[0] = (long)anotherstr;
    tcp.u_arg[1] = 260;
    tcp.u_arg[2] = 0;
    tcp.u_arg[3] = 0;
    tcp.u_arg[4] = 0;
    tcp.u_arg[5] = 0;

    res = syscall_entering_decode(&tcp);
    // We must save the return value from here
    res = syscall_entering_trace(&tcp, 0);
    // And pass it in here
    syscall_entering_finish(&tcp, res);

    // Must have a struct ts allocated even if we don't use it
    res = syscall_exiting_decode(&tcp, &ts);
    // We must also save the return value here
    syscall_exiting_trace(&tcp, NULL, res);
    // And pass it in here
    syscall_exiting_finish(&tcp);

}
