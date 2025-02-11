# import statistics
import helpers

class Results:
    def __init__(self):
        self._timePack = 999999999999999
        self._timeUnpack = 999999999999999
        self._sizeBase = -1
        self._sizePacked = -1
        self._ramUsagePack = 999999999999999
        self._ramUsageUnpack = 999999999999999


    def addTimes(self, packResult, unpackResult):
        if self._timePack > packResult:
            self._timePack = packResult
        
        if self._timeUnpack > unpackResult:
            self._timeUnpack = unpackResult

    
    def addRamUsage(self, packResult, unpackResult):
        if self._ramUsagePack > packResult:
            self._ramUsagePack = packResult
        
        if self._ramUsageUnpack > unpackResult:
            self._ramUsageUnpack = unpackResult


    def setBaseSize(self, size):
        self._sizeBase = size

    def setPackedSize(self, size):
        self._sizePacked = size

    def getPackedSize(self):
        return self._sizePacked

    def getBaseSize(self):
        return self._sizeBase

    def getTime(self, cmdMode):
        return self._getTimeMin(cmdMode)


    def getRamUsage(self, cmdMode):
        if cmdMode == helpers.CmdMode.PACK:
            return self._ramUsagePack
        else:
            return self._ramUsageUnpack

    def _getTimeMin(self, cmdMode):
        if cmdMode == helpers.CmdMode.PACK:
            return self._timePack
        else:
            return self._timeUnpack


