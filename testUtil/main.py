import os
import statistics
import itertools

from testRunner import *
from ReportGenerator import *


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

tk2kCommandGen = lambda mode, alg, pathToAdd, archivePath, blockSize="16": f'{helpers.tk2kPath} --mode={mode} --alg={alg} --archive={archivePath} --fileToAdd={pathToAdd} --blockSize={blockSize}'

tk2k= TestRunner(
    name = 'tk2k',
    default= lambda pathToAdd, archivePath: tk2kCommandGen(mode='compress', alg='1', pathToAdd=pathToAdd, archivePath=archivePath),
    max= lambda pathToAdd, archivePath: tk2kCommandGen(mode='compress', alg='2', pathToAdd=pathToAdd, archivePath=archivePath),
    unpack= lambda pathOutput, archivePath: f'{helpers.tk2kPath} --mode=decompress --archive={archivePath} --output={pathOutput}',
    tk2kBlockSize = "16")


tk2k_customRunnerGenerator = lambda alg, blockSize: TestRunner(
    name = f'tk2k_{alg}',
    default= lambda pathToAdd, archivePath: tk2kCommandGen(mode='compress', alg=alg, pathToAdd=pathToAdd, archivePath=archivePath, blockSize=blockSize),
    max= None,
    unpack= lambda pathOutput, archivePath: f'{helpers.tk2kPath} --mode=decompress --alg={alg} --archive={archivePath} --output={pathOutput}',
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
            runner.runTest(cmdType=helpers.CmdType.DEFAULT, filePath=path, amount=amount)
            runner.runTest(cmdType=helpers.CmdType.MAX, filePath=path, amount=amount)
            current_counter += 1













def writeString(stringToSave, folderPath= "wynikiV2/", filename= "output.txt"):
    
    os.makedirs(
        name=folderPath,
        exist_ok=True)

    outputPath = folderPath + filename

    with open(outputPath, "w") as f:
        f.write(stringToSave)

    print(f'File saved, path: "{outputPath}"')


runnersToRun = []
# runnersToRun = []
# runnersToRun += [zip]
# runnersToRun += [_7zip]
# runnersToRun += [bzip2]
# runnersToRun += [rar]
# runnersToRun += [gzip]
# runnersToRun += [tk2k]


filesToMeasure = []
# filesToMeasure += getFilePathsFromFolder(helpers.testFolderPath + "mini")
# filesToMeasure += getFilePathsFromFolder(helpers.malePath)
# filesToMeasure += getFilePathsFromFolder(helpers.sredniePath)
# filesToMeasure += getFilePathsFromFolder(helpers.duzePath)
# filesToMeasure += getFilePathsFromFolder(helpers.bardzoDuzePath)

filesToMeasure += ['/home/pc/Desktop/testFiles/male/alice29.txt']
# filesToMeasure += ['/home/pc/Desktop/testFiles/srednie/mozilla']
# filesToMeasure += ['/home/pc/Desktop/testFiles/srednie/x-ray']




def main():
    # myRunners = makeExperimentalTk2kRunners()#[bzip2]#allRunners#[zip, rar]
    myRunners = allRunners
    pathsToTest = getFilePathsFromFolder(helpers.malePath)
    # pathsToTest = ['/home/pc/Desktop/testFiles/male/alice29.txt']
    amountOfTestRuns = 1

    gatherResults(testRunners= myRunners, filePaths= pathsToTest, amount=amountOfTestRuns)
    report = ReportGenerator().generateFullReport(runners= myRunners, filePaths= pathsToTest, amountOfTestRuns= amountOfTestRuns)
    writeString(stringToSave=report, filename="male.txt")

main()