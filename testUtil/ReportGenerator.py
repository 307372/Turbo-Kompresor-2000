import os
import helpers


def joinStringList(stringList, sep='\t'):
    return sep.join(map(str, stringList))



class ReportGenerator():
    def __init__(self):
        self._runIdentHeaders = [] # string name to be written on top of column in report
        self._runIdentGetters = [] # takes lambdas, that take "TestRunner" object and return string
        # said string contains info that allows us to identify what run we're checking the results for

        self._partialHeaderNames = [] # string name to be written on top of column in report with file name attached
        self._testResultGetters = []      # takes lambdas, that take "TestRunner" object and return string
        # said string contains test results for a given run


        self.addIdentificationColumn(
            name="program",
            valueGetter = lambda run, cmdType, filePath: run.name)
        
        self.addIdentificationColumn(
            name="tryb",
            valueGetter = lambda run, cmdType, filePath: helpers.getCmdTypeStr(cmdType))

        # reportGenerator.addColumn(  # (TK2K only)
        #     name="rozmiar_bloku",
        #     valueGetter = lambda run: run.tk2kBlockSize if run.tk2kBlockSize is not None else "")

        # reportGenerator.addColumn(  # (TK2K only)
        #     name="algorytm",
        #     valueGetter = lambda run: run.tk2kAlgorithm if run.tk2kAlgorithm is not None else "")


        self.addColumnPerFile(
            name="czas_pakowania",
            valueGetter = lambda run, cmdType, filePath: run.getTime(cmdType=cmdType, cmdMode=helpers.CmdMode.PACK, filePath=filePath))

        self.addColumnPerFile(
            name="czas_rozpakowania",
            valueGetter = lambda run, cmdType, filePath: run.getTime(cmdType=cmdType, cmdMode=helpers.CmdMode.UNPACK, filePath=filePath))

        self.addColumnPerFile(
            name="rozmiar_spakowany",
            valueGetter = lambda run, cmdType, filePath: run.getSize(cmdType=cmdType, cmdMode=helpers.CmdMode.PACK, filePath=filePath))

        self.addColumnPerFile(
            name="ram_pakowania",
            valueGetter = lambda run, cmdType, filePath: run.getRamUsage(cmdType=cmdType, cmdMode=helpers.CmdMode.PACK, filePath=filePath))

        self.addColumnPerFile(
            name="ram_rozpakowania",
            valueGetter = lambda run, cmdType, filePath: run.getRamUsage(cmdType=cmdType, cmdMode=helpers.CmdMode.UNPACK, filePath=filePath))


    def addColumnPerFile(self, name, valueGetter):
        self._partialHeaderNames += [name]
        self._testResultGetters += [valueGetter]


    def addIdentificationColumn(self, name, valueGetter):
        self._runIdentHeaders += [name]
        self._runIdentGetters += [valueGetter]


    def getRepeatedFileNames(self, filename, amount=None): # for ease of use with excel
        if amount is None:
            amount = len(self._testResultGetters)
         
        return joinStringList([filename] * amount)


    def getPrependedFileNameToColumnNames(self, filename):
        fileName_columnNames = [f"{filename}_{columnName}" for columnName in self._partialHeaderNames]
        return joinStringList(fileName_columnNames)

    # testcase = Wiemy co za algorytm, jak skonfigurowany i na jakim pliku byl uruchamiany
    # jest to wiedza wystarczajaca do zidentyfikowania testu
    def getTestcaseData(self, runner, cmdType, filePath):
        values = []
        for getter in self._testResultGetters:
            values += [getter(run=runner, cmdType=cmdType, filePath=filePath)]

        return joinStringList(values)


    def getProgramNameAndProfile(self, runner, cmdType, filePath):
        values = []

        for getter in self._runIdentGetters:
            line = getter(run=runner, cmdType=cmdType, filePath=filePath)
            values += [line]

        return joinStringList(values)

    # class RawReportGenerator(): #ReportGenerator
    #     pass TODO: make rawReport and processed reports separate objects?


    def getLinesOfRawTestcaseData(self, runner, cmdType, filePaths):
        resultsForFile = [self.getProgramNameAndProfile(runner=runner, cmdType=cmdType, filePath='')]
        for path in filePaths:
            resultsForFile += [self.getTestcaseData(runner=runner, cmdType=cmdType, filePath=path)]
        return joinStringList(resultsForFile)

        # filling raw report horizontally
        # a = namedtuple('TestId', ['runner', 'cmdtype', 'filepath'])
        # from collections import namedtuple
        # TestId = namedtuple('TestId', ['runner', 'cmdType', 'filePath'])
        # identification = TestId(runner=runner, cmdType=cmdType, filePath=path)
        #(*identification)] works

    def generateHeadersForRawReport(self, filePaths):
        columnNames = [] + self._runIdentHeaders
        filenames = [] + self._runIdentHeaders
        
        for path in filePaths:
            columnNames += [self.getPrependedFileNameToColumnNames(filename=os.path.basename(path))]
            filenames += [self.getRepeatedFileNames(filename=os.path.basename(path))] # for the benefit of excel only
        
        return [joinStringList(columnNames),
                joinStringList(filenames)]


    def generateRawReport(self, runners, filePaths):
        lines = []
        lines += self.generateHeadersForRawReport(filePaths= filePaths)

        for runner in runners:
            for cmdType in helpers.CmdType.__members__.values():
                if runner._cmd[cmdType]:
                    lines += [self.getLinesOfRawTestcaseData(runner= runner, cmdType= cmdType, filePaths= filePaths)]

        return joinStringList(lines, '\n')


    def generateHeadersForProcessedReport(self, filePaths):
        filenames = [] + self._runIdentHeaders
        for path in filePaths:
            filenames += [os.path.basename(path)]

        return [joinStringList(filenames)]


    def getProcessedTestcaseData(self, runner, cmdType, filePaths, getter):
        resultsForFile = [self.getProgramNameAndProfile(runner= runner, cmdType= cmdType, filePath= '')]

        for path in filePaths:
            resultsForFile += [getter(run= runner, cmdType= cmdType, filePath= path)]
        
        return joinStringList(resultsForFile)


    def generateProcessedReport(self, runners, filePaths, getter):
        lines = []
        lines += self.generateHeadersForProcessedReport(filePaths= filePaths)

        for runner in runners:
            for cmdType in helpers.CmdType.__members__.values():
                if runner._cmd[cmdType]:
        #####################################################################
                    lines += [self.getProcessedTestcaseData(runner= runner, cmdType= cmdType, filePaths= filePaths, getter=getter)]

        return joinStringList(lines, '\n')


    def generateAmountOfTestRunsStats(self, amount):
        return f"Amount of test runs of each algorithm:\t{amount}"


    def generateOriginalFileSizeTable(self, filePaths):
        lines = ["File\tSize [B]"]
        for path in filePaths:
            lines += [f"{os.path.basename(path)}\t{helpers.getFileSize(path)}"]
        return joinStringList(lines, sep='\n')


    def generateCompressionEffectivenessReport(self, runners, filePaths):
            getCompEffectiveness = lambda run, cmdType, filePath: run.getSize(cmdType=cmdType, cmdMode=helpers.CmdMode.PACK, filePath=filePath) / run.getSize(cmdType=helpers.CmdType.DEFAULT, cmdMode=helpers.CmdMode.UNPACK, filePath=filePath)
            # compression effectiveness = rozmiar_spakowany / rozmiar_pierwotny [bez jednostki]
            return "CompressionEffectivenessReport\n" +\
                    self.generateProcessedReport(runners=runners, 
                                                filePaths=filePaths,
                                                getter=getCompEffectiveness)
    

    def generateProcessingSpeedReport(self, runners, filePaths):
        getProcessingSpeed = lambda run, cmdType, filePath: run.getSize(cmdType=helpers.CmdType.DEFAULT, cmdMode=helpers.CmdMode.UNPACK, filePath=filePath) / (run.getTime(cmdType=cmdType, cmdMode=helpers.CmdMode.PACK, filePath=filePath) + run.getTime(cmdType=cmdType, cmdMode=helpers.CmdMode.UNPACK, filePath=filePath))
        # processing speed = rozmiar_pierwotny / (czas_pakowania+czas_rozpakowania)  [bajt / mikrosekunde]
        return "ProcessingSpeedReport\n" +\
                self.generateProcessedReport(runners=runners, 
                                            filePaths=filePaths,
                                            getter=getProcessingSpeed)


    def generateRamEfficiencyReport(self, runners, filePaths):
        kilobytesToBytesMultiplier = 1000
        getRamEfficiency = lambda run, cmdType, filePath: (run.getRamUsage(cmdType=cmdType, cmdMode=helpers.CmdMode.PACK, filePath=filePath) + run.getRamUsage(cmdType=cmdType, cmdMode=helpers.CmdMode.UNPACK, filePath=filePath)) * kilobytesToBytesMultiplier / run.getSize(cmdType=helpers.CmdType.DEFAULT, cmdMode=helpers.CmdMode.UNPACK, filePath=filePath)
        # ram efficiency = (ram_pakowania + ram_rozpakowania) * kilobytesToBytesMultiplier / rozmiar_pierwotny [bez jednostki]

        return "RamEfficiencyReport\n" +\
                self.generateProcessedReport(runners=runners, 
                                            filePaths=filePaths,
                                            getter=getRamEfficiency)


    def generateFullReport(self, runners, filePaths, amountOfTestRuns):
        tables = [
            self.generateAmountOfTestRunsStats(amount=amountOfTestRuns),
            self.generateOriginalFileSizeTable(filePaths=filePaths),
            self.generateRawReport(runners=runners, filePaths=filePaths),

            self.generateCompressionEffectivenessReport(runners, filePaths),
            self.generateProcessingSpeedReport(runners, filePaths),
            self.generateRamEfficiencyReport(runners, filePaths)
            ]
        
        return joinStringList(tables, sep="\n\n\n")


