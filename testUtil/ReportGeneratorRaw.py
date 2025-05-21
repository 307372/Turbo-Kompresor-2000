import helpers
from helpers import joinStringList
import os
from ReportGeneratorBase import *

class ReportGeneratorRaw(ReportGeneratorBase):
    def __init__(self):
        super().__init__()


    def generateRawReport(self, runners, filePaths):
        lines = []
        lines += self.generateHeadersForRawReport(filePaths= filePaths)
        for runner in runners:
            for cmdType in helpers.CmdType.__members__.values():
                if runner._cmd[cmdType]:
                    lines += [self.getTestcaseData(runner= runner, cmdType= cmdType, filePaths= filePaths, getter=self.getRawTestcaseData)]
        return joinStringList(lines, '\n')


    def generateHeadersForRawReport(self, filePaths):
        columnNames = [] + self._runIdentHeaders
        filenames = [] + self._runIdentHeaders
        for path in filePaths:
            columnNames += [self.getPrependedFileNameToColumnNames(filename=os.path.basename(path))]
            filenames += [self.getRepeatedFileNames(filename=os.path.basename(path))] # for the benefit of excel only
        
        return ["Zebrane dane bez przetwarzania",
                joinStringList(columnNames),
                joinStringList(filenames)]


    def getRepeatedFileNames(self, filename, amount=None): # for ease of use with excel
        if amount is None:
            amount = len(self._testResultGetters)
        return joinStringList([filename] * amount)


    def getPrependedFileNameToColumnNames(self, filename):
        fileName_columnNames = [f"{filename}_{columnName}" for columnName in self._partialHeaderNames]
        return joinStringList(fileName_columnNames)
    

    def getRawTestcaseData(self, runner, cmdType, filePath):
            values = []
            for getter in self._testResultGetters:
                values += [getter(run=runner, cmdType=cmdType, filePath=filePath)]
            return joinStringList(values)
    
    
