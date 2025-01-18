from enum import Enum

class CmdType(Enum):
    DEFAULT=11111
    MAX=22222

class CmdMode(Enum):
    PACK=33333
    UNPACK=44444



tk2kPath = '~/repos/Turbo-Kompresor-2000/builds/build-Turbo-Kompresor-2000-Desktop-Release/Turbo-Kompresor-2000'
# tk2kPath = '~/repos/Turbo-Kompresor-2000/builds/build-Turbo-Kompresor-2000-Desktop-Debug/Turbo-Kompresor-2000'
testFolderPath = '../testFiles/'
packedFilePathDefault = testFolderPath + 'packedDefault.res'
packedFilePathMax = testFolderPath + 'packedMax.res'
unpackedArchivePath = testFolderPath + 'unpacked'

malePath = testFolderPath + 'male'
sredniePath = testFolderPath + 'srednie'
duzePath = testFolderPath + 'duze'
bardzoDuzePath = testFolderPath + 'bardzoDuze'
