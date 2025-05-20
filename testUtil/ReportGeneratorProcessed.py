from helpers import joinStringList 
import helpers 
import os 


class ReportGeneratorProcessed():
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


    def addIdentificationColumn(self, name, valueGetter):
        self._runIdentHeaders += [name]
        self._runIdentGetters += [valueGetter]


    def addColumnPerFile(self, name, valueGetter):
        self._partialHeaderNames += [name]
        self._testResultGetters += [valueGetter]


    
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
                    lines += [self.getProcessedTestcaseData(runner= runner, cmdType= cmdType, filePaths= filePaths, getter=getter)]

        return joinStringList(lines, '\n')

    def getProgramNameAndProfile(self, runner, cmdType, filePath):
            values = []

            for getter in self._runIdentGetters:
                line = getter(run=runner, cmdType=cmdType, filePath=filePath)
                values += [line]

            return joinStringList(values)
    
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
