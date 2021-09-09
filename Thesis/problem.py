from datasets_analysis import datasets_noise_exams, noise_exam_by_components
import networkx as nx
import os 
import math
from itertools import combinations
import logging
from copy import deepcopy
from concurrent.futures import ThreadPoolExecutor
from enum import Enum
from functools import lru_cache
from tqdm import tqdm
from matplotlib import pyplot as plt
import numpy as npy
import pickle


penalty=[16,8,4,2,1]
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
        return sum([penalty[abs(self.s_periods[node1]-self.s_periods[node2])-1] for node1,node2 in self.G.edges])

    def compute_normilized_cost(self):
        return self.compute_cost()/self.normalized
    
    def __str__(self):
        print('component_id:{}'.format(self.cid))
        print(self.G.nodes)
        print('Cost Contribution:{}'.format(self.compute_cost()))
        print('Normalized Cost:{}'.format(self.compute_normilized_cost()))

class Problem:
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
    
    def is_feasible(self):
        return len([(node1,node2) for node1,node2 in list(self.G.edges) if self.s_periods[node1]==self.s_periods[node2]])==0

    def compute_cost(self):
        return sum([penalty[abs(self.s_periods[node_1]-self.s_periods[node_2])-1] * self.G[node_1][node_2]['weight'] for node_1,node_2 in list(self.G.edges)])
    
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

class ProblemAnalyzer:
    @staticmethod
    def save_statistics():
        with open(os.path.join(folder.results_folder,'statistics.csv'),'w') as WF:
            WF.write('Problem,Exams,Students,Enrollments,Periods,Noisy Students,Clean Students,Noisy Exams,Clean Exams,Noisy Enrollments,Clean Enrollments,Density,Clean Density,Max number of exams per student,average number of exams per student\n')
            for dataset in datasets.names:
                problem=Problem(dataset)
                problem.noise_out()
                WF.write(f'{dataset},{problem.E},{problem.S},{problem.enrollments},{len(problem.noisy_students)},{problem.S-len(problem.noisy_students)},{problem.noisy_enrollments()},{problem.clean_enrollments()},{problem.conflict_density()},{problem.clean_density()},{problem.max_number_of_exams_per_student()},{problem.average_number_of_exams_per_student()}\n')

    @staticmethod    
    def save_noise():
        from tabulate import tabulate
        with open(os.path.join(folder.results_folder,'datasets_noise.out'),'w') as WF:
            for dataset in tqdm(datasets.names):
                problem=Problem(dataset)
                lesson_number_noise,component_noise,degree_noise=problem.calculate_noise()
                WF.write(f'{problem.pid}_Noise\n')
                WF.write(tabulate([['Noise by students',",".join([str(exam) for exam in problem.noisy_exams_by_students])],['Noise by components',",".join([str(exam) for exam in problem.noisy_exams_by_component])],['Noise by degree',",".join([str(exam) for exam in problem.noise_by_exam_degree])]],headers=['Noise Type','Exams'],tablefmt='textfile')+'\n\n')

    @staticmethod
    def save_identical():
        with open(os.path.join(folder.results_folder,'identical_exams.out'),'w') as WF:
            for dataset in tqdm(datasets.names):
                problem=Problem(dataset)
                identical_type_1,identical_type_2,identical_type_3=problem.identical_exams()
                WF.write(f'Problem:{dataset}\n')
                WF.write('Identical Type 1:')
                WF.write('[\n')
                for identical in identical_type_1:
                    WF.write('\t{'+','.join([str(exam) for exam in identical])+'}\n')
                WF.write(']\n\n')
                WF.write('Identical type 2:')
                WF.write('[\n')
                for identical in identical_type_2:
                    WF.write('\t{'+','.join([str(exam) for exam in identical])+'}\n')
                WF.write(']\n\n')
                WF.write('Identical Type 3:')
                WF.write('[\n')
                for identical in identical_type_3:
                    WF.write('\t{'+','.join([str(exam) for exam in identical])+'}\n')
                WF.write(']\n\n')

    @staticmethod    
    def greedy_coloring_stats():
        with open(os.path.join(folder.results_folder,'greedy_coloring.csv'),'w') as WF:
            WF.write('Problem,')
            WF.write(','.join(strategies))
            WF.write('Valid Coloring\n')
            for dataset in tqdm(datasets.names):
                WF.write(f'{dataset}')
                problem=Problem(dataset)
                start=True
                for strategy in strategies:
                    if start:
                        start=False
                        WF.write(f'{max(problem.parallel_coloring[strategy].values())+1}')
                        continue
                    WF.write(f',{problem.parallel_coloring[strategy]}')
                WF.write('\n')
    
    
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
        # for dataset in tqdm(carterhandler.datasets):
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
        contents=list()
        from tabulate import tabulate
        for dataset in tqdm(carterhandler.datasets):
            problem=Problem(dataset)
            _,ident_by_neighbors,_=problem.identical_exams()
            ident_by_neighbors=[list(ident) for ident in ident_by_neighbors]
            contents.append([dataset,'['+','.join([str(x) for x in ident_by_neighbors]+']')])
        print(tabulate(contents,headers=['Dataset','Ident Neighbors'],tablefmt='fancygrid'))
        with open(os.path.join('','Statistics','carter_identical_exams.out'),'w') as WF:
            WF.write(tabulate(contents,headers=['Dataset','Ident Neighbors'],tablefmt='textfile'))
    
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
        contents=list()
        from tabulate import tabulate
        for dataset in tqdm(itchandler.datasets):
            problem=Problem(dataset)
            _,ident_by_neighbors,_=problem.identical_exams()
            ident_by_neighbors=[list(ident) for ident in ident_by_neighbors]
            contents.append(ident_by_neighbors)
        print(tabulate(contents,headers=['Dataset','Ident Neighbors'],tablefmt='fancygrid'))
        with open(os.path.join('','Statistics','itc_identical_exams.out'),'w') as WF:
            WF.write(tabulate(contents,headers=['Dataset','Ident Neighbors'],tablefmt='textfile'))
    
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
