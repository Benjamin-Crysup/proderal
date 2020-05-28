Package_Start(`whodun_cigfq',`Codes for mangling cigar strings, quality scores and other sam/fq information.')

Function_Arg(`Byte',`testChar',`The character to test')
Function_Ret(`Boolean',`Whether the test character is a digit')
Function_Start(`isCigarLetterDigit',`Gets whether a cigar character is a digit.')
	Lit_C(zero, 48)
	Lit_C(nine, 57)
	Var_B(gteZ) Op_GTE_C(gteZ, testChar, zero)
	Var_B(lteN) Op_LTE_C(lteN, testChar, nine)
	Var_B(retV) Op_AND_B(retV,gteZ,lteN)
	Return(retV)
Function_End

Function_Arg(`ByteArray',`cigar',`The cigar string in question.')
Function_Arg(`Integer',`refStart',`The first index for the cigar string')
Function_Ret(`IntArray',`The locations the characters in the cigar string map to, the number of sequence characters to skip, the number of characters actually in the cigar string.')
Function_Start(`cigarStringToReferenceLocations',`Figures out the reference locations given a cigar string.')
	Lit_I(zero, 0)
	Lit_I(one, 1)
	Lit_C(casciiS, 83)
	Lit_C(casciiH, 72)
	Lit_C(casciiP, 80)
	Lit_C(casciiD, 68)
	Lit_C(casciiN, 78)
	Lit_C(casciiI, 73)
	Lit_C(casciiM, 77)
	Lit_C(casciiEq, 61)
	Lit_C(casciiX, 88)
	Var_I(cigLen) Len_CA(cigLen, cigar)
	Var_I(firstOff) Set_I(firstOff, zero)
	Var_I(realLen) Set_I(realLen, zero)
	Var_I(curRLoc) Set_I(curRLoc, refStart)
	Var_IA(cigLocs, zero)
	Var_CA(lastNum, zero)
	Var_I(i) Set_I(i,zero)
	Var_B(im) Op_LT_I(im,i,cigLen)
	While_Start(im)
		Var_C(curChar) Get_CA(curChar, cigar, i)
		Var_B(isDig) Call(isDig, isCigarLetterDigit, curChar)
		If_Start(isDig)
			Add_CA(lastNum, curChar)
		Else
			Var_I(lnumLen) Len_CA(lnumLen, lastNum)
			Var_B(numlpro) Op_EQ_I(numlpro, lnumLen, zero)
			If_Start(numlpro)
				Error(`Cigar operation missing count.')
			If_End
			Var_I(lnum) ParseInt(lnum, lastNum)
			Clear_CA(lastNum)
			Op_LT_I(numlpro, lnum, zero)
			If_Start(numlpro)
				Error(`Cigar operation count less than zero.')
			If_End
			Var_B(isS) Op_EQ_C(isS, curChar, casciiS)
			Var_B(isH) Op_EQ_C(isH, curChar, casciiH)
			Var_B(isP) Op_EQ_C(isP, curChar, casciiP)
			Var_B(isD) Op_EQ_C(isD, curChar, casciiD)
			Var_B(isN) Op_EQ_C(isN, curChar, casciiN)
			Var_B(isDN) Op_OR_B(isDN, isD, isN)
			Var_B(isI) Op_EQ_C(isI, curChar, casciiI)
			Var_B(isM) Op_EQ_C(isM, curChar, casciiM)
			Var_B(isEq) Op_EQ_C(isEq, curChar, casciiEq)
			Var_B(isX) Op_EQ_C(isX, curChar, casciiX)
			Var_B(isMEX) Op_OR_B(isMEX, isM, isEq) Op_OR_B(isMEX, isMEX, isX)
			If_Start(isS)
				Var_I(cigLSize) Len_CA(cigLSize, cigLocs)
				Var_B(cigLZ) Op_EQ_I(cigLZ, cigLSize, zero)
				If_Start(cigLZ)
					Op_ADD_I(firstOff, firstOff, lnum)
				If_End
			Else_If(isH)
			Else_If(isP)
			Else_If(isDN)
				Op_ADD_I(curRLoc, curRLoc, lnum)
			Else_If(isI)
				Var_I(j) Set_I(j,zero)
				Var_B(jm) Op_LT_I(jm,j,lnum)
				While_Start(jm)
					Var_I(negRLoc) Op_ADD_I(negRLoc, curRLoc, one) Op_NEG_I(negRLoc, negRLoc)
					Add_IA(cigLocs, negRLoc)
					Op_ADD_I(j, j, one)
					Op_LT_I(jm,j,lnum)
				While_End
				Op_ADD_I(realLen, realLen, lnum)
			Else_If(isMEX)
				Var_I(j) Set_I(j,zero)
				Var_B(jm) Op_LT_I(jm,j,lnum)
				While_Start(jm)
					Add_IA(cigLocs, curRLoc)
					Op_ADD_I(curRLoc, curRLoc, one)
					Op_ADD_I(j, j, one)
					Op_LT_I(jm,j,lnum)
				While_End
				Op_ADD_I(realLen, realLen, lnum)
			Else
				Error(`Unknown operation code in CIGAR string.')
			If_End
		If_End
		Op_ADD_I(i, i, one)
		Op_LT_I(im,i,cigLen)
	While_End
	Add_IA(cigLocs, firstOff)
	Add_IA(cigLocs, realLen)
	Return(cigLocs)
Function_End

Function_Arg(`ByteArray',`phredAscii',`The phred score string to convert.')
Function_Ret(`FloatArray',`The phred scores as log probabilities.')
Function_Start(`phredAsciiToLogProb',`Turns ascii phred scores to log probabilities.')
	Lit_I(zero, 0)
	Lit_I(one, 1)
	Lit_F(negThirtyThree, -33.0)
	Lit_F(negTen, -10.0)
	Var_I(phredLen) Len_CA(phredLen, phredAscii)
	Var_FA(probVals, phredLen)
	Var_I(i) Set_I(i,zero)
	Var_B(im) Op_LT_I(im,i,phredLen)
	While_Start(im)
		Var_C(curChar) Get_CA(curChar, phredAscii, i)
		Var_F(curCharF) Con_FC(curCharF, curChar)
		Op_ADD_F(curCharF, curCharF, negThirtyThree)
		Op_DIV_F(curCharF, curCharF, negTen)
		Set_FA(probVals, i, curCharF)
		Op_ADD_I(i, i, one)
		Op_LT_I(im,i,phredLen)
	While_End
	Return(probVals)
Function_End

Function_Arg(`FloatArray',`probVals',`The log probability values to convert.')
Function_Ret(`ByteArray',`The phred quality scores')
Function_Start(`logProbToPhredAscii',`Turns log probabilities into quality scores.')
	Lit_I(zero, 0)
	Lit_I(one, 1)
	Lit_I(threeThreeI, 33)
	Lit_I(oneTwoSix, 126)
	Lit_F(negTen, -10.0)
	Var_I(phredLen) Len_FA(phredLen, probVals)
	Var_CA(phredAscii, phredLen)
	Var_I(i) Set_I(i,zero)
	Var_B(im) Op_LT_I(im,i,phredLen)
	While_Start(im)
		Var_F(curChar) Get_FA(curChar, probVals, i)
		Op_MUL_F(curChar, curChar, negTen)
		Var_I(curCharI) Con_IF(curCharI, curChar)
		Op_ADD_I(curCharI, curCharI, threeThreeI)
		Var_B(isPro)
		Op_LT_I(isPro, curCharI, threeThreeI)
		If_Start(isPro)
			Set_I(curCharI, threeThreeI)
		If_End
		Op_GT_I(isPro, curCharI, oneTwoSix)
		If_Start(isPro)
			Set_I(curCharI, oneTwoSix)
		If_End
		Var_C(curCharC) Con_CI(curCharC, curCharI)
		Set_CA(phredAscii, i, curCharC)
		Op_ADD_I(i, i, one)
		Op_LT_I(im,i,phredLen)
	While_End
	Return(phredAscii)
Function_End

Function_Arg(`Byte',`phredAscii',`The phred score to convert.')
Function_Ret(`Float',`The phred score as a log probability.')
Function_Start(`phredCharToLogProb',`Turns ascii phred scores to log probabilities.')
	Lit_F(negThirtyThree, -33.0)
	Lit_F(negTen, -10.0)
	Var_C(curChar) Set_C(curChar, phredAscii)
	Var_F(curCharF) Con_FC(curCharF, curChar)
	Op_ADD_F(curCharF, curCharF, negThirtyThree)
	Op_DIV_F(curCharF, curCharF, negTen)
	Return(curCharF)
Function_End

Function_Arg(`Float',`probVals',`The log probability value to convert.')
Function_Ret(`Byte',`The phred quality score')
Function_Start(`logProbToPhredChar',`Turns a log probability into a quality score.')
	Lit_I(threeThreeI, 33)
	Lit_I(oneTwoSix, 126)
	Lit_F(negTen, -10.0)
	Var_F(curChar) Set_F(curChar, probVals)
	Op_MUL_F(curChar, curChar, negTen)
	Var_I(curCharI) Con_IF(curCharI, curChar)
	Op_ADD_I(curCharI, curCharI, threeThreeI)
	Var_B(isPro)
	Op_LT_I(isPro, curCharI, threeThreeI)
	If_Start(isPro)
		Set_I(curCharI, threeThreeI)
	If_End
	Op_GT_I(isPro, curCharI, oneTwoSix)
	If_Start(isPro)
		Set_I(curCharI, oneTwoSix)
	If_End
	Var_C(curCharC) Con_CI(curCharC, curCharI)
	Return(curCharC)
Function_End

Package_End
