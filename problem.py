
from re import L
from typing import overload
import networkx as nx
from networkx.classes.function import density, non_edges
import os 
import math
from itertools import combinations
import logging
from copy import deepcopy
from concurrent.futures import ThreadPoolExecutor
from enum import Enum
from functools import lru_cache
from numpy.core.numeric import roll
from tqdm import tqdm
from matplotlib import pyplot as plt
import numpy as npy
import pickle
from time import time
import sys
import statistics as stats
from copy import deepcopy
from threading import Lock
import random
import logging
from queue import LifoQueue


# from inspect import getfullargspec

# class Function(object):
#   """Function is a wrap over standard python function.
#   """
#   def __init__(self, fn):
#     self.fn = fn

#   def __call__(self, *args, **kwargs):
#     """Overriding the __call__ function which makes the
#     instance callable.
#     """
#     # fetching the function to be invoked from the virtual namespace
#     # through the arguments.
#     fn = Namespace.get_instance().get(self.fn, *args)
#     if not fn:
#         raise Exception("no matching function found.")

#     # invoking the wrapped function and returning the value.
#     return fn(*args, **kwargs)

#   def key(self, args=None):
#     """Returns the key that will uniquely identify
#     a function (even when it is overloaded).
#     """
#     # if args not specified, extract the arguments from the
#     # function definition
#     if args is None:
#       args = getfullargspec(self.fn).args

#     return tuple([
#       self.fn.__module__,
#       self.fn.__class__,
#       self.fn.__name__,
#       len(args or []),
#     ])

# class Namespace(object):
#   """Namespace is the singleton class that is responsible
#   for holding all the functions.
#   """
#   __instance = None

#   def __init__(self):
#     if self.__instance is None:
#       self.function_map = dict()
#       Namespace.__instance = self
#     else:
#       raise Exception("cannot instantiate a virtual Namespace again")

#   @staticmethod
#   def get_instance():
#     if Namespace.__instance is None:
#       Namespace()
#     return Namespace.__instance

#   def register(self, fn):
#     """registers the function in the virtual namespace and returns
#     an instance of callable Function that wraps the
#     function fn.
#     """
#     func = Function(fn)
#     self.function_map[func.key()] = fn
#     return func
  
#   def get(self, fn, *args):
#     """get returns the matching function from the virtual namespace.

#     return None if it did not fund any matching function.
#     """
#     func = Function(fn)
#     return self.function_map.get(func.key(args=args))

# def overload(fn):
#   """overload is the decorator that wraps the function
#   and returns a callable object of type Function.
#   """
#   return Namespace.get_instance().register(fn)

strategies=['largest_first','random_sequential','smallest_last','independent_set','connected_sequential_bfs','connected_sequential_dfs','saturation_largest_first','largest_first_interchange','random_sequential_interchange','smallest_last_interchange','connected_sequential_bfs_interchange','connected_sequential_dfs_interchange']

class folder:
    results_folder=os.path.join('','Statistics')
    datasets_folder=os.path.join('','datasets')
    best_known=os.path.join('','datasets','uniud_stats.csv')
    results=os.path.join('','datasets','solutions')

class Instance:
    def __init__(self,id,st,p,ex,enr,bc):
        self.problem_id=id
        self.students=st
        self.periods=p
        self.exams=ex
        self.enrollments=enr
        self.best_cost=bc

class DEF_DATA:
    REHEAT_TEMPERATURE=5.0
    BEST_SOL_ITERATIONS=6

class datasets:
    names=[
        "car91",
        "car92",
        "ear83",
        "hec92",
        "kfu93",
        "lse91",
        "pur93",
        "rye93",
        "sta83",
        "tre92",
        "uta92",
        "ute92",
        "yor83",
        "ITC2007_1",
        "ITC2007_2",
        "ITC2007_3",
        "ITC2007_4",
        "ITC2007_5",
        "ITC2007_6",
        "ITC2007_7",
        "ITC2007_8",
        "ITC2007_9",
        "ITC2007_10",
        "ITC2007_11",
        "ITC2007_12",
        "D1-2-17",
        "D5-1-17",
        "D5-1-18",
        "D5-2-17",
        "D5-2-18",
        "D5-3-18",
        "D6-1-18",
        "D6-2-18"
    ]

    instance_info=dict()

    @staticmethod
    def load_incstance_info():
        start=True
        with open(folder.best_known,'r') as RF:
            for line in RF:
                if start:
                    start=False
                    continue
                data=line.split(',')
                if len(data)!=7: continue
                key=data[0]
                datasets.instance_info[key]=Instance(data[0],int(data[2]),int(data[3]),int(data[4]),int(data[5]),float(data[6]))


class Student:
    def __init__(self,i):
        self.id=i
        self.exams=list()
    
    def add_exam(self,exam_id):
        self.exams.append(exam_id)
    
    def __lt__(self,oth):
        return self.id<oth.id
    
    def __str__(self):
        print(f"Student {self.id}:",end=" ")
        print(self.exams)

    def __hash__(self):
        return self.id

    def __iter__(self):
        self.iteration_id=0
        return self.exams
    
    def __next__(self):
        if self.iteration_id>=len(self.exams):
            raise StopIteration
        else:
            value=self.exams[self.iteration_id]
            self.iteration_id+=1
            return value
    
    def __eq__(self,oth):
        return self.id==oth

class Exam:
    def __init__(self,i):
        self.id=i
        self.students=list()
    
    def add_student(self,student_id):
        self.students.append(student_id)
    
    def __eq__(self,oth):
        return self.id==oth
    
    def __lt__(self,oth):
        return self.id==oth.id
    
    def common_students(self,exam):
        return len(set(self.students).intersection(set(exam.students)))

    def __str__(self):
        print("Exam {}".format(self.id),end=":")
        print(self.students)

class MinifiedProblem:
    def __init__(self,cid,exams,students,graph,component_coloring,st):
        self.id=cid
        self.students=students
        self.exams=exams
        self.G=graph
        self.E=len(self.exams)
        self.S=len(self.students)
        self.normalized=st
        self.s_periods=component_coloring
    
    def compute_cost(self):
        return sum([Problem.penalty[self.s_periods[node1]-self.s_periods[node2]] for node1,node2 in self.G.edges if self.s_periods[node1]-self.s_periods[node2] in Problem.penalty])

    def compute_normilized_cost(self):
        return self.compute_cost()/self.normalized
    
    def __str__(self):
        print('component_id:{}'.format(self.cid))
        print(self.G.nodes)
        print('Cost Contribution:{}'.format(self.compute_cost()))
        print('Normalized Cost:{}'.format(self.compute_normilized_cost()))

class Problem:
    penalty={
        1:16,
        -1:16,
        2:8,
        -2:8,
        3:4,
        -3:4,
        4:2,
        -4:2,
        5:1,
        -5:1
    }
    def __init__(self,d_id):
        self.students=list()
        self.exams=list()
        self.enrollments=0
        self.pid=d_id
        start=True
        with open(os.path.join("","datasets",d_id+".in")) as RF:
            for line in RF:
                if start:
                    start=False
                    continue
                line=line.strip()
                if len(line)==0: continue
                if line.startswith('s'):
                    self.enrollments+=1
                    line=line.replace('s','')
                    data=line.split()
                    exam=int(data[1])
                    student=int(data[0])
                    if student not in self.students:
                        self.students.append(Student(student))
                    self.students[self.students.index(student)].add_exam(exam)
                    self.exams[self.exams.index(exam)].add_student(student)
                else:
                    data=line.split()
                    exam=int(data[0])
                    self.exams.append(Exam(exam))

        self.students.sort()
        self.exams.sort()
        self.E=len(self.exams)
        self.S=len(self.students)
        self.G=None
        self.parallel_coloring=dict()
        self.s_periods=dict()
        self.noisy_students=list()
        self.ident_coloring_exams=dict()
        self.noisy_exams=list()
        self.create_graph()
        self.greedy_coloring()
        self.P=max(self.s_periods.values())+1
        self.noise_by_lesson_number()
        self.noise_by_component()
        self.noise_by_exam_degree()
        self.noisy_exams.extend(self.noisy_exams_by_students)
        self.noisy_exams.extend(self.noisy_exams_by_degree)
        self.noisy_exams.extend(self.noisy_exams_by_component)
        self.noisy_exams=list(set(self.noisy_exams))
        self.noisy_students=list(set(self.noisy_students))
        self.periods={i:list() for i in range(self.P)}
        for node,period in self.s_periods.items():
            self.periods[period].append(node)

    @lru_cache(maxsize=1000)
    def cost_among_periods(self,period1,period2):
        cs=0
        for exam in self.periods[period1]:
            for exam2 in  self.periods[period2]:
                if self.G.has_edge(exam,exam2):
                    cs+=self.G[exam][exam2]['weight'] * penalty[abs(period1-period2)]
        return cs

    
    def create_graph(self):
        self.G=nx.Graph()
        self.G.add_nodes_from([exam.id for exam in self.exams])
        for index_i in range(len(self.exams)):
            for index_j in range(index_i+1,len(self.exams)):
                cs=self.exams[index_i].common_students(self.exams[index_j])
                if cs>0:
                    self.G.add_edge(self.exams[index_i].id,self.exams[index_j].id,weight=cs)

    def get_connected_components(self):
        components=list(nx.connected_components(self.G))
        subproblems=list()
        cnt=1
        for component in components:
            compname=self.pid+'_component'+str(cnt)
            cnt+=1
            sg=self.G.subgraph(component)
            cexams=[exam for exam in sg.nodes]
            cstudents=set([self.students[self.students.index(student)] for exam in list(sg.nodes) for student in self.exams[self.exams.index(exam)].students])
            subproblems.append(MinifiedProblem(compname,cexams,cstudents,sg,[self.s_periods[node] for node in list(sg.nodes)],self.S))
        return subproblems
    

    def max_number_of_exams_per_student(self):
        return max([len(student.exams) for student in self.students])

    def average_number_of_exams_per_student(self):
        return sum([len(student.exams) for student in self.students])/self.S
    
    def conflict_density(self):
        if self.G!=None:
            return self.G.number_of_edges()/self.E
        else:
            for index_i in range(len(self.exams)):
                cd=0
                for index_j in range(index_i+1,len(self.exams)):
                    cs=self.exams[index_i].common_students(self.exams[index_j])
                    if cs>0:
                        cd+=1
            return cd/self.E

    def density_networkx(self):
        return density(self.G)
    
    def clean_density_networkx(self):
        return density(self.G)

    def clean_density(self):
       self.noise_out()
       return self.conflict_density()

    def clean_students(self):
        return len([student.id for student in self.students if student.id not in self.noisy_students])
    
    def noise_students(self):
        return len(self.noisy_students)

    def noisy_enrollments(self):
        return sum([len(student.exams) for student in self.students if student.id in self.noisy_students])
    
    def clean_enrollments(self):
        return self.enrollments-self.noisy_enrollments()
    
    def noise_by_lesson_number(self):
        self.noisy_exams_by_students=list()
        for student in self.students:
            if len(student.exams)==1:
                self.noisy_students.append(student.id)

        for exam in self.exams:
            if set(exam.students).issubset(set(self.noisy_students)):
                self.noisy_exams_by_students.append(exam.id)
    
    def noise_by_component(self):
        components=self.get_connected_components()
        limit=math.floor(self.P/6)+1
        self.noisy_exams_by_component=list()
        for component in components:
            if component.E<=limit:
                self.noisy_exams_by_component.extend([node for node in list(component.G.nodes)])
        self.noisy_exams_by_component=list(set(self.noisy_exams_by_component))
        self.noisy_students.extend([student for exam in self.noisy_exams_by_component for student in self.exams[self.exams.index(exam)].students])
    
    def noise_by_exam_degree(self):
        limit=math.floor(self.P/11)
        self.noisy_exams_by_degree=list()
        graphcopy=deepcopy(self.G)
        while True:
            removed_nodes=list()
            for exam in list(graphcopy.nodes):
                if graphcopy.degree[exam]<=limit:
                    self.noisy_exams_by_degree.append(exam)
                    removed_nodes.append(exam)
            if len(removed_nodes)==0:
                break
            graphcopy.remove_nodes_from(removed_nodes)

            
        self.noisy_exams_by_degree=list(set(self.noisy_exams_by_degree))
        self.noisy_students.extend([student for exam in self.noisy_exams_by_degree for student in self.exams[self.exams.index(exam)].students])
    
    def identical_exams(self):
        exam_combinations=combinations([exam.id for exam in self.exams],2)
        identical_type_1,identical_type_2,identical_type_3=list(),list(),list()
        type_1,type_2,type_3=list(),list(),list()
        for node_1,node_2 in exam_combinations:
            if self.exams[self.exams.index(node_1)].students==self.exams[self.exams.index(node_2)].students:
                type_1.append((node_1,node_2))

            node_a_neighbors=set(self.G.neighbors(node_1))
            node_b_neighbors=set(self.G.neighbors(node_2))
            if len(node_a_neighbors)==0 and len(node_b_neighbors)==0: continue
            ident_neighbor_exams=True
            if node_a_neighbors==node_b_neighbors:
                for neighbor in node_a_neighbors:
                    if self.G[node_1][neighbor]['weight']!=self.G[node_2][neighbor]['weight']:
                        ident_neighbor_exams=False
                        break
                if ident_neighbor_exams:
                    type_2.append((node_1,node_2))
            
            elif node_a_neighbors.symmetric_difference(node_b_neighbors)=={node_1,node_2}:
                ident_ipp=True
                for neighbor in node_a_neighbors:
                    if neighbor!=node_2:
                        if self.G[node_1][neighbor]['weight']!=self.G[node_2][neighbor]['weight']:
                            ident_ipp=False
                if ident_ipp:
                    type_3.append((node_1,node_2))
        
        for node_1,node_2 in type_1:
            found_in=False
            for index,fs in enumerate(identical_type_1):
                    if node_1 in fs or node_2 in fs:
                        identical_type_1[index].add(node_1) 
                        identical_type_1[index].add(node_2)
                        found_in=True
                        break
            
            if not found_in:
                identical_type_1.append({node_1,node_2})

        for node_1,node_2 in type_2:
            found_in=False
            for index,fs in enumerate(identical_type_2):
                if node_1 in fs or node_2 in fs:
                    found_in=True
                    identical_type_2[index].add(node_1)
                    identical_type_2[index].add(node_2)
                    break
            if not found_in:
                identical_type_2.append({node_1,node_2})
        
        for node_1,node_2 in type_3:
            found_in=False
            for index,fs in enumerate(identical_type_3):
                if node_1 in fs or node_2 in fs:
                    found_in=True
                    identical_type_3[index].add(node_1)
                    identical_type_3[index].add(node_2)
                    break
            if not found_in:
                identical_type_3.append({node_1,node_2})
        return identical_type_1,identical_type_2,identical_type_3
    
    def seriallize_noise(self):
        noise_root_path=os.path.join('','Statistics','Noise')
        if not os.path.exists(noise_root_path):
            os.mkdir(noise_root_path)
        dataset_noise_folder=os.path.join(noise_root_path,self.dataset_id)
        if not os.path.exists(dataset_noise_folder):
            os.mkdir(datasets_noise_exams)
        with open(os.path.join(dataset_noise_folder,'noise_by_students'),'wb') as WF:
            pickle.dump(self.noisy_exams_by_students,WF)
        with open(os.path.join(dataset_noise_folder,'noise_by_components'),'wb') as WF:
            pickle.dump(self.noisy_exams_by_component,WF)
        with open(os.path.join(dataset_noise_folder,'noise_by_degree'),'wb') as WF:
            pickle.dump(self.noisy_exams_by_degree,WF)
        with open(os.path.join(dataset_noise_folder),'w') as SN:
            SN.write('N1:[',",".join(self.noisy_exams_by_students),']\n')
            SN.write('N2:[',",".join(self.noisy_exams_by_component),']\n')
            SN.write('N3:[',",".join(self.noisy_exams_by_degree),']\n')
            SN.write('TN:[',",".join(self.noisy_exams),']\n')

    def log_noise(self):
        logging.basicConfig(format="[%(asctime)s]:%(message)s",level=logging.INFO)
        logging.info("Noise Exams[Exams with similar students]")
        print(self.noisy_exams_by_students,end='\n\n')
        logging.info("Noise Exams by degree[Exams with degree lower than floor(P/11)]")
        print(self.noisy_exams_by_degree,end='\n\n')
        logging.info("Noise Exams by Component[Exams belong to a component with toal nodes lower than floor(P/6)+1]")
        print(self.noisy_exams_by_component,end='\n\n')    

    def log_identical(self):
        ident_type_1,ident_type_2,ident_type_3=self.identical_exams()
        logging.basicConfig(format="[%(asctime)s]:%(message)s")
        logging.info("Exams with the same students")
        print(ident_type_1,end='\n\n')
        logging.info("Exams with same neighbors")
        print(ident_type_2,end='\n\n')
        logging.info("Adjacent Exams with the same neighbors")
        print(ident_type_3,end='\n\n')

    def calculate_noise(self):
        return len(self.noisy_exams_by_students),len(self.noisy_exams_by_component),len(self.noisy_exams_by_degree)
    
    def total_noise(self):
        return len(self.noisy_exams)

    def noise_out(self):
        self.Graph_copy=deepcopy(self.G)
        self.fixed_exams=dict()
        _,ident_type_2,_=self.identical_exams()
        for identical in ident_type_2:
            key=-1
            for index,node in enumerate(identical):
                if index==0:
                    key=node
                    self.ident_coloring_exams[key]=list()
                    continue
                self.ident_coloring_exams[key].append(node)
                self.fixed_exams.update({node:self.s_periods[key]})
        self.G.remove_nodes_from(self.noisy_exams)

    
    def update_fixed_exams(self):
        for exam,ident_color_exams in self.ident_coloring_exams.items():
            for exam2 in ident_color_exams:
                self.s_periods[exam2]=self.s_periods[exam]

    def Coloring(self,strategy):
        color=nx.greedy_color(self.G,strategy=strategy)
        self.parallel_coloring[strategy]=color
        if strategy not in ['saturation_largest_first','independent_set']:
            color=nx.greedy_color(self.G,strategy=strategy,interchange=True)
            self.parallel_coloring[strategy+"_interchange"]=color
    
    def greedy_coloring(self):
        with ThreadPoolExecutor(max_workers=os.cpu_count()) as executor:
            executor.map(self.Coloring,strategies)
        
        min_colors=self.G.number_of_nodes()
        for _,coloring in self.parallel_coloring.items():
            periods_used=max(coloring.values())+1
            if periods_used<min_colors:
                min_colors=periods_used
                self.s_periods=coloring
    
    def renew_solution(self,a_solution):
        self.s_periods=a_solution

    def is_feasible(self):
        return len([(node1,node2) for node1,node2 in list(self.G.edges) if self.s_periods[node1]==self.s_periods[node2]])==0

    def compute_cost(self):
        return sum([Problem.penalty[self.s_periods[node_1]-self.s_periods[node_2]] * self.G[node_1][node_2]['weight'] for node_1,node_2 in list(self.G.edges) if self.s_periods[node_1]-self.s_periods[node_2] in Problem.penalty])
    
    def compute_normalized_cost(self):
        return self.compute_cost()/self.S
    
    def graph_out(self):
        print('Nodes\n'+'*'*8)
        print(list(self.G.nodes),end='\n\n')
        print('Edges\n'+'*'*8)
        for node_1,node_2 in list(self.G.edges):
            print(f'\t{node_1}---{node_2}')
        print('Schedule Exams\n'+'*'*8)
        for node,period in self.s_periods.items():
            print(f'{node}-->{period}')
        
    def greedy_coloring_diag(self):
        add_ons={'largest_first':'LF','random_sequential':'RS','smallest_last':'SL','independent_set':'IS','connected_sequential_bfs':'CSB','connected_sequential_dfs':'CSD','saturation_largest_first':'DS','largest_first_interchange':'LFI','random_sequential_interchange':'RSI','smallest_last_interchange':'SLI','connected_sequential_bfs_interchange':'CSBI','connected_sequential_dfs_interchange':'CSDI'}
        print([(key,max(value.values())+1) for key,value in self.parallel_coloring.items()])
        x_data=[max(self.parallel_coloring[strategy].values())+1 for strategy in strategies]
        y_range=npy.arange(len(strategies))
        _,ax=plt.subplots()
        ax.barh(y_range,x_data,align='center')
        ax.set_yticks(y_range)
        ax.set_yticklabels([add_ons[strategy] for strategy in strategies])
        ax.set_xlabel('Periods')
        ax.set_ylabel('strategies')
        limit_period_number=datasets.instance_info[self.dataset_id].periods
        ax.axvline(limit_period_number,c='r',label='period limitation:{}'.format(limit_period_number))
        ax.legend(loc='upper right')
        ax.set_xlim(0,max(x_data)+10)
        plt.title('Greedy coloring info for dataset:'+self.dataset_id)
        plt.show()
    
    def noise_chart(self):
        percent_data=[len(self.noisy_exams),self.E-len(self.noisy_exams)]
        labels=['Noisy exams','Clean exams']
        explode=(0.1,0)
        plt.pie(percent_data,labels=labels,autopct='%1.1f%%',explode=explode,shadow=True,startangle=90)
        plt.axis('equal')
        plt.show()

    def solution_per_period(self):
        periods=dict()
        for i in range(self.P):
            periods[i]=list()
        for node,period in self.s_periods.items():
            periods[period].append(node)
        return periods

    def nodes_to_be_examinee(self):
        return list(self.G.nodes)
    
    def __str__(self):
        msg='Problem:{}\n'.format(self.dataset_id)
        msg+='Exams:{}\n'.format(self.E)
        msg+='Students:{}\n'.format(self.S)
        msg+='Periods:{}\n'.format(self.P)
        msg+='Edges:{}\n'.format(self.G.number_of_edges())
        msg+='Conflict Density:{}'.format(self.conflict_density())
        return msg
    
    
class carterhandler:
    datasets={
        "car92":32,
        "car91":35,
        "ear83":24,
        "hec92":18,
        "kfu93":20,
        "lse91":18,
        "pur93":42,
        "rye93":23,
        "sta83":13,
        "tre92":23,
        "uta92":35,
        "ute92":10,
        "yor83":21
    }

    @staticmethod
    def connected_components():
        contents=list()
        from tabulate import tabulate
        for dataset in tqdm(carterhandler.datasets):
            problem=Problem(dataset)
            components=problem.get_connected_components()
            problem_significant_components=list()
            counter=1
            for component in components:
                significant_components=list()
                if component.E>1:
                    problem_significant_components.append(('component_'+str(counter),component.E))
                counter+=1
            significant_components.append(dataset)
            significant_components.append('['+','.join([index[0]+':'+str(index[1]) for index in problem_significant_components])+']')
            contents.append(significant_components)
        print(tabulate(contents,headers=['problem','significant components'],tablefmt='fancygrid'))
        with open(os.path.join('','Statistics','carter_significant_components.out'),'w') as WF:
            WF.write(tabulate(contents,headers=['problem','significant components'],tablefmt='textfile'))

    @staticmethod
    def identical_exams():
        with open(os.path.join('','Statistics','carter_identical_exams.out'),'w') as WF:
            for dataset in tqdm(carterhandler.datasets):
                problem=Problem(dataset)
                _,ident_by_neighbors,_=problem.identical_exams()
                identical_info=''
                start=True
                for ident in  ident_by_neighbors:
                    if  start:
                        start=False
                        identical_info+='['+','.join([str(i) for i in ident])+']'
                    else:
                        identical_info+=',['+','.join([str(i) for i in ident])+']'
                WF.write(dataset)
                WF.write(identical_info)
    
    @staticmethod
    def noisy_exams():
        contents=list()
        from tabulate import tabulate
        for dataset in tqdm(carterhandler.datasets):
            problem=Problem(dataset)
            contents.append([dataset,'['+','.join([str(con) for con in problem.noisy_exams_by_students])+']','['+','.join([str(con) for con in problem.noisy_exams_by_component])+']','['+','.join([str(con) for con in problem.noisy_exams_by_degree])+']'])
        print(tabulate(contents,headers=['dataset','Noise by Student','Noise by component','noise by exam'],tablefmt='textfile'))
        with open(os.path.join('','Statistics','carter_noise_exams.out'),'w') as WF:
            WF.write(tabulate(contents,headers=['dataset','Noise by Student','Noise by component','noise by exam'],tablefmt='textfile'))
    
    @staticmethod
    def greedy_coloring():
        contents=list()
        from tabulate import tabulate
        for dataset in tqdm(carterhandler.datasets):
            problem=Problem(dataset)
            record=[dataset]
            record.extend([max(color.values())+1 for _,color in problem.parallel_coloring.items()])
            contents.append(record)
        headers=['dataset']
        headers.extend([strategy for strategy in problem.parallel_coloring])
        print(tabulate(contents,headers=headers,tablefmt='textfile'))
    
    @staticmethod
    def noise_exams_by_degree():
        contents=list()
        from tabulate import tabulate
        with open(os.path.join('','Statistics','carter_noise_exams_by_degree.out'),'w') as WF:
            WF.write('Dataset\nNoise_exams_by_degree\n')
            for dataset in tqdm(carterhandler.datasets):
                problem=Problem(dataset)
                record=[dataset]
                record.append(','.join([str(x) for x in problem.noisy_exams_by_component]))
                contents.append(record)
                WF.write(f"{dataset}\n{','.join([str(x) for x in problem.noisy_exams_by_component])}\n")
            headers=['dataset','noisy_exams']
        print(tabulate(contents,headers=headers,tablefmt='fancygrid'))

    @staticmethod
    def greedy_coloring():
        with open(os.path.join('','Statistics','greedy_coloring.csv'),'w') as WF:
            WF.write('Dataset,')
            WF.write(','.join(strategies)+'\n')
            for dataset in tqdm(carterhandler.datasets):
                problem=Problem(dataset)
                WF.write(dataset+',')
                WF.write(','.join([str(max(problem.parallel_coloring[strategy].values())+1) for strategy in strategies])+'\n')



class itchandler:
    datasets={
        "ITC2007_1":54,
        "ITC2007_2":40,
        "ITC2007_3":36,
        "ITC2007_4":21,
        "ITC2007_5":42,
        "ITC2007_6":16,
        "ITC2007_7":80,
        "ITC2007_8":80,
        "ITC2007_9":25,
        "ITC2007_10":32,
        "ITC2007_11":26,
        "ITC2007_12":12
    }

    @staticmethod
    def connected_components():
        contents=list()
        from tabulate import tabulate
        for dataset in tqdm(itchandler.datasets):
            problem=Problem(dataset)
            components=problem.get_connected_components()
            problem_significant_components=list()
            counter=1
            for component in components:
                if component.E>1:
                    problem_significant_components.append(('component_'+str(counter),component.E))
                counter+=1
            contents.append([dataset,','.join([index[0]+':'+str(index[1]) for index in problem_significant_components])])
        print(tabulate(contents,headers=['problem','significant components'],tablefmt='fancygrid'))
        with open(os.path.join('','Statistics','itc_significant_components.out'),'w') as WF:
            WF.write(tabulate(contents,headers=['problem','significant components'],tablefmt='textfile'))

    @staticmethod
    def identical_exams():
        with open(os.path.join('','Statistics','itc_identical_exams.out'),'w') as WF:
            for dataset in tqdm(itchandler.datasets):
                problem=Problem(dataset)
                _,ident_by_neighbors,_=problem.identical_exams()
                identical_info=''
                start=True
                for ident in  ident_by_neighbors:
                    if  start:
                        start=False
                        identical_info+='['+','.join([str(i) for i in ident])+']'
                    else:
                        identical_info+=',['+','.join([str(i) for i in ident])+']'
                WF.write(dataset,identical_info,'\n')
    
    @staticmethod
    def noisy_exams():
        contents=list()
        from tabulate import tabulate
        for dataset in tqdm(itchandler.datasets):
            problem=Problem(dataset)
            contents.append([dataset,'['+','.join([str(con) for con in problem.noisy_exams_by_students])+']','['+','.join([str(con) for con in problem.noisy_exams_by_component])+']','['+','.join([str(con) for con in problem.noisy_exams_by_degree])+']'])
        print(tabulate(contents,headers=['dataset','Noise by Student','Noise by component','noise by exam'],tablefmt='textfile'))
        with open(os.path.join('','Statistics','itc_noise_exams.out'),'w') as WF:
            WF.write(tabulate(contents,headers=['dataset','Noise by Student','Noise by component','noise by exam'],tablefmt='textfile'))

    @staticmethod
    def greedy_coloring():
        with open(os.path.join('','Statistics','greedy_coloring.csv'),'w') as WF:
            WF.write('Dataset,')
            WF.write(','.join(strategies)+'\n')
            for dataset in tqdm(itchandler.datasets):
                problem=Problem(dataset)
                WF.write(dataset+',')
                WF.write(','.join([str(max(problem.parallel_coloring[strategy].values())+1) for strategy in strategies])+'\n')
    
    @staticmethod
    def noise_exams_by_degree():
        contents=list()
        from tabulate import tabulate
        with open(os.path.join('','Statistics','itc_noise_exams_by_degree.out'),'w') as WF:
            WF.write('Dataset\nNoise_exams_by_degree\n')
            for dataset in tqdm(itchandler.datasets):
                problem=Problem(dataset)
                record=[dataset]

                record.append(','.join([str(x) for x in problem.noisy_exams_by_component]))
                contents.append(record)
                WF.write(f"{dataset}\n{','.join([str(x) for x in problem.noisy_exams_by_component])}\n")
            headers=['dataset','noisy_exams']
        print(tabulate(contents,headers=headers,tablefmt='fancygrid'))
        

class solution(Problem):
    def __init__(self,dataset):
        super().__init__(dataset)
        self.solutions={exam:-1 for exam in self.s_periods}
        self.moves_made=0
        self.p_periods={i:list() for i in range(self.P)}
        self.cost=0
        for exam,period in self.s_periods.items():
            self.schedule(exam,period)
        self.moves_simulation_buffer=dict()

    def cost_delta(self,exam,period):
        cs=0
        for neighbor in list(self.G.neighbors(exam)):
            distance=period-self.solutions[neighbor]
            if abs(distance)<=5 and self.solutions[exam]!=-1 and self.solutions[neighbor]!=-1:
                cs+=Problem.penalty[distance] * self.G[exam][neighbor]['weight']
        return cs

    def schedule(self,exam,period):
        self.p_periods[period].append(exam)
        self.solutions[exam]=period
        self.cost+=self.cost_delta(exam,period)
    
    def contribute_to_cost(self,exam):
        return len([1 for neighbor in list(self.G.neighbors(exam)) if(abs(self.solutions[exam]-self.solutions[neighbor])<=5)])!=0

    def random_exam(self):
        exam=random.choice(list(self.G.nodes))
        while not self.contribute_to_cost(exam):
            exam=random.choice(list(self.G.nodes))
        return exam

    def select_move(self):
        move_choice=random.randint(1,9)
        exam=self.random_exam()
        if move_choice==1:
            return self.move_exam(exam)
        elif move_choice==2:
            return self.swap_exams(exam)  
        elif move_choice==3:
            return self.swap_periods()
        elif move_choice==4:
            return self.shift_periods()
        elif move_choice==5:
            return self.kempe_chain(exam)
        elif move_choice==6:
            return self.kick_exams(exam)
        elif move_choice==7:
            return self.double_kick_exam(exam)
        elif move_choice==8:
            return self.double_kempe_chain(exam)
        elif move_choice==9:
            return self.swap_two_exams(exam)
    
    def select_specific_move(self,move_choice):
        exam=self.random_exam()
        if move_choice==1:
            return self.move_exam(exam)
        elif move_choice==2:
            return self.swap_exams(exam)  
        elif move_choice==3:
            return self.swap_periods()
        elif move_choice==4:
            return self.shift_periods()
        elif move_choice==5:
            return self.kempe_chain()
        elif move_choice==6:
            return self.kick_exams(exam)
        elif move_choice==7:
            return self.double_kick_exam(exam)
        elif move_choice==8:
            return self.double_kempe_chain(exam)

    def can_be_moved(self,exam,period,simulate_moves=None,exclude=list()):
        if simulate_moves==None:
            for neighbor in list(self.G.neighbors(exam)):
                if neighbor in exclude: continue
                if period==self.solutions[neighbor]:
                    return False
            return True
        else:
            for neighbor in list(self.G.neighbors(exam)):
                if neighbor in exclude: continue
                neighbor_state=simulate_moves[neighbor] if neighbor in simulate_moves else self.solutions[neighbor]
                if period==neighbor_state:
                    return False
            return True

    # def can_be_moved_extended(self,exam,period,simulate_moves,exclude=list()):
    #     for neighbor in list(self.G.neighbors(exam)):
    #         if neighbor in exclude: continue
    #         neighbor_state=simulate_moves[neighbor] if neighbor in simulate_moves else self.solutions[neighbor]
    #         if period==neighbor_state:
    #             return False
    #     return True

    def reposition(self,exam=-1,period=-1,moves=None):
        if moves==None and exam!=-1 and period!=-1:
            old_period=self.solutions[exam]
            self.p_periods[old_period].remove(exam)
            self.p_periods[period].append(exam)
            self.solutions[exam]=period
            
            cs=0
            for neighbor in list(self.G.neighbors(exam)):
                distance=abs(period-self.solutions[neighbor])
                distance_old=abs(old_period-self.solutions[neighbor])
                if distance==distance_old: continue
                if 1<=distance_old<=5:
                    cs-=Problem.penalty[distance_old] * self.G[exam][neighbor]['weight']
                if 1<=distance<=5:
                    cs+=Problem.penalty[distance] * self.G[exam][neighbor]['weight']
            self.cost+=cs
        elif moves!=None:
            cs=0
            for exam,period in moves.items():
                old_period=self.solutions[exam]
                self.p_periods[old_period].remove(exam)
                self.p_periods[period].append(exam)
                self.solutions[exam]=period
                for neighbor in list(self.G.neighbors(exam)):
                    distance=abs(period-self.solutions[neighbor])
                    distance_old=abs(old_period-self.solutions[neighbor])
                    if distance==distance_old: continue
                    if 1<=distance_old<=5:
                        cs-=Problem.penalty[distance_old] * self.G[exam][neighbor]['weight']
                    if 1<=distance<=5:
                        cs+=Problem.penalty[distance] * self.G[exam][neighbor]['weight']
            self.cost+=cs

    
    # @overload
    # def reposition(self,moves):
    #     for exam,period in moves.items():
    #         self.reposition(exam,period)

    # @overload
    # def pre_calculation(self,exam,period,cached=dict()):
    #     cs=0
    #     for neighbor in list(self.G.neighbors(exam)):
    #         neighbor_state=cached[neighbor] if neighbor in cached else self.solutions[neighbor]
    #         distance=abs(period-neighbor_state)
    #         distance_old=abs(self.solutions[exam]-neighbor_state)
    #         if distance==distance_old: continue
    #         if 1<=distance_old<=5:
    #             cs-=Problem.penalty[distance_old] * self.G[exam][neighbor]['weight']
    #         if 1<=distance<=5:
    #             cs+=Problem.penalty[distance] * self.G[exam][neighbor]['weight']
    #     return cs
    
    # @overload
    # def pre_calculation(self,exam,period,memory:dict,cached_moves):
    #     cs=0
    #     old_period=self.moves_simulation_buffer[exam] if exam in self.moves_simulation_buffer else self.solutions[exam]
    #     for neighbor in list(self.G.neighbors(exam)):
    #         neighbor_state=cached_moves[neighbor] if neighbor in cached_moves else self.moves_simulation_buffer[neighbor] if neighbor in self.moves_simulation_buffer else self.solutions[neighbor]
    #         distance=period-neighbor_state
    #         distance_old=old_period-neighbor_state
    #         if abs(distance)==abs(distance_old): continue
    #         if abs(distance_old)<=5:
    #             cs-=Problem.penalty[distance_old] * self.G[exam][neighbor]['weight']
    #         if 1<=abs(distance)<=5:
    #             cs+=Problem.penalty[distance] * self.G[exam][neighbor]['weight']
    #     return cs

    # @overload
    # def pre_reposition(self,moves:dict):
    #     print(type(moves))
    #     cs=0
    #     cached_moves=dict()
    #     for exam,period in moves.items():
    #         cs+=self.pre_calculation(exam,period,cached_moves)
    #         cached_moves[exam]=period
    #     return cs

    # @overload
    # def pre_reposition(self,moves:dict,memory:dict):
    #     cached_moves=dict()
    #     cs=0
    #     for exam,period in moves.items():
    #         cs+=self.pre_calculation(exam,period,cached_moves)
    #         cached_moves[exam]=period
    #     return cs
    
    # @overload
    # def pre_reposition(self,exam:int,period:int,exclude:list):
    #     cs=0
    #     for neighbor in list(self.G.neighbors(exam)):
    #         distance=abs(period-self.solutions[neighbor])
    #         distance_before=abs(self.solutions[exam]-self.solutions[neighbor])
    #         if distance==distance_before: continue
    #         if 1<=distance_before<=5:
    #             cs-=Problem.penalty[distance_before] * self.G[exam][neighbor]['weight']
    #         if 1<=distance<=5:
    #             cs+=Problem.penalty[distance] * self.G[exam][neighbor]['weight']
    #     return cs
    
    def pre_reposition(self,exam=-1,period=-1,moves=None,simulated_moves=None,exclude=list()):
        if simulated_moves==None:
            if moves==None:
                if exam==-1 or period==-1:
                    raise Exception("No valid function called exam pre_repositon")
                cs=0
                for neighbor in list(self.G.neighbors(exam)):
                    if neighbor in exclude: continue
                    distance=abs(period-self.solutions[neighbor])
                    distance_before=abs(self.solutions[exam]-self.solutions[neighbor])
                    if distance==distance_before: continue
                    if 1<=distance_before<=5:
                        cs-=Problem.penalty[distance_before] * self.G[exam][neighbor]['weight']
                    if 1<=distance<=5:
                        cs+=Problem.penalty[distance] * self.G[exam][neighbor]['weight']
                return cs
            else:
                cs=0
                cached_moves=dict()
                for exam,period in moves.items():
                    for neighbor in list(self.G.neighbors(exam)):
                        if neighbor in exclude: continue
                        neighbor_state=cached_moves[neighbor] if neighbor in cached_moves else self.solutions[neighbor]
                        distance=abs(period-neighbor_state)
                        distance_before=abs(self.solutions[exam]-neighbor_state)
                        if distance==distance_before: continue
                        if 1<=distance_before<=5:
                            cs-=Problem.penalty[distance_before] * self.G[exam][neighbor]['weight']
                        if 1<=distance<=5:
                            cs+=Problem.penalty[distance] * self.G[exam][neighbor]['weight']
                    cached_moves[exam]=period
                return cs
        else:
            if moves==None:
                if exam==-1 or period==-1:
                    raise Exception("Not a valid move to be done")
                else:
                    cs=0
                    old_period=simulated_moves[exam] if exam in simulated_moves else self.solutions[exam]
                    for neighbor in list(self.G.neighbors(exam)):
                        if neighbor in exclude: continue
                        neighbor_state=simulated_moves[neighbor] if neighbor in simulated_moves else self.solutions[neighbor]
                        distance=abs(period-neighbor_state)
                        distance_old=abs(old_period-neighbor_state)
                        if distance==distance_old: continue
                        if 1<=distance_old<=5:
                            cs-=Problem.penalty[distance_old] * self.G[exam][neighbor]['weight']
                        if 1<=distance<=5:
                            cs+=Problem.penalty[distance] * self.G[exam][neighbor]['weight']        
                    return cs
            else:
                cs=0
                cached_moves=dict()
                for exam,period in moves.items():
                    old_period=self.moves_simulation_buffer[exam] if exam in simulated_moves else self.solutions[exam]
                    for neighbor in list(self.G.neighbors(exam)):
                        if neighbor in exclude: continue
                        neighbor_state=cached_moves[neighbor] if neighbor in cached_moves else simulated_moves[neighbor] if neighbor in simulated_moves else self.solutions[neighbor]
                        distance=abs(period-neighbor_state)
                        distance_old=abs(old_period-neighbor_state)
                        if distance==distance_old: continue
                        if distance_old in Problem.penalty:
                            cs-=Problem.penalty[distance_old] * self.G[exam][neighbor]['weight']
                        if distance in Problem.penalty:
                            cs+=Problem.penalty[distance] * self.G[exam][neighbor]['weight']
                    cached_moves[exam]=period
                return cs

            

    def move_exam(self,exam):
        for p in range(self.P):
            if self.can_be_moved(exam,p)==True:
                return {exam:p}
        return dict()
    
    def swap_exams(self,exam):
        for neighbor in list(self.G.neighbors(exam)):
            period1=self.solutions[neighbor]
            period2=self.solutions[exam]
            if self.can_be_moved(exam,period1,None,{neighbor})==True and self.can_be_moved(neighbor,period2,None,{exam})==True:
                return {
                    exam:period1,
                    neighbor:period2
                }
        return dict()

    def swap_periods(self):
        p1=random.randint(0,self.P-1)
        p2=random.randint(0,self.P-1)
        moves=dict()
        while p1==p2:
            p2=random.randint(0,self.P-1)
        for exam in self.p_periods[p1]:
            moves[exam]=p2
        for exam in self.p_periods[p2]:
            moves[exam]=p1
        return moves
    
    def shift_periods(self):
        p1=random.randint(0,self.P-1)
        p2=(p1+random.randint(1,5))%self.P
        i_period=p1+1
        moves=dict()
        while True:
            for exam in self.p_periods[i_period%self.P]:
                moves[exam]=(i_period-1)%self.P
            if i_period%self.P==p2:
                break
            i_period+=1
        for exam in self.p_periods[p1]:
            moves[exam]=p2
        return moves
    
    def kempe_chain(self,exam):
        neighbor=random.choice(list(self.G.neighbors(exam)))
        versus_period={
          self.solutions[exam]:self.solutions[neighbor],
          self.solutions[neighbor]:self.solutions[exam]
        }
        kc=LifoQueue()
        moves=dict()
        kc.put(exam)
        while kc.empty()==False:
            current_exam=kc.get()
            current_period=self.solutions[current_exam]
            new_period=versus_period[current_period]
            moves[current_exam]=new_period
            for neighbor in list(self.G.neighbors(current_exam)):
                if neighbor in moves: continue
                if self.solutions[neighbor]==new_period:
                    kc.put(neighbor)
        return moves
    
    def kick_exams(self,exam):
        for neighbor in list(self.G.neighbors(exam)):
            period1=self.solutions[neighbor]
            if self.can_be_moved(exam,period1,None,{neighbor})==True:
                for p in range(self.P):
                    if p==period1 or p==self.solutions[exam]: continue
                    if self.can_be_moved(neighbor,p)==True:
                        return {
                            exam:period1,
                            neighbor:p
                        }
        return dict()
    
    def double_kick_exam(self,exam):
        for neighbor in list(self.G.neighbors(exam)):
            period1=self.solutions[neighbor]
            if self.can_be_moved(exam,period1,None,{neighbor})==True:
                for neighbor2 in list(self.G.neighbors(neighbor)):
                    period2=self.solutions[neighbor2]
                    if neighbor2==exam: continue
                    if self.can_be_moved(neighbor,period2,None,{neighbor2}):
                        for p in range(self.P):
                            if p==period1 or p==period2  or p==self.solutions[exam]: continue
                            if self.can_be_moved(neighbor2,p)==True:
                                return {
                                    exam:period1,
                                    neighbor:period2,
                                    neighbor2:p
                                }
        return dict()

    def eject_vertices(self,best,best_sol,**kwargs):
        logging.basicConfig(level=logging.INFO,format="%(asctime)s\t%(message)s")
        mean_weight=stats.mean([self.G[node_1][node_2]['weight'] for node_1,node_2 in list(self.G.edges)])
        weight_acceptance_propability=random.uniform(0.7,0.9)
        weight_acceptance_value=weight_acceptance_propability * mean_weight
        edges_to_examine=[(node_1,node_2) for node_1,node_2 in list(self.G.edges) if self.G[node_1][node_2]['weight']>=weight_acceptance_value] 
        for node_1,node_2 in edges_to_examine:
            best_period_edge_cost=sys.maxsize
            best_moves_will_be_made=dict()
            for p in range(self.P):
                moves=dict()
                best_move_cost=sys.maxsize
                if self.can_be_moved(node_1,p,None,{node_2}):
                    move_cost=self.pre_reposition(node_1,p,None,None)
                    if move_cost<best_move_cost:
                        best_move_cost=move_cost
                        moves[node_1]=p
                    period=moves[node_1]
                    accepted_periods=list()
                    for i_period in range(self.P):
                        if i_period==period: continue
                        if i_period<period-5 or i_period>period+5:
                            accepted_periods.append(i_period) 
                    if len(accepted_periods)==0:
                        best_pair=dict()
                        best_simulate_cost=sys.maxsize
                        periods=list()
                        left_bound=period-5 if period-5>0 else 0
                        right_bound=period+5 if period+5<self.P-1 else self.P-1
                        periods=list(range(left_bound,right_bound+1))
                        for p in periods:
                            if p==period: continue
                            if self.can_be_moved(node_2,p,None,None,{node_1}):
                                move_cost=self.pre_reposition(node_2,p,None,moves)
                                if move_cost<best_simulate_cost:
                                    best_simulate_cost=move_cost
                                    best_pair={node_2:p}
                        if len(best_pair)==0: 
                            continue
                        moves.update(best_pair)
                        move_total_cost=self.cost+self.pre_reposition(-1,-1,None,moves)
                        if move_total_cost<best_period_edge_cost:
                            best_period_edge_cost=move_total_cost
                            best_moves_will_be_made=moves
                            
                    else:
                        best_contribution=sys.maxsize
                        best_pair=dict()
                        for period_id in accepted_periods:
                            if p==period: continue
                            if self.can_be_moved(node_2,period_id,None,moves):
                                move_cost=self.pre_reposition(node_2,period_id,None,moves)
                                if move_cost<best_contribution:
                                    best_contribution=move_cost
                                    best_pair={node_2:period}
                        if len(best_pair)==0: 
                            best_simulate_cost=sys.maxsize
                            left_bound=period-5 if period-5>0 else 0
                            right_bound=period+5 if period+5<self.P-1 else self.P-1
                            periods=list(range(left_bound,right_bound+1))
                            for p in periods:
                                if p==period: continue
                                if self.can_be_moved(node_2,p,None,{node_1}):
                                    move_cost=self.pre_reposition(node_2,p,None,moves)
                                    if move_cost<best_simulate_cost:
                                        best_simulate_cost=move_cost
                                        best_pair={node_2:p}
                            if best_pair==dict(): 
                                continue
                            moves.update(best_pair)
                            move_total_cost=self.cost+self.pre_reposition(-1,-1,moves)
                            if move_total_cost<best_period_edge_cost:
                               best_period_edge_cost=move_total_cost
                               best_moves_will_be_made=moves
                            continue
                        
                        moves.update(best_pair)
                        print(moves)
                        move_cost=self.cost+self.pre_reposition(-1,-1,moves)
                        if move_cost<best_period_edge_cost:
                               best_period_edge_cost=move_cost
                               best_moves_will_be_made=moves

            if len(best_moves_will_be_made)==0:
                continue
            else:
                if best_period_edge_cost<best:
                    self.reposition(-1,-1,best_moves_will_be_made)
                    best=self.cost
                    best_sol=self.solutions
                    assert(best_period_edge_cost==self.cost)
                    time_needed=time()-kwargs['start_time']
                    logging.info(f"Eject Vertices|New best Solution S={best} Time needed:{time_needed}")
        return best,best_sol

    def double_kempe_chain(self,exam):
        exam2=self.random_exam()
        while exam==exam2 or exam2 in list(self.G.neighbors(exam)):
            exam2=self.random_exam()
        neighbor=random.choice(list(self.G.neighbors(exam)))
        neighbor2=random.choice(list(self.G.neighbors(exam2)))
        period1=self.solutions[exam]
        period2=self.solutions[neighbor]
        period3=self.solutions[exam2]
        period4=self.solutions[neighbor2]
        if period3 in [period1,period2] or period4 in [period1,period2]: return dict()
        kc=LifoQueue()
        kc.put(exam)
        versus_period={period1:period2,period2:period1}
        moves=dict()
        while not kc.empty():
            current_exam=kc.get()
            current_period=self.solutions[current_exam]
            new_period=versus_period[current_period]
            moves[current_exam]=new_period
            for neighbor in list(self.G.neighbors(current_exam)):
                if neighbor in moves: continue
                if self.solutions[neighbor]==new_period: 
                    kc.put(neighbor)
        del kc
        del versus_period
        kc=LifoQueue()
        kc.put(exam2)
        versus_period={period3:period4,period4:period3}
        while not kc.empty():
            current_exam=kc.get()
            current_period=self.solutions[current_exam]
            new_period=versus_period[current_period]
            moves[current_exam]=new_period
            for neighbor in list(self.G.neighbors(current_exam)):
                if neighbor in moves: continue
                if self.solutions[neighbor]==new_period: 
                    kc.put(neighbor)
        return moves
    
    def swap_two_exams(self,exam):
        exam2=self.random_exam()
        while exam==exam2 or exam2 in list(self.G.neighbors(exam)):
            exam2=self.random_exam()
        neighbor=random.choice(list(self.G.neighbors(exam)))
        neighbor2=random.choice(list(self.G.neighbors(exam2)))
        if exam2 in [exam,neighbor]: return dict()
        period1=self.solutions[neighbor]
        period2=self.solutions[exam]
        period3=self.solutions[neighbor2]
        period4=self.solutions[exam2]
        moves=dict()
        if self.can_be_moved(exam,period1,None,{exam2})==True and self.can_be_moved(exam2,period2,None,{exam})==True:
            moves[exam]=period1
            moves[exam2]=period2
        if neighbor==neighbor2: return dict()
        if self.can_be_moved(exam2,period3,None,{neighbor2})==True and self.can_be_moved(neighbor2,period4,None,{exam2})==True:
            moves[exam2]=period3
            moves[neighbor2]=period4    
        return moves    

    def permuting_periods(self,best,best_sol):
        logging.basicConfig(level=logging.INFO,format='%(asctime)s\t%(message)s')
        start_timer=time()
        while True:
            for p1 in range(self.P):
                best_contribution=self.cost
                best_exam_contribution=dict()
                for p2 in range(p1+1,self.P):
                    moves=dict()
                    for exam in self.p_periods[p1]:
                        moves[exam]=p2
                    for exam in self.p_periods[p2]:
                        moves[exam]=p1
                    move_cost=self.cost+self.pre_reposition(-1,-1,moves,None)
                    if move_cost<best_contribution:
                        best_contribution=move_cost
                        best_exam_contribution=moves
                if len(best_exam_contribution)==0: continue
                if best_contribution<best:
                    logging.info("Permuting periods| New best solution S={}".format(best_contribution))
                    self.reposition(-1,-1,best_exam_contribution)
                    best=self.cost
                    best_sol=self.solutions
            if time()-start_timer<10:
                break
        return best,best_sol
    
    def polish(self,best,best_sol,execution_time):
        start_timer=time()
        logging.basicConfig(level=logging.INFO,format="%(asctime)s\t%(message)s")
        exams=list(self.G.nodes)
        while True:
            #Polishing move_exam
            for exam in exams:
                for p in range(self.P):
                    if self.can_be_moved(exam,p):
                        move_cost=self.cost+self.pre_reposition(exam,p)
                        if move_cost<best:  
                            self.reposition(exam,p)
                            best=self.cost
                            best_sol=self.solutions
                            logging.info("Polishing Move exam| New best solution found S={}\Move[{}->{}]".format(best,exam,p))
            
            if time()-start_timer>execution_time:
                break

            #Polishing swap exams
            for exam in exams:
                best_contribution_to_cost=sys.maxsize
                moves_will_be_made=dict()
                period2=self.solutions[exam]
                for neighbor in list(self.G.neighbors(exam)):
                    period1=self.solutions[neighbor]
                    if self.can_be_moved(exam,period1,{neighbor}) and self.can_be_moved(neighbor,period2,{exam}):
                        moves={
                            exam,period1,
                            neighbor,period2
                        }
                        move_cost=self.cost+self.pre_reposition(-1,-1,moves)
                        if move_cost<best_contribution_to_cost:
                            best_contribution_to_cost=move_cost
                            moves_will_be_made=moves
                if len(moves_will_be_made)==0:
                    continue
                contribution_of_possible_move=self.cost+best_contribution_to_cost
                if contribution_of_possible_move<best:
                    self.reposition(moves_will_be_made)
                    best=self.cost
                    best_sol=self.solutions
                    logging.info("Polishing Swap Exams|New best solution found S={}".format(best))
            
            if time()-start_timer>execution_time:
                break

            #Polishing Kempe Chain
            for exam in exams:
                best_contribution_to_cost=sys.maxsize
                moves_will_be_made=dict()
                period1=self.solutions[exam]
                for neighbor in list(self.G.neighbors):
                    period2=self.solutions[neighbor]
                    moves=dict()
                    versus_periods={
                        period1:period2,
                        period2:period1
                    }
                    kc=LifoQueue()
                    kc.put(exam)
                    while not kc.empty():
                        current_exam=kc.get()
                        current_period=self.solutions[current_exam]
                        new_period=versus_periods[current_period]
                        moves[current_exam]=new_period
                        for neighbor in list(self.G.neighbors(current_exam)):
                            if neighbor in moves:
                                continue
                            if self.solutions[neighbor]==new_period:
                                kc.put(neighbor)
                    possible_cost=self.pre_reposition(-1,-1,moves)
                    if possible_cost<best_contribution_to_cost:
                        best_contribution_to_cost=possible_cost
                        moves_will_be_made=moves
                if best_contribution_to_cost<best:
                    self.reposition(-1,-1,moves_will_be_made)
                    best=self.cost
                    best_sol=self.solutions
                    logging.info("Polish Kempe Chain|New best solution S={}".format(best))

            if time()-start_timer>execution_time:
                break

            #Polishing Kick Exams
            for exam in exams:
                for neighbor in list(self.G.neighbors(exam)):
                    period1=self.solutions[neighbor]
                    if self.can_be_moved(exam,period1,{neighbor}):
                        for p in range(self.P):
                            if p==period1 or p==self.solutions[exam]: continue
                            if self.can_be_moved(neighbor,p):
                                moves={
                                    exam:period1,
                                    neighbor:p
                                }
                                cost_prediction=self.cost+self.pre_reposition(-1,-1,moves)
                                if cost_prediction<best:
                                    self.reposition(moves)
                                    best=self.cost
                                    best_sol=self.solutions
                                    logging.info("Polish kick exam|New best solution S={}".format(best))

            if time()-start_timer>execution_time:
                break

            #Polishing double kick exam
            for exam in exams:
                for neighbor in list(self.G.neighbors(exam)):
                    period1=self.solutions[neighbor]
                    if self.can_be_moved(exam,period1,{neighbor}):
                        for neighbor2 in list(self.G.neighbors(neighbor)):
                            if neighbor2==exam: continue
                            period2=self.solutions[neighbor2]
                            if self.can_be_moved(neighbor,period2,{neighbor2}):
                                for p in range(self.P):
                                    if p in [period1,period2,self.solutions[exam]]: continue
                                    moves={
                                        exam:period1,
                                        neighbor:period2,
                                        neighbor2:p
                                    }
                                    cost_prediction=self.cost+self.pre_reposition(-1,-1,moves)
                                    if cost_prediction<best:
                                        self.reposition(moves)
                                        best=self.cost
                                        best_sol=self.solutions
                                        logging.info("Polishing Double Kick Exam|New best solution S={}".format(best))
            
            if time()-start_timer>execution_time:
                break
        return best,best_sol
                            

    # def parallel_moves_execution(self,move,exam):
    #     if move==1:
    #         best_cost_contribution=self.simulated_cost
    #         for p in range(self.P):
    #             if self.can_be_moved(exam,p):
    #                 move_cost=self.simulated_cost+self.pre_reposition(exam,p)
    #                 if move_cost<best_contribution:
    #                     best_contribution=move_cost
    #                     self.parallel_depth_moves[move]=({exam:p},best_contribution)
        
    #     elif move==2:
    #         period1=self.moves_simulation_buffer[exam] if exam in self.moves_simulation_buffer else self.solutions[exam]
    #         for exam2 in list(self.G.neighbors(exam)):
    #             period2=self.moves_simulation_buffer[exam2] if exam2 in self.moves_simulation_buffer else self.solutions[exam2]
    #             best_contribution=self.simulated_cost
    #             if self.can_be_moved(exam,period2,None,{exam2}) and self.can_be_moved(exam,period1,None,{exam}):
    #                 move_contribution=self.simulated_cost+self.pre_reposition(-1,-1,{exam:period2,exam2:period1})
    #                 if move_contribution<best_contribution:
    #                     best_contribution=move_contribution
    #                     best_moves_made={
    #                         exam:period2,
    #                         exam2:period1
    #                     }
    #         self.parallel_depth_moves[move]=(best_moves_made,best_contribution)
        
    #     elif move==3:
    #         best_moves_made=dict()
    #         best_cost_contribution=self.simulated_cost
    #         period1=self.moves_simulation_buffer[exam] if exam in self.moves_simulation_buffer else self.solutions[exam]
    #         for exam2 in list(self.G.neighbors(exam)):
    #             period2=self.moves_simulation_buffer[exam2] if exam2 in self.moves_simulation_buffer else self.solutions[exam2]
    #             versus_period={
    #                 period1:period2,
    #                 period2:period1
    #             }
    #             moves=dict()
    #             kc=LifoQueue()
    #             kc.put(exam)
    #             while not kc.empty():
    #                 current_exam=kc.get()
    #                 current_period=self.moves_simulation_buffer[current_exam] if current_exam in self.moves_simulation_buffer else self.solutions[current_exam]
    #                 new_period=versus_period[current_period]
    #                 moves[exam]=new_period
    #                 for neighbor in list(self.G.neighbors(current_exam)):
    #                     if neighbor in moves: continue
    #                     neighbor_state=self.moves_simulation_buffer[neighbor] if neighbor in self.moves_simulation_buffer else self.solutions[neighbor]
    #                     if neighbor_state==new_period:
    #                         kc.put(neighbor)
    #             cost_contribution=self.simulated_cost+self.pre_reposition(-1,-1,moves)
    #             if cost_contribution<best_contribution:
    #                 best_contribution=cost_contribution
    #                 best_moves_made=moves
    #         self.parallel_depth_moves[move]=(best_moves_made,best_contribution)
        
    #     elif move==4:
    #         best_moves_made=dict()
    #         best_cost_contribution=self.simulated_cost
    #         for exam2 in list(self.G.neighbors(exam)):
    #             period1=self.moves_simulation_buffer[exam2] if exam2 in self.moves_simulation_buffer else self.solutions[exam2]
    #             if self.can_be_moved(exam,period1,None,{exam2}):
    #                 for p in range(self.P):
    #                     if p==period1: 
    #                         continue
    #                     if self.can_be_moved(exam2,p,None,{exam}):
    #                         moves={exam:period1,exam2:p}
    #                         move_contribution=self.simulated_cost+self.pre_reposition(-1,-1,moves)
    #                         if move_contribution<best_cost_contribution:
    #                             best_cost_contribution=move_contribution
    #                             best_moves_made=moves
    #         self.parallel_depth_moves[move]=(best_moves_made,best_cost_contribution)
        
    #     elif move==5:
    #         best_moves_made=dict()
    #         best_cost_contribution=self.simulated_cost
    #         for exam2 in list(self.G.neighbors(exam)):
    #             period1=self.moves_simulation_buffer[exam2] if exam2 in self.moves_simulation_buffer else self.solutions[exam2]
    #             if self.can_be_moved(exam,period1,None,{exam2}):
    #                 for exam3 in list(self.G.neighbors(exam2)):
    #                     if exam3==exam: continue
    #                     period2=self.moves_simulation_buffer[exam3] if exam3 in self.moves_simulated_buffer else self.solutions[exam3]
    #                     if self.can_be_moved(exam2,period2,None,{exam3}):
    #                         for p in range(self.P):
    #                             if p==period1 or p==period2: continue
    #                             if self.can_be_moved(exam3,p):
    #                                 moves={
    #                                     exam:period1,
    #                                     exam2:period2,
    #                                     exam3:p
    #                                 }
    #                                 move_contribution=self.simulated_cost+self.pre_reposition(-1,-1,moves)
    #                                 if move_contribution<best_cost_contribution:
    #                                     best_cost_contribution=move_contribution
    #                                     best_moves_made=moves
    #         self.parallel_depth_moves[move]=(best_moves_made,best_cost_contribution)

    # def depth_moves(self,best,best_sol):
    #     self.moves_simulation_buffer=dict()
    #     depth=4
    #     self.simulated_cost=self.cost
    #     for _ in range(depth):
    #         best_contribution=self.simulated_cost
    #         self.parallel_depth_moves=dict()
    #         moves_will_be_executed=dict()
    #         exam=self.random_exam()
    #         with ThreadPoolExecutor(max_workers=os.cpu_count()) as exec:
    #             exec.map(self.parallel_moves_execution,[(1,exam),(2,exam),(3,exam),(4,exam),(5,exam)])
    #         best_objective=sys.maxsize
    #         moves_will_be_made=dict()
    #         for _,smoves,scost in self.parallel_depth_moves.items():
    #                 if scost<best_contribution:
    #                     best_contribution=scost
    #                 if len(smoves)==0: continue
    #                 if scost<best_objective:
    #                     best_objective=scost
    #                     moves_will_be_made=smoves
    #         if len(moves_will_be_made)!=0:
    #             for exam,period in moves_will_be_made.items():
    #                 self.moves_simulation_buffer[exam]=period
    #             self.simulated_cost=best_objective
    #     depth_moves_cost=self.cost+self.pre_reposition(-1,-1,moves_will_be_made,self.moves_simulation_buffer)
    #     if depth_moves_cost<best:
    #         self.reposition(-1,-1,self.moves_simulation_buffer)
    #         best=self.cost
    #         best_sol=self.s_periods
    #     return best,best_sol

def simulated_annealing(exec_time,dataset):
    logging.basicConfig(level=logging.INFO,format='%(asctime)s \t %(message)s')
    temp=1000
    start_temp=1000
    alpha=0.9999
    freeze=0.0001
    plateu=0
    solution_improvement_made=False
    temp_delay_counter=DEF_DATA.BEST_SOL_ITERATIONS
    s=solution(dataset)
    best=s.cost
    round=1
    best_sol=s.solutions
    start_timer=time()
    number_of_reheats=3
    best,best_sol=s.eject_vertices(best,best_sol,start_time=start_timer) 
    while True:
        moves=s.select_move()
        if len(moves)==0:
            continue
        pcost=s.cost
        rollback=dict()
        for exam in moves:
            rollback[exam]=s.solutions[exam]
        s.reposition(-1,-1,moves)
        delta=s.cost-pcost
        if delta<0:
            if s.cost<best:
                best=s.cost
                best_sol=deepcopy(s.solutions)
                plateu=0
                solution_improvement_made=True
                logging.info(f"Simulated Annealing|New best solution found:S={best} T={temp}")
        elif delta>0:
            acceptance_propability=math.exp(-delta/temp)
            if acceptance_propability>random.random():
                pass
            else:
                s.reposition(-1,-1,rollback)

        if temp_delay_counter>0:
            temp_delay_counter-=1
        else:
            temp*=alpha
        if temp<freeze:
            if solution_improvement_made==True and number_of_reheats>0:
                solution_improvement_made=False
                number_of_reheats-=1
                temp=DEF_DATA.REHEAT_TEMPERATURE
                logging.info('Simulated Annealing|Reheating temperature-New value T={}'.format(temp))
                continue
            
            previous_best=best
            best,best_sol=s.polish(best,best_sol,80)
            if previous_best>best:
                logging.info('Simulated Annealing|New best solution found S={} T={}'.format(best,temp))
                plateu=0
                continue
            plateu+=1
            round+=1
            temp=random.uniform(1.2,2.0) * start_temp
            logging.info("New round started:R={} T={}".format(round,temp))
            number_of_reheats=3
            if plateu==10:
                logging.info('Simulated Annealing| After {plateu} iterations no improvement made,canceling procedure')
                break
            if plateu==20:
                break
        if time()-start_timer>exec_time:
            break
    s.reposition(-1,-1,best_sol)
    logging.info('Simulated Annealing| After {} seconds of execution best solution found S={}'.format(exec_time,s.cost))
    s.renew_solution(best_sol)
    # print(str(s.is_feasible()))


class psolution(Problem):
    def __init__(self,dataset):
        super().__init__(dataset)
        self.p_periods={i:list() for i in range(self.P)}
        self.solutions={exam:-1 for exam in list(self.G.nodes)}
        self.cost=0
        for exam,period in self.s_periods.items():
            self.schedule(exam,period)
    
    def cost_delta(self,exam,period):
        cs=0
        for neighbor in list(self.G.neighbors(exam)):
            distance=period-self.solutions[neighbor]
            if abs(distance)<=5 and self.solutions[exam]!=-1 and self.solutions[neighbor]!=-1:
                cs+=self.G[exam][neighbor]['weight'] * Problem.penalty[distance]
        return cs

    def schedule(self,exam,period):
        self.solutions[exam]=period
        self.p_periods[period].append(exam)
        self.cost+=self.cost_delta(exam,period)
    
    def re_schedule(self,exam,period):
        old_period=self.solutions[exam]
        self.solutions[exam]=period
        self.p_periods[old_period].remove(exam)
        self.p_periods[period].append(exam)
    
    def can_be_moved(self,exam,period,exclude=list()):
        for neighbor in self.G.neighbors(exam):
            if neighbor in exclude: continue
            if period==self.solutions[neighbor]:
                return False
        return True

    def pre_reposition(self,exam=-1,period=-1,moves=None):
        if moves==None:
            if exam==-1 or period==-1: raise Exception("Not a move inserted")
            cs=0
            for neighbor in list(self.G.neighbors(exam)):
                distance=abs(period-self.solutions[neighbor])
                distance_before=abs(self.solutions[exam]-self.solutions[neighbor])
                if distance==distance_before: continue
                if distance_before in Problem.penalty:
                    cs-=Problem.penalty[distance_before] * self.G[exam][neighbor]['weight']
                if distance in Problem.penalty:
                    cs+=Problem.penalty[distance] * self.G[exam][neighbor]['weight']
            return cs
        else:
            cached_moves=dict()
            cs=0
            for mexam,mperiod in moves.items():
                for neighbor in list(self.G.neighbors(mexam)):
                    neighbor_state=cached_moves[neighbor] if neighbor in cached_moves else self.solutions[neighbor]
                    distance=abs(mperiod-neighbor_state)
                    distance_before=abs(self.solutions[mexam]-neighbor_state)
                    if distance==distance_before:
                        continue
                    if distance_before in Problem.penalty:
                        cs-=Problem.penalty[distance_before] * self.G[mexam][neighbor]['weight']
                    if distance in Problem.penalty:
                        cs+=Problem.penalty[distance] * self.G[mexam][neighbor]['weight']
                cached_moves[mexam]=mperiod
            return cs
    
    def reposition(self,exam=-1,period=-1,moves=None):
        if moves==None:
            if exam==-1 or period==-1: raise Exception("Not a valid reposition")
            old_period=self.solutions[exam]
            self.re_schedule(exam,period)
            cs=0
            for neighbor in list(self.G.neighbors(exam)):
                distance=abs(period-self.solutions[neighbor])
                distance_before=abs(old_period-self.solutions[neighbor])
                if distance==distance_before: continue
                if distance_before in Problem.penalty:
                    cs-=self.G[exam][neighbor]['weight'] * Problem.penalty[distance_before]
                if distance in Problem.penalty:
                    cs+=self.G[exam][neighbor]['weight'] * Problem.penalty[distance]
            self.cost+=cs
        else:
            cs=0
            for mexam,mperiod in moves.items():
                old_period=self.solutions[mexam]
                self.re_schedule(mexam,mperiod)
                for neighbor in list(self.G.neighbors(mexam)):
                    distance=abs(mperiod-self.solutions[neighbor])
                    distance_before=abs(old_period-self.solutions[neighbor])
                    if distance==distance_before: continue
                    if distance_before in Problem.penalty:
                        cs-=self.G[mexam][neighbor]['weight'] * Problem.penalty[distance_before]
                    if distance in Problem.penalty:
                        cs+=self.G[mexam][neighbor]['weight'] * Problem.penalty[distance]
            self.cost+=cs
    

    def select_move(self,pass_info):
        i=pass_info[0]
        exam=pass_info[1]
        lock_data=Lock()
        with lock_data:
            if i==1:
                for p in range(self.P):
                    if self.can_be_moved(exam,p):
                        return {exam:p},self.pre_reposition(exam,p)
                return ({},sys.maxsize)
            elif i==2:
                neighbor=random.choice(list(self.G.neighbors(exam)))
                period1=self.solutions[neighbor]
                period2=self.solutions[exam]
                if self.can_be_moved(exam,period1,{neighbor}) and self.can_be_moved(neighbor,period2,{exam}):
                    moves={
                        exam:period1,
                        neighbor:period2
                    }
                    return moves,self.pre_reposition(-1,-1,moves) 
                return ({},sys.maxsize)
            
            elif i==3:
                neighbor=random.choice(list(self.G.neighbors(exam)))
                p1=self.solutions[exam]
                p2=self.solutions[neighbor]
                versus_period={
                    p1:p2,
                    p2:p1
                }
                kc=LifoQueue()
                kc.put(exam)
                kc_moves=dict()
                while not kc.empty():
                    current_exam=kc.get()
                    current_period=self.solutions[current_exam]
                    new_period=versus_period[current_period]
                    kc_moves[current_exam]=new_period
                    for neighbor in list(self.G.neighbors(current_exam)):
                        if neighbor in kc_moves: continue
                        if self.solutions[neighbor]==new_period:
                            kc.put(neighbor)
                return kc_moves,self.pre_reposition(-1,-1,kc_moves)
            
            elif i==4:
                neighbor=random.choice(list(self.G.neighbors(exam)))
                period1=self.solutions[neighbor]
                if self.can_be_moved(exam,period1,{neighbor}):
                    for p in range(self.P):
                        if p==period1 or p==self.solutions[exam]: continue
                        if self.can_be_moved(neighbor,p):    
                            moves={
                                exam:period1,
                                neighbor:p
                            }
                            return moves,self.pre_reposition(-1,-1,moves) 
                return ({},sys.maxsize)
            
            elif i==5:
                neighbor=random.choice(list(self.G.neighbors(exam)))
                period1=self.solutions[neighbor]
                if self.can_be_moved(exam,period1,{neighbor}):
                    for neighbor2 in list(self.G.neighbors(neighbor)):
                        if neighbor2==exam: continue
                        period2=self.solutions[neighbor2]
                        if self.can_be_moved(neighbor,period2,{neighbor2}):
                            for p in range(self.P):
                                if p==period1 or p==period2 or self.solutions[exam]==p: continue
                                if self.can_be_moved(neighbor2,p):
                                    moves={
                                        exam:period1,
                                        neighbor:period2,
                                        neighbor2:p
                                    }
                                    return moves,self.pre_reposition(-1,-1,moves)
                return ({},sys.maxsize)

            elif i==6:
                exam1=self.random_exam()
                exam2=self.random_exam()
                while  exam2 in list(self.G.neighbors(exam)) or exam1==exam2:
                    exam2=self.random_exam()
                neighbor1=random.choice(list(self.G.neighbors(exam1)))
                neighbor2=random.choice(list(self.G.neighbors(exam2)))
                period1=self.solutions[exam1]
                period2=self.solutions[neighbor1]
                period3=self.solutions[exam2]
                period4=self.solutions[neighbor2]
                if period3 in [period1,period2] or period4 in [period1,period2]:
                    return ({},sys.maxsize)
                
                move_type={
                    "swap_exams":0,
                    "kempe_chain":0
                }

                #try swap exams
                moves=dict()
                if self.can_be_moved(exam1,period2,{neighbor1}) and self.can_be_moved(neighbor1,period1,{exam1}):
                    moves[exam1]=period2
                    moves[neighbor1]=period1
                if self.can_be_moved(exam2,period4,{neighbor2}) and self.can_be_moved(neighbor2,period3,{exam}):
                    moves[exam2]=period4 
                    moves[neighbor2]=period3
                move_type["swap_exams"]=(moves,self.pre_reposition(-1,-1,moves))

                #try kempe
                moves=dict()
                versus_period={
                    period1:period2,
                    period2:period1
                }
                kc=LifoQueue()
                kc.put(exam1)
                while not kc.empty():
                    current_exam=kc.get()
                    current_period=self.solutions[current_exam]
                    new_period=versus_period[current_period]
                    moves[current_exam]=new_period
                    for neighbor in list(self.G.neighbors(current_exam)):
                        if neighbor in moves: continue
                        if self.solutions[neighbor]==new_period:
                            kc.put(neighbor)
                del kc
                del versus_period
                versus_period={
                    period3:period4,
                    period4:period3
                }
                kc=LifoQueue()
                kc.put(exam2)
                while not kc.empty():
                    current_exam=kc.get()
                    current_period=self.solutions[current_exam]
                    new_period=versus_period[current_period]
                    moves[current_exam]=new_period
                    for neighbor in list(self.G.neighbors(current_exam)):
                        if neighbor in moves:
                            continue
                        if self.solutions[neighbor]==new_period: 
                            kc.put(neighbor)
                best_cost_contribution=sys.maxsize
                moves_to_be_tested=dict()
                move_type['kempe_chain']=(moves,self.pre_reposition(-1,-1,moves))
                for type,movement in move_type.items():
                    moves,mcost=movement
                    if mcost<best_cost_contribution:
                        best_cost_contribution=mcost
                        moves_to_be_tested=moves
                return (moves_to_be_tested,best_cost_contribution)

    def contribute_to_cost(self,exam):
        return len([1 for neighbor in list(self.G.neighbors(exam)) if(abs(self.solutions[exam]-self.solutions[neighbor])<=5)])!=0

    def random_exam(self):
        exam=random.choice(list(self.G.nodes))
        while not self.contribute_to_cost(exam):
            exam=random.choice(list(self.G.nodes))
        return exam

    def permuting_periods(self,best):
        logging.basicConfig(level=logging.INFO,message="%(asctime)s\t%(message)s")
        for i in range(self.P):
            best_cost_move=sys.maxsize
            best_moves=dict()
            for j in range(i,self.P):
                if i==j: continue
                moves=dict()
                for exam in self.p_periods[i]:
                    moves[exam]=j
                for exam in self.p_periods[j]:
                    moves[exam]=i
                move_cost=self.pre_reposition(-1,-1,moves)
                if move_cost<best_cost_move:
                    best_cost_move=move_cost
                    best_moves=moves
            if len(best_moves)==0: continue
            cost_prediction=self.cost+self.pre_reposition(-1,-1,best_moves)
            if cost_prediction<best:
                self.reposition(-1,-1,best_moves)
                best=self.cost
                print(self.cost,cost_prediction)
                assert(self.cost==cost_prediction)
                logging.info("Permuting Periods|New best solution found S={}\periods effected:[{}->{}]".format(best,i,j))
        return best

    def feasibility(self):
        for node_1,node_2 in list(self.G.edges):
            if self.solutions[node_1]==self.solutions[node_2]:
                print('E',node_1,'E',node_2)
                print(self.solutions[node_1],self.solutions[node_2])
                return False
        return True

    def execute_moves(self):
        exam=self.random_exam()
        params=[(1,exam),(2,exam),(3,exam),(4,exam),(5,exam),(6,exam)]
        data=list()
        with ThreadPoolExecutor(max_workers=os.cpu_count()) as exec:
            data=exec.map(self.select_move,params)
        args=list(data)
        best_moves=dict()
        lowest_cost=sys.maxsize
        for moves,move_cost in args:
            if len(moves)==0: continue
            if move_cost<lowest_cost:
                lowest_cost=move_cost
                best_moves=moves
        return best_moves

def hill_climbing(dataset,exec_time):
    logging.basicConfig(level=logging.INFO,format='%(asctime)s\t%(message)s')
    sol=psolution(dataset)
    best=sol.cost
    start_timer=time()
    logo=' Permuting Periods '
    print('-'*5+logo+'-'*5)
    best=sol.permuting_periods(best)
    print('-'*(len(logo)+10),end='\n\n')
    while True:
        moves=sol.execute_moves()
        if len(moves)==0: 
            if time()-start_timer>exec_time:
                break
            continue
        rollback=dict()
        for exam in moves:
            rollback[exam]=sol.solutions[exam]
        sol.reposition(-1,-1,moves)
        if sol.cost<best:
            best=sol.cost
            logging.info("Hill Climbing|New best solution found S={} and time T={}`s".format(sol.cost,time()-start_timer))
        else:
            sol.reposition(-1,-1,rollback)
        if time()-start_timer>exec_time:
            break
    sol.renew_solution(sol.solutions)
    print(sol.is_feasible(),sol.compute_cost())


def main():
    ds_names=[dataset for dataset in carterhandler.datasets]
    for index,dataset in enumerate(ds_names):
        print(f'{index+1}.{dataset}')
    choice_d=int(input('Select a dataset:'))
    dataset=ds_names[choice_d-1]
    simulated_annealing(20,dataset)

def main_1():
    ds_names=[dataset for dataset in carterhandler.datasets]
    for index,dataset in enumerate(ds_names):
        print(f'{index+1}.{dataset}')
    print('='*20)
    choice_d=int(input('Select a dataset:'))
    dataset_name=ds_names[choice_d-1]
    hill_climbing(dataset_name,40)

if __name__=='__main__':
    # main()
    main_1()