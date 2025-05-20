import helpers
from helpers import joinStringList
import os

class ReportGeneratorRaw():
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



    def addIdentificationColumn(self, name, valueGetter):
        self._runIdentHeaders += [name]
        self._runIdentGetters += [valueGetter]
        
    def addColumnPerFile(self, name, valueGetter):
        self._partialHeaderNames += [name]
        self._testResultGetters += [valueGetter]

    def generateRawReport(self, runners, filePaths):
        lines = []
        lines += self.generateHeadersForRawReport(filePaths= filePaths)
        for runner in runners:
            for cmdType in helpers.CmdType.__members__.values():
                if runner._cmd[cmdType]:
                    lines += [self.getLinesOfRawTestcaseData(runner= runner, cmdType= cmdType, filePaths= filePaths)]
        return joinStringList(lines, '\n')

    def generateHeadersForRawReport(self, filePaths):
        columnNames = [] + self._runIdentHeaders
        filenames = [] + self._runIdentHeaders
        for path in filePaths:
            columnNames += [self.getPrependedFileNameToColumnNames(filename=os.path.basename(path))]
            filenames += [self.getRepeatedFileNames(filename=os.path.basename(path))] # for the benefit of excel only
        return [joinStringList(columnNames),
                joinStringList(filenames)]
    
    def getRepeatedFileNames(self, filename, amount=None): # for ease of use with excel
        if amount is None:
            amount = len(self._testResultGetters)
         
        return joinStringList([filename] * amount)

    
    def getLinesOfRawTestcaseData(self, runner, cmdType, filePaths):
        resultsForFile = [self.getProgramNameAndProfile(runner=runner, cmdType=cmdType, filePath='')]
        for path in filePaths:
            resultsForFile += [self.getTestcaseData(runner=runner, cmdType=cmdType, filePath=path)]
        return joinStringList(resultsForFile)
    
    def getPrependedFileNameToColumnNames(self, filename):
        fileName_columnNames = [f"{filename}_{columnName}" for columnName in self._partialHeaderNames]
        return joinStringList(fileName_columnNames)
    
    #duplicated from ReportGenerator
    def getProgramNameAndProfile(self, runner, cmdType, filePath):
        values = []

        for getter in self._runIdentGetters:
            line = getter(run=runner, cmdType=cmdType, filePath=filePath)
            values += [line]

        return joinStringList(values)
    
    def getTestcaseData(self, runner, cmdType, filePath):
            values = []
            for getter in self._testResultGetters:
                values += [getter(run=runner, cmdType=cmdType, filePath=filePath)]
            # print(values)
            return joinStringList(values)
    
    
