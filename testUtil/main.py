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
    unpack= lambda pathOutput, archivePath: f'{consts.tk2kPath} --mode=decompress --archive={archivePath} --output={pathOutput}')

tk2kRunnerXmb = lambda alg, blockSize: TestRunner(
    name = f'{alg}\t{blockSize}',
    default= lambda pathToAdd, archivePath: tk2kCommandGen(mode='compress', alg=alg, pathToAdd=pathToAdd, archivePath=archivePath, blockSize=blockSize),
    max= None,
    unpack= lambda pathOutput, archivePath: f'{consts.tk2kPath} --mode=decompress --alg={alg} --archive={archivePath} --output={pathOutput}')

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
    blockList = ['2', '8', '16']
    runners = []
    for block in blockList:
        for alg in algList:
            runners.append(tk2kRunnerXmb(alg=alg, blockSize=block))
    return runners

allRunners = [zip, _7zip, bzip2, rar, gzip, tk2k]



def getTimings(cmd, path, amount):
    fullCommand = cmd + ' ' + path
    compressionResults = []
    for _ in range(amount):
        compressionResults.append(testExecutionTime(fullCommand))
    return round(statistics.fmean(compressionResults))

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
        report = makeTk2kResultString(testRunners, path)
        writeTk2kResult(report, path)

def runTests(testRunners, filePaths):
    for runner in testRunners:
        for path in filePaths:
            runner.runCorrectnessTest(cmdType=consts.CmdType.DEFAULT, filePath=path)
            runner.runCorrectnessTest(cmdType=consts.CmdType.MAX, filePath=path)

def makeTimeTable(runners, filePaths):
    return makeResultsTableForRunners(runners=runners, filePaths=filePaths, valueGetter=TestRunner.getTime)

def makeSizeTable(runners, filePaths):
    return makeResultsTableForRunners(runners=runners, filePaths=filePaths, valueGetter=TestRunner.getSize)

def makeResultsTableForRunners(runners, filePaths, valueGetter):
    output = '\t' + '\t'.join([os.path.basename(path) for path in filePaths]) + '\n'
    for runner in runners:
        output += makeResultsLinesForRunner(runner=runner, filePaths=filePaths, valueGetter=valueGetter)
    return output


def makeResultsLinesForRunner(runner, filePaths, valueGetter):
    # top row
    
    # print()
    # rows with data
    output = ""
    for cmdType in consts.CmdType.__members__.values():
        for cmdMode in consts.CmdMode.__members__.values():
            if runner._cmd[cmdType]:
                output += makeResultsLine(runner=runner, cmdType=cmdType, cmdMode=cmdMode, filePaths=filePaths, valueGetter=valueGetter)
    # print(output)
    return output
            
    
def makeResultsLine(runner, cmdType, cmdMode, filePaths, valueGetter):
    # cmd = runner.getPackCmd(cmdType=cmdType, pathToAdd="FILE_TO_ADD", archivePath="PATH_TO_ARCHIVE") if cmdMode == consts.CmdMode.PACK else runner.getUnpackCmd(cmdType=cmdType, unpackPath="EXTRACTION_PATH", archivePath="PATH_TO_ARCHIVE")
    # output = f"{cmd.split()[0]} ({'max' if cmdType == consts.CmdType.MAX else 'default'} {'pack' if cmdMode == consts.CmdMode.PACK else 'unpack'})\t"
    output = f"{runner.name} {'max' if cmdType == consts.CmdType.MAX else 'default'} {'pack' if cmdMode == consts.CmdMode.PACK else 'unpack'}\t"
    # print(cmd, end='\t') # name of command
    for path in filePaths: # results for each file in a single line
        result = valueGetter(self=runner, cmdType=cmdType, cmdMode=cmdMode, filePath=path)
        output += f"{result}\t"
        # print(result, end='\t')
    # print()
    # print(output)
    return output + '\n'




def makeTk2kResultsHeader(runner, filePath):
    output = f'\t\t{os.path.basename(filePath)}\trozmiar_pierwotny:\t{runner.getSize(cmdType=consts.CmdType.DEFAULT, cmdMode=consts.CmdMode.UNPACK, filePath=filePath)}\n'
    output += f'algorytm\trozmiar_bloku\tczas_pakowania\tczas_rozpakowania\trozmiar_spakowany\tram_pakowania\tram_rozpakowania\n'
    return output


def makeTk2kResultsLine(runner, filePath):
    output = [runner.name]
    output += [f'{runner.getTime(cmdType=consts.CmdType.DEFAULT, cmdMode=consts.CmdMode.PACK, filePath=filePath)}']
    output += [f'{runner.getTime(cmdType=consts.CmdType.DEFAULT, cmdMode=consts.CmdMode.UNPACK, filePath=filePath)}']
    
    output += [f'{runner.getSize(cmdType=consts.CmdType.DEFAULT, cmdMode=consts.CmdMode.PACK, filePath=filePath)}']
    # output += [f'{runner.getSize(cmdType=consts.CmdType.DEFAULT, cmdMode=consts.CmdMode.PACK, filePath=filePath) / runner.getSize(cmdType=consts.CmdType.DEFAULT, cmdMode=consts.CmdMode.UNPACK, filePath=filePath)}']

    output += [f'{runner.getRamUsage(cmdType=consts.CmdType.DEFAULT, cmdMode=consts.CmdMode.PACK, filePath=filePath)}']
    output += [f'{runner.getRamUsage(cmdType=consts.CmdType.DEFAULT, cmdMode=consts.CmdMode.UNPACK, filePath=filePath)}']
    return '\t'.join(output) + '\n'


def makeTk2kResultString(runners, filePath):
    output = makeTk2kResultsHeader(runners[0], filePath)
    for runner in runners:
        output += makeTk2kResultsLine(runner, filePath)
    return output + '\n'


def writeTk2kResult(outputString, filePath):
    with open(f"wyniki/tk2k_{os.path.basename(filePath)}.txt", "w") as f:
        f.write(outputString)



def saveTk2kResults(runners, filePaths):
    for path in filePaths:
        report = makeTk2kResultString(runners, path)
        writeTk2kResult(report, path)




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


def testTk2k():
    runnersToRun = makeExperimentalTk2kRunners()
    gatherResults(testRunners = runnersToRun, filePaths = filesToMeasure, amount=3)
    saveTk2kResults(runners=runnersToRun, filePaths=filesToMeasure)


def main():
    gatherResults(testRunners = runnersToRun, filePaths = filesToMeasure, amount=3)
    times = makeTimeTable(runnersToRun, filesToMeasure)
    effects = makeSizeTable(runnersToRun, filesToMeasure)
    with open("wyniki/tk2k_enwik9.txt", "w") as f:
        f.write(times)
        f.write(effects)
    print(times)
    print(effects)




# main()
testTk2k()
# runTests(runnersToRun, filesToMeasure)



# plotter.plotData(runnersToRun, filesToMeasure)
# printResultsTable(runner = zip, filePaths = filesToMeasure)
# printResultsTable(runner = _7zip, filePaths = filesToMeasure)

# runTests(testRunners = [tk2k], filePaths = [testFile])
# runTests(testRunners = allRunners, filePaths = filesToMeasure)
# gatherResults(testRunners = [bzip2], filePaths = [testFile], amount=1)
# printResultsTable(runner = bzip2, filePaths = [testFile])


# gatherResults(testRunners = [zip], filePaths = [testFile])
