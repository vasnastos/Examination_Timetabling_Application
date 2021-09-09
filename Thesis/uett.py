from concurrent.futures import ThreadPoolExecutor
from problem import Problem,carterhandler,itchandler
from problem import ProblemAnalyzer
from problem import datasets
import os

def main():
    dataset='sta83'
    problem=Problem(dataset)
    print(problem)
    problem.log_noise()
    problem.log_identical()

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
    datasets.load_incstance_info()
    # main()
    # ProblemAnalyzer.save_noise()
    # ProblemAnalyzer.save_statistics()
    # ProblemAnalyzer.save_identical()
    # ProblemAnalyzer.greedy_coloring_stats()
    # problem.greedy_coloring_diag()
    # save_to_dataset()
    with ThreadPoolExecutor(max_workers=os.cpu_count()) as exec:
        exec.map(concurrent_statistical_execution,[1,2,3,4,5,6])
    
