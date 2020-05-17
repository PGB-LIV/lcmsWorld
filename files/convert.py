import os

import sys
from functools import partial


  
def write(filename, overwrite = False):
    name = filename.split(".")[0]

    if not overwrite:
        if os.path.isfile(name+".h"):
            return
            
    outfile = open(name+".h" ,"w")    
    print("const unsigned char "+name+"[] = {", file=outfile)
    n = 0
    with open(filename, "rb") as in_file:
      for c in iter(partial(in_file.read, 1), b''):
        print("0x%02X," % ord(c), end='', file=outfile)
        n += 1
        if n % 16 == 0:
          print("", file=outfile)
    print("};", file=outfile)
    outfile.close()
    
print("start .h conversion")
overWrite = False    
if  len(sys.argv) > 2:
    overWrite = True
path = "."    
if  len(sys.argv) > 1:
    path = sys.argv[1]

os.chdir(path)

for filename in os.listdir(os.getcwd()):
    if os.path.isfile(filename):
        if not filename.endswith(".h"):
            write(filename,overWrite)