import os
import numpy as np
from statistics import mean, median



if os.path.exists("toto_8.dat"):
    os.remove("toto_8.dat")
for _ in range(100):
    cmd = "./51-fibonacci 8 2>/dev/null | awk -F' ' '{print $7; exit}' >> toto_8.dat" 
    os.system(cmd)

    
if os.path.exists("toto_15.dat"):
    os.remove("toto_15.dat")
for _ in range(100):
    cmd = "./51-fibonacci 15 2>/dev/null | awk -F' ' '{print $7; exit}' >> toto_15.dat" 
    os.system(cmd)

if os.path.exists("toto_16.dat"):
    os.remove("toto_16.dat")
for _ in range(100):
    cmd = "./51-fibonacci 16 2>/dev/null | awk -F' ' '{print $7; exit}' >> toto_16.dat" 
    os.system(cmd)


with open('toto_8.dat') as f:
    lines = f.readlines()
data8 = [float(x.rstrip('\n')) for x in lines]


with open('toto_15.dat') as f:
    lines = f.readlines()
data15 = [float(x.rstrip('\n')) for x in lines]


with open('toto_16.dat') as f:
    lines = f.readlines()
data16 = [float(x.rstrip('\n')) for x in lines]


print("Fibo 8 : min :"+str(min(data8))+" | max :"+str(max(data8))+" | moyenne :"+str(mean(data8))+" | median :"+str(median(data8)))

print("Fibo 15 : min :"+str(min(data15))+" | max :"+str(max(data15))+" | moyenne :"+str(mean(data15))+" | median :"+str(median(data15)))

print("Fibo 16 : min :"+str(min(data16))+" | max :"+str(max(data16))+" | moyenne :"+str(mean(data16))+" | median :"+str(median(data16)))