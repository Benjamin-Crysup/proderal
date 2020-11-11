
import sys
from argfo import *

# get the argument
allArgs = []
for line in sys.stdin:
    if line.strip() == "":
        continue
    lineS = line.split("\t")
    lineS = [cv.strip() for cv in lineS]
    allArgs.append(ArgInfo(lineS))

# start outputting the gui
print("import tkinter")
print("import tkinter.filedialog")
print("import tkinter.scrolledtext")
print("")
print("class ArgumentGUIFrame(tkinter.Frame):")
print("    def updateSingleFileName(self, forBox, newName):")
print("        if newName:")
print("            forBox.delete(0, tkinter.END)")
print("            forBox.insert(0, newName)")
print("    def updateMultiFileName(self, forBox, newName):")
print("        if newName:")
print("            forBox.insert(tkinter.END, '\\n' + newName)")
print("    def __init__(self,master=None):")
print("        self.selArgs = None")
print("        super().__init__(master)")
print("        self.master = master")
print("        canv = tkinter.Canvas(self)")
# make the gui components
handArgs = set()
gridR = 0
for i in range(len(allArgs)):
    if i in handArgs:
        continue
    curArg = allArgs[i]
    if curArg.hidden:
        continue
    argNam = "self.arg" + repr(i)
    if curArg.flagTp == "BOOL":
        print("        " + argNam + "V = tkinter.IntVar()")
        print("        " + argNam + " = tkinter.Checkbutton(canv, text='" + (curArg.presName if (not curArg.presName is None) else curArg.flagName) + "', variable=" + argNam + "V)")
        print("        " + argNam + ".grid(column = 0, row = " + repr(gridR) + ")")
        gridR = gridR + 1;
    elif curArg.flagTp == "ENUM":
        #get all the args in the group (note the default, if any: if none, default to the first)
        enumGrp = {}
        enumGrp[i] = curArg
        winEnum = (i if curArg.defValue else -1)
        if not (curArg.enumGroup is None):
            print("        " + argNam + "GL = tkinter.Label(canv, text='" + curArg.enumGroup + "')")
            print("        " + argNam + "GL.grid(column = 0, row = " + repr(gridR) + ")")
            gridR = gridR + 1
            for j in range(i+1, len(allArgs)):
                if j in handArgs:
                    continue
                testArg = allArgs[j]
                if testArg.enumGroup is None:
                    continue
                if testArg.enumGroup != curArg.enumGroup:
                    continue
                if testArg.hidden:
                    continue
                handArgs.add(j)
                enumGrp[j] = testArg
                if testArg.defValue:
                    winEnum = j
        if winEnum < 0:
            winEnum = i
        # the reporting variable
        print("        " + argNam + "V = tkinter.StringVar()")
        print("        " + argNam + "V.set('" + allArgs[winEnum].flagName + "')")
        # add radio buttons
        gridC = 0
        for j in enumGrp:
            enumArg = enumGrp[j]
            enumNam = "self.arg" + repr(j)
            print("        " + enumNam + " = tkinter.Radiobutton(canv, text='" + (enumArg.presName if (not enumArg.presName is None) else enumArg.flagName) + "', value='" + enumArg.flagName + "', variable = " + argNam + "V)")
            print("        " + enumNam + ".grid(column = " + repr(gridC) + ", row = " + repr(gridR) + ")")
            gridC = gridC + 1
        gridR = gridR + 1
    elif curArg.flagTp == "INT":
        print("        " + argNam + "L = tkinter.Label(canv, text='" + (curArg.presName if (not curArg.presName is None) else curArg.flagName) + "')")
        print("        " + argNam + "L.grid(column = 0, row = " + repr(gridR) + ")")
        print("        " + argNam + " = tkinter.Entry(canv)")
        print("        " + argNam + ".grid(column = 1, row = " + repr(gridR) + ")")
        print("        " + argNam + ".delete(0, tkinter.END)")
        print("        " + argNam + ".insert(0, '" + repr(curArg.defValue) + "')")
        gridR = gridR + 1
    elif curArg.flagTp == "FLOAT":
        print("        " + argNam + "L = tkinter.Label(canv, text='" + (curArg.presName if (not curArg.presName is None) else curArg.flagName) + "')")
        print("        " + argNam + "L.grid(column = 0, row = " + repr(gridR) + ")")
        print("        " + argNam + " = tkinter.Entry(canv)")
        print("        " + argNam + ".grid(column = 1, row = " + repr(gridR) + ")")
        print("        " + argNam + ".delete(0, tkinter.END)")
        print("        " + argNam + ".insert(0, '" + repr(curArg.defValue) + "')")
        gridR = gridR + 1
    elif curArg.flagTp == "STRING":
        print("        " + argNam + "L = tkinter.Label(canv, text='" + (curArg.presName if (not curArg.presName is None) else curArg.flagName) + "')")
        print("        " + argNam + "L.grid(column = 0, row = " + repr(gridR) + ")")
        print("        " + argNam + " = tkinter.Entry(canv)")
        print("        " + argNam + ".grid(column = 1, row = " + repr(gridR) + ")")
        print("        " + argNam + ".delete(0, tkinter.END)")
        print("        " + argNam + ".insert(0, '" + curArg.defValue + "')")
        if curArg.strIsFile:
            if curArg.strFileIsWrite:
                if len(curArg.strFileExts) > 0:
                    print("        " + argNam + "B = tkinter.Button(canv, text='Browse', command = lambda: self.updateSingleFileName(" + argNam + ", tkinter.filedialog.asksaveasfilename(title = '" + (curArg.presName if (not curArg.presName is None) else curArg.flagName) + "', filetypes = " + repr(tuple([(cv, "*" + cv) for cv in curArg.strFileExts])) + ")))")
                else:
                    print("        " + argNam + "B = tkinter.Button(canv, text='Browse', command = lambda: self.updateSingleFileName(" + argNam + ", tkinter.filedialog.asksaveasfilename(title = '" + (curArg.presName if (not curArg.presName is None) else curArg.flagName) + "')))")
            else:
                if len(curArg.strFileExts) > 0:
                    print("        " + argNam + "B = tkinter.Button(canv, text='Browse', command = lambda: self.updateSingleFileName(" + argNam + ", tkinter.filedialog.askopenfilename(title = '" + (curArg.presName if (not curArg.presName is None) else curArg.flagName) + "', filetypes = " + repr(tuple([(cv, "*" + cv) for cv in curArg.strFileExts])) + ")))")
                else:
                    print("        " + argNam + "B = tkinter.Button(canv, text='Browse', command = lambda: self.updateSingleFileName(" + argNam + ", tkinter.filedialog.askopenfilename(title = '" + (curArg.presName if (not curArg.presName is None) else curArg.flagName) + "')))")
            print("        " + argNam + "B.grid(column = 2, row = " + repr(gridR) + ")")
        elif curArg.strIsFold:
            print("        " + argNam + "B = tkinter.Button(canv, text='Browse', command = lambda: self.updateSingleFileName(" + argNam + ", tkinter.filedialog.askdirectory(title = '" + (curArg.presName if (not curArg.presName is None) else curArg.flagName) + "', mustexist=" + repr(curArg.strFoldIsWrite) + ")))")
            print("        " + argNam + "B.grid(column = 2, row = " + repr(gridR) + ")")
        gridR = gridR + 1
    elif curArg.flagTp == "INTVEC":
        print("        " + argNam + "L = tkinter.Label(canv, text='" + (curArg.presName if (not curArg.presName is None) else curArg.flagName) + "')")
        print("        " + argNam + "L.grid(column = 0, row = " + repr(gridR) + ")")
        gridR = gridR + 1
        print("        " + argNam + " = tkinter.scrolledtext.ScrolledText(canv, height = 4, width = 40)")
        print("        " + argNam + ".grid(column = 0, row = " + repr(gridR) + ", columnspan = 2, rowspan = 4)")
        gridR = gridR + 4
    elif curArg.flagTp == "FLOATVEC":
        print("        " + argNam + "L = tkinter.Label(canv, text='" + (curArg.presName if (not curArg.presName is None) else curArg.flagName) + "')")
        print("        " + argNam + "L.grid(column = 0, row = " + repr(gridR) + ")")
        gridR = gridR + 1
        print("        " + argNam + " = tkinter.scrolledtext.ScrolledText(canv, height = 4, width = 40)")
        print("        " + argNam + ".grid(column = 0, row = " + repr(gridR) + ", columnspan = 2, rowspan = 4)")
        gridR = gridR + 4
    elif curArg.flagTp == "STRINGVEC":
        print("        " + argNam + "L = tkinter.Label(canv, text='" + (curArg.presName if (not curArg.presName is None) else curArg.flagName) + "')")
        print("        " + argNam + "L.grid(column = 0, row = " + repr(gridR) + ")")
        gridR = gridR + 1
        print("        " + argNam + " = tkinter.scrolledtext.ScrolledText(canv, height = 4, width = 40)")
        print("        " + argNam + ".grid(column = 0, row = " + repr(gridR) + ", columnspan = 2, rowspan = 4)")
        gridR = gridR + 4
        if curArg.strIsFile:
            if curArg.strFileIsWrite:
                if len(curArg.strFileExts) > 0:
                    print("        " + argNam + "B = tkinter.Button(canv, text='Browse', command = lambda: self.updateMultiFileName(" + argNam + ", tkinter.filedialog.asksaveasfilename(title = '" + (curArg.presName if (not curArg.presName is None) else curArg.flagName) + "', filetypes = " + repr(tuple([(cv, "*" + cv) for cv in curArg.strFileExts])) + ")))")
                else:
                    print("        " + argNam + "B = tkinter.Button(canv, text='Browse', command = lambda: self.updateMultiFileName(" + argNam + ", tkinter.filedialog.asksaveasfilename(title = '" + (curArg.presName if (not curArg.presName is None) else curArg.flagName) + "')))")
            else:
                if len(curArg.strFileExts) > 0:
                    print("        " + argNam + "B = tkinter.Button(canv, text='Browse', command = lambda: self.updateMultiFileName(" + argNam + ", tkinter.filedialog.askopenfilename(title = '" + (curArg.presName if (not curArg.presName is None) else curArg.flagName) + "', filetypes = " + repr(tuple([(cv, "*" + cv) for cv in curArg.strFileExts])) + ")))")
                else:
                    print("        " + argNam + "B = tkinter.Button(canv, text='Browse', command = lambda: self.updateMultiFileName(" + argNam + ", tkinter.filedialog.askopenfilename(title = '" + (curArg.presName if (not curArg.presName is None) else curArg.flagName) + "')))")
            print("        " + argNam + "B.grid(column = 1, row = " + repr(gridR) + ")")
            gridR = gridR + 1
        elif curArg.strIsFold:
            print("        " + argNam + "B = tkinter.Button(canv, text='Browse', command = lambda: self.updateMultiFileName(" + argNam + ", tkinter.filedialog.askdirectory(title = '" + (curArg.presName if (not curArg.presName is None) else curArg.flagName) + "', mustexist=" + repr(curArg.strFoldIsWrite) + ")))")
            print("        " + argNam + "B.grid(column = 2, row = " + repr(gridR) + ")")
            gridR = gridR + 1
gridR = gridR + 1
# make the ok and cancel buttons
print("        self.canBtn = tkinter.Button(canv, text = 'Cancel', command = lambda: self.goCancel())")
print("        self.canBtn.grid(column = 0, row = " + repr(gridR) + ")")
print("        self.goBtn = tkinter.Button(canv, text = 'OK', command = lambda: self.goGo())")
print("        self.goBtn.grid(column = 1, row = " + repr(gridR) + ")")
# add a scroll bar
print("        canv.pack(side=tkinter.LEFT)")
print("        mainscr = tkinter.Scrollbar(self, command=canv.yview)")
print("        mainscr.pack(side=tkinter.LEFT, fill='y')")
print("        canv.configure(yscrollcommand = mainscr.set)")
print("        self.pack()")
print("    def getArgs(self):")
print("        return self.selArgs")
print("    def goCancel(self):")
print("        self.master.destroy()")
print("    def goGo(self):")
print("        retArgs = []")
for i in range(len(allArgs)):
    if i in handArgs:
        continue
    curArg = allArgs[i]
    if curArg.hidden:
        continue
    argNam = "self.arg" + repr(i)
    if curArg.flagTp == "BOOL":
        print("        if " + argNam + "V.get() != 0:")
        print("            retArgs.append('" + curArg.flagName + "')")
    elif curArg.flagTp == "ENUM":
        print("        retArgs.append(" + argNam + "V.get())")
    elif curArg.flagTp == "INT":
        if curArg.flagName != "<|>":
            print("        retArgs.append('" + curArg.flagName + "')")
        print("        retArgs.append(" + argNam + ".get())")
    elif curArg.flagTp == "FLOAT":
        if curArg.flagName != "<|>":
            print("        retArgs.append('" + curArg.flagName + "')")
        print("        retArgs.append(" + argNam + ".get())")
    elif curArg.flagTp == "STRING":
        if curArg.flagName != "<|>":
            print("        retArgs.append('" + curArg.flagName + "')")
        print("        retArgs.append(" + argNam + ".get())")
    elif curArg.flagTp == "INTVEC":
        print("        curRetOpts = " + argNam + ".get('1.0', tkinter.END).split('\\n')")
        print("        for cv in curRetOpts:")
        print("            if len(cv.strip()) == 0:")
        print("                continue")
        if curArg.flagName != "<|>":
            print("            retArgs.append('" + curArg.flagName + "')")
        print("            retArgs.append(cv.strip())")
    elif curArg.flagTp == "FLOATVEC":
        print("        curRetOpts = " + argNam + ".get('1.0', tkinter.END).split('\\n')")
        print("        for cv in curRetOpts:")
        print("            if len(cv.strip()) == 0:")
        print("                continue")
        if curArg.flagName != "<|>":
            print("            retArgs.append('" + curArg.flagName + "')")
        print("            retArgs.append(cv.strip())")
    elif curArg.flagTp == "STRINGVEC":
        print("        curRetOpts = " + argNam + ".get('1.0', tkinter.END).split('\\n')")
        print("        for cv in curRetOpts:")
        print("            if len(cv.strip()) == 0:")
        print("                continue")
        if curArg.flagName != "<|>":
            print("            retArgs.append('" + curArg.flagName + "')")
        print("            retArgs.append(cv.strip())")
print("        self.selArgs = retArgs")
print("        self.master.destroy()")
print("")
print("")
