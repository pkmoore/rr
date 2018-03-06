"""
Supplies functions to be called from within rr to handle collecting and
exporting state needed within the CrashSim Injector
"""

from __future__ import print_function
import json


state_dict = {}
state_dict['open_fds'] = [0, 1, 2]
state_dict['syscalls_made'] = []

def process_syscall(state):
    if state['name'] == 'open' and not state['entering']:
        open_exit_handler(state)
    if state['name'] == 'close' and not state['entering']:
        close_exit_handler(state)
    state_dict['syscalls_made'].append(state)

def dump_state(event):
    name = str(event) + '_state.json'
    with open(name, 'w') as f:
        json.dump(state_dict, f)

def open_exit_handler(state):
    if state['result'] in state_dict['open_fds']:
        raise Exception('Open call returned already open file descriptor')
    state_dict['open_fds'].append(state['result'])

def close_exit_handler(state):
    if state['arg1'] not in state_dict['open_fds'] and state['result'] != -1:
        raise Exception('Successful close call on non-open file descriptor')
    state_dict['open_fds'].remove(state['arg1'])
