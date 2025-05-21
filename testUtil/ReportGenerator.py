import os
import helpers
from helpers import joinStringList
from ReportGeneratorRaw import *
from ReportGeneratorProcessed import *



class ReportGenerator(ReportGeneratorProcessed, ReportGeneratorRaw):
    def __init__(self):
        super().__init__()
        self.addIdentificationColumnsGenerators()
        self.addColumnNameGenerators()

        # reportGenerator.addColumn(  # (TK2K only)
        #     name="rozmiar_bloku",
        #     valueGetter = lambda run: run.tk2kBlockSize if run.tk2kBlockSize is not None else "")

        # reportGenerator.addColumn(  # (TK2K only)
        #     name="algorytm",
        #     valueGetter = lambda run: run.tk2kAlgorithm if run.tk2kAlgorithm is not None else "")


    def generateAmountOfTestRunsStats(self, amount):
        return f"Amount of test runs of each algorithm:\t{amount}"


    def generateOriginalFileSizeTable(self, filePaths):
        lines = ["File\tSize [B]"]
        for path in filePaths:
            lines += [f"{os.path.basename(path)}\t{helpers.getFileSize(path)}"]
        return joinStringList(lines, sep='\n')


    def generateFullReport(self, runners, filePaths, amountOfTestRuns):
        tables = [
            self.generateAmountOfTestRunsStats(amount=amountOfTestRuns),
            self.generateOriginalFileSizeTable(filePaths=filePaths),
            self.generateRawReport(runners=runners, filePaths=filePaths),

            self.generateCompressionEffectivenessReport(runners, filePaths),
            self.generateProcessingSpeedReport(runners, filePaths),
            self.generateRamEfficiencyReport(runners, filePaths)
            ]
        
        return joinStringList(tables, sep="\n\n\n")


