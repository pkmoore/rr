"""
Supplies functions to be called from within rr to handle collecting and
exporting state needed within the CrashSim Injector
"""

from __future__ import print_function
import json
import os
import os.path


state_dict = {}
state_dict['open_fds'] = [0, 1, 2]
state_dict['syscalls_made'] = []
state_dict['time_call_results'] = []
state_dict['brks'] = []
state_dict['gettimeofdays'] = []
proc_pipe = None
proc_pipe_name = 'rrdump_proc.pipe'

def write_to_pipe(data):
    global proc_pipe
    print('Inside write to pipe')
    if not proc_pipe:
        if os.path.exists(proc_pipe_name):
            os.unlink(proc_pipe_name)
        os.mkfifo(proc_pipe_name)
        proc_pipe = open(proc_pipe_name, 'w', 0)
    print('Writing:', data)
    proc_pipe.write(data)

def close_pipe():
    global proc_pipe
    proc_pipe.close()

def process_syscall(state):
    if state['name'] == 'open' and not state['entering']:
        add_result_fd(state)
    if state['name'] == 'epoll_create' and not state['entering']:
        add_result_fd(state)
    if state['name'] == 'close' and not state['entering']:
        close_exit_handler(state)
    if state['name'] == 'socketcall' and not state['entering']:
        # socket, accept, accept4
        if state['arg1'] == 1 or state['arg1'] == 5 or state['arg1'] == 18:
            add_result_fd(state)
    if state['name'] == 'time' and not state['entering']:
        time_exit_handler(state)
    state_dict['syscalls_made'].append(state)

def process_brk(flags, start, size, prot):
    print('In process brk')
    state_dict['brks'].append({'flags': flags,
                               'start': start,
                               'size': size,
                               'prot': prot})

def process_pipe(fd1, fd2):
    global state_dict
    if fd1 in state_dict['open_fds'] or fd2 in state_dict['open_fds']:
        raise Exception('Pipe returned already open file descriptor: ({}->{})'
                        .format(fd1, fd2))
    state_dict['open_fds'].append(fd1)
    state_dict['open_fds'].append(fd2)

def process_gettimeofday(seconds, microseconds):
    state_dict['gettimeofdays'].append({'seconds': seconds,
                                        'microseconds': microseconds})

def dump_state(event):
    name = str(event) + '_state.json'
    with open(name, 'w') as f:
        json.dump(state_dict, f)

def add_result_fd(state):
    if state['result'] < 0:
        return
    if state['result'] in state_dict['open_fds']:
        raise Exception('Tried to add already open file descriptor: {}'
                         .format(state['result']))
    state_dict['open_fds'].append(state['result'])

def close_exit_handler(state):
    if state['arg1'] not in state_dict['open_fds'] and state['result'] == 0 :
        raise Exception('Successful close call on non-open file descriptor: {}'
                        .format(state['arg1']))
    state_dict['open_fds'].remove(state['arg1'])

def time_exit_handler(state):
    state_dict['time_call_results'].append(state['result'])
