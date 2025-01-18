from Results import Results
import subprocess
import tempfile
import os
from pathlib import Path

import consts


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
    if cmdType == consts.CmdType.DEFAULT:
        return consts.packedFilePathDefault
    elif cmdType == consts.CmdType.MAX:
        return consts.packedFilePathMax
    else:
        raise ValueError(f"cmdType not recognized, value: {cmdType}")


def testExecutionTime(command):
    with tempfile.TemporaryFile() as tempf: 
        timedCommand = f't1=$(date +%s%6N) ; {command} ; t2=$(date +%s%6N) ; echo ; echo $((t2-t1))'
        # timedCommand = f't1=$(date +%s%6N) ; {command} ; t2=$(date +%s%6N) ; echo $((t2-t1))'
        # print("TIMED COMMAND:")
        # print(timedCommand)
        subprocess.run(timedCommand, shell=True, executable="/bin/bash", stderr=tempf, stdout=tempf)
        tempf.seek(0)
        output = str(tempf.read())
        print()
        print(output)
        print()
        return int(output.split('\\n')[-2])

def getFileSize(filepath):
    return os.path.getsize(filepath)

class TestRunner:
    def __init__(self, name, default, max, unpack):
        self.name = name
        self._cmd = {consts.CmdType.DEFAULT: default, consts.CmdType.MAX: max} 
        self._unpack = unpack
        self._results = {consts.CmdType.DEFAULT: {}, consts.CmdType.MAX: {}}

    def getPackCmd(self, cmdType, pathToAdd, archivePath=None):
        if archivePath is None:
            archivePath = getPackedFilePath(cmdType)
        return self._cmd[cmdType](pathToAdd=pathToAdd, archivePath=archivePath)
    

    def getUnpackCmd(self, cmdType, unpackPath=consts.unpackedArchivePath, archivePath=None):
        if archivePath is None:
            archivePath = getPackedFilePath(cmdType)
        return self._unpack(pathOutput=unpackPath, archivePath=archivePath)


    def getTime(self, cmdType, cmdMode, filePath):
        if self._results[cmdType][filePath]:
            return self._results[cmdType][filePath].getTime(cmdMode)
        raise ValueError(f"getTime: something's missing, values: cmdType: {cmdType}, cmdMode: {cmdMode}, filePath: {filePath}")
    
    def getEffect(self, cmdType, cmdMode, filePath):
        if self._results[cmdType][filePath]:
            if cmdMode == consts.CmdMode.UNPACK:
                return self._results[cmdType][filePath].getBaseSize()
            return self._results[cmdType][filePath].getPackedSize()
        raise ValueError(f"getEffect: something's missing, values: cmdType: {cmdType}, cmdMode: {cmdMode}, filePath: {filePath}")


    def runTest(self, cmdType, filePath, amount):
        if self._cmd[cmdType]:
            self._runCmd(cmdType=cmdType, filePath=filePath, amount=amount)
        else:
            print(f"self._cmd[consts.CmdType.{cmdType}] not available!")    


    def _runCmd(self, cmdType, filePath, amount):
        # print(f"test: self._results[cmdType] = {self._results[cmdType]}")
        if filePath not in self._results[cmdType].keys():
            self._results[cmdType][filePath] = Results()
            
            # print(f"test: self._results[cmdType][filePath] = {self._results[cmdType][filePath]}")
        else:
            print("apparently filePath not in self._results[cmdType].keys()")
        

        packCmd = self.getPackCmd(cmdType=cmdType, pathToAdd=filePath)
        unpackCmd = self.getUnpackCmd(cmdType=cmdType)
        self._results[cmdType][filePath].setBaseSize(getFileSize(filePath))

        self._cleanOutput(cmdType=cmdType)
        packingTime = self._runTimed(packCmd)
        self._results[cmdType][filePath].setPackedSize(getFileSize(getPackedFilePath(cmdType)))
        unpackingTime = self._runTimed(unpackCmd)
        self._results[cmdType][filePath].addResults(packingTime, unpackingTime)
        self._cleanOutput(cmdType=cmdType)
        
        for _ in range(amount-1):
            packingTime = self._runTimed(packCmd)
            unpackingTime = self._runTimed(unpackCmd)
            self._results[cmdType][filePath].addResults(packingTime, unpackingTime)
            self._cleanOutput(cmdType=cmdType)


    def _runTimed(self, command):
        print(f"========== starting command: {command} ==========")
        result = testExecutionTime(command=command)
        # print(f"========== finished command: {command} ==========")
        return result


    def _cleanOutput(self, cmdType):
         removeFile(getPackedFilePath(cmdType))
         removeFile(consts.unpackedArchivePath)

    def runCorrectnessTest(self, cmdType, filePath):
        if not self._cmd[cmdType]:
            return
        self._cleanOutput(cmdType=cmdType)
        packCmd = self.getPackCmd(cmdType=cmdType, pathToAdd=filePath)
        unpackCmd = self.getUnpackCmd(cmdType=cmdType)
        packingTime = self._runTimed(packCmd)
        print(f'packingTime: {packingTime}')
        # input(">>> press enter to continue <<<")
        unpackingTime = self._runTimed(unpackCmd)
        print(f'unpackingTime: {unpackingTime}')

        fileName = os.path.basename(filePath)
        unpackedFilePath = ""
        if os.path.isfile(consts.unpackedArchivePath):
            unpackedFilePath = consts.unpackedArchivePath
        else:
            paths = getRecursiveFilePathsFromFolder(consts.unpackedArchivePath)
            
            unpackedFilePath = findFileInPathList(fileName=fileName, filePaths=paths)
        # print(f"\n\n==MILIKOWI== fileName: {fileName}")
        # for elem in resultPaths:
        #     print(elem)
        # print()
        self._verifySuccessfulUnpack(beforePath=filePath, afterPath=unpackedFilePath)
        # self._cleanOutput(cmdType=cmdType)

        # self._verifySuccessfulUnpack(beforePath=filePath, )
        
    

    def _verifySuccessfulUnpack(self, beforePath, afterPath):
        import hashlib
        
        beforeHash = hashlib.sha256()
        afterHash = hashlib.sha256()
        BUF_SIZE = 65536
        with open(beforePath, 'rb') as f:
            while True:
                data = f.read(BUF_SIZE)
                if not data:
                    break
                beforeHash.update(data)
        

        with open(afterPath, 'rb') as f:
            while True:
                data = f.read(BUF_SIZE)
                if not data:
                    break
                afterHash.update(data)
        beforeDigest = beforeHash.hexdigest()
        afterDigest = afterHash.hexdigest()
        # ------------------------------------------
        # with open(beforePath, 'rb') as f:
        #     beforeDigest = hashlib.file_digest(f, "sha256")
        # with open(afterPath, 'rb') as f:
        #     afterDigest = hashlib.file_digest(f, "sha256")
        if beforePath == afterPath:
            raise RuntimeError(f"ERROR: beforePath == afterPath!: \"{beforePath}\"")
        print(f'beforePath:{beforePath}, afterPath:{afterPath}')
        print(f'beforeDigest:\t{beforeDigest}\nafterDigest:\t{afterDigest}')
        print()
        
        if beforeDigest != afterDigest:
            print(f"!!!HASH DOES NOT ADD UP!\nbefore:\t{beforeDigest}\nafter:\t{afterDigest}")


