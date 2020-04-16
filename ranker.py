import sys

class ranker(object):
    """docstring for ."""

    def __init__(self, argv):
        self.functions = self.parse_input(argv)

    def score(self):
        for func in self.functions:
            self.functions[func].init_score()
        scores = {}
        for func in self.functions:
            scores[func] = self.functions[func].score
        print(sorted(scores.items(), key = lambda kv:(kv[1], kv[0])))
        AllConverged = False
        i = 1
        while not AllConverged:
            AllConverged = True
            for func in self.functions:
                callees = self.functions[func].get_callees()
                callee_score = []
                for name in callees:
                    callee_score.append(self.functions[name].score * callees[name])
                if not self.functions[func].update_score(callee_score, beta = 0.9*(i**(-0.5))):
                    AllConverged = False
            i+=1
        scores = {}
        total_scores = 0
        max_len = 0
        for func in self.functions:
            scores[func] = self.functions[func].score
            total_scores += scores[func]
            if len(func) > max_len:
                max_len = len(func)
        for func in self.functions:
            scores[func] /= (total_scores+1)
        sorted_scores = sorted(scores.items(), key = lambda kv:(kv[1], kv[0]))
        print("===============================================\n")
        print("===============================================\n")
        print("===============================================\n")
        print("=============== Analysis Result ===============\n")
        print("===============================================\n")
        print("Function Name       Relative Score        Usage\n")
        print("===============================================\n")
        for score in sorted_scores[::-1]:
            print(score[0], "       ",score[1],"        ",self.functions[score[0]].dynamic_called,"\n")

        print("===============================================\n")
        print("===================== END =====================\n")
        print("===============================================\n")

    def parse_input(self, argv):
        CALL = open(argv[2], "r")
        funcs = {}
        data = []
        with open(argv[2], "r") as CALL:
            while True:
                line = CALL.readline().strip(' \n\t')
                if line is "":
                    data = []
                    break
                if line.split()[0] is "#":
                    funcs[data[0].split()[0]] = Function(data[0], data[1:])
                    data = []
                else:
                    data.append(line)

        with open(argv[1], "r") as CC:
            while True:
                line = CC.readline().strip(' \n\t')
                if line is "":
                    data = []
                    break
                if line.split()[0] is "#":
                    funcs[data[0].split()[0]].add_computationScore(data[1:])
                    data = []
                else:
                    data.append(line)

        functionName = None
        with open(argv[3], "r") as DYN:
            while True:
                line = DYN.readline().strip(' \n\t')
                if line is "":
                    break
                if functionName is None:
                    functionName = line.split()[0]
                    data.append(line)
                else:
                    if not line.startswith(functionName+":"):
                        funcs[functionName].add_dynamicInfo(data)
                        data = []
                        functionName = line.split()[0]
                        data.append(line)
                    else:
                        data.append(line)

        funcs[functionName].add_dynamicInfo(data)
        data = []
        functionName = None
        with open(argv[0], "r") as Loop:
            while True:
                line = Loop.readline().strip(' \n\t')
                if line is "":
                    data = []
                    break
                if functionName is None:
                    functionName = line
                    data = []
                else:
                    data.append(line)
                    if line == "#END":
                        funcs[functionName].add_loopInfo(data)
                        functionName = None

        for name in funcs:
            funcs[name].extract_data()
        return funcs

class Function(object):
    """docstring fs Function."""

    dependency_degradation = 4000/400

    def __init__(self, functionName, calls):
        self.functionName = functionName
        self.static_called = calls
        self.dynamic_called = 0
        self.loops = None
        self.dynamicInfomation = []
        self.computationScore = None

        self.dependency = 0
        self.callsMap = {}
        self.loopScores = []
        self.score = 0


    def add_computationScore(self, scoretext):
        self.computationScore = scoretext

    def add_dynamicInfo(self, di):
        self.dynamicInfomation = di
        # print(self.functionName)
        # print(di)

    def add_loopInfo(self, LI):
        self.loops = LI
        # print(self.functionName)
        # print(LI)

    def extract_data(self):
        for called in self.static_called:
            self.callsMap[called.split()[0]] = int(called.split(":")[-1])

        counted_in_loops = 0
        if len(self.dynamicInfomation):
            self.dynamic_called = int(self.dynamicInfomation[0].split()[-1])
            # print(self.dynamic_called)
            j = 0
            for i in range(1, len(self.dynamicInfomation)):
                iterations = int(self.dynamicInfomation[i].split()[-1])
                j = self.loops.index(self.dynamicInfomation[i].split()[-2].split(":")[-1])
                computation_value = (1-int(self.loops[j+3]))*(1+int(self.loops[j+2]))
                counted_in_loops -= computation_value
                self.loopScores.append(computation_value*iterations)
                j+=1
        # print(self.loopScores)

        self.dependency = Function.dependency_degradation*int(self.computationScore[1].split()[-1])
        self.computationScore = float(int(self.computationScore[0].split()[-1]) + counted_in_loops)
        ###
        # print(self.computationScore)

    def update_score(self, callees, beta=0.9):
        old_score = self.score
        self.score =  (beta)*sum(callees) + self.score
        return abs(self.score - old_score) < 0.1

    def init_score(self):
        self.score = sum(self.loopScores)
        self.score+= self.computationScore
        self.score-= self.dependency
        self.score /= 1000.0

    def get_callees(self):
        return self.callsMap



Ranker = ranker(sys.argv[1:])
Ranker.score()
