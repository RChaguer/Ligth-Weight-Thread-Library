import os
import numpy as np
import matplotlib.pyplot as plt

install_dir = "install/bin/"
img_dir = "graphs/imgs/"
data_dir = "graphs/data/"

def test_threads(test_name, threads_range, data_pattern, index=""):
    full_name = data_dir+test_name+index 
    if os.path.exists(full_name+".dat"):
        os.remove(full_name+".dat")

    for i in threads_range:
        cmd = "./"+install_dir+test_name+" "+str(i)+index.replace("_"," ")+" 2>/dev/null | awk -F' ' '{print "+data_pattern+"}' >> "+full_name+".dat" 
        os.system(cmd)

    with open(full_name+'.dat') as f:
        lines = f.readlines()

    data = [x.split('\t')[-1].rstrip('\n') for x in lines]
    return data

def test_yields_threads(test_name, threads_range, yields_range, data_pattern):
    data = []
    for y in yields_range:
        data.append(test_threads(test_name, threads_range, data_pattern, "_"+str(y)))
    return data

def draw_graph(test_name, threads_range, thread_data, pthread_data, index=""):        
    plt.figure(test_name+"_graph")
    if (index != ""):
        plt.title(test_name+"_graph_yields"+index)
    else :
        plt.title(test_name+"_graph")
    #plt.plot(threads_range, thread_data, 'b', label="thread")
    plt.plot(threads_range, pthread_data, 'r', label="pthread")
    plt.xlabel('Number of threads')
    plt.ylabel('Execution Time')
    plt.yticks(color='w')
    plt.legend()
    plt.savefig(img_dir+test_name+index+'.png')
    plt.clf()

def get_graph(test_name, data_pattern, threads_max, threads_step=10,  yields_max=None, yields_step=10):
    if (yields_max is None):
        using_yield = False
    else:
        using_yield = True

    threads_range = np.arange(1, threads_max, threads_step)
    if (using_yield):
        yields_range = np.arange(1, yields_max, yields_step)
    
    if (using_yield):
        #thread_data = test_threads(test_name, threads_range, data_pattern)
        pthread_data = test_yields_threads("pthread_"+test_name, threads_range, yields_range, data_pattern)
    else:
        #thread_data = test_threads(test_name, threads_range, data_pattern)
        pthread_data = test_threads("pthread_"+test_name, threads_range, data_pattern)

    if (using_yield):
        for i in range(len(yields_range)):
            #draw_graph(test_name, thread_data[y], pthread_data[y], "_"+str(y))
            draw_graph(test_name, threads_range, None, pthread_data[i], "_"+str(yields_range[i]))
    else:
        #draw_graph(test_name, thread_data, pthread_data)
        draw_graph(test_name, threads_range, None, pthread_data)

if __name__ == "__main__":
    print("Exporting 21-create-many graph ...")
    get_graph("21-create-many", "$1 \"\\t\" $8", 100)
    print("Exporting 22-create-many-recursive graph ...")
    get_graph("22-create-many-recursive", "$1 \"\\t\" $8", 100)
    print("Exporting 23-create-many-once graph ...")
    get_graph("23-create-many-once", "$1 \"\\t\" $10", 100)
    print("Exporting 31-switch-many graphs ...")
    get_graph("31-switch-many", "$1 \"\\t\" $4 \"\\t\" $6", 100, yields_max=100)
    print("Exporting 32-switch-many-join graphs ...")
    get_graph("32-switch-many-join", "$9; exit", 100, yields_max=100)
    print("Exporting 33-switch-many-cascade graphs ...")
    get_graph("33-switch-many-cascade",  "$9; exit", 100, yields_max=100)
    print("Exporting 51-fibonacci graph ...")
    get_graph("51-fibonacci", "$3 \"\\t\" $7", 15)