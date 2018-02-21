from __future__ import print_function
import sys
import os
import json


state_dict = {}
state_dict['open_fds'] = []
state_dict['syscalls_made'] = []

# Make this track open file descriptors
def process_syscall(regs):
    state_dict['syscalls_made'].append(regs)

def dump_state(event):
    name = str(event) + '_state.json'
    with open(name, 'w') as f:
        json.dump(state_dict, f)
