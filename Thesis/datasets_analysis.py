import os
from networkx.algorithms import components
from networkx.algorithms.bridges import bridges
from networkx.algorithms.coloring.greedy_coloring import greedy_color
from networkx.classes.function import subgraph
from tabulate import tabulate
from tqdm import tqdm
datasetspath=os.path.join('','datasets')


def load_dataset(pid):
    exams=dict()
    students=dict()
    enrollments=0
    with open(os.path.join(datasetspath,pid+'.in'),'r') as RF:
        start=True
        for line in RF:
            if start:
                start=False
                continue
            if len(line.strip())==0: continue
            data=line.split()
            if line.startswith('s'):
                enrollments+=1
                data[0]=data[0].replace('s','')
                exam=int(data[1])
                student=int(data[0])
                exams[exam].append(student)
                if student not in students:
                    students[student]=list()
                students[student].append(exam)
            else:
                exams[int(data[0])]=list()
    return exams,students,enrollments

def conflict_density(exams):
    cd=0
    for exam_index_i in range(1,len(exams)):
        for exam_index_j in range(exam_index_i+1,len(exams)):
            if len(set(exams[exam_index_i]).intersection(set(exams[exam_index_j])))>0:
                cd+=1
    return cd*2/len(exams)**2

def main_carter():
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
    pdata=list()
    for dataset,timeslots in tqdm(datasets.items()):
        exams,students,enrollments=load_dataset(dataset)
        pdata.append([dataset,len(exams),len(students),enrollments,timeslots,conflict_density(exams)])
    print(tabulate(pdata,headers=['dataset','exams','students','enrollments','periods','conflict density'],tablefmt='fancy_grid'))

def main_ITC():
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
    pdata=list()
    for dataset,timeslots in tqdm(datasets.items()):
        exams,students,enrollments=load_dataset(dataset)
        pdata.append([dataset,len(exams),len(students),enrollments,timeslots,conflict_density(exams)])
    print(tabulate(pdata,headers=['dataset','exams','students','enrollments','periods','conflict density'],tablefmt='fancy_grid'))

def main_D():
    datasets={
        "D1-2-17":38,
        "D5-1-17":45,
        "D5-1-18":45,
        "D5-2-17":45,
        "D5-2-18":59,
        "D5-3-18":22,
        "D6-1-18":60,
        "D6-2-18":78
    }
    pdata=list()
    for dataset,timeslots in tqdm(datasets.items()):
        exams,students,enrollments=load_dataset(dataset)
        pdata.append([dataset,len(exams),len(students),enrollments,timeslots,conflict_density(exams)])
    print(tabulate(pdata,headers=['dataset','exams','students','enrollments','periods','conflict density'],tablefmt='fancy_grid'))

def datasets_noise_students():
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
        "yor83":21,
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
        "ITC2007_12":12,
        "D1-2-17":38,
        "D5-1-17":45,
        "D5-1-18":45,
        "D5-2-17":45,
        "D5-2-18":59,
        "D5-3-18":22,
        "D6-1-18":60,
        "D6-2-18":78
    }
    with open(os.path.join('','Statistics','noise_students_1.csv'),'w') as WF:
        WF.write('dataset,students,noise_students\n')
        for dataset in tqdm(datasets):
            _,students,_=load_dataset(dataset)
            noise_students=[student for student,student_exams in students.items() if len(student_exams)==1]
            WF.write(f'{dataset},{len(students)},{len(noise_students)}\n')

def datasets_noise_exams():
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
        "yor83":21,
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
        "ITC2007_12":12,
        "D1-2-17":38,
        "D5-1-17":45,
        "D5-1-18":45,
        "D5-2-17":45,
        "D5-2-18":59,
        "D5-3-18":22,
        "D6-1-18":60,
        "D6-2-18":78
    }
    with open(os.path.join('','Statistics','noise_exams_1.csv'),'w') as WF:
        WF.write('dataset,students,noise_students\n')
        for dataset in tqdm(datasets):
            exams,students,_=load_dataset(dataset)
            noise_students=[student for student,student_exams in students.items() if len(student_exams)==1]
            noise_exams=[exam for exam,exam_students in exams.items() if set(exam_students).issubset(set(noise_students))]
            WF.write(f'{dataset},{len(exams)},{len(noise_exams)}\n')

def save_all_to_file():
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
        "yor83":21,
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
        "ITC2007_12":12,
        "D1-2-17":38,
        "D5-1-17":45,
        "D5-1-18":45,
        "D5-2-17":45,
        "D5-2-18":59,
        "D5-3-18":22,
        "D6-1-18":60,
        "D6-2-18":78
    }
    pdata=list()
    for dataset,timeslots in tqdm(datasets.items()):
        exams,students,enrollments=load_dataset(dataset)
        pdata.append([dataset,len(exams),len(students),enrollments,timeslots,conflict_density(exams)])
    with open(os.path.join('','results','statistics.out'),'w') as WF:
        WF.write(tabulate(pdata,headers=['dataset','exams','students','enrollments','periods','conflict density'],tablefmt='textfile'))

def datasets_connected_components():
    import networkx as nx
    def common_students(set_of_students_1,set_of_students_2):
        return len(set(set_of_students_1).intersection(set_of_students_2))

    def convert_to_nx_graph(exams):
        G=nx.Graph()    
        G.add_nodes_from([exam for exam in exams])
        for index_i in range(1,len(exams)+1):
            for index_j in range(index_i+1,len(exams)+1):
                cs=common_students(exams[index_i],exams[index_j])
                if cs>0:
                    G.add_edge(index_i,index_j,weight=cs)
        return G

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
        "yor83":21,
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
        "ITC2007_12":12,
        "D1-2-17":38,
        "D5-1-17":45,
        "D5-1-18":45,
        "D5-2-17":45,
        "D5-2-18":59,
        "D5-3-18":22,
        "D6-1-18":60,
        "D6-2-18":78
    }
    with open(os.path.join('','Statistics','connected_components.csv'),'w') as WF:
        WF.write('dataset,components,bridges,bridge components\n')
        for dataset in tqdm(datasets):
            exams,students,_=load_dataset(dataset)
            G=convert_to_nx_graph(exams)
            components=len(list(nx.connected_components(G)))
            bridges=list(nx.bridges(G))
            G.remove_edges_from(bridges)
            bridgescomponents=len(list(nx.connected_components(G)))
            WF.write(f'{dataset},{components},{len(bridges)},{bridgescomponents}\n')    

def noise_exam_by_components():
    import networkx as nx
    def common_students(set_of_students_1,set_of_students_2):
        return len(set(set_of_students_1).intersection(set_of_students_2))

    def convert_to_nx_graph(exams):
        G=nx.Graph()    
        G.add_nodes_from([exam for exam in exams])
        for index_i in range(1,len(exams)+1):
            for index_j in range(index_i+1,len(exams)+1):
                cs=common_students(exams[index_i],exams[index_j])
                if cs>0:
                    G.add_edge(index_i,index_j,weight=cs)
        return G

    def greedy_coloring(G):
        strategies=['largest_first','random_sequential','smallest_last','independent_set','connected_sequential_bfs','connected_sequential_dfs','saturation_largest_first']
        coloring=dict()
        for strategy in strategies:
            color=nx.greedy_color(G,strategy)
            coloring[strategy]=color
            if strategy not in ['independent_set','saturation_largest_first']:
                color=nx.greedy_color(G,strategy,interchange=True)
                coloring[strategy+'_interchange']=color
        min_colors=G.number_of_nodes()
        best_res=dict()
        for strategy,colres in coloring.items():
            colors_used=max(colres.values())+1
            if colors_used<min_colors:
                min_colors=colors_used
                best_res=colres
        return best_res
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
        "yor83":21,
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
        "ITC2007_12":12,
        "D1-2-17":38,
        "D5-1-17":45,
        "D5-1-18":45,
        "D5-2-17":45,
        "D5-2-18":59,
        "D5-3-18":22,
        "D6-1-18":60,
        "D6-2-18":78
    }
    with open(os.path.join('','Statistics','noise_by_component.csv'),'w') as WF:
        WF.write('dataset,noise_exams,Nr noise students\n')
        for dataset in tqdm(datasets):
            exams,students,_=load_dataset(dataset)
            noise_students=[student for student,student_exams in students.items() if len(student_exams)==1]
            noise_exams=[exam for exam,exam_students in exams.items() if set(exam_students).issubset(set(noise_students))]
            G=convert_to_nx_graph(exams)
            speriods=greedy_coloring(G)
            import math
            P=max(speriods.values())+1 
            limitation=math.floor((P-1)/6)+1
            components=[G.subgraph(sg) for sg in list(nx.connected_components(G))]
            noise_by_component=list()
            for component in components:
                if component.number_of_nodes()<limitation:
                    noise_by_component.extend([exam for exam in list(component.nodes())])
                    noise_students.extend([student for exam in list(component.nodes()) for student in exams[exam]])
            WF.write(f'{dataset},[{"-".join([str(exam) for exam in noise_by_component])}],{len(noise_students)}\n')

            



if __name__=='__main__':
    # main_carter()
    # main_ITC()
    # main_D()
    # datasets_noise_students()
    datasets_noise_exams()
    # datasets_connected_components()
    # noise_exam_by_components()
    # save_all_to_file()
