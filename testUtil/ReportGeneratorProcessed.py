from helpers import joinStringList 
import helpers 
import os 
from ReportGeneratorBase import *

class ReportGeneratorProcessed(ReportGeneratorBase):
    def __init__(self):
        super().__init__()


    def generateHeadersForProcessedReport(self, filePaths):
        filenames = [] + self._runIdentHeaders
        for path in filePaths:
            filenames += [os.path.basename(path)]

        return [joinStringList(filenames)]


    def generateProcessedReport(self, runners, filePaths, getter):
        lines = []
        lines += self.generateHeadersForProcessedReport(filePaths= filePaths)
        for runner in runners:
            for cmdType in helpers.CmdType.__members__.values():
                if runner._cmd[cmdType]:
                    lines += [self.getTestcaseData(runner= runner, cmdType= cmdType, filePaths= filePaths, getter=getter)]
        return joinStringList(lines, '\n')
    

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
