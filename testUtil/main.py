import subprocess
import tempfile
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

tk2kCommandGen = lambda mode, alg, pathToAdd, archivePath: f'{consts.tk2kPath} --mode={mode} --alg={alg} --archive={archivePath} --fileToAdd={pathToAdd}'

tk2k= TestRunner(
    name = 'tk2k',
    default= lambda pathToAdd, archivePath: tk2kCommandGen(mode='compress', alg='1', pathToAdd=pathToAdd, archivePath=archivePath),
    max= lambda pathToAdd, archivePath: tk2kCommandGen(mode='compress', alg='2', pathToAdd=pathToAdd, archivePath=archivePath),
    unpack= lambda pathOutput, archivePath: f'{consts.tk2kPath} --mode=decompress --archive={archivePath} --output={pathOutput}')

tk2kRunnerGen = lambda alg: TestRunner(
    name = f'tk2k_{alg}',
    default= lambda pathToAdd, archivePath: tk2kCommandGen(mode='compress', alg=alg, pathToAdd=pathToAdd, archivePath=archivePath),
    max= None,
    unpack= lambda pathOutput, archivePath: f'{consts.tk2kPath} --mode=decompress --alg={alg} --archive={archivePath} --output={pathOutput}')

def makeExperimentalTk2kRunners():
    toReorder = {'BWT2', 'MTF', 'RLE'}
    combinations = []
    for i in range(len(toReorder)+1):
        combinations += list(itertools.permutations(toReorder,i))
    # l = list(itertools.permutations(toReorder,3))
    # print(combinations)
    algList = []
    for comb in combinations:
        algList += ['+'.join(comb)]

    for comb in combinations:
        algList += ['+'.join(list(comb) + ['AC2'])]
    print(algList)
    runners = []
    for alg in algList:
        runners.append(tk2kRunnerGen(alg=alg))
    return runners

        

# ~/repos/Turbo-Kompresor-2000/builds/build-Turbo-Kompresor-2000-Desktop-Release/Turbo-Kompresor-2000 --mode=compress --alg=1 --archive=../testFiles/packedDefault.res --fileToAdd=../testFiles/male/alice29.txt
# --mode=decompress --archive=../testFiles/packedDefault.res --output=../testFiles/unpacked
allRunners = [zip, _7zip, bzip2, rar, gzip, tk2k]
# bzip2 --keep --stdout male/alice29.txt > compressed.res
# bzip2 --decompress --stdout compressed.res > decompressed

# def gatherResults(
#         commands,  # list of commands
#         filePaths, # list of files
#         amount=10):
#     print('\t' + '\t'.join([os.path.basename(path) for path in filePaths]))
#     for cmd in commands:
#         print(cmd, end='\t')
#         for path in filePaths:
#             result = getTimings(cmd, path, amount)
#             print(result, end='\t')
#         print()


def getTimings(cmd, path, amount):
    fullCommand = cmd + ' ' + path
    compressionResults = []
    for _ in range(amount):
        compressionResults.append(testExecutionTime(fullCommand))
    return round(statistics.fmean(compressionResults))


def gatherResults(testRunners, filePaths, amount=10):
    for path in filePaths:
        for runner in testRunners:
            runner.runTest(cmdType=consts.CmdType.DEFAULT, filePath=path, amount=amount)
            runner.runTest(cmdType=consts.CmdType.MAX, filePath=path, amount=amount)

def runTests(testRunners, filePaths):
    for runner in testRunners:
        for path in filePaths:
            runner.runCorrectnessTest(cmdType=consts.CmdType.DEFAULT, filePath=path)
            runner.runCorrectnessTest(cmdType=consts.CmdType.MAX, filePath=path)

def makeTimeTable(runners, filePaths):
    return makeResultsTableForRunners(runners=runners, filePaths=filePaths, valueGetter=TestRunner.getTime)

def makeEffectTable(runners, filePaths):
    return makeResultsTableForRunners(runners=runners, filePaths=filePaths, valueGetter=TestRunner.getEffect)

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




runnersToRun = []
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

testFile = '../testFiles/male/alice29.txt'
# filesToMeasure += [testFile]
filesToMeasure += ['/home/pc/Desktop/testFiles/srednie/mozilla']

# fullCommand = '7z'
# testExecutionTime(fullCommand)
# zip, _7zip

runnersToRun = makeExperimentalTk2kRunners()

def main():
    gatherResults(testRunners = runnersToRun, filePaths = filesToMeasure, amount=1)
    times = makeTimeTable(runnersToRun, filesToMeasure)
    effects = makeEffectTable(runnersToRun, filesToMeasure)
    with open("results.txt", "w") as f:
        f.write(times)
        f.write(effects)
    print(times)
    print(effects)

main()
# runTests(runnersToRun, filesToMeasure)



# plotter.plotData(runnersToRun, filesToMeasure)
# printResultsTable(runner = zip, filePaths = filesToMeasure)
# printResultsTable(runner = _7zip, filePaths = filesToMeasure)

# runTests(testRunners = [tk2k], filePaths = [testFile])
# runTests(testRunners = allRunners, filePaths = filesToMeasure)
# gatherResults(testRunners = [bzip2], filePaths = [testFile], amount=1)
# printResultsTable(runner = bzip2, filePaths = [testFile])


# gatherResults(testRunners = [zip], filePaths = [testFile])
