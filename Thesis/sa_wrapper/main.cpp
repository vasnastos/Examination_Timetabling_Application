#include "Solution.cpp"
#include <iostream>

std::string dataset_Selection()
{
    int counter=1;
    for(auto &dataset:datasets::names)
    {
        std::cout<<counter<<"."<<dataset<<std::endl;
        counter++;
    }
    std::cout<<"================"<<std::endl;
    int i;
    std::cout<<"Select dataset:";
    std::cin>>i;
    std::cout<<std::endl;
    return datasets::names.at(i-1);
}

void Scenario_1()
{
    problem p(dataset_Selection());
    hill_climber hc(p,30);
    hc.solve();
}

void Scenario_2()
{
    problem p(dataset_Selection());
    // int e_time;
    // std::cout<<"Give execution time:";
    // std::cin>>e_time;
    simulated_annealer sa(p,200);
    sa.solve();
    sa.Export();
}

void Scenario_3()
{
    for(auto &dataset:datasets::names)
    {
        std::cout<<"#"<<dataset<<std::endl;
        problem p(dataset);
        simulated_annealer *sa=new simulated_annealer(p,400);
        sa->solve();
        sa->Export();
        delete sa;
    }
}

void menu()
{
    int v;
    std::cout<<"Select Scenario::"<<std::endl;
    std::cout<<"-------------"<<std::endl;
    std::cout<<"1.Hill Climbing"<<std::endl;
    std::cout<<"2.Simulated Annealing"<<std::endl;
    std::cout<<"3.Massive Solution"<<std::endl;
    std::cout<<"Select:";
    std::cin>>v;
    std::cout<<std::endl;
    switch(v)
    {
        case 1:
            Scenario_1();
            break;
        case 2:
            Scenario_2();
            break;
        case 3:
            Scenario_3();
            break;
        default:
            return;
    }
}

int main(int argc,char **argv)
{
    menu();
    return EXIT_SUCCESS;
}