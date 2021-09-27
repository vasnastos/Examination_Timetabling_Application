import platform
import os
import psutil
import subprocess
import re

def get_cpu_type():
    command = "cat /proc/cpuinfo"
    return subprocess.check_output(command,shell=True).strip()

with open('diag.txt','w') as WF:
    WF.write('Processor:{}\n'.format(platform.processor()))
    cpu_name=get_cpu_type()
    WF.write('Cpu:{}\n'.format(cpu_name))
    WF.write('Cores:{}\n'.format(os.cpu_count()))
    WF.write('Threads:{}\n'.format(psutil.cpu_count()))
    WF.write('Memory:{}\n'.format(psutil.virtual_memory()))