import os
import subprocess
import tkinter
from tkinter import ttk
from tkinter import filedialog
from tkinter import messagebox

class Proderal(tkinter.Frame):
    def __init__(self, master=None):
        super().__init__(master)
        self.saveWids = {}
        self.master = master
        self.create_widgets()
        self.pack()
    def create_widgets(self):
        # input sam
        self.setUpFileOption(0, "Input SAM*", "Open SAM", (("SAM Files","*.sam"),("All Files","*.*")), "is")
        # reference
        self.setUpFileOption(1, "Reference*", "Open Fasta", (("Fasta Files","*.fa"),("All Files","*.*")), "ref")
        # problem or promiscuous
        self.isPromiV = tkinter.IntVar()
        self.isPromiB = tkinter.Checkbutton(self, text="Realign Everything", variable=self.isPromiV)
        self.isPromiB.grid(column = 1, row = 2)
        self.setUpFileOption(3, "Tough Regions", "Open Bed", (("BED Files","*.bed"),("All Files","*.*")), "prob")
        # cost
        self.setUpFileOption(4, "Cost Specification*", "Open PDC", (("PDCost Files","*.pdc"),("All Files","*.*")), "pdc")
        # overrun
        self.setUpNumericOption(5, "Overrun", "20", "over")
        # local, semi or global
        self.amodeV = tkinter.StringVar()
        self.amodeV.set("S")
        self.radSem = tkinter.Radiobutton(self, text="Semi-Global", value="S", variable = self.amodeV)
        self.radSem.grid(column = 0, row=6)
        self.radLoc = tkinter.Radiobutton(self, text="Local", value="L", variable = self.amodeV)
        self.radLoc.grid(column = 1, row=6)
        # rank
        self.setUpNumericOption(7, "Rank", "0", "rank")
        # count
        self.setUpNumericOption(8, "Count", "1", "count")
        # reclaim soft
        self.setUpNumericOption(9, "Soft Clip Reclaim", "0", "scr")
        # quality mangle
        self.setUpFileOption(10, "Quality Mangle", "Open Qualm", (("Qualm Files","*.qca"),("All Files","*.*")), "qualm")
        # hfuzz
        self.setUpNumericOption(11, "Repeat Score Skip", "0", "hfuzz")
        # output sam
        self.setUpFileOption(12, "Output SAM*", "Save SAM", (("SAM Files","*.sam"),("All Files","*.*")), "os")
        # the go button
        self.goBtn = ttk.Button(self, text="Go", command = lambda: self.makeGo())
        self.goBtn.grid(column=1, row=12)
    def setUpFileOption(self, onRow, labelTxt, diaTitle, diaTypes, savePref):
        uselab = ttk.Label(self, text=labelTxt)
        uselab.grid(column = 0, row = onRow)
        usenam = tkinter.Entry(self)
        usenam.grid(column = 1, row = onRow)
        usebtn = ttk.Button(self, text="Browse", command = lambda: self.browse_file(usenam, diaTitle, diaTypes))
        usebtn.grid(column = 2, row = onRow)
        self.saveWids[savePref + "lab"] = uselab
        self.saveWids[savePref + "fnm"] = usenam
        self.saveWids[savePref + "btn"] = usebtn
    def setUpOutFileOption(self, onRow, labelTxt, diaTitle, diaTypes, savePref):
        uselab = ttk.Label(self, text=labelTxt)
        uselab.grid(column = 0, row = onRow)
        usenam = tkinter.Entry(self)
        usenam.grid(column = 1, row = onRow)
        usebtn = ttk.Button(self, text="Browse", command = lambda: self.browse_out_file(usenam, diaTitle, diaTypes))
        usebtn.grid(column = 2, row = onRow)
        self.saveWids[savePref + "lab"] = uselab
        self.saveWids[savePref + "fnm"] = usenam
        self.saveWids[savePref + "btn"] = usebtn
    def setUpNumericOption(self, onRow, labelTxt, defVal, savePref):
        overLab = ttk.Label(self, text=labelTxt)
        overLab.grid(column = 0, row = onRow)
        overBox = tkinter.Entry(self)
        overBox.grid(column = 1, row = onRow)
        overBox.delete(0, tkinter.END)
        overBox.insert(0, defVal)
        self.saveWids[savePref + "lab"] = overLab
        self.saveWids[savePref + "box"] = overBox
    def browse_file(self, forBox, diaTitle, diaTypes):
        newName = filedialog.askopenfilename(title = diaTitle, filetype = diaTypes)
        if newName:
            forBox.delete(0, tkinter.END)
            forBox.insert(0, newName)
    def browse_out_file(self, forBox, diaTitle, diaTypes):
        newName = filedialog.asksaveasfilename(title = diaTitle, filetype = diaTypes)
        if newName:
            forBox.delete(0, tkinter.END)
            forBox.insert(0, newName)
    def makeGo(self):
        inName = self.saveWids["isfnm"].get()
        refName = self.saveWids["reffnm"].get()
        needRealn = self.isPromiV.get() != 0
        toughName = self.saveWids["probfnm"].get()
        costName = self.saveWids["pdcfnm"].get()
        overR = self.saveWids["overbox"].get()
        alnMode = self.amodeV.get()
        rankR = self.saveWids["rankbox"].get()
        countR = self.saveWids["countbox"].get()
        softR = self.saveWids["scrbox"].get()
        mangName = self.saveWids["qualmfnm"].get()
        hfuzzR = self.saveWids["hfuzz"].get()
        outName = self.saveWids["osfnm"].get()
        fullCom = [os.path.join(os.path.dirname(os.path.abspath(__file__)), "proderal")]
        if len(refName) == 0:
            messagebox.showwarning("Bad Input","Missing reference file.")
            return
        fullCom.append("--ref")
        fullCom.append(refName)
        if (len(toughName) == 0) and not needRealn:
            messagebox.showwarning("Bad Input","If not realigning everything, need to specify problematic regions.")
            return
        if needRealn:
            fullCom.append("--promiscuous")
        if len(toughName) != 0:
            fullCom.append("--prob")
            fullCom.append(toughName)
        if len(costName) == 0:
            messagebox.showwarning("Bad Input","Missing position dependent cost.")
            return
        fullCom.append("--cost")
        fullCom.append(costName)
        if alnMode == "S":
            fullCom.append("--atsemi")
        else:
            fullCom.append("--atlocal")
        try:
            overRN = int(overR)
        except ValueError:
            messagebox.showwarning("Bad Input","Malformed overrun specificiation.")
            return
        if overRN < 0:
            messagebox.showwarning("Bad Input","Overrun must be non-negative.")
            return
        fullCom.append("--overrun")
        fullCom.append(repr(overRN))
        try:
            rankRN = int(rankR)
        except ValueError:
            messagebox.showwarning("Bad Input","Malformed rank specificiation.")
            return
        if rankRN < 0:
            messagebox.showwarning("Bad Input","Rank must be non-negative.")
            return
        fullCom.append("--rank")
        fullCom.append(repr(rankRN))
        try:
            countRN = int(countR)
        except ValueError:
            messagebox.showwarning("Bad Input","Malformed count specificiation.")
            return
        if countRN < 0:
            messagebox.showwarning("Bad Input","Count must be non-negative.")
            return
        fullCom.append("--count")
        fullCom.append(repr(countRN))
        softRN = 0
        if len(softR) != 0:
            try:
                softRN = int(softR)
            except ValueError:
                messagebox.showwarning("Bad Input","Malformed soft clip reclaim specificiation.")
                return
            if softRN < 0:
                messagebox.showwarning("Bad Input","Soft clip reclaim must be non-negative.")
                return
        fullCom.append("--reclaimsoft")
        fullCom.append(repr(softRN))
        if len(mangName) != 0:
            fullCom.append("--qualm")
            fullCom.append(mangName)
        try:
            hfuzzRN = int(hfuzzR)
        except ValueError:
            messagebox.showwarning("Bad Input","Malformed duplicate score skip threshold.")
            return
        if hfuzzRN < 0:
            messagebox.showwarning("Bad Input","Duplicate score skip threshold must be non-negative.")
            return
        fullCom.append("--hfuzz")
        fullCom.append(repr(hfuzzRN))
        if len(inName) == 0:
            messagebox.showwarning("Bad Input","Missing input sam file.")
            return
        if len(outName) == 0:
            messagebox.showwarning("Bad Input","Missing output sam file.")
            return
        try:
            fromFil = open(inName, "r")
        except OSError as errO:
            messagebox.showerror("File Problem","Problem reading file: " + str(errO))
            return
        try:
            toFil = open(outName, "w")
        except OSError as errO:
            fromFil.close()
            messagebox.showerror("File Problem","Problem writing file: " + str(errO))
            return
        try:
            runRes = subprocess.run(fullCom, stdin=fromFil, stdout=toFil, stderr=subprocess.PIPE)
        except OSError as errO:
            fromFil.close()
            toFil.close()
            messagebox.showerror("Run Problem","Problem running proderal: " + str(errO))
            return
        fromFil.close()
        toFil.close()
        if len(runRes.stderr) != 0:
            messagebox.showerror("Run Problem","Problem running proderal: " + runRes.stderr)
            return
        if runRes.returncode != 0:
            messagebox.showerror("Run Problem","Proderal returned error code: " + repr(runRes.returncode))
            return
        self.master.destroy()

root = tkinter.Tk()
root.title("proderal GUI Helper")
app = Proderal(master=root)
app.mainloop()
