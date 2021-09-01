from pilot import construct_problem
from pilot import ProblemAnalyzer
from pilot import datasets

def main():
    dataset='sta83'
    problem=construct_problem(dataset)
    print(problem)
    problem.log_noise()
    problem.log_identical()

if __name__=='__main__':
    datasets.load_incstance_info()
    # main()
    # ProblemAnalyzer.save_noise()
    # ProblemAnalyzer.save_statistics()
    # ProblemAnalyzer.save_identical()
    # ProblemAnalyzer.greedy_coloring_stats()
    problem=construct_problem('kfu93')
    problem.greedy_coloring_diag()
