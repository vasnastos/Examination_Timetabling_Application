#include <iostream>
#include <random>
#include <thread>
#include <omp.h>
#include <chrono>
#include <string>
#include <set>
#include <numeric>
#include <cassert>
#include <cctype>
#include <windows.h>
#include <sstream>
#include <map>
#include <algorithm>
#include <tuple>
#include <ctime>
#include <iterator>
#include <mutex>
#include <fstream>
#include <iomanip>
#include <unordered_map>
#include <stack>
// #include <bits/stdc++.h>
#define LOG_INFO 2
#define LOG_DEBUG 1
#define LOG_ERROR 3
#define FREEZE_PROC 4
#define TEMP_SIGN "\370"
#define REHEAT_TEMP 5.0
#define REHEAT_LIMIT 20
#define POLISH_TIME 5
#define REHEAT_ITERATIONS 3
#define PS 4
#define MOVE_DEPTH 4
#define MOVECAP 6
#define LIMP 1000
#define MOVES_NUMBER 10


// https://github.com/haarcuba/cpp-text-table/blob/master/TextTable.h
#ifdef TEXTTABLE_ENCODE_MULTIBYTE_STRINGS
#include <clocale>
#ifndef TEXTTABLE_USE_EN_US_UTF8
#define TEXTTABLE_USE_EN_US_UTF8
#endif
#endif

class TextTable {

    public:
    enum class Alignment { LEFT, RIGHT }; 
    typedef std::vector< std::string > Row;
    TextTable() :
        _horizontal( '-' ),
        _vertical( '|' ),
        _corner( '+' ),
		_has_ruler(true)
    {}

    TextTable( char horizontal, char vertical, char corner ) :
        _horizontal( horizontal ),
        _vertical( vertical ),
        _corner( corner ),
		_has_ruler(true)
    {}
    
    explicit TextTable( char vertical ) :
        _horizontal( '\0' ),
        _vertical( vertical ),
        _corner( '\0' ),
		_has_ruler( false )
    {}

    void setAlignment( unsigned i, Alignment alignment )
    {
        _alignment[ i ] = alignment;
    }

    Alignment alignment( unsigned i ) const
    { return _alignment[ i ]; }

    char vertical() const
    { return _vertical; }

    char horizontal() const
    { return _horizontal; }

    void add( std::string const & content )
    {
        _current.push_back( content );
    }

    void endOfRow()
    {
        _rows.push_back( _current );
        _current.assign( 0, "" );
    }

    template <typename Iterator>
    void addRow( Iterator begin, Iterator end )
    {
        for( auto i = begin; i != end; ++i ) {
           add( * i ); 
        }
        endOfRow();
    }

    template <typename Container>
    void addRow( Container const & container )
    {
        addRow( container.begin(), container.end() );
    }

    std::vector< Row > const & rows() const
    {
        return _rows;
    }

    void setup() const
    {
        determineWidths();
        setupAlignment();
    }

    std::string ruler() const
    {
        std::string result;
        result += _corner;
        for( auto width = _width.begin(); width != _width.end(); ++ width ) {
            result += repeat( * width, _horizontal );
            result += _corner;
        }

        return result;
    }

    int width( unsigned i ) const
    { return _width[ i ]; }

	bool has_ruler() const { return _has_ruler;}

	int correctDistance(std::string string_to_correct) const
		{
			return static_cast<int>(string_to_correct.size()) - static_cast<int>(glyphLength(string_to_correct));
		};
	
    private:
    const char _horizontal;
    const char _vertical;
    const char _corner;
    const bool _has_ruler;
    Row _current;
    std::vector< Row > _rows;
    std::vector< unsigned > mutable _width;
	std::vector< unsigned > mutable _utf8width;
    std::map< unsigned, Alignment > mutable _alignment;
	
    static std::string repeat( unsigned times, char c )
    {
        std::string result;
        for( ; times > 0; -- times )
            result += c;

        return result;
    }

    unsigned columns() const
    {
        return _rows[ 0 ].size();
    }

	unsigned glyphLength( std::string s ) const
	{
		unsigned int _byteLength = s.length();
#ifdef TEXTTABLE_ENCODE_MULTIBYTE_STRINGS
#ifdef TEXTTABLE_USE_EN_US_UTF8
		std::setlocale(LC_ALL, "en_US.utf8");
#else
#error You need to specify the encoding if the TextTable library uses multybyte string encoding!
#endif
		unsigned int u = 0;
		const char *c_str = s.c_str();
		unsigned _glyphLength = 0;
		while(u < _byteLength)
		{
			u += std::mblen(&c_str[u], _byteLength - u);
			_glyphLength += 1;
		}
		return _glyphLength;
#else
		return _byteLength;
#endif
	}
	
    void determineWidths() const
    {
        _width.assign( columns(), 0 );
		_utf8width.assign( columns(), 0 );
        for ( auto rowIterator = _rows.begin(); rowIterator != _rows.end(); ++ rowIterator ) {
            Row const & row = * rowIterator;
            for ( unsigned i = 0; i < row.size(); ++i ) {
                _width[ i ] = _width[ i ] > glyphLength(row[ i ]) ? _width[ i ] : glyphLength(row[ i ]);
            }
        }
    }

    void setupAlignment() const
    {
        for ( unsigned i = 0; i < columns(); ++i ) {
            if ( _alignment.find( i ) == _alignment.end() ) {
                _alignment[ i ] = Alignment::LEFT;
            }
        }
    }
};

inline std::ostream & operator<<( std::ostream & stream, TextTable const & table )
{
    table.setup();
	if (table.has_ruler()) {
	    stream << table.ruler() << "\n";
	}
    for ( auto rowIterator = table.rows().begin(); rowIterator != table.rows().end(); ++ rowIterator ) {
        TextTable::Row const & row = * rowIterator;
        stream << table.vertical();
        for ( unsigned i = 0; i < row.size(); ++i ) {
            auto alignment = table.alignment( i ) == TextTable::Alignment::LEFT ? std::left : std::right;
			// std::setw( width ) works as follows: a string which goes in the stream with byte length (!) l is filled with n spaces so that l+n=width.
			// For a utf8 encoded string the glyph length g might be smaller than l. We need n spaces so that g+n=width which is equivalent to g+n+l-l=width ==> l+n = width+l-g
			// l-g (that means glyph length minus byte length) has to be added to the width argument.
			// l-g is computed by correctDistance.
            stream << std::setw( table.width( i ) + table.correctDistance(row[ i ])) << alignment << row[ i ];
            stream << table.vertical();
        }
        stream << "\n";
		if (table.has_ruler()) {
        	stream << table.ruler() << "\n";
		}
    }

    return stream;
}
// ==================================================================

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

namespace random {
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
    std::string dataset_folder="../../datasets/";
    std::string coloring_folder="../../../Stats/Greedy_Coloring/";
    std::string hill_climbing="../../../Stats/hill_climbing/";
    std::string sa_folder="../../../Stats/simulated_annealing/";
}

struct problem
{
    std::string ds_name;
    std::map <int,std::map <int,int>> graph;
    std::map <int,int> pr_period;
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
                if(line[0]=='#' || line.length()==0) continue;
                std::istringstream iss(line);
                while(std::getline(iss,word,','))
                {
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

// https://stackoverflow.com/questions/33282680/retrieve-the-name-of-the-computer-and-saved-it-in-a-variable
std::string get_computer_name()
{
    const int buffer_size = MAX_COMPUTERNAME_LENGTH + 1;
    char buffer[buffer_size];
    DWORD lpnSize = buffer_size;
    if (GetComputerNameA(buffer, &lpnSize) == FALSE)
        throw std::runtime_error("Something went wrong.");
    return std::string{ buffer };
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
