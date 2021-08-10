import sys
import os
import numpy as np 
import matplotlib.pyplot as plt

n = len(sys.argv)

if(n != 2):
	assert('Invalid number of arguments passed')
	exit(-1)

executable = sys.argv[1]

for K_i in [1,4]:
	name = 'K_i'+str(K_i)
	for K_m in [1,1.5]:
		t4 = name
		name = name + 'K_m'+str(K_m)
		for K_n in [0.5,1]:
			t3 = name
			name = name + 'K_n'+str(K_n)
			for K_f in [0.1,0.3]:
				t2 = name
				name = name+'K_f'+str(K_f)
				for P_s in [0.01,0.0001]:
					t1 = name
					name = name + 'P_s'+str(P_s)
					os.system(executable+' -i '+str(K_i)+' -m '+str(K_m)+' -n '+str(K_n)+' -f '+str(K_f)+' -s '+str(P_s)+' -T '+'3000'+' -o '+name+'.txt')
					y = np.loadtxt(name+'.txt')
					plt.title(name)
					plt.ylabel('C_W value')
					plt.xlabel('updates_count')
					plt.plot(np.arange(start=0,stop=3000,step=1),y)
					plt.savefig(name+'.png')
					plt.close()
					name = t1
				name = t2
			name = t3
		name = t4
