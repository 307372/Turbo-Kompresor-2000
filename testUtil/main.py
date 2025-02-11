import os
import statistics
import itertools

from testRunner import *


zip= TestRunner(
    name = 'zip',
    default = lambda pathToAdd, archivePath: f'zip -o {archivePath} -j {pathToAdd}',
    max = lambda pathToAdd, archivePath: f'zip -9o {archivePath} -j {pathToAdd}',
    unpack = lambda pathOutput, archivePath: f'unzip {archivePath} -d {pathOutput}')


_7zip= TestRunner(
    name = '7zip',
    default= lambda pathToAdd, archivePath: f'7z a {archivePath} {pathToAdd}',
    max= lambda pathToAdd, archivePath: f'7z a -mx=9 {archivePath} {pathToAdd}',
    unpack= lambda pathOutput, archivePath: f'7z x -o{pathOutput} {archivePath}')

bzip2= TestRunner(
    name = 'bzip2',
    default= lambda pathToAdd, archivePath: f'bzip2 --keep --stdout {pathToAdd} > {archivePath}',
    max= None,
    unpack= lambda pathOutput, archivePath: f'bzip2 --decompress --keep --stdout {archivePath} > {pathOutput}')

rar= TestRunner(
    name = 'rar',
    default= lambda pathToAdd, archivePath: f'rar a {archivePath} {pathToAdd}',
    max= lambda pathToAdd, archivePath: f'rar a -m5 {archivePath} {pathToAdd}',
    unpack= lambda pathOutput, archivePath: f'unrar e {archivePath} {pathOutput}/')

gzip= TestRunner(
    name = 'gzip',
    default= lambda pathToAdd, archivePath: f'gzip --stdout {pathToAdd} > {archivePath}',
    max= lambda pathToAdd, archivePath: f'gzip --best --stdout {pathToAdd} > {archivePath}',
    unpack= lambda pathOutput, archivePath: f'gzip --decompress --stdout {archivePath} > {pathOutput}')

tk2kCommandGen = lambda mode, alg, pathToAdd, archivePath, blockSize="16": f'{consts.tk2kPath} --mode={mode} --alg={alg} --archive={archivePath} --fileToAdd={pathToAdd} --blockSize={blockSize}'

tk2k= TestRunner(
    name = 'tk2k',
    default= lambda pathToAdd, archivePath: tk2kCommandGen(mode='compress', alg='1', pathToAdd=pathToAdd, archivePath=archivePath),
    max= lambda pathToAdd, archivePath: tk2kCommandGen(mode='compress', alg='2', pathToAdd=pathToAdd, archivePath=archivePath),
    unpack= lambda pathOutput, archivePath: f'{consts.tk2kPath} --mode=decompress --archive={archivePath} --output={pathOutput}',
    tk2kBlockSize = "16")


tk2k_customRunnerGenerator = lambda alg, blockSize: TestRunner(
    name = f'tk2k_{alg}',
    default= lambda pathToAdd, archivePath: tk2kCommandGen(mode='compress', alg=alg, pathToAdd=pathToAdd, archivePath=archivePath, blockSize=blockSize),
    max= None,
    unpack= lambda pathOutput, archivePath: f'{consts.tk2kPath} --mode=decompress --alg={alg} --archive={archivePath} --output={pathOutput}',
    tk2kBlockSize = blockSize,
    tk2kAlgorithm = alg)



def makeExperimentalTk2kRunners():
    toReorder = {'BWT2', 'MTF', 'RLE'}
    combinations = []
    for i in range(len(toReorder)+1):
        combinations += list(itertools.permutations(toReorder,i))

    algList = []
    # for comb in combinations:
        # algList += ['+'.join(comb)]

    for comb in combinations:
        algList += ['+'.join(list(comb) + ['AC'])]

    for comb in combinations:
        algList += ['+'.join(list(comb) + ['AC2'])]
    print(algList)
    # blockList = ['1', '0.5', '0.25', '0.125']
    # blockList = ['2', '8', '16']
    blockList = ['16']
    runners = []
    for block in blockList:
        for alg in algList:
            runners.append(tk2k_customRunnerGenerator(alg=alg, blockSize=block))
    return runners

allRunners = [zip, _7zip, bzip2, rar, gzip, tk2k]




def printProgressCounter(current, max):
    print(f'===== Postep: {current}/{max} =====')


def gatherResults(testRunners, filePaths, amount=10):
    total_ops = len(filePaths) * len(testRunners)
    current_counter = 0
    for path in filePaths:
        for runner in testRunners:
            printProgressCounter(current_counter, total_ops)
            runner.runTest(cmdType=consts.CmdType.DEFAULT, filePath=path, amount=amount)
            runner.runTest(cmdType=consts.CmdType.MAX, filePath=path, amount=amount)
            current_counter += 1




def joinStringList(stringList, sep='\t'):
    return sep.join(map(str, stringList))




def generateOriginalFileSizeTable(filePaths):
    lines = ["File\tSize [B]"]
    for path in filePaths:
        lines += [f"{os.path.basename(path)}\t{getFileSize(path)}"]
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
            valueGetter = lambda run, cmdType, filePath: consts.getCmdTypeStr(cmdType))

        # reportGenerator.addColumn(  # (TK2K only)
        #     name="rozmiar_bloku",
        #     valueGetter = lambda run: run.tk2kBlockSize if run.tk2kBlockSize is not None else "")

        # reportGenerator.addColumn(  # (TK2K only)
        #     name="algorytm",
        #     valueGetter = lambda run: run.tk2kAlgorithm if run.tk2kAlgorithm is not None else "")


        self.addColumnPerFile(
            name="czas_pakowania",
            valueGetter = lambda run, cmdType, filePath: run.getTime(cmdType=cmdType, cmdMode=consts.CmdMode.PACK, filePath=filePath))

        self.addColumnPerFile(
            name="czas_rozpakowania",
            valueGetter = lambda run, cmdType, filePath: run.getTime(cmdType=cmdType, cmdMode=consts.CmdMode.UNPACK, filePath=filePath))

        self.addColumnPerFile(
            name="rozmiar_spakowany",
            valueGetter = lambda run, cmdType, filePath: run.getSize(cmdType=cmdType, cmdMode=consts.CmdMode.PACK, filePath=filePath))

        self.addColumnPerFile(
            name="ram_pakowania",
            valueGetter = lambda run, cmdType, filePath: run.getRamUsage(cmdType=cmdType, cmdMode=consts.CmdMode.PACK, filePath=filePath))

        self.addColumnPerFile(
            name="ram_rozpakowania",
            valueGetter = lambda run, cmdType, filePath: run.getRamUsage(cmdType=cmdType, cmdMode=consts.CmdMode.UNPACK, filePath=filePath))



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
        originalSize = runner.getSize(cmdType=consts.CmdType.DEFAULT, cmdMode=consts.CmdMode.UNPACK, filePath=filePath)
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
            for cmdType in consts.CmdType.__members__.values():
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





runnersToRun = []
# runnersToRun = []
# runnersToRun += [zip]
# runnersToRun += [_7zip]
# runnersToRun += [bzip2]
# runnersToRun += [rar]
# runnersToRun += [gzip]
# runnersToRun += [tk2k]


filesToMeasure = []
# filesToMeasure += getFilePathsFromFolder(consts.testFolderPath + "mini")
# filesToMeasure += getFilePathsFromFolder(consts.malePath)
# filesToMeasure += getFilePathsFromFolder(consts.sredniePath)
# filesToMeasure += getFilePathsFromFolder(consts.duzePath)
# filesToMeasure += getFilePathsFromFolder(consts.bardzoDuzePath)

filesToMeasure += ['/home/pc/Desktop/testFiles/male/alice29.txt']
# filesToMeasure += ['/home/pc/Desktop/testFiles/srednie/mozilla']
# filesToMeasure += ['/home/pc/Desktop/testFiles/srednie/x-ray']




def main():
    # myRunners = makeExperimentalTk2kRunners()#[bzip2]#allRunners#[zip, rar]
    myRunners = allRunners
    # pathsToTest = getFilePathsFromFolder(consts.malePath)
    pathsToTest = ['/home/pc/Desktop/testFiles/male/alice29.txt']
    amountOfTestRuns = 1

    gatherResults(testRunners= myRunners, filePaths= pathsToTest, amount=amountOfTestRuns)
    report = ReportGenerator().generateFullReport(runners= myRunners, filePaths= pathsToTest, amountOfTestRuns= amountOfTestRuns)
    writeString(stringToSave=report, filename="male.txt")

main()