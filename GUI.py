
import tkinter
import subprocess
import proderalgui

root = tkinter.Tk()
root.title("proderal GUI Helper")
app = proderalgui.ArgumentGUIFrame(master = root)
app.mainloop()

appArgs = app.getArgs()
if not (appArgs is None):
    subprocess.run(["proderal"] + appArgs)

