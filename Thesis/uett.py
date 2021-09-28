from concurrent.futures import ThreadPoolExecutor
from problem import Problem,carterhandler,itchandler
from problem import datasets
import os

# def main():
#     dataset='sta83'
#     problem=Problem(dataset)
#     print(problem)
#     problem.log_noise()
#     problem.log_identical()

def dataset_selection():
    for index,dataset in enumerate(datasets.names):
        print(f'{index+1}.{dataset}')

def concurrent_statistical_execution(i):
    if i==1:
        carterhandler.connected_components()
    elif i==2:
        carterhandler.noisy_exams()        
    elif i==3:
        carterhandler.identical_exams()
    elif i==4:
        itchandler.connected_components()
    elif i==5:
        itchandler.noisy_exams()
    elif i==6:
        itchandler.identical_exams()   




if __name__=='__main__':
    # datasets.load_incstance_info()
    # with ThreadPoolExecutor(max_workers=os.cpu_count()) as exec:
    #     exec.map(concurrent_statistical_execution,[1,2,3,4,5,6])
    # carterhandler.noise_exams_by_degree()
    # itchandler.noise_exams_by_degree()
    # carterhandler.greedy_coloring()
    # carterhandler.noise_exams_by_degree()
    carterhandler.identical_exams()
    itchandler.identical_exams()
    
    
