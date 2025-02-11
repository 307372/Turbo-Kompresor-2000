from Results import Results
import subprocess
import tempfile
import os
from pathlib import Path

import helpers


def getRecursiveFilePathsFromFolder(folderPath):
    # return [folderPath + '/' + filename for filename in os.listdir(folderPath)]
    return sorted([filepath for filepath in Path(folderPath).rglob('*')], key= lambda strong: str(strong).count('/'), reverse=False)

def getFilePathsFromFolder(folderPath):
    return [folderPath + '/' + filename for filename in os.listdir(folderPath)]
    # return sorted([filepath for filepath in Path(folderPath).rglob('*.*')], key= lambda strong: str(strong).count('/'), reverse=False)


def findFileInPathList(fileName, filePaths):
        for path in filePaths:
            if os.path.isfile(path):
                if os.path.basename(path) == fileName:
                    return path
        raise ValueError(f"ERROR: findFileInPathList did not find \"{fileName}\"")


def removeFile(path):
    try:
        os.remove(path)
        # print(f"removing '{path}'")
    except FileNotFoundError:
        pass
        # print('[removeFile] file not found. I guess it\'s fine')
    except IsADirectoryError:
        # print("[removeFile] found folder instead. switching to dir removal")
        removeDirectory(path)


def removeDirectory(path):
    filesToRemoveList = getFilePathsFromFolder(path)
    for filePath in filesToRemoveList:
        removeFile(filePath)
    os.rmdir(path)


def getPackedFilePath(cmdType):
    if cmdType == helpers.CmdType.DEFAULT:
        return helpers.packedFilePathDefault
    elif cmdType == helpers.CmdType.MAX:
        return helpers.packedFilePathMax
    else:
        raise ValueError(f"cmdType not recognized, value: {cmdType}")


def runInBashAndGetResult(command):
    print(f"========== starting command: {command} ==========")
    with tempfile.TemporaryFile() as tempf:
        subprocess.run(command, shell=True, executable="/bin/bash", stderr=tempf, stdout=tempf)
        tempf.seek(0)
        output = str(tempf.read())
        # print()
        print(output)
        # print()
        return int(output.split('\\n')[-2])

def testExecutionTime(command):
    timedCommand = f't1=$(date +%s%6N) ; {command} ; t2=$(date +%s%6N) ; echo ; echo $((t2-t1))'
    return runInBashAndGetResult(timedCommand)

def testPeakRamUsage(command):
    measuredCommand = f'echo ; /usr/bin/time -f "%M" {command}'
    return runInBashAndGetResult(measuredCommand)

def printRoundCounter(i, max):
    print(f'cmd left: {i}/{max}')



class TestRunner:
    def __init__(self, name, default, max, unpack, tk2kBlockSize = None, tk2kAlgorithm = None):
        self.name = name
        self._cmd = {helpers.CmdType.DEFAULT: default, helpers.CmdType.MAX: max} 
        self._unpack = unpack
        self._results = {helpers.CmdType.DEFAULT: {}, helpers.CmdType.MAX: {}}
        self.tk2kBlockSize = tk2kBlockSize
        self.tk2kAlgorithm = tk2kAlgorithm

    def getPackCmd(self, cmdType, pathToAdd, archivePath=None):
        if archivePath is None:
            archivePath = getPackedFilePath(cmdType)
        return self._cmd[cmdType](pathToAdd=pathToAdd, archivePath=archivePath)
    

    def getUnpackCmd(self, cmdType, unpackPath=helpers.unpackedArchivePath, archivePath=None):
        if archivePath is None:
            archivePath = getPackedFilePath(cmdType)
        return self._unpack(pathOutput=unpackPath, archivePath=archivePath)


    def getTime(self, cmdType, cmdMode, filePath):
        if self._results[cmdType][filePath]:
            return self._results[cmdType][filePath].getTime(cmdMode)
        raise ValueError(f"getTime: something's missing, values: cmdType: {cmdType}, cmdMode: {cmdMode}, filePath: {filePath}")
    
    def getSize(self, cmdType, cmdMode, filePath):
        if self._results[cmdType][filePath]:
            if cmdMode == helpers.CmdMode.UNPACK:
                return self._results[cmdType][filePath].getBaseSize()
            return self._results[cmdType][filePath].getPackedSize()
        raise ValueError(f"getSize: something's missing, values: cmdType: {cmdType}, cmdMode: {cmdMode}, filePath: {filePath}")

    def getRamUsage(self, cmdType, cmdMode, filePath):
        if self._results[cmdType][filePath]:
            return self._results[cmdType][filePath].getRamUsage(cmdMode)
        raise ValueError(f"getRamUsage: something's missing, values: cmdType: {cmdType}, cmdMode: {cmdMode}, filePath: {filePath}")

    def runTest(self, cmdType, filePath, amount):
        if self._cmd[cmdType]:
            self._runCmd(cmdType=cmdType, filePath=filePath, amount=amount)
        else:
            if "bzip2" not in self.name and cmdType == helpers.CmdType.MAX:
                print(f"================ NOWY ERROR W 'runTest'! ================")
                print(f"self._cmd[helpers.CmdType.{cmdType}] not available!")

    def isTk2k(self):
        return "tk2k" in self.name

    # def hasCmd(self, cmdType):
    #     return self._cmd[cmdType]


    def _runCmd(self, cmdType, filePath, amount):
        if filePath not in self._results[cmdType].keys():
            self._results[cmdType][filePath] = Results()
        else:
            print("apparently filePath not in self._results[cmdType].keys()")
        

        packCmd = self.getPackCmd(cmdType=cmdType, pathToAdd=filePath)
        unpackCmd = self.getUnpackCmd(cmdType=cmdType)
        self._results[cmdType][filePath].setBaseSize(helpers.getFileSize(filePath))

        self._cleanOutput(cmdType=cmdType)
        packingPeakRamUsage = testPeakRamUsage(packCmd)
        self._results[cmdType][filePath].setPackedSize(helpers.getFileSize(getPackedFilePath(cmdType)))
        unpackingPeakRamUsage = testPeakRamUsage(unpackCmd)
        self._results[cmdType][filePath].addRamUsage(packingPeakRamUsage, unpackingPeakRamUsage)
        self._verifySuccessfulUnpack(originalFilePath=filePath)
        self._cleanOutput(cmdType=cmdType)
        
        for round in range(amount):
            printRoundCounter(round, amount)
            packingTime = testExecutionTime(packCmd)
            unpackingTime = testExecutionTime(unpackCmd)
            self._results[cmdType][filePath].addTimes(packingTime, unpackingTime)
            self._cleanOutput(cmdType=cmdType)
        # printRoundCounter(amount, amount)

    def _cleanOutput(self, cmdType):
         removeFile(getPackedFilePath(cmdType))
         removeFile(helpers.unpackedArchivePath)

    def findFileInTree(self, fileName):
        if os.path.isfile(helpers.unpackedArchivePath):
            return helpers.unpackedArchivePath
        else:
            paths = getRecursiveFilePathsFromFolder(helpers.unpackedArchivePath)
            return findFileInPathList(fileName=fileName, filePaths=paths)

    def runCorrectnessTest(self, cmdType, filePath):
        if not self._cmd[cmdType]:
            return
        self._cleanOutput(cmdType=cmdType)
        packCmd = self.getPackCmd(cmdType=cmdType, pathToAdd=filePath)
        unpackCmd = self.getUnpackCmd(cmdType=cmdType)
        packingTime = testExecutionTime(packCmd)
        print(f'packingTime: {packingTime}')
        # input(">>> press enter to continue <<<")
        unpackingTime = testExecutionTime(unpackCmd)
        print(f'unpackingTime: {unpackingTime}')

        
        # print(f"\n\n==MILIKOWI== fileName: {fileName}")
        # for elem in resultPaths:
        #     print(elem)
        # print()
        self._verifySuccessfulUnpack(originalFilePath=filePath)
        # self._cleanOutput(cmdType=cmdType)

        # self._verifySuccessfulUnpack(originalFilePath=filePath, )
        
    

    def _verifySuccessfulUnpack(self, originalFilePath):
        fileName = os.path.basename(originalFilePath)
        unpackedFilePath=self.findFileInTree(fileName)
        import hashlib
        
        beforeHash = hashlib.sha256()
        afterHash = hashlib.sha256()
        BUF_SIZE = 65536
        with open(originalFilePath, 'rb') as f:
            while True:
                data = f.read(BUF_SIZE)
                if not data:
                    break
                beforeHash.update(data)
        

        with open(unpackedFilePath, 'rb') as f:
            while True:
                data = f.read(BUF_SIZE)
                if not data:
                    break
                afterHash.update(data)
        beforeDigest = beforeHash.hexdigest()
        afterDigest = afterHash.hexdigest()
        # ------------------------------------------
        # with open(originalFilePath, 'rb') as f:
        #     beforeDigest = hashlib.file_digest(f, "sha256")
        # with open(unpackedFilePath, 'rb') as f:
        #     afterDigest = hashlib.file_digest(f, "sha256")
        if originalFilePath == unpackedFilePath:
            raise RuntimeError(f"ERROR: originalFilePath == unpackedFilePath!: \"{originalFilePath}\"")
        # print(f'originalFilePath:{originalFilePath}, unpackedFilePath:{unpackedFilePath}')
        # print(f'beforeDigest:\t{beforeDigest}\nafterDigest:\t{afterDigest}')
        # print()
        
        if beforeDigest != afterDigest:
            print(f"!!!HASH DOES NOT ADD UP!\nbefore:\t{beforeDigest}\nafter:\t{afterDigest}")
        print(f"verifySuccessfulUnpack successful for {fileName}")

