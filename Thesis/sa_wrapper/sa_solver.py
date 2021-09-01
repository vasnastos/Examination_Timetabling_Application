import cppyy
from cppyy.gbl.std import map,vector
from timetabling.uniud import datasets
from timetabling.problem import Problem, construct_problem
from datetime import datetime
from functools import reduce
import os
import ctypes
cppyy.include("timetabling/sa_wrapper/sa_cpp/Solution_.cpp")
from cppyy.gbl import simulated_annealing

class SA:
    results=os.path.join("","Stats")
    def __init__(self,problem_instance,exec_seconds):
        self.prob=problem_instance
        self.execution_time=exec_seconds
        self.sa_stats=dict()
        self.move_counter=ctypes.c_int();

    def cppgraph(self,g:dict)->map[int,map[int,int]]:
        mapgraph=map[int,map[int,int]]()
        for key,value in g.items():
            mapgraph[key]=map[int,int]()
            for neighbor,weight in value.items():
                mapgraph[key][neighbor]=weight
        return mapgraph

    def cpp_solution(self,dsol:dict)->map[int,int]:
        mapsol=map[int,int]()
        for node,period in dsol.items():
            mapsol[node]=period
        return mapsol
    
    def pythonhits(self):
        move_hits=dict()
        itr=self.hits.begin()
        while itr!=self.hits.end():
            move_hits[str(itr.__deref__().first)]=int(itr.__deref__().second)
            itr.__preinc__()
        return move_hits
    
    def python_solution(self,solutionmap:map[int,int])->dict:
        sols=dict()
        itr=solutionmap.begin()
        while itr!=solutionmap.end():
            sols[int(itr.__deref__().first)]=int(itr.__deref__().second)
            itr.__preinc__()
        return sols

    def python_statistics(stats:map[str,map[str,int]])->dict:
        statistics=dict()
        itr=stats.begin()
        while itr!=stats.end():
            statistics[int(itr.__deref__().first)]=dict()
            itrs=itr.__deref__().second.begin()
            while itrs!=itr.__deref__().second.end():
                statistics[int(itr.__deref__().first)][str(itrs.__deref__().first)]=int(itrs.__deref__().second)
                itrs.__preinc__()
            itr.__preinc__()
        return statistics

    def solve(self):
        if len(self.prob.periodssol)==0:
            self.prob.greedy_coloring()
        cppgraph=self.cppgraph(self.prob.convert_nx_graph_to_dict())
        cppsol=self.cpp_solution(self.prob.periodssol)
        hits=map[str,map[str,int]]()
        sa_solution=simulated_annealing(self.prob.pid,cppgraph,cppsol,self.prob.S,self.execution_time)
        self.prob.set_solution(self.python_solution(sa_solution))
        print(self.prob.is_feasible())
            




