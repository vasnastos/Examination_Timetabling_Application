import networkx as nx
from networkx.classes.function import density
import random 
import matplotlib.pyplot as plt
from time import time
import pydot
import os
from networkx.drawing.nx_pydot import to_pydot



def import_old_dataset_version():
    students=dict()
    exams=dict()
    student_id=1
    y=open('toy.ds')
    for student in y:
        students[student_id]=list()
        line_exams=student.split()
        for exam in line_exams:
            actual_exam=int(exam)
            if actual_exam not in exams:
                exams[actual_exam]=list()
            exams[actual_exam].append(student_id)
        student_id+=1
    y.close()
    return exams,students

def common_students(s1,s2):
    return len(set(s1).intersection(set(s2)))


def create_nx_graph(exams):
    G=nx.Graph()
    G.add_nodes_from([exam for exam in exams])
    for exam,students in exams.items():
        for exam2,students2 in exams.items():
            if exam==exam2: continue
            cs=common_students(students,students2)
            if cs!=0:
                G.add_edge(exam,exam2,weight=cs)
    return G

def can_be_moved(exam,p,G,periods,exclude={}):
    for neighbor in (G.neighbors(exam)):
        if neighbor in exclude: continue
        if p==periods[neighbor]:
            return False
    return True

def move_exam(perioding_exams,P,G):
    exam=random.choice([exam for exam in perioding_exams])
    for p in range(P):
        if can_be_moved(exam,p,G,perioding_exams):
            if p==perioding_exams[exam]: continue
            perioding_exams[exam]=p
            return True,perioding_exams
    return False,perioding_exams

def swap_exams(perioding_exams,P,G):
    exam=random.choice([exam for exam in perioding_exams])
    exam2=random.choice([exam for exam in perioding_exams])
    check=False
    while exam==exam2 or perioding_exams[exam]==perioding_exams[exam2] or exam2 not in list(G.neighbors(exam)):
        exam2=random.choice([exam for exam in perioding_exams])
    period1=perioding_exams[exam2]
    period2=perioding_exams[exam]
    if can_be_moved(exam,period1,G,perioding_exams,{exam2})==True and can_be_moved(exam2,period2,G,perioding_exams,{exam})==True:
        perioding_exams[exam]=period1
        perioding_exams[exam2]=period2
        print(period1,period2)
        check=True
    return check,perioding_exams

def swap_periods(per_periods,perioding_exams,P):
    period1=random.randint(0,P-1)
    period2=random.randint(0,P-1)
    while period2==period1:
        period2=random.randint(0,P-1)
    for exam in per_periods[period1]:
        perioding_exams[exam]=period2
    for exam in per_periods[period2]:
        perioding_exams[exam]=period1
    return perioding_exams

def kick_exam(perioding_exams,P,G):
    exam=random.choice([exam for exam in perioding_exams])
    for neighbor in list(G.neighbors(exam)):
        moves=dict()
        period1=perioding_exams[neighbor]
        if can_be_moved(exam,period1,G,perioding_exams,{neighbor}):
            for p in range(P):
                if p==period1 or p==perioding_exams[exam]: continue
                if can_be_moved(neighbor,p,G,perioding_exams):
                    moves[exam]=period1
                    moves[neighbor]=p
                    return True,moves
    return False,dict()

def double_kick_exam(perioding_exams,P,G):
    exam=random.choice([exam for exam in perioding_exams])
    for neighbor in list(G.neighbors(exam)):
        moves=dict()
        period1=perioding_exams[neighbor]
        if can_be_moved(exam,period1,G,perioding_exams,{neighbor}):
            for neighbor2 in list(G.neighbors(neighbor)):
                period2=perioding_exams[neighbor2]
                if can_be_moved(neighbor,period2,G,perioding_exams,{neighbor2}):
                    for p in range(P):
                        if p==period1 or p==period2 or p==perioding_exams[exam] or neighbor2==exam: continue
                        if can_be_moved(neighbor2,p,G,perioding_exams):
                            moves[exam]=period1
                            moves[neighbor]=period2
                            moves[neighbor2]=p
                            return True,moves
    return False,dict()                        

def main():
    exams,students=import_old_dataset_version()
    G=create_nx_graph(exams)
    print(f'Conflict Density:{density(G)}')
    periods=nx.greedy_color(G,strategy='largest_first')
    P=max(periods.values())+1
    colors={0:'#0e3066',1:'#4a080a',2:'#8f7d1a',3:'#8f4d1a',4:'#146e1e'}
    # pos=nx.kamada_kawai_layout(G)
    pos=nx.spring_layout(G)
    color_map=list()
    for node in list(G.nodes):
        period=periods[node]
        color_map.append(colors[period])
    # nx.draw(G,pos,width=1,labels={exam:str(exam) for exam in exams},node_color=color_map)
    # plt,plt.savefig(os.path.join('','Before_Move_Exam.png'))
    # plt.show()
    pdot=to_pydot(G)
    for index,node in enumerate(pdot.get_nodes()):
        node.set_label(str(index+1))
        node.set_fillcolor(color_map[index])
        node.set_style('filled')
        node.set_fontcolor('black')
        node.set_color(color_map[index])
    pdot.write_png('me1.png')
    print(P,periods)
    # pos=nx.spring_layout(G)
    while True:
        check,periods=move_exam(periods,P,G)
        if check==True:
            break
    print(periods)
    color_map=list()
    for node in list(G.nodes):
        period=periods[node]
        color_map.append(colors[period])
    # nx.draw(G,pos,width=1,labels={exam:str(exam) for exam in exams},node_color=color_map)
    # plt,plt.savefig(os.path.join('','After_Move_Exam.png'))
    # plt.show()
    pdot=to_pydot(G)
    for index,node in enumerate(pdot.get_nodes()):
        node.set_label(str(index+1))
        node.set_fillcolor(color_map[index])
        node.set_style('filled')
        node.set_fontcolor('black')
        node.set_color(color_map[index])
    pdot.write_png('me2.png')

def main_1():
    exams,students=import_old_dataset_version()
    G=create_nx_graph(exams)
    print(f'Conflict Density:{density(G)}')
    periods=nx.greedy_color(G,strategy='largest_first')
    P=max(periods.values())+1
    colors={0:'#0e3066',1:'#4a080a',2:'#8f7d1a',3:'#8f4d1a',4:'#146e1e',5:'#8c0ba3',6:'#5c7d10'}
    # pos=nx.kamada_kawai_layout(G)
    pos=nx.spring_layout(G)
    color_map=list()
    for node in list(G.nodes):
        period=periods[node]
        color_map.append(colors[period])
    # nx.draw(G,pos,width=1,labels={exam:str(exam) for exam in exams},node_color=color_map)
    # plt,plt.savefig(os.path.join('','Before_Move_Exam.png'))
    # plt.show()
    pdot=to_pydot(G)
    for index,node in enumerate(pdot.get_nodes()):
        node.set_label(str(index+1))
        node.set_fillcolor(color_map[index])
        node.set_style('filled')
        node.set_fontcolor('black')
        node.set_color(color_map[index])
    pdot.write_png('se1.png')
    # pos=nx.spring_layout(G)
    while True:
        check,periods=swap_exams(periods,P,G)
        if check==True:
            break
    color_map=list()
    for node in list(G.nodes):
        period=periods[node]
        color_map.append(colors[period])
    # nx.draw(G,pos,width=1,labels={exam:str(exam) for exam in exams},node_color=color_map)
    # plt,plt.savefig(os.path.join('','After_Move_Exam.png'))
    # plt.show()
    pdot=to_pydot(G)
    for index,node in enumerate(pdot.get_nodes()):
        node.set_label(str(index+1))
        node.set_fillcolor(color_map[index])
        node.set_style('filled')
        node.set_fontcolor('black')
        node.set_color(color_map[index])
    pdot.write_png('se2.png')

def main_2():
    exams,students=import_old_dataset_version()
    G=create_nx_graph(exams)
    print(f'Conflict Density:{density(G)}')
    periods=nx.greedy_color(G,strategy='largest_first')
    P=max(periods.values())+1
    colors={0:'#0e3066',1:'#4a080a',2:'#8f7d1a',3:'#8f4d1a',4:'#146e1e',5:'#8c0ba3',6:'#5c7d10'}
    # pos=nx.kamada_kawai_layout(G)
    pos=nx.spring_layout(G)
    color_map=list()
    for node in list(G.nodes):
        period=periods[node]
        color_map.append(colors[period])
    # nx.draw(G,pos,width=1,node_color=color_map,labels={exam:str(exam) for exam in periods})
    # plt.savefig('swap_periods_1.png')
    # plt.show()
    pdot=to_pydot(G)
    for index,node in enumerate(pdot.get_nodes()):
        node.set_label(str(index+1))
        node.set_fillcolor(color_map[index])
        node.set_style('filled')
        node.set_fontcolor('black')
        node.set_color(color_map[index])
    pdot.write_png('sp1.png')
    per_period={i:list() for i in range(P)}
    for exam,p in periods.items():
        per_period[p].append(exam)
    periods=swap_periods(per_period,periods,P)
    color_map=list()
    for node in list(G.nodes):
        period=periods[node]
        color_map.append(colors[period])
    # nx.draw(G,pos,width=1,node_color=color_map,labels={exam:str(exam) for exam in periods})
    # plt.savefig('swap_periods_2.png')
    # plt.show()
    pdot=to_pydot(G)
    for index,node in enumerate(pdot.get_nodes()):
        node.set_label(str(index+1))
        node.set_fillcolor(color_map[index])
        node.set_style('filled')
        node.set_fontcolor('black')
        node.set_color(color_map[index])
    pdot.write_png('sp2.png')

def main_3():
    exams,students=import_old_dataset_version()
    G=create_nx_graph(exams)
    print(f'Conflict Density:{density(G)}')
    periods=nx.greedy_color(G,strategy='largest_first')
    P=max(periods.values())+1
    colors={0:'#0e3066',1:'#4a080a',2:'#8f7d1a',3:'#8f4d1a',4:'#146e1e',5:'#8c0ba3',6:'#5c7d10'}
    # pos=nx.kamada_kawai_layout(G)
    pos=nx.spring_layout(G)
    color_map=list()
    for node in list(G.nodes):
        period=periods[node]
        color_map.append(colors[period])
    # nx.draw(G,pos,width=1,node_color=color_map,labels={exam:str(exam) for exam in periods})
    # plt.savefig('kick_exam_1.png')
    # plt.show()
    pdot=to_pydot(G)
    for index,node in enumerate(pdot.get_nodes()):
        node.set_label(str(index+1))
        node.set_fillcolor(color_map[index])
        node.set_style('filled')
        node.set_fontcolor('black')
        node.set_color(color_map[index])
    pdot.write_png('kick_exam_1.png')
    check,moves=kick_exam(periods,P,G)
    while not check:
        check,moves=kick_exam(periods,P,G)
    counter=2
    for exam,period in moves.items():
        periods[exam]=period
        color_map=list()
        for node in list(G.nodes):
            period=periods[node]
            color_map.append(colors[period])
        # nx.draw(G,pos,width=1,node_color=color_map,labels={exam:str(exam) for exam in periods})
        # plt.savefig(f'kick_exam_{counter}.png')
        pdot=to_pydot(G)
        for index,node in enumerate(pdot.get_nodes()):
            node.set_label(str(index+1))
            node.set_fillcolor(color_map[index])
            node.set_style('filled')
            node.set_fontcolor('black')
            node.set_color(color_map[index])
        pdot.write_png(f'kick_exam_{counter}.png')
        counter+=1

def main_4():
    exams,students=import_old_dataset_version()
    G=create_nx_graph(exams)
    print(f'Conflict Density:{density(G)}')
    periods=nx.greedy_color(G,strategy='largest_first')
    P=max(periods.values())+1
    colors={0:'#0e3066',1:'#4a080a',2:'#8f7d1a',3:'#8f4d1a',4:'#146e1e',5:'#8c0ba3',6:'#5c7d10'}
    # pos=nx.kamada_kawai_layout(G)
    # pos=nx.kamada_kawai_layout(G)
    color_map=list()
    for node in list(G.nodes):
        period=periods[node]
        color_map.append(colors[period])
    pdot=to_pydot(G)
    for index,node in enumerate(pdot.get_nodes()):
        node.set_label(str(index+1))
        node.set_fillcolor(color_map[index])
        node.set_style('filled')
        node.set_fontcolor('black')
        node.set_color(color_map[index])
    pdot.write_png('double_kick_1.png')
    # nx.draw(G,pos,width=1,node_color=color_map,labels={exam:str(exam) for exam in periods},font_size=16,font_color='black')
    check,moves=double_kick_exam(periods,P,G)
    while not check:
        check,moves=double_kick_exam(periods,P,G)
    counter=2
    for exam,period in moves.items():
        periods[exam]=period
        color_map=list()
        for node in list(G.nodes):
           period=periods[node]
           color_map.append(colors[period])
        pdot=to_pydot(G)
        for index,node in enumerate(pdot.get_nodes()):
            node.set_label(str(index+1))
            node.set_fillcolor(color_map[index])
            node.set_style('filled')
            node.set_fontcolor('black')
            node.set_color(color_map[index])
        pdot.write_png(f'double_kick_{counter}.png')
        # nx.draw(G,pos,width=1,node_color=color_map,labels={exam:str(exam) for exam in periods},font_size=16,font_color='black')
        # plt.savefig(f'double_kick_exam_{counter}.png')
        counter+=1
        # plt.show()


if __name__=='__main__':
    main()
    # main_1()
    # main_2()
    # main_3()
    # main_4()