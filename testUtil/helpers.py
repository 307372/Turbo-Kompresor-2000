from enum import Enum
import os

class CmdType(Enum):
    DEFAULT=11111
    MAX=22222

def getCmdTypeStr(cmdType):
    return "DEFAULT" if cmdType == CmdType.DEFAULT else "MAX"



class CmdMode(Enum):
    PACK=33333
    UNPACK=44444


def getFileSize(filepath):
    return os.path.getsize(filepath)


tk2kPath = '/home/pc/repos/Turbo-Kompresor-2000/builds/build-Turbo-Kompresor-2000-Desktop-Release/Turbo-Kompresor-2000'
# tk2kPath = '/home/pc/repos/Turbo-Kompresor-2000/builds/build-Turbo-Kompresor-2000-Desktop-Debug/Turbo-Kompresor-2000'
testFolderPath = '/home/pc/Desktop/testFiles/'
packedFilePathDefault = testFolderPath + 'packedDefault.res'
packedFilePathMax = testFolderPath + 'packedMax.res'
unpackedArchivePath = testFolderPath + 'unpacked'

malePath = testFolderPath + 'male'
sredniePath = testFolderPath + 'srednie'
duzePath = testFolderPath + 'duze'
bardzoDuzePath = testFolderPath + 'bardzoDuze'
