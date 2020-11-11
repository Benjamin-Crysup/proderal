import sys

refFa = None
defCostFo = [1, -6, -5, -5]
curCostFo = [1, -6, -4, -0]
allBedCs = []
i = 1
while i < len(sys.argv):
    carg = sys.argv[i]
    if (carg == "--help") or (carg == "-h") or (carg == "/?"):
        print("python GenPDAlnCosts.py --ref REF.fa (COSTS --regs REG.bed)* (COSTS --def)?")
        print("Generates position dependent alignment parameter files.")
        print("COSTS specify the cost information in for the next set of regions.")
        print("  --ref REF.fa")
        print("      Specifies the reference to generate parameters for.")
        print("  --cm MATCH")
        print('      Specify the "cost" of a match: defaults to 1.')
        print("  --cx MISMATCH")
        print('      Specify the cost of a mismatch: defaults to -6.')
        print("  --cgo GAPOPEN")
        print('      Specify the cost of opening and closing a gap: defaults to -4.')
        print("  --cgx GAPEXTEND")
        print('      Specify the cost of extending a gap: defaults to 0.')
        print("  --regs REG.bed")
        print("      Specify a set of regions for the current costs to apply to..")
        print("      Can have multiple --regs, each with different costs.")
        print("  --def")
        print("      Set the default cost to the current cost.")
        sys.exit()
    if carg == "--version":
        print("GenPDAlnCosts 0.0")
        print("Copyright (C) 2020 UNT HSC Center for Human Identification")
        print("License LGPL")
        print("This is free software: you are free to change and redistribute it.")
        print("There is NO WARRANTY, to the extent permitted by law")
        sys.exit()
    if carg == "--def":
        defCostFo = curCostFo[:]
        i = i + 1
        continue
    i = i + 1
    if i >= len(sys.argv):
        raise ValueError("Missing data for " + carg)
    cdat = sys.argv[i]
    i = i + 1
    if carg == "--ref":
        refFa = cdat
    elif carg == "--cm":
        curCostFo[0] = int(cdat)
    elif carg == "--cx":
        curCostFo[1] = int(cdat)
    elif carg == "--cgo":
        curCostFo[2] = int(cdat)
    elif carg == "--cgx":
        curCostFo[3] = int(cdat)
    elif carg == "--regs":
        allBedCs.append([cdat, curCostFo[:]])
    else:
        raise ValueError("Unknown argument " + carg)
if refFa is None:
    raise ValueError("Missing reference fasta, --ref.")

# load in the reference fasta
refLens = {}
refFAF = open(refFa, "r")
curRName = None
curRef = []
for line in refFAF:
    if line.strip() == "":
        continue
    if line[0] == ">":
        if not (curRName is None):
            refLens[curRName] = len("".join(curRef))
        curRName = line[1:].split()[0]
        curRef = []
    else:
        curRef.append(line.strip())
if not (curRName is None):
    refLens[curRName] = len("".join(curRef))
refFAF.close()

# load in the bed files
bedLocs = []
for binf in allBedCs:
    bnam = binf[0]
    curBLoc = []
    bedF = open(bnam, "r")
    for line in bedF:
        if (len(line) > 0) and (line[0] == '#'):
            continue
        lineS = line.split("\t")
        if len(lineS) < 3:
            continue
        curBLoc.append([lineS[0], int(lineS[1]), int(lineS[2])])
    bedF.close()
    bedLocs.append(curBLoc)

def costToString(costArr):
    toArr = []
    toArr.append(repr(costArr[2]))
    toArr.append(repr(costArr[3]))
    toArr.append(repr(costArr[2]))
    toArr.append("4 65 67 71 84")
    for i in range(4):
        for j in range(4):
            toArr.append(repr(costArr[0]) if (i==j) else repr(costArr[1]))
    return " ".join(toArr)

# run through the references
for refNam in refLens:
    print(refNam)
    print(costToString(defCostFo))
    allBS = []
    allBE = []
    allBC = []
    for bedi in range(len(allBedCs)):
        curCos = allBedCs[bedi][1]
        for bedEnt in bedLocs[bedi]:
            if bedEnt[0] == refNam:
                allBS.append(bedEnt[1])
                allBE.append(bedEnt[2])
                allBC.append(curCos)
    print(len(allBS))
    for i in range(len(allBS)):
        toPri = []
        toPri.append(repr(allBS[i]))
        toPri.append(repr(allBE[i]))
        toPri.append("-1 -1")
        toPri.append(costToString(allBC[i]))
        print(" ".join(toPri))
    print("|")

