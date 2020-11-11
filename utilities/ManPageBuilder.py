
import sys
from argfo import *

progName = sys.argv[1]
progOneFo = sys.argv[2]
progDescFN = sys.argv[3]

# load in the description file (to include as is)
progDescF = open(progDescFN)
progDesc = progDescF.read()
progDescF.close()

# get the arguments
allArgs = []
for line in sys.stdin:
    if line.strip() == "":
        continue
    lineS = line.split("\t")
    lineS = [cv.strip() for cv in lineS]
    allArgs.append(ArgInfo(lineS))

def sanitizeString(toSan):
    toRet = toSan
    toRet = toSan.replace("-", "\\-")
    return toRet

print(".TH " + progName.upper() + " 1")
print(".SH NAME")
print(progName + " \- " + progOneFo)
print(".SH SYNOPSIS")
print(".B " + progName)
for curArg in allArgs:
    if curArg.flagName == "<|>":
        continue
    fullPrint = ["\\fB", sanitizeString(curArg.flagName)]
    if curArg.flagTp == "BOOL":
        pass
    elif curArg.flagTp == "ENUM":
        pass
    elif curArg.flagTp == "INT":
        fullPrint.append(" ")
        fullPrint.append("\\fI")
        fullPrint.append(repr(curArg.defValue))
    elif curArg.flagTp == "FLOAT":
        fullPrint.append(" ")
        fullPrint.append("\\fI")
        fullPrint.append(repr(curArg.defValue))
    elif curArg.flagTp == "STRING":
        fullPrint.append(" ")
        fullPrint.append("\\fI")
        fullPrint.append(curArg.defValue)
    elif curArg.flagTp == "INTVEC":
        fullPrint.append(" ")
        fullPrint.append("\\fI")
        fullPrint.append("num")
    elif curArg.flagTp == "FLOATVEC":
        fullPrint.append(" ")
        fullPrint.append("\\fI")
        fullPrint.append("num")
    elif curArg.flagTp == "STRINGVEC":
        fullPrint.append(" ")
        fullPrint.append("\\fI")
        fullPrint.append("arg")
    fullPrint.append("\\fR")
    print("".join(fullPrint))
print(".SH DESCRIPTION")
print(".B " + progName)
print(progDesc)
print(".SH OPTIONS")
namelessMap = {"INT":"num","FLOAT":"num","STRING":"text","INTVEC":"num*","FLOATVEC":"num*","STRINGVEC":"text*"}
for curArg in allArgs:
    print(".TP")
    if curArg.flagName == "<|>":
        print(".BR " + namelessMap[curArg.flagTp])
    else:
        fullPrint = [".BR ", sanitizeString(curArg.flagName)]
        if curArg.flagTp in ["INT","FLOAT"]:
            fullPrint.append(" \\ \\fI" + repr(curArg.defValue) + "\\fR")
        elif curArg.flagTp == "STRING":
            fullPrint.append(" \\ \\fI" + curArg.defValue + "\\fR")
        elif curArg.flagTp in ["INTVEC","FLOATVEC"]:
            fullPrint.append(" \\ \\fInum\\fR")
        elif curArg.flagTp == "STRINGVEC":
            fullPrint.append(" \\ \\fItext\\fR")
        print("".join(fullPrint))
    # print the first line
    if (curArg.varDoc is None):
        print("A spooky option.")
    else:
        print(sanitizeString(curArg.varDoc.split("\n")[0].strip()))


