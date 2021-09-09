#include "cmd_table.hpp"
#include "machine_info.hpp"

// Graph Class--Used in Solution
class Graph
{
    public:
        static void change_datasets_path(std::string &newpath);
        static std::string datasets_path;
        std::string id;
        static std::map <int,int> penalty;
        std::vector <std::pair <int,int>> dual_comb;
        std::map <int,int> s_periods;
        std::map <int,int> node_factor;
        double average_node_factor;
        double average_edge_weight;
        Graph() {}

        Graph(std::string &instance_n,std::map <int,std::map<int,int>> &graph,std::map <int,int> &starting_point,int &nf):id(instance_n),adj_table(graph),NF(nf)
        {
            this->P=std::max_element(starting_point.begin(),starting_point.end(),[](const std::pair <int,int> &p1,const std::pair <int,int> &p2) {return p1.second<p2.second;})->second+1;
            int ns;
            bool non_zero;
            for(auto &node:this->adj_table)
            {
                non_zero=true;
                this->nodes.emplace_back(node.first);
                this->neighbors[node.first]=std::vector<int>();
                this->node_factor[node.first]=0;
                for(auto &pn:node.second)
                {
                    ns=pn.second;
                    if(ns>0)
                    {
                        this->node_factor[node.first]+=ns;
                        non_zero=false;
                        this->neighbors[node.first].emplace_back(pn.first);
                        if(node.first<pn.first)
                        {
                            average_edge_weight+=ns;
                            this->edges.emplace_back(std::pair<int,int>(node.first,pn.first));
                        }
                    }
                }
                if(!non_zero)
                {
                    this->non_zero_neighbors_exams.emplace_back(node.first);
                }
            }
            for(auto &node:this->nodes)
            {
                this->s_periods[node]=-1;
            }
            for(int i=0;i<this->P;i++)
            {
                this->Periods.emplace_back(i);
            }
            this->average_node_factor=std::accumulate(this->node_factor.begin(),this->node_factor.end(),0,[](int s,const std::pair <int,int> &p) {return s+p.second;})/(2.0*this->get_number_of_nodes());
            this->average_edge_weight/=this->edges.size();
        }
        
        int get_max_weighted_vertex(int exam)
        {
            int max=-1;
            int max_weight=0;
            for(auto &n:this->neighbors[exam])
            {
                if(this->adj_table[exam][n]>max_weight)
                {
                    max_weight=this->adj_table[exam][n];
                    max=n;
                }
            }
            return max;
        }

        int compute_cost()
        {
            int ds=0;
            for(auto &pn:this->edges)
            {
                ds+=this->adj_table[pn.first][pn.second] * Graph::penalty[abs(this->s_periods[pn.first]-this->s_periods[pn.second])];
            }
            return ds;
        }
        
        int neighbors_delta(int exam,int period)
        {

            int cost_sum=0;
            int distance;
            for(auto &node:this->neighbors[exam])
            {
                if(this->s_periods[node]==-1) continue;
                distance=abs(period-this->s_periods[node]);
                if(distance<=5)
                {
                    cost_sum+=this->adj_table[exam][node] * Graph::penalty[distance];
                }
            }
            return cost_sum;
        }

        inline int normalized_factor()const  {return this->NF;}

        double compute_normalized_cost()
        {
            return double(this->compute_cost())/static_cast<double>(this->NF);
        }

        double conflict_density()
        {
            int cd=0;
            for(auto &pn:this->neighbors)
            {
                cd+=pn.second.size();
            }
            return static_cast<double>(cd)/pow(this->nodes.size(),2);
        }

        bool has_edge(int &exam1,int &exam2)
        {
            return this->adj_table[exam1][exam2]!=0;
        }

        bool is_feasible()
        {
            return std::all_of(this->edges.begin(),this->edges.end(),[&](const std::pair <int,int> &pn) {return this->s_periods[pn.first]!=this->s_periods[pn.second];});
        }

        bool is_feasible(int exam,int period)
        {
            return std::all_of(this->neighbors[exam].begin(),this->neighbors[exam].end(),[&](const int &exam2) {return period!=this->s_periods[exam2];});
        }

        bool is_feasible(int exam,int period,std::map <int,int> &exclude)
        { 
            int neighbor_period;
            for(auto &neighbor:this->neighbors[exam])
            {
                neighbor_period=exclude.find(exam)!=exclude.end()?exclude[exam]:this->s_periods[exam];
                if(period==neighbor_period)
                {
                    return false;
                }
            }
            return true;
        }

        bool is_feasible(std::map <int,int> &moves)
        {
            for(auto &move:moves)
            {
                if(!this->is_feasible(move.first,move.second,moves))
                {
                    return false;
                }
            }
            return true;
        }

        std::vector <std::pair<int,int>> combinations()
        {
            std::vector <std::pair <int,int>> combs;
            for(auto &node:this->nodes)
            {
                for(auto &node1:this->nodes)
                {
                    if(node==node1)
                    {
                        continue;
                    }
                    if(node<node1)
                    combs.emplace_back(std::pair <int,int>(node,node1));
                }
            }
            return combs;
        }

        int get_periods()const
        {
            return this->P;
        }

        std::vector <int>& n_neighbors(int exam)
        {
            return this->neighbors[exam];
        }

        std::vector <int>& Nodes()
        {
            return this->nodes;
        }

        int get_number_of_nodes()const
        {
            return this->nodes.size();
        }

        void export_cost_contribution()
        {
            std::string filename="../../../Stats/"+this->id+"_cost_contribution.csv";
            std::fstream fs(filename,std::ios::out);
            fs<<"Dataset,"<<this->id<<std::endl;
            fs<<"Cost,"<<this->compute_cost()<<","<<this->compute_normalized_cost()<<std::endl<<std::endl;
            fs<<"Exam,Cost"<<std::endl;
            for(auto &node:this->nodes)
            {
                fs<<node<<","<<this->neighbors_delta(node,this->s_periods[node])<<std::endl;
            }
            fs.close();
        }

    protected:
        std::map <int,std::map <int,int>> adj_table;
        std::vector <int> nodes;
        std::map <int,std::vector <int>> neighbors;
        std::vector <int> non_zero_neighbors_exams;
        std::vector <std::pair <int,int>> edges;
        std::vector <std::vector <int>> round_components;
        std::vector <int> Periods;
        int NF;
        int P;
};

// Static Members of Graph
std::string Graph::datasets_path="../../datasets/";
std::map <int,int> Graph::penalty=std::map <int,int>({{1,16},{2,8},{3,4},{4,2},{5,1},{-1,16},{-2,8},{-3,4},{-4,2},{-5,1}});
void Graph::change_datasets_path(std::string &newpath)
{
    Graph::datasets_path=newpath;
}

