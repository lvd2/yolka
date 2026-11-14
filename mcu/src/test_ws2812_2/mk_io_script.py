#!/usr/bin/env python3

import os,re,sys,subprocess,tempfile


OBJDUMP = 'arm-none-eabi-objdump'
OBJCOPY = 'arm-none-eabi-objcopy'
LD      = 'arm-none-eabi-ld'


def parse_obj(filename):
    # [ ('name1', binary),
    #   ('name2', binary),
    # ... ]

    section_prefix = '__io_scripts__'

    args = [OBJDUMP, '-s', filename]
    res = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    if res.returncode!=0:
        print('<{0}> failed with returncode {1}!'.format(OBJDUMP, res.returncode))
        print('stdout:\n{0}\nstderr:\n{1}'.format(res.stdout,res.stderr))
        sys.exit(1)

    # parse output of objdump

    sec_start = re.compile(r'^Contents of section ([_\.0-9A-Za-z]+):$')
    note      = re.compile(r'^ NOTE:.*')
    hexdump   = re.compile(r'^ ([0-9a-f]+) ((?:[0-9a-f]+ ){1,4}) +.*$')


    ST_IDLE = 0
    ST_HEX  = 1

    st=ST_IDLE


    result=[]

    name = None
    data = []

    for l in res.stdout.decode("utf-8").splitlines():
        #print(l)

        s = sec_start.match(l)
        n = note.match(l)
        h = hexdump.match(l)

        if n:
            continue
        elif s:
            if st==ST_HEX:
                if len(data)>0:
                    result += [(name, data)]
                else:
                    print('matching section without data! name {}'.format(name))
                    sys.exit(1);

            section_name = s.group(1)
            if section_name.startswith(section_prefix):
                name = section_name.removeprefix(section_prefix)
                st = ST_HEX
                data = []
            else:
                st = ST_IDLE
        elif st==ST_HEX and h:
            data += h.group(2).split()
            #print('data {}'.format(data))
        elif st==ST_HEX:
            st=ST_IDLE
            if len(data)>0:
                result += [(name, data)]


    #print(result)
    return result



def bin_cvt(hlist):
    # convert hex dumps to binary
    
    result=[]
    for h in hlist:
        (name,hexdata) = h

        intdata=[]
        for h in hexdata:
            if len(h)!=8:
                print('detected hex data {}: not 4 bytes!'.format(h))
                sys.exit(1)
            hh = h[6:8]+h[4:6]+h[2:4]+h[0:2]
            ii = int(hh,16)
            intdata += [ii]

        result += [(name,intdata)]

    return result


def mask_gen(data):
    if len(data)==1:
        mask = data[0]
        data = [data[0],data[0]]
    
    mask = data[0] | data[1]

    mask1 = mask  | (mask <<1 )
    mask1 = mask1 | (mask1<<2 )
    mask1 = mask1 | (mask1<<4 )
    mask1 = mask1 | (mask1<<8 )
    mask1 = mask1 | (mask1<<16)

    mask2 = mask  | (mask >>1 )
    mask2 = mask2 | (mask2>>2 )
    mask2 = mask2 | (mask2>>4 )
    mask2 = mask2 | (mask2>>8 )
    mask2 = mask2 | (mask2>>16)

    mask = mask1 & mask2 & 0xFFFFFFFF

    mwidth = mask.bit_count()

    if mask:
        mshift = 0
        shcalc = mask
        while not (shcalc & 1):
            shcalc  = shcalc>>1
            data[0] = data[0]>>1
            data[1] = data[1]>>1
            mshift  = mshift+1
    else:
        mshift = 0
        mwidth = 1

    if mshift>31:
        print('mshift is {}!'.format(mshift))
        sys.exit(1)

    if mwidth<=6:
        mmmm = 8 | ((mshift>>2)&7)
        w0 = ((mshift<<6)&0xC0) | (data[0] & 0x3F)
        w1 = ((mshift<<6)&0xC0) | (data[1] & 0x3F)
        b0 = w0.to_bytes(1,byteorder='little')
        b1 = w1.to_bytes(1,byteorder='little')
    elif mwidth<=13:
        mmmm = 4 | ((mshift>>3)&3)
        w0 = ((mshift<<13)&0xE000) | (data[0] & 0x1FFF)
        w1 = ((mshift<<13)&0xE000) | (data[1] & 0x1FFF)
        b0 = w0.to_bytes(2,byteorder='little')
        b1 = w1.to_bytes(2,byteorder='little')
    else:
        mmmm = 0
        w0 = data[0]
        w1 = data[1]
        b0 = w0.to_bytes(4,byteorder='little')
        b1 = w1.to_bytes(4,byteorder='little')

    return (mmmm,[b0,b1])



def gen_packed(intdata):
    
    OP_STOP     =0
    OP_READ     =1
    OP_WRITE    =2
    OP_BIC      =3
    OP_BIS      =4
    OP_READMASK =5
    OP_BICS     =6
    OP_WAITMASK =7

    op_args = [0,0,1,1,1,1,2,2]

    OP_SHIFT=5
    OP_AMASK=0x10
    OP_MMASK=0x0F

    byteflow = bytearray()

    prevaddr= None

    while len(intdata)>0:
        pack = intdata[0:4]
        intdata = intdata[4:]
        if len(pack)!=4:
            print('less than 4 elements fetched from data list!')
            sys.exit(1)

        cmd = pack[0]
        addr = pack[1]
        arg = []
        arg +=[pack[2]]
        arg +=[pack[3]]

        if cmd<0 or 7<cmd:
            print('found cmd {}, not in 0..7 range!'.format(cmd))
            sys.exit(1)

        if addr&3:
            print('found addr {}, not aligned to 4!'.format(hex(addr)))
            sys.exit(1)

        if cmd!=OP_STOP and (addr>>16)!=0x4000 and (addr>>16)!=0x4001 and (addr>>16)!=0x4002 and (addr>>16)!=0x4800:
            print('found addr {}, not in 0x4000xxxx or 0x4001xxxx or 0x4002xxxx or 0x4800xxxx range!'.format(hex(addr)))
            sys.exit(1)

        if addr==prevaddr:
            new_addr = None
        else:
            new_addr = addr & 0xFFFC
            new_addr = new_addr | ((addr>>16)&3)
            if (addr>>16)==0x4800:
                new_addr = new_addr | 3

        prevaddr = addr

        if op_args[cmd]==0:
            mmmm = 0
            b0=None
            b1=None
        elif op_args[cmd]==1:
            (mmmm,[b0,b1]) = mask_gen([arg[0]])
            b1=None
        else: # 2 args
            (mmmm,[b0,b1]) = mask_gen(arg)

        opcode = (cmd<<OP_SHIFT) | (OP_AMASK if new_addr!=None and cmd!=OP_STOP else 0) | mmmm;
        cmdbytes = opcode.to_bytes(1,byteorder='little')

        if new_addr!=None and cmd!=OP_STOP:
            cmdbytes += new_addr.to_bytes(2,byteorder='little')

        if b0!=None:
            cmdbytes += b0
        if b1!=None:
            cmdbytes += b1

        """
        if cmd==OP_STOP:
            ppack=[pack[0]]
        elif b0==None:
            ppack = pack[0:2]
        elif b1==None:
            ppack = pack[0:3]
        else:
            ppack = pack
        ppp=[format(p,'08x') for p in ppack]
        op_txt = ['OP_STOP','OP_READ','OP_WRITE','OP_BIC','OP_BIS','OP_READMASK','OP_BICS','OP_WAITMASK']
        ppp[0] = op_txt[ppack[0]]
        print(ppp)
        print(cmdbytes.hex(' '))
        """

        byteflow += cmdbytes

    return byteflow



def mk_obj(tmpath, name):
    # make object for binary file 'name' at path 'tmpath'

    objfile = name+'.o'

    args  = [OBJCOPY, '-I', 'binary', '-O', 'elf32-littlearm', '-B', 'arm']
#    args += ['--binary-symbol-prefix={}'.format(name)]
    args += ['--redefine-sym', '_binary_{}_start={}'.format(name,name)]
    args += ['--strip-symbol=_binary_{}_end'.format(name), '--strip-symbol=_binary_{}_size'.format(name)]
    args += ['--rename-section', '.data=.rodata,alloc,load,readonly,data,contents']
    args += ['--set-section-alignment', '.rodata=1']
    args += [name, objfile]
    print(' '.join(args))

    res = subprocess.run(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, cwd=tmpath)

    if res.returncode!=0:
        print('<{0}> failed with returncode {1}!'.format(OBJCOPY, res.returncode))
        sys.exit(1)

    return os.path.join(tmpath,objfile)


def main():
    if len(sys.argv)<2:
        print("Usage: mk_scripts.py file1.o [file2.o] ...")
        print(" output will be __io_scripts__.o")
        sys.exit(1)

    files = sys.argv[1:]

    scripts = []

    # parse data into list
    for f in files:
        scripts += parse_obj(f)

    # convert all data to binary numbers
    datas = bin_cvt(scripts)
    #print(datas)

    # put scripts data into a dictionary to check against repeating names
    sdict = {}
    for s in datas:
        (name,data) = s
        if name in sdict:
            print('name <{}> is already in dictionary!'.format(name))
            sys.exit(1)
        sdict[name]=gen_packed(data)
    #print(sdict)

    with tempfile.TemporaryDirectory(delete=True) as tmpd:

        # dump every script into binary file and then create object file with that contents
        objs = []
        for k in sdict.keys():
            #print(k)
            with open(os.path.join(tmpd,k),"wb") as binfile:
                binfile.write(sdict[k])
            objs += [mk_obj(tmpd,k)]

        # link separate scripts together
        args = [LD, '-r', '-o', '__io_scripts__.o'] + objs
        print(' '.join(args))
        res = subprocess.run(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        if res.returncode!=0:
            print('<{0}> failed with returncode {1}!'.format(LD, res.returncode))
            sys.exit(1)

    
    sys.exit(0)



if __name__=="__main__":
    main()

