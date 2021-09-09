#include "Graph.hpp"
#define MOVES_NUMBER 12
#define LOG_INFO 2
#define LOG_DEBUG 1
#define LOG_ERROR 3
#define FREEZE_PROC 4
#define TEMP_SIGN '\370'
#define REHEAT_TEMP 5.0
#define REHEAT_LIMIT 20
#define POLISH_TIME 5
#define REHEAT_ITERATIONS 3
#define PS 4
#define MOVE_DEPTH 4
#define MOVECAP 6

std::mt19937 mt(std::chrono::high_resolution_clock::now().time_since_epoch().count());

namespace datasets
{
    int common_students(std::vector <int> &s1,std::vector <int> &s2)
    {
        std::vector <int> s3;
        std::sort(s1.begin(),s1.end());
        std::sort(s2.begin(),s2.end());
        std::set_intersection(s1.begin(),s1.end(),s2.begin(),s2.end(),std::back_inserter(s3));
        return s3.size();
    }

    std::vector <std::string> names{
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
};
}

namespace random_terms {
    double normalized_temp()
    {
    auto utemp=std::uniform_real_distribution<double>(1.9,2.9);
    return utemp(mt);
    }

    double random_normalized()
    {
        auto wsol=std::uniform_real_distribution<double>(0.0,1.0);
        return wsol(mt);
    }

    double temp_derived()
    {
    auto dtemp=std::uniform_real_distribution<double>(0.4,0.7);
    return dtemp(mt);
    }


    int random_boundary(int bound)
    {
        auto rb=std::uniform_int_distribution<int>(0,bound-1);
        return rb(mt);
    }

    int get_depth(double temp,int periods)
    {
        int possible_move_nums=int(floor(temp/periods));
        if(possible_move_nums==1)
        {
            return 1;
        }
        auto upm=std::uniform_int_distribution<int>(1,possible_move_nums);
        return upm(mt);
    }

    double accept_rate()
    {
        std::uniform_real_distribution <double> acc(0.7,1.0);
        return acc(mt);
    }

}

namespace data
{
    std::string dataset_folder="../datasets/";
    std::string coloring_folder="../Greedy_Coloring/";
    std::string hill_climbing="../Stats/hill_climbing/";
    std::string sa_folder="../Stats/simulated_annealing/";
}

struct problem
{
    std::string ds_name;
    std::map <int,std::map <int,int>> graph;
    std::map <int,int> pr_period;
    std::map <int,int> fixed_exams;
    int students;
    int exams;
    
    void graph_import()
    {
        std::string filename=data::dataset_folder+this->ds_name+".in";
        std::set <int> ss;
        std::fstream file(filename,std::ios::in);
        std::string line,word;
        std::map <int,std::vector <int>> exams;
        if(!file.is_open())
        {
            std::cerr<<"File did not open"<<std::endl;
            return;
        }
        while(std::getline(file,line))
        {
            if(line.length()==0) continue;
            std::istringstream iss(line);
            std::vector <std::string> fdata;
            while(iss>>word)
            {
                fdata.push_back(word);
            }
            if(fdata[0][0]=='s')
            {
                std::replace_if(fdata[0].begin(),fdata[0].end(),[](char &l) {return l=='s';},' ');
                int student=std::stoi(fdata[0]);
                ss.insert(student);
                int examid=std::stoi(fdata[1]);
                exams[examid].push_back(student);
            }
            else
            {
                int exam=std::stoi(fdata[0]);
                if(exams.find(exam)==exams.end())
                {
                    exams[exam]=std::vector<int>();
                }
            }
        }
        std::map <int,std::map <int,int>> graph;
        for(auto &rc:exams)
        {
            for(auto &x:exams)
            {
                if(x.first>rc.first)
                {
                    int cs=datasets::common_students(rc.second,x.second);
                    this->graph[rc.first][x.first]=cs;
                    this->graph[x.first][rc.first]=cs;
                }
            }
        }
        this->students=ss.size();
    }

    void gc_import()
    {
        std::string gc_file=data::coloring_folder+this->ds_name+".gsol";
        std::fstream fs;
        try
            {
                fs=std::fstream(gc_file,std::ios::in);
            }
            catch(const std::ifstream::failure &e)
            {
                std::cerr << e.what() << '\n';
            }
            std::string line,word;
            while(std::getline(fs,line))
            {
                std::vector <int> fdata;
                if(line[0]=='#') continue;
                if(line.length()==0) continue;
                std::stringstream ss(line);
                while(std::getline(ss,word,','))
                {
                    std::cout<<line<<std::endl;
                    std::cout<<"Word:"<<word<<std::endl;
                    if(word=="") continue;
                    fdata.emplace_back(std::stoi(word));
                }
                if(fdata.size()!=2) continue;
                this->pr_period[fdata[0]]=fdata[1];
            }                
    }

    problem(std::string dataset)
    {
        this->ds_name=dataset;
        this->students;
        std::string pfile=data::dataset_folder+dataset+".in";
        this->graph_import();
        this->gc_import();    
    }

   problem(std::string name,std::map <int,std::map<int,int>> &g,std::map <int,int> &s,int ns):ds_name(name),graph(g),pr_period(s),students(ns) {}
    
    inline int getExams()
    {
        return this->graph.size();
    }
};

std::string Bool2String(bool bvar)
{
   if(!bvar) return "False";
   return "True";
}

std::string getdatetime()
{
    std::time_t datetime=std::chrono::system_clock ::to_time_t(std::chrono::system_clock::now());
    std::string time=std::ctime(&datetime);
    std::replace_if(time.begin(),time.end(),[](char &k) {return k=='\n';},'\t');
    return time;
}


std::string getLevel(int lv)
{
   switch(lv)
   {
      case 1:
         return "DEBUG";
      case 2:
         return "INFO";
      case 3:
         return "ERROR";
      default:
         return "";
   }
}

// Logging
struct logger
{
   int level;
   logger(int lv)
   {
      this->level=lv;
   }
   void info(const std::string message)
   {
      if(this->level>LOG_INFO) return;
      std::cout<<getdatetime();
      std::cout<<message<<std::endl;
   }
   void debug(const std::string message)
   {
      if(this->level>LOG_DEBUG) return;
      std::cout<<getdatetime();
      std::cout<<message<<std::endl<<std::endl;
   }
   void error(const std::string message)
   {
      if(this->level>LOG_ERROR) return;
      std::cout<<getdatetime();
      std::cout<<message<<std::endl<<std::endl;
   }
};


class pmove
{
    private:
        enum class namedmove
        {
            NONE,
            MOVE_EXAM,
            SWAP_EXAMS,
            SWAP_PERIODS,
            SHIFT_PERIODS,
            KEMPE_CHAIN,
            KICK_EXAM,
            DOUBLE_KICK_EXAM,
            ROUND_KICK_EXAM,
            MOVE_EXAM_EXTENDED,
            DOUBLE_KEMPE,
            DEPTH_MOVE,
            PARALLEL_EXECUTION,
            POLISH_MOVES
        };
        namedmove mm;
        int count,nullcounter;

    public:
        pmove(int i=0):mm(namedmove(i)),count(0),nullcounter(0) {}
        operator std::string()const{
            std::stringstream ss;
            switch(this->mm)
            {
                case namedmove::MOVE_EXAM:
                    ss<<"Move_Exam";
                    break;
                case namedmove::SWAP_EXAMS:
                    ss<<"Swap_Exams";
                    break;
                case namedmove::SWAP_PERIODS:
                    ss<<"Swap_Periods";
                    break;
                case namedmove::SHIFT_PERIODS:
                    ss<<"Shift_Periods";
                    break;
                case namedmove::KEMPE_CHAIN:
                    ss<<"Kempe_Chain";
                    break;
                case namedmove::KICK_EXAM:
                    ss<<"Kick_Exam";
                    break;
                case namedmove::DOUBLE_KICK_EXAM:
                    ss<<"Double_Kick_Exam";
                    break;
                case namedmove::ROUND_KICK_EXAM:
                    ss<<"Round_Kick_Exam";
                    break;
                case namedmove::MOVE_EXAM_EXTENDED:
                    ss<<"Move_Exam_Extended";
                    break;
                case namedmove::DOUBLE_KEMPE:
                    ss<<"Double_Kempe_Chain";
                    break;
                case namedmove::DEPTH_MOVE:
                    ss<<"Depth_Move";
                    break;
                case namedmove::PARALLEL_EXECUTION:
                    ss<<"Parallel_Move_Execution";
                    break;
                case namedmove::POLISH_MOVES:
                    ss<<"Polish_Moves";
                    break;
                default:
                    ss<<"INVALID MOVE";
                    break;
            }
            return ss.str();
        }
        
        int at(const std::string asked)
        {
            std::string s=asked;
            std::for_each(s.begin(),s.end(),[](char c) {c=::tolower(c);});
            if(s=="count")
            {
                return this->count;
            }
            else if(s=="null")
            {
                return this->nullcounter;
            }
            else if(s=="sum")
            {
                return this->count+this->nullcounter;
            }
            return -1;
        }

        pmove operator++(int)
        {
            this->count++;
            return *this;
        }

        pmove operator--(int)
        {
            this->nullcounter++;
            return *this;
        }
};

struct move_buffer
{
    std::vector <std::pair <int,int>> move_sequence;
    std::map <int,int> period_simulator;
    int cost_simulator;
    move_buffer() {}
    move_buffer &operator+=(std::pair <int,int> &p)
    {
        this->move_sequence.emplace_back(p);
        this->period_simulator[p.first]=p.second;
        return *this;
    }
    void flush()
    {
        this->move_sequence.clear();
        this->period_simulator.clear();
    }
};
