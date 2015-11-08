import os
outputlist = []
for filename in os.listdir(os.getcwd()):
    f = open(filename, 'r')
    print filename
    for line in f:
        outputlist.append(filename+' '+line)

output = open("output.txt",'w')
for line in outputlist:
    output.write(line)
