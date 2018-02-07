from __future__ import print_function
import sys
import os
import json


state_dict = {}
state_dict['open_fds'] = []
state_dict['syscalls_made'] = []

def process_syscall(arg1):
    state_dict['syscalls_made'].append(arg1)

def dump_state(event):
    with open(str(event) + '_state.json', 'w') as f:
        json.dump(state_dict, f)

def print_state():
    print(state_dict)
    

