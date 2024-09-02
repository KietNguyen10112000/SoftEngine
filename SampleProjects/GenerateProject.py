# python GenerateProject.py <ProjectName> <DirecttoryToGenerate>

import shutil
import sys
import os

if len(sys.argv) != 3:
    print('Usage: python GenerateProject.py <ProjectName> <DirecttoryToGenerate>')
    exit(-1)

# Print iterations progress
def printProgressBar (iteration, total, prefix = '', suffix = '', decimals = 1, length = 100, fill = '█', printEnd = "\r"):
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print(f'\r{prefix} |{bar}| {percent}% {suffix}', end = printEnd)
    # Print New Line on Complete
    if iteration == total: 
        print()


templateName = 'TemplateProject'
srcTemplateProject = './TemplateProject'

projectName = sys.argv[1]
projectDir = sys.argv[2]


totalFiles = sum([len(files) for r, d, files in os.walk(srcTemplateProject)])
totalFiles = totalFiles * 2
processedFiles = 0

for root, subdirs, files in os.walk(srcTemplateProject):
    destRoot = os.path.join(projectDir, root.replace(templateName, projectName))
    for srcFile in files:
        destFile = srcFile.replace(templateName, projectName)
        destPath = destRoot + '/' + destFile
        os.makedirs(os.path.dirname(destPath), exist_ok=True)
        shutil.copyfile(root + '/' + srcFile, destPath)

        processedFiles += 1
        printProgressBar(processedFiles, totalFiles)


projectDir = os.path.join(projectDir, projectName)
for root, subdirs, files in os.walk(projectDir):
    for fileName in files:
        data = None
        filePath = root + '/' + fileName
        with open(filePath, 'r', encoding='UTF-8') as file:
            data = file.read().replace(templateName, projectName)
            file.close()

        with open(filePath, 'w', encoding='UTF-8') as file:
            file.write(data)
            file.close()

        processedFiles += 1
        printProgressBar(processedFiles, totalFiles)

print("Generated project to: \'" + projectDir + "\'")