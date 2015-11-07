import os
output = open("output.txt",'w')
for filename in os.listdir(os.getcwd()):
    f = open(filename, 'r')
    for line in f:
        output.write(filename+' '+line)
