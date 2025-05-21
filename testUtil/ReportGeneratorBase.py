from helpers import joinStringList 
import helpers 
import os 


class ReportGeneratorBase():
    def __init__(self):
        self._runIdentHeaders = [] # string name to be written on top of column in report
        self._runIdentGetters = [] # takes lambdas, that take "TestRunner" object and return string
        # said string contains info that allows us to identify what run we're checking the results for

        self._partialHeaderNames = [] # string name to be written on top of column in report with file name attached
        self._testResultGetters =  [] # takes lambdas, that take "TestRunner" object and return string
        # said string contains test results for a given run


    def addIdentificationColumn(self, name, valueGetter):
        self._runIdentHeaders += [name]
        self._runIdentGetters += [valueGetter]
        
    def addColumnPerFile(self, name, valueGetter):
        self._partialHeaderNames += [name]
        self._testResultGetters += [valueGetter]


    def addIdentificationColumnsGenerators(self):
        self.addIdentificationColumn(
            name="program",
            valueGetter = lambda run, cmdType, filePath: run.name)
        
        self.addIdentificationColumn(
            name="tryb",
            valueGetter = lambda run, cmdType, filePath: helpers.getCmdTypeStr(cmdType))


    def addColumnNameGenerators(self):
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

    def getProgramNameAndProfile(self, runner, cmdType, filePath):
        values = []
        for getter in self._runIdentGetters:
            line = getter(run=runner, cmdType=cmdType, filePath=filePath)
            values += [line]
        return joinStringList(values)

    def getTestcaseData(self, runner, cmdType, filePaths, getter):
        resultsForFile = [self.getProgramNameAndProfile(runner= runner, cmdType= cmdType, filePath= '')]
        for path in filePaths:
            resultsForFile += [getter(runner, cmdType= cmdType, filePath= path)]
        return joinStringList(resultsForFile)
    
