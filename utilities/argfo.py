
class ArgInfo:
    """Information on an argument"""
    flagTp = None
    """The type of this argument."""
    flagName = None
    """The flag used for this argument."""
    presName = None
    """The presentation name of the argument."""
    varDoc = None
    """Documentation for the argument."""
    hidden = False
    """Whether this argument is hidden."""
    defValue = None
    """The default value of the argument, if any."""
    enumGroup = None
    """For enums, the group."""
    strIsFile = False
    """For string, whether it is a file."""
    strFileIsWrite = False
    """For file strings, whether it is a write file."""
    strFileExts = None
    """Required extensions."""
    strIsFold = False
    """For string, whether it is a folder."""
    strFoldIsWrite = False
    """For folder strings, whether it is a write folder."""
    def __init__(self, lineS):
        self.flagTp = lineS[0]
        self.flagName = lineS[1]
        i = 2
        if lineS[0] == "BOOL":
            self.defValue = (lineS[2] == "TRUE")
            i = 3
        elif lineS[0] == "ENUM":
            self.defValue = (lineS[2] == "TRUE")
            i = 3
        elif lineS[0] == "INT":
            self.defValue = int(lineS[2])
            i = 3
        elif lineS[0] == "FLOAT":
            self.defValue = float(lineS[2])
            i = 3
        elif lineS[0] == "STRING":
            self.defValue = lineS[2]
            i = 3
        elif lineS[0] == "INTVEC":
            pass
        elif lineS[0] == "FLOATVEC":
            pass
        elif lineS[0] == "STRINGVEC":
            pass
        else:
            raise ValueError("Unknown argument type " + lineS[0])
        while i < len(lineS):
            if (lineS[i] == "\r\n") or (lineS[i] == "\n"):
                i = i + 1
            if lineS[i] == "NAME":
                i = i + 1
                if i >= len(lineS):
                    raise ValueError("Missing name")
                self.presName = lineS[i]
                i = i + 1
            elif lineS[i] == "PRIVATE":
                self.hidden = True
                i = i + 1
            elif lineS[i] == "DOCUMENT":
                i = i + 1
                if i >= len(lineS):
                    raise ValueError("Missing documentation")
                self.varDoc = bytearray.fromhex(lineS[i]).decode()
                i = i + 1
            elif lineS[0] == "BOOL":
                raise ValueError("Unknown options for bool " + lineS[i])
            elif lineS[0] == "ENUM":
                if lineS[i] == "GROUP":
                    i = i + 1
                    if i >= len(lineS):
                        raise ValueError("Missing group name")
                    self.enumGroup = lineS[i]
                    i = i + 1
                else:
                    raise ValueError("Unknown options for enum " + lineS[i])
            elif lineS[0] == "INT":
                raise ValueError("Unknown options for int " + lineS[i])
            elif lineS[0] == "FLOAT":
                raise ValueError("Unknown options for float " + lineS[i])
            elif lineS[0] == "STRING":
                if lineS[i] == "FILE":
                    self.strIsFile = True
                    i = i + 1
                    if i >= len(lineS):
                        raise ValueError("Missing file read/write mode.")
                    self.strFileIsWrite = (lineS[i] == "WRITE")
                    i = i + 1
                    if i >= len(lineS):
                        raise ValueError("Missing file extension count.")
                    numExt = int(lineS[i])
                    allExt = []
                    for j in range(numExt):
                        i = i + 1
                        if i >= len(lineS):
                            raise ValueError("Missing file extension.")
                        allExt.append(lineS[i])
                    self.strFileExts = allExt
                    i = i + 1
                elif lineS[i] == "FOLDER":
                    self.strIsFold = True
                    i = i + 1
                    if i >= len(lineS):
                        raise ValueError("Missing folder read/write mode.")
                    self.strFoldIsWrite = (lineS[i] == "WRITE")
                    i = i + 1
                else:
                    raise ValueError("Unknown options for string " + lineS[i])
            elif lineS[0] == "INTVEC":
                raise ValueError("Unknown options for int vector " + lineS[i])
            elif lineS[0] == "FLOATVEC":
                raise ValueError("Unknown options for float vector " + lineS[i])
            elif lineS[0] == "STRINGVEC":
                if lineS[i] == "FILE":
                    self.strIsFile = True
                    i = i + 1
                    if i >= len(lineS):
                        raise ValueError("Missing file read/write mode.")
                    self.strFileIsWrite = (lineS[i] == "WRITE")
                    i = i + 1
                    if i >= len(lineS):
                        raise ValueError("Missing file extension count.")
                    numExt = int(lineS[i])
                    allExt = []
                    for j in range(numExt):
                        i = i + 1
                        if i >= len(lineS):
                            raise ValueError("Missing file extension.")
                        allExt.append(lineS[i])
                    self.strFileExts = allExt
                    i = i + 1
                elif lineS[i] == "FOLDER":
                    self.strIsFold = True
                    i = i + 1
                    if i >= len(lineS):
                        raise ValueError("Missing folder read/write mode.")
                    self.strFoldIsWrite = (lineS[i] == "WRITE")
                    i = i + 1
                else:
                    raise ValueError("Unknown options for string " + lineS[i])

