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
        if state['arg1'] == 1:
            add_result_fd(state)
    state_dict['syscalls_made'].append(state)

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
