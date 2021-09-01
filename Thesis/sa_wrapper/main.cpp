#include "Solution.cpp"

void Scenario_1()
{
    problem p("car91");
    hill_climber hc(p,30);
    hc.solve();
}

void Scenario_2()
{
    problem p("kfu93");
    simulated_annealer sa(p,200);
    sa.solve();
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
    std::cout<<"1.Hill Climbing--Ds:lse91"<<std::endl;
    std::cout<<"2.Simulated Annealing--DS:lse91"<<std::endl;
    std::cout<<"Select:";
    std::cin>>v;
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