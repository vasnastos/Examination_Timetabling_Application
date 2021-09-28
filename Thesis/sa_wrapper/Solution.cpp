#include "parameters.hpp"
#include <bits/stdc++.h>

class Solution:public Graph{
    protected:
        std::map <int,std::vector <int>> periods;
        std::vector <pmove> stats;
        std::set <int> fixed_exams;
        std::map <int,std::vector <int>> ident_period_exams;
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
                // if(this->fixed_exams.find(exam)!=this->fixed_exams.end()) continue;
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
        decltype(std::chrono::high_resolution_clock::now()) start_timer;
        int cost;
        int thermal_periods;
        
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

        ~Solution() {}

        void set_fixed_exams(std::set <int> &exams)
        {
            this->fixed_exams=exams;
        }

        void add_fixed_exam(int exam)
        {
            this->fixed_exams.insert(exam);
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

        int pre_calculation(int exam,int period,std::map <int,int> &previous_made_moves,std::map <int,int> &cached)
        {
            int cs=0,nnstate;
            int old_state=previous_made_moves.find(exam)!=previous_made_moves.end()?previous_made_moves[exam]:this->s_periods[exam];
            int distance,distance_old;
            for(auto &neighbor:this->neighbors[exam])
            {
                nnstate=cached.find(neighbor)!=cached.end()?cached[neighbor]:previous_made_moves.find(neighbor)!=previous_made_moves.end()?previous_made_moves[neighbor]:this->s_periods[neighbor];
                distance_old=abs(old_state-nnstate);
                distance=abs(period-nnstate);
                if(distance_old==distance) continue;
                cs-=Graph::penalty[distance_old] * this->adj_table[exam][neighbor];
                cs+=Graph::penalty[distance] * this->adj_table[exam][neighbor];
            }
            return cs;
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

        int pre_reposition(std::map <int,int> &moves,std::map <int,int> &previous_moves_made)
        {
            std::map <int,int> cached;
            int cs=0;
            for(auto &move:moves)
            {
                cs+=this->pre_calculation(move.first,move.second,previous_moves_made,cached);
                cached.insert(move);
            }
            return cs;
        }

        int pre_reposition(int exam,int p,std::map <int,int> &previous_moves_made)
        {
            int cs=0;
            int distance,distance_before;
            int neighbor_state;
            int exam_old_state=previous_moves_made.find(exam)!=previous_moves_made.end()?previous_moves_made[exam]:this->s_periods[exam];
            for(auto &neighbor:this->neighbors[exam])
            {
                neighbor_state=previous_moves_made.find(neighbor)!=previous_moves_made.end()?previous_moves_made[neighbor]:this->s_periods[neighbor];
                distance=abs(p-neighbor_state);
                distance_before=abs(exam_old_state-neighbor_state);
                if(distance==distance_before) continue;
                cs-=this->adj_table[exam][neighbor] * Graph::penalty[distance_before];
                cs+=this->adj_table[exam][neighbor] * Graph::penalty[distance];
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
            std::uniform_int_distribution<int> r(1,10);
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
                case 9:
                    return this->move_exam_extend(exam);
                    break;
                case 10:
                    return this->double_kick_exam(exam);
                    break;
                case 11:
                    return this->depth_moves();
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
                case 9:
                    return this->move_exam_extend(exam);
                    break;
                case 10:
                    return this->double_kick_exam(exam);
                    break;
                default:
                    return std::map <int,int>();
                    break;
            }
        }

        std::map <int,int> select_move(int i,int exam,std::map <int,int> &sim_moves,int sim_cost)
        {
            int best_exam_cost_contribution,old_period;        
            std::map <int,int> moves;
            int cost_prediction;
            std::map <int,int> fmoves;
            best_exam_cost_contribution=sim_cost;
            if(i==1)
            {

                for(int p=0;p<this->P;p++)
                {
                    old_period=sim_moves.find(exam)!=sim_moves.end()?sim_moves[exam]:this->s_periods[exam];
                    if(p==old_period) continue;
                    if(this->can_be_moved(exam,p,sim_moves))
                    {   
                        cost_prediction=sim_cost+this->pre_reposition(exam,p,sim_moves);
                        if(cost_prediction<best_exam_cost_contribution)
                        {
                            best_exam_cost_contribution=cost_prediction;
                            fmoves={{exam,p}};
                        }
                    }
                }
                return fmoves;
            }
            else if(i==2)
            {
                int period_exam=sim_moves.find(exam)!=sim_moves.end()?sim_moves[exam]:this->s_periods[exam];
                int period_exam_2;
                for(auto &neighbor:this->neighbors[exam])
                {
                    moves=std::map <int,int>();
                    period_exam_2=sim_moves.find(neighbor)!=sim_moves.end()?sim_moves[neighbor]:this->s_periods[neighbor];
                    if(this->can_be_moved(exam,period_exam_2,sim_moves,{neighbor}) && this->can_be_moved(neighbor,period_exam,sim_moves,{exam}))
                    {
                        moves={
                            {exam,period_exam_2},
                            {neighbor,period_exam}
                        };
                        cost_prediction=sim_cost+this->pre_reposition(moves,sim_moves);
                        if(cost_prediction<best_exam_cost_contribution)
                        {
                            best_exam_cost_contribution=cost_prediction;
                            fmoves=moves;
                        }
                    }
                }
                return fmoves;
            }
            else if(i==3)
            {
                int period_1=sim_moves.find(exam)!=sim_moves.end()?sim_moves[exam]:this->s_periods[exam];
                int period_2;
                std::map <int,int> versus_period;
                std::stack <int> backlog;
                int current_period,current_exam,new_period,neighbor_state;
                for(auto &neighbor:this->neighbors[exam])
                {
                    period_2=sim_moves.find(neighbor)!=sim_moves.end()?sim_moves[neighbor]:this->s_periods[neighbor];
                    versus_period={
                        {period_1,period_2},
                        {period_2,period_1}
                    };
                    backlog=std::stack<int>();
                    backlog.push(exam);
                    moves=std::map<int,int>();
                    while(!backlog.empty())
                    {
                        current_exam=backlog.top();
                        backlog.pop();
                        current_period=sim_moves.find(current_exam)!=sim_moves.end()?sim_moves[current_exam]:this->s_periods[current_exam];
                        new_period=versus_period[current_period];
                        moves[current_exam]=new_period;
                        for(auto &neighbor:this->neighbors[current_exam])
                        {
                            if(moves.find(neighbor)!=moves.end()) continue;
                            neighbor_state=sim_moves.find(neighbor)!=sim_moves.end()?sim_moves[neighbor]:this->s_periods[neighbor];
                            if(neighbor_state==new_period)
                            {
                                backlog.push(neighbor);
                            }
                        }
                    }
                    cost_prediction=sim_cost+this->pre_reposition(moves,sim_moves);
                    if(cost_prediction<best_exam_cost_contribution)
                    {
                        best_exam_cost_contribution=cost_prediction;
                        fmoves=moves;
                    }
                }
                return fmoves;
            }
            else if(i==4)
            {
                int period1;
                int old_exam1_period=sim_moves.find(exam)!=sim_moves.end()?sim_moves[exam]:this->s_periods[exam];
                for(auto &neighbor:this->neighbors[exam])
                {
                    moves=std::map <int,int>();
                    period1=sim_moves.find(neighbor)!=sim_moves.end()?sim_moves[neighbor]:this->s_periods[neighbor];
                    if(this->can_be_moved(exam,period1,sim_moves,{neighbor}))
                    {
                        for(int p=0;p<this->P;p++)
                        {
                            if(p==period1  || p==old_exam1_period) continue;
                            if(this->can_be_moved(neighbor,p,sim_moves))
                            {
                                moves={
                                    {exam,period1},
                                    {neighbor,p}
                                };
                                cost_prediction=sim_cost+this->pre_reposition(moves,sim_moves);
                                if(cost_prediction<best_exam_cost_contribution)
                                {
                                    best_exam_cost_contribution=cost_prediction;
                                    fmoves=moves;
                                }
                            }
                        }
                    }
                }
                return fmoves;
            }
            return std::map <int,int>();
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
            for(auto &exam2:this->neighbors[exam])
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

        std::map <int,int> move_exam_extend(int exam)
        {
            this->move_executed=8;
            const int period_selector=this->s_periods[exam];
            auto exam_selector=std::uniform_int_distribution<int>(0,this->periods[period_selector].size()-1);
            int exam1=exam;
            int exam2=this->periods[period_selector].at(exam_selector(mt));
            while(exam2==exam1)
            {
                exam2=this->periods[period_selector].at(exam_selector(mt));
            }
            int min_cost_contribution_exam1=INT_MAX;
            int min_cost_contribution_exam2=INT_MAX;
            int cs;
            std::map <int,int> moves;
            int period_exam1=this->s_periods[exam1];
            int period_exam2=this->s_periods[exam2];
            std::pair <int,int> moves_exam1;
            std::pair <int,int> moves_exam2;
            for(auto &neighbor:this->neighbors[exam1])
            {
                cs=this->pre_calculation(neighbor,period_exam1);
                if(this->can_be_moved(neighbor,period_exam1,{exam1}))
                {
                    if(cs<min_cost_contribution_exam1)
                    {
                        min_cost_contribution_exam1=cs;
                        moves_exam1={neighbor,period_exam1};
                    }
                }
            }
            if(moves_exam1==std::pair<int,int>()) return std::map <int,int>();
            for(auto &neighbor:this->neighbors[exam2])
            {
                cs=this->pre_calculation(neighbor,period_exam2);
                if(this->can_be_moved(neighbor,period_exam2,{exam2}))
                {
                    if(cs<min_cost_contribution_exam2)
                    {
                        min_cost_contribution_exam2=cs;
                        moves_exam2={neighbor,period_exam2};
                    }
                }
            }
            if(moves_exam2==std::pair<int,int>()) return std::map <int,int>();
            if(moves_exam1.first==moves_exam2.first)
            {
                if(min_cost_contribution_exam1<=min_cost_contribution_exam2)
                {
                    moves.insert(moves_exam1);
                }
                else
                {
                    moves.insert(moves_exam2);
                }
            }
            else{
                moves.insert(moves_exam1);
                moves.insert(moves_exam2);
            }
            std::map <int,int> me;
            min_cost_contribution_exam1=min_cost_contribution_exam2=INT_MAX;
            bool found1=false,found2=false;
            for(int p=0;p<this->P;p++)
            {
                me=moves;
                if(this->can_be_moved(exam1,p,moves))
                {
                    me[exam1]=p;
                    cs=this->pre_reposition(me);
                    if(cs<min_cost_contribution_exam1)
                    {
                        min_cost_contribution_exam1=cs;
                        moves_exam1={exam1,p};
                        found1=true;
                    }
                }
                if(this->can_be_moved(exam2,p,moves))
                {
                    me=moves;
                    me[exam2]=p;
                    cs=this->pre_reposition(me);
                    if(cs<min_cost_contribution_exam2)
                    {
                        min_cost_contribution_exam2=cs;
                        moves_exam2={exam2,p};
                        found2=true;
                    }
                }
            }
            if(!found1 || !found2) return std::map <int,int>();
            moves.insert(moves_exam1);
            moves.insert(moves_exam2);
            return moves;
        }

        std::map <int,int> double_kempe_chain(int exam)
        {
            this->move_executed=9;
            int exam1=exam;
            auto neighbor_selector=std::uniform_int_distribution<int>(0,this->neighbors[exam1].size()-1);
            int exam2=this->neighbors[exam1].at(neighbor_selector(mt));
            int c1,c2,c3,c4;
            c1=this->s_periods[exam1];
            c2=this->s_periods[exam2];
            int exam3,exam4;
            std::set <int> occurences;
            while(true)
            {
                exam3=this->random_exam();
                c3=this->s_periods[exam3];
                if(c3==c1 or c3==c2) continue;
                break;
            }
            neighbor_selector=std::uniform_int_distribution<int>(0,this->neighbors[exam3].size()-1);
            while(true)
            {
                exam4=this->neighbors[exam3].at(neighbor_selector(mt));
                c4=this->s_periods[exam4];
                occurences.insert(exam4);
                if(occurences.size()==this->neighbors[exam3].size()) return std::map <int,int>();
                if(c4==c1 || c4==c2) continue;
                break;
            }
            std::stack <int> backlog;
            int current_exam,current_period,new_period;
            backlog.push(exam1);
            std::map <int,int> versus_period={
                {c1,c2},
                {c2,c1}
            };
            std::map <int,int> moves;
            while(!backlog.empty())
            {
                current_exam=backlog.top();
                backlog.pop();
                current_period=this->s_periods[current_exam];
                new_period=versus_period[current_period];
                moves[current_exam]=new_period;
                for(auto &neighbor:this->neighbors[current_exam])
                {
                    if(moves.find(neighbor)!=moves.end()) continue;
                    if(this->s_periods[neighbor]==new_period)
                    {
                        backlog.push(neighbor);
                    }
                }
            }

            versus_period={
                {c3,c4},
                {c4,c3}
            };
            backlog=std::stack<int>();
            backlog.push(exam3);
            while(!backlog.empty())
            {
                current_exam=backlog.top();
                backlog.pop();
                current_period=this->s_periods[current_exam];
                new_period=versus_period[current_period];
                moves[current_exam]=new_period;
                for(auto &neighbor:this->neighbors[current_exam])
                {
                    if(moves.find(neighbor)!=moves.end())
                    {
                        continue;
                    }
                    if(this->s_periods[neighbor]==new_period)
                    {
                        backlog.push(neighbor);
                    }
                }
            }
            return moves;
        }

        void eject_vertices(int &best,std::map <int,int> &best_sol)
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
                if(mv.size()!=0 && this->cost+this->pre_reposition(mv)<best)
                {
                    this->reposition(mv);
                    int texam;
                    for(auto &mid:mv)
                    {
                        texam=(mid.first==ed.first?ed.second:ed.first);
                        std::cout<<"Ejecting exam|Exam Eject->"<<mid.first<<"->Period:"<<mid.second<<"|Second exam particapate->"<<texam<<"=>P:"<<this->s_periods[texam]<<std::endl;
                    }
                    best=this->cost;
                    best_sol=this->s_periods;
                }
            }
        }
        
        void multimove_storage_adder(omp_lock_t &t,std::map <int,std::pair <int,std::map <int,int>>> &move_saver,int move_id,int exam,std::map <int,int> &moves_memory,int cost_simulation)
        {
            omp_set_lock(&t);
            std::map <int,int> moves_will_be_made=this->select_move(move_id,exam,moves_memory,cost_simulation);
            int cs=this->pre_reposition(moves_will_be_made,moves_memory);
            std::pair <int,std::map <int,int>> move_pair(cs,moves_will_be_made);
            move_saver[move_id]=move_pair;
            omp_unset_lock(&t);
        }

        std::map <int,int> depth_moves()
        {
            std::map <int,int> moves;
            // int possible_moves_lower_bound=this->thermal_periods-(time_now/this->thermal_periods);
            // if(possible_moves_lower_bound==this->thermal_periods) {depth==this->thermal_periods;
            int up_bound=this->thermal_periods;
            auto dselector=std::uniform_int_distribution<int>(1,up_bound);
            int depth=dselector(mt);
            this->move_executed=10;
            int cost_simulation=this->cost;
            std::map <int,int> moves_simulation;
            int i_depth=0;
            omp_lock_t l;
            omp_init_lock(&l);
            int exam,min_cost,move_cost;
            while(i_depth<depth)
            {
                exam=this->random_exam();
                std::map <int,std::pair <int,std::map <int,int>>> moves_will_be_tested;
                #pragma omp parallel for
                for(int i=1;i<=4;i++)
                {
                    this->multimove_storage_adder(l,moves_will_be_tested,i,exam,moves_simulation,cost_simulation);
                }
                #pragma omp barrier
                min_cost=cost_simulation;
                std::map <int,int>  round_low_cost_moves;
                for(auto &move_id:moves_will_be_tested)
                {
                    move_cost=cost_simulation+move_id.second.first;
                    if(move_cost<min_cost)
                    {
                        min_cost=move_cost;
                        round_low_cost_moves=move_id.second.second;
                    }
                }
                for(auto &mid:round_low_cost_moves)
                {
                    moves_simulation[mid.first]=mid.second;
                }
                cost_simulation=min_cost;
                i_depth++;

            }

            omp_destroy_lock(&l);
            return moves_simulation;
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
            int acceptance_rate=random_terms::accept_rate()*this->average_node_factor;
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
            this->move_executed=11;
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
                    assert(this->cost==mcost);
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
            std::cout<<best<<std::endl;
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
            this->move_executed=12;
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
            this->move_executed=13;
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
                        assert(mcost==this->cost);
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
                            bsol=this->s_periods;
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
                        if(best_exam_contribution<best)
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
                std::shuffle(this->non_zero_neighbors_exams.begin(),this->non_zero_neighbors_exams.end(),std::default_random_engine{});
                int period3;
                for(auto &exam:this->non_zero_neighbors_exams)
                {
                    best_exam_contribution=this->cost;
                    best_moves=std::map<int,int>();
                    for(auto &exam2:this->neighbors[exam])
                    {
                        period1=this->s_periods[exam2];
                        if(this->can_be_moved(exam,period1,{exam2}))
                        {
                            for(auto &exam3:this->neighbors[exam2])
                            {
                                period2=this->s_periods[exam3];
                                if(this->can_be_moved(exam2,period2,{exam3}))
                                {
                                    for(int p=0;p<this->P;p++)
                                    {
                                        if(p==period2 || p==period1 || p==this->s_periods[exam])
                                        {
                                            continue;
                                        }
                                        if(this->can_be_moved(exam3,p))
                                        {
                                            moves={
                                                {exam,period1},
                                                {exam2,period2},
                                                {exam3,p}
                                            };
                                            mcost=this->cost+this->pre_reposition(moves);
                                            if(mcost<best_exam_contribution)
                                            {
                                                best_moves=moves;
                                                best_exam_contribution=mcost;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if(!best_moves.empty())
                    {
                        if(this->cost+this->pre_reposition(best_moves)<best)
                        {   
                            start=true;
                            this->reposition(best_moves);
                            best=this->cost;
                            bsol=this->s_periods;
                            std::cout<<"Polish DKE|New best Solution found:"<<best<<"{"<<std::endl;
                            for(auto &mid:moves)
                            {
                                if(start)
                                {
                                    start=false;
                                    std::cout<<"("<<mid.first<<"->"<<mid.second<<")";
                                    continue;
                                }
                                std::cout<<",("<<mid.first<<"->"<<mid.second<<")"<<std::endl;
                                
                            }
                        }
                    }
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
                            best_sol=this->s_periods;
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

        ~hill_climber() {}

        void set_execution_time(int exec_seconds)
        {
            this->execution_time=exec_seconds;
        }

        void solve()
        {
            int best=this->cost;
            std::map <int,int> best_sol;
            int mcost;
            logger log{LOG_INFO};
            auto stime=clock();
            int cnull=0;
            int move_id=1;
            while(true)
            {
                std::map <int,int> moves=this->depth_moves();
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
            this->thermal_periods=es/this->P;
            if(this->thermal_periods<=1)
            {
                thermal_periods=1;
            }
        }

        ~simulated_annealer() {}

        void set_time(int es) {this->execution_time=es; this->thermal_periods=this->execution_time/this->P; if(this->thermal_periods<=1) {this->thermal_periods=1;}}

        void pre_work_optimization(int &best,std::map <int,int> &bsol)
        {
            std::cout.precision(6);
            std::cout<<"========== Start With Period permutation ============"<<std::endl;
            this->permute_periods(best,bsol);
            std::cout<<"====================================================="<<std::endl<<std::endl;
            std::cout<<"==================== Eject high cost Vertices ==================="<<std::endl;
            this->eject_vertices(best,bsol);
            std::cout<<"================================================================="<<std::endl;
        }

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
            this->pre_work_optimization(best,best_sol);
            auto stime=clock();
            this->start_timer=std::chrono::high_resolution_clock::now();
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
                    if(acceptance_propability>random_terms::random_normalized())
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
                    bool fbest=this->pwork(this->execution_time/this->thermal_periods,best,best_sol,mc);
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
                        temperature/=random_terms::temp_derived();
                        int previous_best=best;
                        this->polish(this->execution_time/this->P,best,best_sol);
                        this->add_occurence(best!=previous_best);
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
                    temperature=start_temp*random_terms::normalized_temp()/round;
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
            std::cout<<Bool2String(this->is_feasible())<<std::endl;
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