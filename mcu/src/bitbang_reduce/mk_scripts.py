#!/usr/bin/env python

import os,re,sys,subprocess

def parse_obj(filename):
    # [ ('name1', binary),
    #   ('name2', binary),
    # ... ]

    objdump = 'arm-none-eabi-objdump'

    args = [objdump, '-s', filename]
    res = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    if res.returncode!=0:
        print('<{0}> failed with returncode {1}!'.format(objdump, res.returncode))
        print('stdout:\n{0}\mstderr:\n{1}'.format(res.stdout,res.stderr))
        sys.exit(1)

    # parse output of objdump

    sec_start = re.compile(r'^Contents of section ([_\.0-9A-Za-z]+):$')
    note      = re.compile(r'^ NOTE:.*')
    hexdump   = re.compile(r'^ ([0-9a-f]+) ((?:[0-9a-f]+ ){1,3}) +.*$')

    for l in res.stdout.decode("utf-8").splitlines():
        print(l)


def main():

    if len(sys.argv)<2:
        print("Usage: mk_scripts.py file1.o [file2.o] ...")
        print(" output will be __io_scripts__.o")
        sys.exit(1)

    files = sys.argv[1:]

    scripts = []

    for f in files:
        scripts += parse_obj(f)



if __name__=="__main__":
    main()
