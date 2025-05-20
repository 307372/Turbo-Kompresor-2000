import os
import helpers
from helpers import joinStringList
from ReportGeneratorRaw import *
from ReportGeneratorProcessed import *







class ReportGenerator():
    def __init__(self):
        self._runIdentHeaders = [] # string name to be written on top of column in report
        self._runIdentGetters = [] # takes lambdas, that take "TestRunner" object and return string
        # said string contains info that allows us to identify what run we're checking the results for

        self._partialHeaderNames = [] # string name to be written on top of column in report with file name attached
        self._testResultGetters = []      # takes lambdas, that take "TestRunner" object and return string
        # said string contains test results for a given run
        self.reportGeneratorRaw = ReportGeneratorRaw()
        self.reportGeneratorProcessed = ReportGeneratorProcessed()

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

    

    

        # filling raw report horizontally
        # a = namedtuple('TestId', ['runner', 'cmdtype', 'filepath'])
        # from collections import namedtuple
        # TestId = namedtuple('TestId', ['runner', 'cmdType', 'filePath'])
        # identification = TestId(runner=runner, cmdType=cmdType, filePath=path)
        #(*identification)] works

    



    def addColumnPerFile(self, name, valueGetter):
        self._partialHeaderNames += [name]
        self._testResultGetters += [valueGetter]


    def addIdentificationColumn(self, name, valueGetter):
        self._runIdentHeaders += [name]
        self._runIdentGetters += [valueGetter]



    def generateAmountOfTestRunsStats(self, amount):
        return f"Amount of test runs of each algorithm:\t{amount}"


    def generateOriginalFileSizeTable(self, filePaths):
        lines = ["File\tSize [B]"]
        for path in filePaths:
            lines += [f"{os.path.basename(path)}\t{helpers.getFileSize(path)}"]
        return joinStringList(lines, sep='\n')




    def generateFullReport(self, runners, filePaths, amountOfTestRuns):
        tables = [
            self.generateAmountOfTestRunsStats(amount=amountOfTestRuns),
            self.generateOriginalFileSizeTable(filePaths=filePaths),
            self.reportGeneratorRaw.generateRawReport(runners=runners, filePaths=filePaths),

            self.reportGeneratorProcessed.generateCompressionEffectivenessReport(runners, filePaths),
            self.reportGeneratorProcessed.generateProcessingSpeedReport(runners, filePaths),
            self.reportGeneratorProcessed.generateRamEfficiencyReport(runners, filePaths)
            ]
        
        return joinStringList(tables, sep="\n\n\n")


