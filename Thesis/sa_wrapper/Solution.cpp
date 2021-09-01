#include "parameters.hpp"

class Solution:public Graph{
    protected:
        std::map <int,std::vector <int>> periods;
        std::vector <pmove> stats;
        int move_executed;

        int pdelta(int exam,int period)
        {
            int lb=period-5>0?period-5:0;
            int rb=period+5<this->P-1?period+5:this->P-1;
            int cs=0;
            for(int i=period-1;i>=lb;i--)
            {
                for(auto &ex:this->periods[i])
                {
                    if(this->has_edge(exam,ex))
                    {
                        cs+=this->adj_table[exam][ex] * Graph::penalty[period-this->s_periods[ex]];
                    }
                }
            }
            for(int i=period+1;i<=rb;i++)
            {
                for(auto &ex:this->periods[i])
                {
                    if(this->has_edge(exam,ex))
                    {
                        cs+=this->adj_table[exam][ex] * Graph::penalty[period-this->s_periods[ex]];
                    }
                }
            }
            return cs;
        }

        int random_exam()
        {
            std::uniform_int_distribution <int> exam_chooser(0,this->non_zero_neighbors_exams.size()-1);
            int exam;
            bool inflicts_cost=false;
            while(true)
            {
                exam=this->non_zero_neighbors_exams.at(exam_chooser(mt));
                for(auto &n:this->neighbors[exam])
                {
                    if(abs(this->s_periods[exam]-this->s_periods[n])<=5)
                    {
                        inflicts_cost=true;
                        break;
                    }
                }
                if(inflicts_cost)
                {
                    break;
                }
            }
            return exam;
        }

        void schedule(int exam,int period){
            this->s_periods[exam]=period;
            this->periods[period].emplace_back(exam);
            this->cost+=this->pdelta(exam,period);
        }

        bool can_be_moved(int exam,int period,std::vector <int> exclude={})
        {
            for(auto &n:this->neighbors[exam])
            {
                if(std::find(exclude.begin(),exclude.end(),n)!=exclude.end()) continue;
                if(period==this->s_periods[n])
                {
                    return false;
                }
            }
            return true;
        }

        bool can_be_moved(int exam,int period,std::map <int,int> &sim_moves,std::vector <int> exclude={})
        {
            int nnstate;
            for(auto &x:this->neighbors[exam])
            {
                if(std::find(exclude.begin(),exclude.end(),x)!=exclude.end()) continue;
                nnstate=sim_moves.find(x)!=sim_moves.end()?sim_moves[x]:this->s_periods[x];
                if(nnstate==period)
                {
                    return false;
                }
            }
            return true;
        }

    public:
        int cost;
        Solution(problem &p):Graph(p.ds_name,p.graph,p.pr_period,p.students)
        {
            for(int i=0;i<this->P;i++)
            {
                this->periods[i]=std::vector <int>();
            }
            this->cost=0;
            for(auto &pn:p.pr_period)
            {
                this->schedule(pn.first,pn.second);
            }
            for(int i=1;i<=MOVES_NUMBER;i++)
            {
                this->stats.emplace_back(pmove(i));
            }
        }
        
        int pre_calculation(int exam,int period)
        {
            int cs=0;
            int distance,distance_before,neigh_state;
            int exam_period=this->s_periods[exam];
            for(auto &nn:this->neighbors[exam])
            {
                neigh_state=this->s_periods[nn];
                distance=period-neigh_state;
                distance_before=exam_period-neigh_state;
                if(distance==distance_before) continue;
                cs-=this->adj_table[exam][nn] * Graph::penalty[distance_before];
                cs+=this->adj_table[exam][nn] * Graph::penalty[distance];
            }
            return cs;
        }

        int pre_calculation(int exam,int period,std::map <int,int> &cached)
        {
            int cs=0;
            int distance,distance_before,neigh_state;
            int exam_period=this->s_periods[exam];
            for(auto &nn:this->neighbors[exam])
            {
                neigh_state=cached.find(nn)!=cached.end()?cached[nn]:this->s_periods[nn];
                distance=period-neigh_state;
                distance_before=exam_period-neigh_state;
                if(distance==distance_before) continue;
                cs-=this->adj_table[exam][nn] * Graph::penalty[distance_before];
                cs+=this->adj_table[exam][nn] * Graph::penalty[distance];
            }
            return cs;
        }
        
        int me_pre_calculation(int exam,int period,std::map <int,int> &include)
        {
            int cs=0;
            int distance,distance_before,old_period,neighbor_state;
            old_period=include.find(exam)!=include.end()?include[exam]:this->s_periods[exam];
            for(auto &node:this->neighbors[exam])
            {
                neighbor_state=include.find(node)!=include.end()?include[node]:this->s_periods[node];
                distance=period-neighbor_state;
                distance_before=old_period-neighbor_state;
                if(distance==distance_before)
                {
                    continue;
                }
                cs-=this->adj_table[exam][node] * Graph::penalty[distance_before];
                cs+=this->adj_table[exam][node] * Graph::penalty[distance];
            }
            return cs;
        }

        int me_pre_calculation(int exam,int period,std::map <int,int> &include,std::map <int,int> &cached)
        {
            int cs=0;
            int distance,distance_before,old_period,neighbor_state;
            old_period=include.find(exam)!=include.end()?include[exam]:this->s_periods[exam];
            for(auto &neighbor:this->neighbors[exam])
            {
                neighbor_state=cached.find(neighbor)!=cached.end()?cached[neighbor]:include.find(neighbor)!=include.end()?include[neighbor]:this->s_periods[neighbor];
                distance=abs(period-neighbor_state);
                distance_before=abs(old_period-neighbor_state);
                if(distance==distance_before) continue;
                cs-=this->adj_table[exam][neighbor] * Graph::penalty[distance_before];
                cs+=this->adj_table[exam][neighbor] * Graph::penalty[distance];
            }
            return cs;
        }

        int ej_pre_reposition(std::map <int,int> &pmoves,std::map <int,int> &include)
        {
            int cc=0;
            std::map <int,int> cache;
            for(auto &mid:pmoves)
            {
                cc+=this->me_pre_calculation(mid.first,mid.second,include,cache);
                cache[mid.first]=mid.second;
            }
            return cc;
        }

        int pre_reposition(std::map <int,int> &moves)
        {
            std::map <int,int> cached_moves;
            int cs=0;
            for(auto &mid:moves){
                cs+=this->pre_calculation(mid.first,mid.second,cached_moves);
                cached_moves[mid.first]=mid.second;
            }
            return cs;
        }

        void reposition(int exam,int period)
        {
            int old_period=this->s_periods[exam];
            this->s_periods[exam]=period;
            this->periods[period].emplace_back(exam);
            this->periods[old_period].erase(std::find(this->periods[old_period].begin(),this->periods[old_period].end(),exam));

            int distance,distance_before;
            for(auto &neighbor:this->neighbors[exam])
            {
                distance=abs(period-this->s_periods[neighbor]);
                distance_before=abs(old_period-this->s_periods[neighbor]);
                if(distance==distance_before) continue;
                this->cost-=this->adj_table[exam][neighbor] * Graph::penalty[distance_before];
                if(distance>=1 && distance<=5)
                this->cost+=this->adj_table[exam][neighbor] * Graph::penalty[distance];
            }
        }

        void reposition(std::map <int,int> &moves)
        {
            for(auto &mid:moves)
            {
                this->reposition(mid.first,mid.second);
            }
        }
        // Old version Schedule_unschedule
        // void schedule(int exam,int p){
        //     this->s_periods[exam]=p;
        //     this->periods[p].emplace_back(exam);;
        //     this->cost+=this->neighbors_delta(exam,p);
        // }

        // void unschedule(int exam){
        //     int op=this->s_periods[exam];
        //     this->s_periods[exam]=INT_MIN;
        //     this->periods[op].erase(std::find(this->periods[op].begin(),this->periods[op].end(),exam));
        //     this->cost-=this->neighbors_delta(exam,op);
        // }

        std::map <int,int> select_move()
        {
            std::uniform_int_distribution<int> r(1,8);
            int exam=this->random_exam();
            switch(r(mt))
            {
                case 1:
                   return this->move_exam(exam);
                   break;
                case 2:
                    return this->swap_exams(exam);
                    break;
                case 3:
                    return this->swap_periods(exam);
                    break;
                case 4:
                    return this->shift_periods(exam);
                    break;
                case 5:
                    return this->kempe_chain(exam);
                    break;
                case 6:
                    return this->kick_exam(exam);
                    break;
                case 7:
                    return this->double_kick_exam(exam);
                    break;
                case 8:
                    return this->round_kick_exam(exam);
                    break;
                default:
                    return std::map <int,int>();
                    break;
            }
        }

        std::map <int,int> select_move(int i,int exam)
        {
            switch(i)
            {
                case 1:
                   return this->move_exam(exam);
                   break;
                case 2:
                    return this->swap_exams(exam);
                    break;
                case 3:
                    return this->swap_periods(exam);
                    break;
                case 4:
                    return this->shift_periods(exam);
                    break;
                case 5:
                    return this->kempe_chain(exam);
                    break;
                case 6:
                    return this->kick_exam(exam);
                    break;
                case 7:
                    return this->double_kick_exam(exam);
                    break;
                case 8:
                    return this->round_kick_exam(exam);
                    break;
                default:
                    return std::map <int,int>();
                    break;
            }
        }

        std::map <int,int> move_exam(int exam)
        {
            this->move_executed=0;
            std::vector <int> dummy;
            for(int i=0;i<this->P;i++)
            {
                if(this->can_be_moved(exam,i))
                {
                    return {{exam,i}};
                }
            }
            return std::map <int,int>();
        }

        std::map <int,int> swap_exams(int exam)
        {
            /*
             * Preconditions 
                exam_id1 should be neighbor of exam_id2
             * Before 
                exam_id1->period_id1
                exam_id2->period_id2
             * After
                exam_id1->period_id2
                erxam_id2->period_id1
            */
            this->move_executed=1;
            std::vector <int> eneighbors=this->neighbors[exam];
            std::uniform_int_distribution <int> nn(0,eneighbors.size()-1);
            int exam2=eneighbors.at(nn(mt));
            int period1=this->s_periods[exam2];
            int period2=this->s_periods[exam];
            assert(period1!=period2);
            if(this->can_be_moved(exam,period1,{exam2}) && this->can_be_moved(exam2,period2,{exam}))
            {
                return {
                    {exam,period1},
                    {exam2,period2}
                };
            }
            return std::map <int,int>();
        }

        std::map <int,int> swap_periods(int exam)
        {
            this->move_executed=2;
            std::uniform_int_distribution <int> pd(0,this->P-1);
            int period1=this->s_periods[exam];
            int period2=pd(mt);
            while(period1==period2)
            {
                period2=pd(mt);
            }
            std::map <int,int> moves;
            for(auto &e:this->periods[period1])
            {
                moves[e]=period2;
            }
            for(auto &e:this->periods[period2])
            {
                moves[e]=period1;
            }
            return moves;
        }

        std::map <int,int> shift_periods(int exam)
        {
            this->move_executed=3;
            int period=this->s_periods[exam];
            int newperiod=(period+5)%this->P;
            std::map <int,int> moves;
            int iperiod=period+1;
            while(true)
            {
                for(auto &exam:this->periods[iperiod%this->P])
                {
                    moves[exam]=(iperiod-1)%this->P;
                }
                if(iperiod%this->P==newperiod)
                {
                    break;
                }
                iperiod++;
            }
            for(auto &exam:this->periods[period])
            {
                moves[exam]=newperiod;
            }
            return moves;
        }

        std::map <int,int> kempe_chain(int exam)
        {
            this->move_executed=4;
            std::vector <int> eneighbors=this->neighbors[exam];
            std::uniform_int_distribution<int> d(0,eneighbors.size()-1);
            int exam2=eneighbors[d(mt)];
            const int sp1=this->s_periods[exam];
            const int sp2=this->s_periods[exam2];
            std::map <int,int> nextp{{sp1,sp2},{sp2,sp1}};
            std::vector <int> kc;
            kc.emplace_back(exam2);
            kc.emplace_back(exam);
            int ce;
            int cp;
            int newperiod;
            std::map <int,int> moves;
            while(!kc.empty())
            {
                ce=kc.back();
                kc.pop_back();
                cp=this->s_periods[ce];
                newperiod=nextp[cp];
                moves[ce]=newperiod;
                for(auto &neighbor:this->neighbors[ce])
                {
                    if(moves.find(neighbor)!=moves.end()) continue;
                    if(neighbor!=exam2 && this->s_periods[neighbor]==newperiod)
                    {
                        kc.emplace_back(neighbor);
                    }
                }
            }
            return moves;
        }
        
        std::map <int,int> kick_exam(int exam)
        {
            /*
                *Preconditions
                    exam_id1 should be neighbor to exam_id2
                    All moves should be valid
                *Before
                    exam_id1->period_id1
                    exam_id2->period_id2
                *After
                    exam_id1->period_id2
                    exam_id2->period_id3
                period_id3->A new valid period
            */
            this->move_executed=5;
            std::vector <int> eneighbors=this->neighbors[exam];
            std::uniform_int_distribution <int> un(0,eneighbors.size()-1);
            int exam2=eneighbors.at(un(mt));
            int period1=this->s_periods[exam2];
            std::uniform_int_distribution <int> dp(0,this->P-1);
            int period2=dp(mt);
            while(period2==period1 || period2==this->s_periods[exam])
            {
                period2=dp(mt);
            }
            if(this->can_be_moved(exam,period1,{exam2}) && this->can_be_moved(exam2,period2))
                return {
                    {exam,period1},
                    {exam2,period2}
                };
            return std::map <int,int>();
        }

        std::map <int,int> double_kick_exam(int exam)
        {
            /*
              *Preconditions
                exam_id2 shouuld be neighbor to exam_id1
                exam_id3 should be neighbor to exam_id2
                All moves should be valid
                ----------
              *Before
                exam_id1->period_id1
                exam_id2->period_id2
                exam_id3->period_id3
              
              * After
                exam_id1->period_id2
                exam_id2->period_id3
                exam_id3->period_id4
                period_id4->A valid new period.!!
            */
            
            this->move_executed=6;
            std::vector <int> eneighbors=this->neighbors[exam];
            std::uniform_int_distribution <int> de(0,eneighbors.size()-1);
            int exam2=eneighbors.at(de(mt));
            int period1=this->s_periods[exam2];
            eneighbors=this->neighbors[exam2];
            de=std::uniform_int_distribution <int> (0,eneighbors.size()-1);
            int exam3=eneighbors.at(de(mt));
            if(eneighbors.size()==1 && exam3==exam) {return std::map <int,int>();}
            while(exam3==exam)
            {
                exam3=eneighbors.at(de(mt));
            }
            int period2=this->s_periods[exam3];
            std::uniform_int_distribution <int> dp(0,this->P-1);
            int period3=dp(mt);
            while(period3==period1 || period3==period2 || period3==this->s_periods[exam])
            {
                period3=dp(mt);
            }
            if(this->can_be_moved(exam,period1,{exam2}) && this->can_be_moved(exam2,period2,{exam3}) && this->can_be_moved(exam3,period3))
            {
                return {
                    {exam,period1},
                    {exam2,period2},
                    {exam3,period3}
                };
            }
            return std::map <int,int>();
        }

        std::map <int,int> round_kick_exam(int exam)
        {
            this->move_executed=7;
            int period1,period2,period3,exam2,exam3;
            period3=this->s_periods[exam];
            for(auto &exam2:this->neighbors[exam2])
            {
                period1=this->s_periods[exam2];
                if(this->can_be_moved(exam,period1,{exam2}))
                {
                    for(auto &exam3:this->neighbors[exam2])
                    {
                        if(exam3==exam) continue;
                        period2=this->s_periods[exam3];
                        if(this->can_be_moved(exam2,period2,{exam3}))
                        {
                            if(this->can_be_moved(exam3,period3,{exam}))
                            return {
                                {exam,period1},
                                {exam2,period2},
                                {exam3,period3}
                            };
                        }
                    }
                }
            }
            return std::map <int,int>();
        }

        void eject_vertices()
        {
            auto ut=std::uniform_real_distribution <double>(0.75,1.0);
            std::vector <std::pair <int,int>> wedges; 
            for(auto &edge:this->edges)
            {
                if(this->adj_table[edge.first][edge.second]>static_cast <int>(this->average_edge_weight))
                {
                    wedges.emplace_back(edge);
                }
            }
            int distance,rb,lb,mcost,pfirst,psecond;
            int best_contribution;
            std::vector <int>  ej;
            std::map <int,int> mw;
            std::map <int,int> mv;
            for(auto &ed:wedges)
            {
                ej={ed.first,ed.second};
                mw={{ed.first,ed.second},{ed.second,ed.first}};
                mv.clear();
                while(!ej.empty())
                {
                    pfirst=ej.back();
                    ej.pop_back();
                    psecond=mw[pfirst];
                    distance=abs(this->s_periods[ed.first]-this->s_periods[ed.second]);
                    best_contribution=this->cost;
                    if(distance<=5)
                    {
                        lb=this->s_periods[pfirst]-5<0?0:this->s_periods[pfirst]-5;
                        rb=this->s_periods[pfirst]+5>this->P-1?this->P-1:this->s_periods[psecond]+5;
                        int j=lb!=0?lb:rb;
                        if(j==rb)
                        {
                            j++;
                            for(j;j<this->P-1;j++)
                            {
                                if(this->can_be_moved(pfirst,j))
                                {
                                    mcost=this->cost+this->pre_calculation(pfirst,j);
                                    if(mcost<best_contribution)
                                    {
                                        best_contribution=mcost;
                                        mv={{pfirst,j}};
                                    }
                                }
                            }
                        }
                        else
                        {
                            for(int j=0;j<lb;j++)
                            {
                                if(this->can_be_moved(pfirst,j))
                                {
                                    mcost=this->cost+this->pre_calculation(pfirst,j);
                                    if(mcost<best_contribution)
                                    {
                                        best_contribution=mcost;
                                        mv={{pfirst,j}};
                                    }
                                }
                            }
                            for(int j=rb+1;j<this->P-1;j++)
                            {
                                if(this->can_be_moved(pfirst,j))
                                {
                                    mcost=this->cost+this->pre_calculation(pfirst,j);
                                    if(mcost<best_contribution)
                                    {
                                        best_contribution=mcost;
                                        mv={{pfirst,j}};
                                    }
                                }
                            }
                        }
                    }
                }
                if(mv.size()!=0)
                {
                    this->reposition(mv);
                    int texam;
                    for(auto &mid:mv)
                    {
                        texam=(mid.first==ed.first?ed.second:ed.first);
                        std::cout<<"Ejecting exam|Exam Eject->"<<mid.first<<"->Period:"<<mid.second<<"|Second exam particapate->"<<texam<<"=>P:"<<this->s_periods[texam]<<std::endl;
                    }
                }
            }
        }


        // Higher factoring nodes optimizing
        // ------------------------------------

        void high_factoring_parallel_moves_execution(omp_lock_t &lock,int i,int exam,std::map <int,std::pair<std::map <int,int>,int>> &moves_exec)
        {
            omp_set_lock(&lock);
            int scost=this->cost;
            int best_contribution=this->cost;
            int mcost;
            int period1,period2,period3;
            std::map <int,int> mvs;
            std::map <int,int> bmoves;
            int exam2;
            switch(i)
            {
                case 1:
                    for(int i=0;i<this->P;i++)
                    {
                        if(this->can_be_moved(exam,i))
                        {
                            mcost=scost+this->pre_calculation(exam,i);
                            if(mcost<best_contribution)
                            {
                                best_contribution=mcost;
                                bmoves={{exam,i}};
                            }
                        }
                    }
                    break;
                case 2:
                    exam2=this->get_max_weighted_vertex(exam);
                    period1=this->s_periods[exam2];
                    if(this->can_be_moved(exam,period1,{exam2}))
                    {
                        for(int i=0;i<this->P;i++)
                        {
                            if(i==this->s_periods[exam] || i==period1) continue;
                            period2=i;
                            if(this->can_be_moved(exam2,period2))
                            {
                                mvs={
                                    {exam,period1},
                                    {exam2,period2}
                                };
                                mcost=scost+this->pre_reposition(mvs);
                                if(mcost<best_contribution)
                                {
                                    best_contribution=mcost;
                                    bmoves=mvs;
                                }
                            }
                        }
                    }
                    break;
                case 3:
                    exam2=this->get_max_weighted_vertex(exam);
                    period1=this->s_periods[exam];
                    if(this->can_be_moved(exam,period1,{exam2}))
                    {
                        for(auto &exam3:this->neighbors[exam2])
                        {
                            if(exam3==exam) continue;
                            period2=this->s_periods[exam2];
                            if(this->can_be_moved(exam2,period2,{exam3}))
                            {
                                for(int period3=0;period3<this->P;period3++)
                                {
                                    if(period3==period2 || period3==period1 || period3==this->s_periods[exam])
                                    {
                                        continue;
                                    }
                                    if(this->can_be_moved(exam3,period3))
                                    {
                                        mvs={
                                            {exam,period1},
                                            {exam2,period2},
                                            {exam3,period3}
                                        };
                                        mcost=scost+this->pre_reposition(mvs);
                                        if(mcost<best_contribution)
                                        {
                                            best_contribution=mcost;
                                            bmoves=mvs;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    break;
                default:
                    bmoves={};
                    break;
            }
            moves_exec[i]=std::pair<std::map <int,int>,int>(bmoves,best_contribution);
            omp_unset_lock(&lock);
        }

        void higher_node_optimizing(int &best,std::map <int,int> &best_sol)
        {
            omp_lock_t lock;
            omp_init_lock(&lock);
            int acceptance_rate=random::accept_rate()*this->average_node_factor;
            std::vector <int> exams;
            for(auto &node:this->nodes)
            {
                if(this->node_factor[node]>acceptance_rate)
                {
                    exams.emplace_back(node);
                }
            }
            std::shuffle(exams.begin(),exams.end(),std::default_random_engine{});
            int cs=INT_MAX,mcost;
            int best_solution=best;
            this->move_executed=10;
            for(auto &exam:exams)
            {
                std::map <int,std::pair<std::map<int,int>,int>> candicate_moves;
                std::map <int,int> cr_moves;
                #pragma omp parallel for
                for(int i=1;i<=3;i++)
                {
                    this->high_factoring_parallel_moves_execution(lock,i,exam,candicate_moves);
                }
                #pragma omp barrier
                cs=INT_MAX;
                for(auto &move:candicate_moves)
                {   
                    if(move.second.second<cs)
                    {
                        cs=move.second.second;
                        cr_moves=move.second.first;
                    }
                }
                int delta;
                mcost=this->cost+this->pre_reposition(cr_moves);
                if(mcost<best)
                {
                    delta=mcost-this->cost;
                    this->reposition(cr_moves);
                    best=this->cost;
                    best_sol=this->s_periods;
                    std::cout<<"Exam:"<<exam<<"|New best Solution S="<<best<<"{";
                    bool start=true;
                    for(auto &m:cr_moves)
                    {
                        if(start)
                        {
                            start=false;
                            std::cout<<"("<<m.first<<"->"<<m.second<<")";
                            continue;
                        }
                        std::cout<<","<<"("<<m.first<<"->"<<m.second<<")";
                    }
                    std::cout<<"} Delta:"<<delta<<std::endl;
                }
            }
            omp_destroy_lock(&lock);
            this->add_occurence(best!=best_solution);
        }

        // Paraller moves execution
        void add_move(omp_lock_t &lockr,int id,int exam,std::map <int,std::pair <std::map <int,int>,int>> &move_making)
        {
            omp_set_lock(&lockr);
            std::map <int,int> moves=this->select_move(id,exam);
            int mcost=this->pre_reposition(moves);
            move_making[id]=std::pair<std::map<int,int>,int>(moves,mcost);
            omp_unset_lock(&lockr);
        }

        int move_pool(std::map <int,int> &pool_moves)
        {
            omp_lock_t lockres;
            std::map <int,std::pair<std::map <int,int>,int>> moves;
            int exam=this->random_exam();
            omp_init_lock(&lockres);
            #pragma omp paraller for
            for(int i=1;i<=7;i++)
            {
                this->add_move(lockres,i,exam,moves);
            }
            #pragma omp barrier
            int cs=INT_MAX;
            for(auto &x:moves)
            {
                if(x.second.second<cs)
                {
                    cs=x.second.second;
                    pool_moves=x.second.first;
                    this->move_executed=x.first;
                }
            }
            omp_destroy_lock(&lockres);
            return cs;
        }

        bool pwork(int exec_seconds,int &best,std::map <int,int> &bsol,int &move_counter)
        {
            this->move_executed=8;
            move_counter=1;
            int starting_best_point=best;
            auto st=clock();
            int cp;
            while(true)
            {
               std::map <int,int> moves;
               cp=this->cost+this->move_pool(moves);
               if(cp<best)
               {
                   this->reposition(moves);
                   this->add_occurence(moves.empty());
                   best=this->cost;
                   auto et=clock()-st;
                   std::cout<<"Paraller Move Execution|New best Solution S="<<best<<" TM="<<et/CLOCKS_PER_SEC<<" Move|"<<move_counter<<"-"<<std::string(this->stats[this->move_executed])<<std::endl;
               }
               auto et=clock()-st;
               if(et/CLOCKS_PER_SEC>exec_seconds)
               {
                   break;
               }
               move_counter++;
            }
            this->add_occurence(starting_best_point!=best);
            return  starting_best_point!=best;
        }
        
        void polish(int exec_time,int &best,std::map <int,int> &bsol)
        {
            this->move_executed=9;
            int mcost,delta;
            int best_exam_contribution;
            std::map <int,int> moves;
            std::map <int,int> best_moves;
            bool start;
            std::map <int,int> np;
            int best_current_contribution;
            int current_exam,current_period,new_period;
            auto stime=clock();
            while(true)
            {
                std::shuffle(this->nodes.begin(),this->nodes.end(),std::default_random_engine{});
                for(auto &exam:this->nodes)
                {
                    best_exam_contribution=this->neighbors_delta(exam,this->s_periods[exam]);
                    for(int i=0;i<this->P;i++)
                    {
                        if(this->can_be_moved(exam,i))
                        {
                            mcost=this->pre_calculation(exam,i);
                            if(mcost<best_exam_contribution)
                            {
                                best_exam_contribution=mcost;
                                moves={{exam,i}};
                            }
                        }
                    }
                    mcost=this->cost+this->pre_reposition(moves);
                    if(mcost<best)
                    {
                        delta=mcost-this->cost;
                        this->reposition(moves);
                        bsol=this->s_periods;
                        best=this->cost;
                        std::cout<<"Polish Move Exam|New best Solution S="<<best<<"{";
                        for(auto &mid:moves)
                        {
                            std::cout<<"("<<mid.first<<"->"<<mid.second<<")";
                        }
                        std::cout<<"}"<<std::endl;
                    }
                }
                auto et=clock()-stime;
                if(et/CLOCKS_PER_SEC>exec_time)
                {
                    break;
                }

                // Polish Swap Exams
                for(auto &node:this->dual_comb)
                {
                    if(this->can_be_moved(node.first,this->s_periods[node.second],{node.second}) && this->can_be_moved(node.second,this->s_periods[node.first],{node.second}))
                    {
                        moves[node.first]=this->s_periods[node.second];
                        moves[node.second]=this->s_periods[node.first];
                        if(this->cost+this->pre_reposition(moves)<best)
                        {
                            this->reposition(moves);
                            best=this->cost;
                            std::cout<<"Polish Swap Exams|New best Solution S="<<best<<" {";
                            start=true;
                            for(auto &mid:moves)
                            {
                                if(start)
                                {
                                    start=false;
                                    std::cout<<"("<<mid.first<<"-"<<mid.second<<")";
                                    continue;
                                }
                                std::cout<<","<<"("<<mid.first<<"-"<<mid.second<<")";
                            }
                            std::cout<<"} Delta:"<<(delta<0?"(-)":"(+)")<<delta<<std::endl;
                        }
                    }
                }
                et=clock()-stime;
                if(et/CLOCKS_PER_SEC>exec_time)
                {
                    break;
                }
                // Polish Swap Exams
                std::shuffle(this->nodes.begin(),this->nodes.end(),std::default_random_engine{});
                for(auto &node:this->nodes)
                {
                    best_exam_contribution=this->cost;
                    for(auto &neighbor:this->neighbors[node])
                    {
                        if(this->can_be_moved(node,this->s_periods[neighbor],{neighbor}) && this->can_be_moved(neighbor,this->s_periods[node],{node}))
                        {
                            moves={
                                {node,this->s_periods[neighbor]},
                                {neighbor,this->s_periods[node]}
                            };
                            mcost=this->cost+this->pre_reposition(moves);
                            if(mcost<best_exam_contribution)
                            {
                                best_exam_contribution=mcost;
                                best_moves=moves;
                            }
                        }
                    }
                    if(!best_moves.empty())
                    {
                        this->reposition(best_moves);
                        best=this->cost;
                        bsol=this->s_periods;
                        std::cout<<"Polish Swap Exams|New best Solution S="<<best<<"{";
                        start=true;
                        for(auto &mid:best_moves)
                        {
                            if(start)
                            {
                                start=false;
                                std::cout<<"("<<mid.first<<"->"<<mid.second<<")";
                                continue;
                            }
                            std::cout<<",("<<mid.first<<"->"<<mid.second<<")";
                        }
                        std::cout<<std::endl;
                    }
                }
                et=clock()-stime;
                if(et/CLOCKS_PER_SEC>exec_time)
                {
                    break;
                }

                // Polish Kempe Chain
                std::shuffle(this->edges.begin(),this->edges.end(),std::default_random_engine{});
                for(auto &pn:this->edges)
                {
                    moves.clear();
                    best_current_contribution=this->cost;
                    const int sp1=this->s_periods[pn.first];
                    const int sp2=this->s_periods[pn.second];
                    np={{sp1,sp2},{sp2,sp1}};
                    std::stack <int> kc;
                    kc.push(pn.first);
                    std::map <int,int> moves;
                    while(!kc.empty())
                    {
                        current_exam=kc.top();
                        kc.pop();
                        current_period=this->s_periods[current_exam];
                        new_period=np[current_period];
                        moves[current_exam]=new_period;
                        for(auto &exam2:this->neighbors[current_exam])
                        {
                            if(moves.find(exam2)!=moves.end()) continue;
                            if(this->s_periods[exam2]==new_period)
                            {
                                kc.push(exam2);
                            }
                        }
                    }
                    if(this->cost+this->pre_reposition(moves)<best)
                    {
                        this->reposition(moves);
                        best=this->cost;
                        bsol=this->s_periods;
                        std::cout<<"Polish Kempe Chain|New best Solution S="<<best<<"{";
                        start=true;
                        for(auto &mid:moves)
                        {
                            if(start)
                            {
                                start=false;
                                std::cout<<"("<<mid.first<<"->"<<mid.second<<")";
                                continue;
                            }
                            std::cout<<",("<<mid.first<<"->"<<mid.second<<")";
                        }
                        std::cout<<"}"<<std::endl;
                    }
                }

                et=clock()-stime;
                if(et/CLOCKS_PER_SEC>exec_time)
                {
                    break;
                }

                // Polish Kick Exam
                std::shuffle(this->non_zero_neighbors_exams.begin(),this->non_zero_neighbors_exams.end(),std::default_random_engine{});
                int period1,period2;
                std::map <int,int> best_moves;
                for(auto &exam:this->non_zero_neighbors_exams)
                {
                    best_exam_contribution=this->cost;
                    for(auto &exam2:this->neighbors[exam])
                    {
                        period1=this->s_periods[exam2];
                        if(this->can_be_moved(exam,period1,{exam2}))
                        {
                            for(int p2=0;p2<this->P;p2++)
                            {
                                if(p2==period1 || p2==this->s_periods[exam]) continue;
                                if(this->can_be_moved(exam2,p2))
                                {
                                    moves={
                                        {exam,period1},
                                        {exam2,p2}
                                    };
                                    mcost=this->cost+this->pre_reposition(moves);
                                    if(mcost<best_exam_contribution)
                                    {
                                        best_exam_contribution=mcost;
                                        best_moves=moves;
                                    }
                                }
                            }
                        }
                        if(best_exam_contribution<best)
                        {
                            this->reposition(best_moves);
                            best=this->cost;
                            bsol=this->s_periods;
                            std::cout<<"Polish Kick Exam|New best Solution found S="<<best<<"=>{";
                            start=true;
                            for(auto &mid:best_moves)
                            {
                                if(start)
                                {
                                    start=false;
                                    std::cout<<"("<<mid.first<<"->"<<mid.second<<")";
                                    continue;
                                }
                                std::cout<<","<<"("<<mid.first<<"->"<<mid.second<<")";
                            }
                            std::cout<<"}"<<std::endl;
                        }
                    }
                }
                et=clock()-stime;
                if(et/CLOCKS_PER_SEC>exec_time)
                {
                    break;
                }
            }
        }

        void permute_periods(int &best,std::map <int,int> &best_sol)
        {
            int mcost;
            bool f=false;
            auto st=clock();
            while(true)
            {
                for(int i=0;i<this->P;i++)
                {
                    for(int j=i+1;j<this->P;j++)
                    {   
                        std::map <int,int> moves;
                        for(auto &exam:this->periods[i])
                        {
                            moves[exam]=j;
                        }
                        for(auto &exam:this->periods[j])
                        {
                            moves[exam]=i;
                        }
                        mcost=this->cost+this->pre_reposition(moves);
                        if(mcost<best)
                        {
                            int delta=mcost-this->cost;
                            this->reposition(moves);
                            best=this->cost;
                            std::cout<<"Permuting Periods|New best Solution S="<<best<<" Moves="<<moves.size()<<"|D="<<(delta<0?"(-)":"(+)")<<abs(delta)<<std::endl;
                        }
                    }
                    auto et=clock()-st;
                    if(et/CLOCKS_PER_SEC>10)
                    {
                        f=true;
                        break;
                    }
                }
                if(f)
                {
                    break;
                }
            }
        }

        void add_occurence(bool is_null=false)
        {
            is_null?this->stats[this->move_executed]--:this->stats[this->move_executed]++;
        }
};

class hill_climber:public Solution
{
    private:
        int execution_time;
    public:
        hill_climber(problem &p,int exec=0):Solution(p)
        {
            this->execution_time=exec;
        }

        void set_execution_time(int exec_seconds)
        {
            this->execution_time=exec_seconds;
        }

        void solve()
        {
            int best=this->cost;
            int mcost;
            logger log{LOG_INFO};
            auto stime=clock();
            int cnull=0;
            int move_id=1;
            while(true)
            {
                std::map <int,int> moves=this->select_move();
                this->add_occurence(moves.empty());
                if(moves.empty())
                {
                    auto et=clock();
                    if(et/CLOCKS_PER_SEC>this->execution_time)
                    {
                        break;
                    }
                    continue;
                }
                mcost=this->cost+this->pre_reposition(moves);
                if(mcost<best)
                {
                    this->reposition(moves);
                    auto tn=(clock()-stime)/CLOCKS_PER_SEC;
                    best=this->cost;
                    log.info("Hill Climbing|New best Solution S="+std::to_string(this->cost)+" T="+std::to_string(tn)+" secs Move|"+std::to_string(move_id)+"-"+std::string(this->stats[this->move_executed]));
                }
                else
                {
                    cnull++;
                }
                auto end_time=clock()-stime;
                if(end_time/CLOCKS_PER_SEC>this->execution_time)
                {
                    break;
                }
                move_id++;
            }
            log.info("Non optimizing Moves:"+std::to_string(cnull));
            log.info("After "+std::to_string(this->execution_time)+" seconds best solution found:"+std::to_string(best));
            std::cout<<Bool2String(this->is_feasible())<<std::endl;
            std::cout<<*this;
        }

        friend std::ostream &operator<<(std::ostream &os,hill_climber &h)
        {
            TextTable t;
            t.add("MOVE");
            t.add("OCCURENCES");
            t.add("NULL OCCURENCES");
            t.add("VALID HITS (PERCENT)");
            t.endOfRow();
            double diff;
            int sum;
            for(auto &stat:h.stats)
            {
                t.add(std::string(stat));
                t.add(std::to_string(stat.at("count")));
                t.add(std::to_string(stat.at("null")));
                sum=stat.at("count")+stat.at("null");
                diff=(stat.at("count")/double(sum))*100.0;
                t.add(std::to_string(diff)+"%");
                t.endOfRow();
            }
            return os<<t;
        }
};

class simulated_annealer:public Solution
{
    private:
        int execution_time;
        int move_counter;
        decltype(clock()) real_exec_time;
    public:
        simulated_annealer(problem &p,int es):Solution(p),execution_time(es),move_counter(0)
        {

        }

        void set_time(int es) {this->execution_time=es;}

        void solve()
        {
            double start_temp=1000.0,temperature=1000.0,freeze_temp=0.0001,alpha=0.999;
            int last_improvement_counter=0;
            int plateu=0;
            double acceptance_propability;
            int temp_delay_counter=0;
            const int temp_delay_iterations=4;
            int best=this->cost;
            std::map <int,int> best_sol=this->s_periods;
            int move_id=1;
            int move_cost;
            int delta;
            int round=1;
            int reheat_counter=REHEAT_ITERATIONS;
            bool bsol=false;
            logger log{LOG_INFO};
            std::cout.precision(6);
            std::cout<<"========== Start With Period permutation ============"<<std::endl;
            this->permute_periods(best,best_sol);
            std::cout<<"====================================================="<<std::endl<<std::endl;
            this->eject_vertices();
            auto stime=clock();
            while(true)
            {
                std::map <int,int> moves=this->select_move();
                this->add_occurence(moves.empty());
                if(moves.size()==0)
                {
                    move_id++;
                    auto etime=clock()-stime;
                    if(etime/CLOCKS_PER_SEC>this->execution_time)
                    {
                        break;
                    }
                    continue;
                }
                move_cost=this->cost+this->pre_reposition(moves);
                delta=move_cost-this->cost;
                if(move_cost<best)
                {
                    this->reposition(moves);
                    best=this->cost;
                    best_sol=this->s_periods;
                    auto tn=(clock()-stime)/CLOCKS_PER_SEC;
                    plateu=0;
                    last_improvement_counter=0;
                    temp_delay_counter=temp_delay_iterations;
                    bsol=true;
                    log.info("Simulated Annealing|New best Solution S="+std::to_string(best)+" T="+std::to_string(temperature)+TEMP_SIGN+" TM="+std::to_string(tn)+"\'s Move|"+std::to_string(move_id)+"-"+std::string(this->stats[move_executed]));
                }
                else if(move_cost>best)
                {
                    last_improvement_counter++;
                    acceptance_propability=exp(-delta/temperature);
                    if(acceptance_propability>random::random_normalized())
                    {
                        this->reposition(moves);
                        log.debug("Worse Solution has been accepted with cost S="+std::to_string(this->cost));
                    }
                }
                else
                {
                    last_improvement_counter++;
                }
                if(temp_delay_counter>0)
                {
                    temp_delay_counter--;
                }
                else
                {
                    temperature*=alpha;
                }
                if(last_improvement_counter>3000)
                {
                    int mc=0;
                    bool fbest=this->pwork(this->execution_time/this->P,best,best_sol,mc);
                    this->move_executed=9;
                    last_improvement_counter=0;
                    this->add_occurence(fbest);
                    if(fbest)
                    {
                        log.info("PSA|New best Solution found S="+std::to_string(best)+" Moves:"+std::to_string(mc));
                        last_improvement_counter=0;
                        move_id++;
                    }
                    else
                    {
                        temperature/=random::temp_derived();
                        this->polish(this->execution_time/this->P,best,best_sol);
                        log.info("No improvement made|Temperature derived T="+std::to_string(temperature)+TEMP_SIGN);
                    }
                }
                if(temperature<freeze_temp)
                {
                    if(bsol && reheat_counter>0)
                    {
                        bsol=false;
                        temperature=REHEAT_TEMP;
                        reheat_counter--;
                        log.info("Simulated Annealing|T="+std::to_string(temperature)+TEMP_SIGN+" R="+std::to_string(round));
                        continue;
                    }
                    int pbest=best;
                    this->higher_node_optimizing(best,best_sol);
                    if(pbest<best)
                    {
                        plateu=0;
                    }
                    else
                       plateu++;
                    reheat_counter=REHEAT_ITERATIONS;
                    temperature=start_temp*random::normalized_temp()/round;
                    round++;
                    last_improvement_counter=0;
                    log.info("Simulated Annealing|New round started==>T="+std::to_string(temperature)+TEMP_SIGN+" R="+std::to_string(round));
                }
                if(plateu==10)
                {
                    auto mjreheat=std::uniform_real_distribution<double>(0.5,1.5);
                    temperature=start_temp*mjreheat(mt);
                    log.info("No improvement rounds=10|Major reheat=>T="+std::to_string(temperature)+TEMP_SIGN);
                }
                if(plateu==20)
                {
                    log.info("After 20 rounds no optimization made!!Exit Annealing process");
                    break;
                }
                auto etime=clock()-stime;
                if(etime/CLOCKS_PER_SEC>this->execution_time)
                {
                    break;
                }
                move_id++;
            }
            this->reposition(best_sol);
            this->move_counter=move_id;
            log.info("Simulated Annealing|Process Ended==>Best Solution S="+std::to_string(this->compute_cost())+"("+std::to_string(this->compute_normalized_cost())+")");
            std::cout<<*this;
            this->real_exec_time=clock()-stime;
        }

        friend std::ostream &operator<<(std::ostream &os,simulated_annealer &h)
        {
            TextTable t;
            t.add("MOVE");
            t.add("OCCURENCES");
            t.add("NULL OCCURENCES");
            t.add("VALID HITS (PERCENT)");
            t.endOfRow();
            double diff;
            int sum;
            for(auto &stat:h.stats)
            {
                t.add(std::string(stat));
                t.add(std::to_string(stat.at("count")));
                t.add(std::to_string(stat.at("null")));
                sum=stat.at("count")+stat.at("null");
                diff=(stat.at("count")/double(sum))*100.0;
                t.add(std::to_string(diff)+"%");
                t.endOfRow();
            }
            return os<<t;
        }

        int getmoves()const {return this->move_counter;}

        void Export()
        {
            std::string filename=data::sa_folder+this->id+".sol";
            std::fstream fs(filename,std::ios::out);
            for(auto &mid:this->s_periods)
            {
                fs<<mid.first<<" "<<mid.second<<"; ";
            }
            fs<<std::endl;
            fs<<"Cost:"<<this->compute_cost()<<std::endl;
            fs<<"Normalized Cost:"<<this->compute_normalized_cost()<<std::endl;
            fs<<"Execution Time:"<<this->real_exec_time/CLOCKS_PER_SEC<<std::endl;
            fs.close();
        }
};

std::map <int,int> simulated_annealing(std::string dataset,std::map <int,std::map<int,int>> &graph,std::map <int,int> &sol,int ns,int execution_time)
{
    problem p(dataset,graph,sol,ns);
    simulated_annealer sa_solver(p,execution_time);
    sa_solver.solve();
    sa_solver.Export();
    return sa_solver.s_periods;
}