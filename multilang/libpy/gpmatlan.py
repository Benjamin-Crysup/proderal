
class NDArray:
	def __init__(self,dimList,zeroEl):
		self.dimList = dimList[:]
		totNumEl = 1
		for dv in dimList:
			totNumEl = totNumEl * dv
		self.conts = [zeroEl for _ in range(0,totNumEl)]
	def __getitem__(self,dimInd):
		workEl = dimInd[0]
		for i in range(1,len(self.dimList)):
			workEl = workEl * self.dimList[i] + dimInd[i]
		return self.conts[workEl]
	def __setitem__(self,dimInd,newVal):
		workEl = dimInd[0]
		for i in range(1,len(self.dimList)):
			workEl = workEl * self.dimList[i] + dimInd[i]
		self.conts[workEl] = newVal

