import os
import helpers


def joinStringList(stringList, sep='\t'):
    return sep.join(map(str, stringList))




def generateOriginalFileSizeTable(filePaths):
    lines = ["File\tSize [B]"]
    for path in filePaths:
        lines += [f"{os.path.basename(path)}\t{helpers.getFileSize(path)}"]
    return joinStringList(lines, sep='\n')



class ReportGenerator():
    def __init__(self):
        self._headerIdentificationNames = [] # string name to be written on top of column in report
        self._lineIdentificationGenerators = [] # takes lambdas, that take "TestRunner" object and return string

        self._headerNames = [] # string name to be written on top of column in report
        self._resultPerFileGenerators = [] # takes lambdas, that take "TestRunner" object and return string


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
        self._headerNames += [name]
        self._resultPerFileGenerators += [valueGetter]


    def addIdentificationColumn(self, name, valueGetter):
        self._headerIdentificationNames += [name]
        self._lineIdentificationGenerators += [valueGetter]



    def _getReportLine_Filename(self, filePath):
        line = [os.path.basename(filePath)] * (len(self._resultPerFileGenerators))
        return joinStringList(line)
        

    def _getReportLine_OriginalFileSize(self, runner, filePath):
        originalSize = runner.getSize(cmdType=helpers.CmdType.DEFAULT, cmdMode=helpers.CmdMode.UNPACK, filePath=filePath)
        line = [str(originalSize)] * (len(self._resultPerFileGenerators))
        return joinStringList(line)


    def _getReportLine_repeatingColumnNames(self):
        return joinStringList(self._headerNames)


    def _getReportLinesPerFileFromRunner(self, runner, cmdType, filePath):
        values = []

        for getter in self._resultPerFileGenerators:
            line = getter( run= runner, cmdType= cmdType, filePath= filePath)

            values += [line]

        return joinStringList(values)


    def _getReportLineIdentificationDataFromRunner(self, runner, cmdType, filePath):
        values = []

        for getter in self._lineIdentificationGenerators:
            line = getter( run= runner, cmdType= cmdType, filePath= filePath)
            values += [line]

        return joinStringList(values)


    def _getResultsFromRunner(self, runner, cmdType, filePaths):
        resultsForFile = [self._getReportLineIdentificationDataFromRunner(runner= runner, cmdType= cmdType, filePath= '')]

        for path in filePaths:
            resultsForFile += [self._getReportLinesPerFileFromRunner(runner= runner, cmdType= cmdType, filePath= path)]
        
        return joinStringList(resultsForFile)



    def _getReportHeaders(self, runner, filePaths):
        originalSizes = ["", ""]
        filenames = ["", ""]
        columnNames = [] + self._headerIdentificationNames

        for path in filePaths:
            originalSizes += [self._getReportLine_OriginalFileSize(runner=runner, filePath=path)]
            filenames += [self._getReportLine_Filename(filePath=path)]
            columnNames += [self._getReportLine_repeatingColumnNames()]
        
        return [joinStringList(originalSizes),
                joinStringList(filenames),
                joinStringList(columnNames)]


    def getReport(self, runners, filePaths):
        lines = []
        lines += self._getReportHeaders(runner= runners[0], filePaths= filePaths)

        for runner in runners:
            for cmdType in helpers.CmdType.__members__.values():
                if runner._cmd[cmdType]:
                    lines += [self._getResultsFromRunner(runner= runner, cmdType= cmdType, filePaths= filePaths)]

        return joinStringList(lines, '\n')

    def generateAmountOfTestRunsStats(self, amount):
        return f"Amount of test runs of each algorithm:\t{amount}"

    def generateFullReport(self, runners, filePaths, amountOfTestRuns):
        tables = [self.generateAmountOfTestRunsStats(amount=amountOfTestRuns),
                  generateOriginalFileSizeTable(filePaths=filePaths),
                  self.getReport(runners=runners, filePaths=filePaths)]
        
        return joinStringList(tables, sep="\n\n\n")


