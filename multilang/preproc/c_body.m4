define(`Package_Start',`
ifdef(`LIBRARY_FOLD',`#include "'LIBRARY_FOLD`$1.h',`#include "$1.h"')
')dnl
define(`Package_End',`')dnl
define(`Function_Arg',`pushdef(`_FuncATypes',`$1')pushdef(`_FuncANames',`$2')')dnl
define(`Function_Ret',`define(`_FuncRTypes',`$1')')dnl
define(`_Function_Arg_Consume',`ifdef(`_FuncATypes',`pushdef(`_FuncATypesB',_FuncATypes)pushdef(`_FuncANamesB',_FuncANames)popdef(`_FuncATypes')popdef(`_FuncANames')_Function_Arg_Consume',`')')dnl
define(`_Function_Arg_Splat',`ifdef(`_FuncATypesB',`_FuncArgSep`GPML_'_FuncATypesB _FuncANamesB`'define(`_FuncArgSep',`, ')popdef(`_FuncATypesB')popdef(`_FuncANamesB')_Function_Arg_Splat')')dnl
define(`Function_Start',`_Function_Arg_Consume`'ifdef(`_FuncRTypes',`GPML_`'_FuncRTypes',`void') $1(define(`_FuncArgSep',`')_Function_Arg_Splat`'_FuncArgSep`'const char** ErrorRet){
	*ErrorRet = 0;
	PointerArray allAllocs;
	allAllocs.curLen = 0;
	allAllocs.maxLen = 0;
	allAllocs.allEnts = 0;
	int InternalSizeCalc;
	void* doNotKill[3];
	void* oldAlloc;
	char nullChar = 0;
')dnl
define(`Function_End',`}')dnl
define(`Lit_C',`char $1 = $2;')dnl
define(`Lit_I',`int $1 = $2;')dnl
define(`Lit_F',`double $1 = $2;')dnl
define(`Var_B',`_Bool $1;')dnl
define(`Var_C',`char $1;')dnl
define(`Var_I',`int $1;')dnl
define(`Var_F',`double $1;')dnl
define(`Var_CA',`
	if(ensurePointerArrayCapacity(&allAllocs, 2)){Error(`Allocation fail.')}
	GPML_ByteArray $1 = allocateByteArray($2);
	if($1==0){Error(`Allocation fail.')}
	addToPointerArray(&allAllocs, $1);
	addToPointerArray(&allAllocs, $1->arrConts);
')dnl
define(`Var_IA',`
	if(ensurePointerArrayCapacity(&allAllocs, 2)){Error(`Allocation fail.')}
	GPML_IntArray $1 = allocateIntArray($2);
	if($1==0){Error(`Allocation fail.')}
	addToPointerArray(&allAllocs, $1);
	addToPointerArray(&allAllocs, $1->arrConts);
')dnl
define(`Var_FA',`
	if(ensurePointerArrayCapacity(&allAllocs, 2)){Error(`Allocation fail.')}
	GPML_FloatArray $1 = allocateFloatArray($2);
	if($1==0){Error(`Allocation fail.')}
	addToPointerArray(&allAllocs, $1);
	addToPointerArray(&allAllocs, $1->arrConts);
')dnl
define(`Var_FAND',`
	if(ensurePointerArrayCapacity(&allAllocs, 3)){Error(`Allocation fail.')}
	GPML_FloatArrayND $1 = allocateFloatArrayND($2->arrLen, $2->arrConts);
	if($1==0){Error(`Allocation fail.')}
	addToPointerArray(&allAllocs, $1);
	addToPointerArray(&allAllocs, $1->arrLen);
	addToPointerArray(&allAllocs, $1->arrConts);
')dnl
define(`Con_CI',`$1 = (char)($2);')dnl
define(`Con_IF',`$1 = (int)($2);')dnl
define(`Con_FC',`$1 = 0x00FF & $2;')dnl
define(`Op_AND_B',`$1 = $2 && $3;')dnl
define(`Op_OR_B',`$1 = $2 || $3;')dnl
define(`Op_EQ_C',`$1 = $2 == $3;')dnl
define(`Op_GTE_C',`$1 = $2 >= $3;')dnl
define(`Op_LTE_C',`$1 = $2 <= $3;')dnl
define(`Op_EQ_I',`$1 = $2 == $3;')dnl
define(`Op_NEQ_I',`$1 = $2 != $3;')dnl
define(`Op_GT_I',`$1 = $2 > $3;')dnl
define(`Op_LT_I',`$1 = $2 < $3;')dnl
define(`Op_ADD_I',`$1 = $2 + $3;')dnl
define(`Op_ADD_F',`$1 = $2 + $3;')dnl
define(`Op_NEG_I',`$1 = -($2);')dnl
define(`Op_MUL_F',`$1 = $2 * $3;')dnl
define(`Op_DIV_F',`$1 = $2 / $3;')dnl
define(`Dim_FAND',`$1 = $2->dim;')dnl
define(`Len_CA',`$1 = $2->arrLen;')dnl
define(`Len_FA',`$1 = $2->arrLen;')dnl
define(`Len_FAND',`$1 = $2->arrLen[$3];')dnl
define(`Set_C',`$1 = $2;')dnl
define(`Set_I',`$1 = $2;')dnl
define(`Set_F',`$1 = $2;')dnl
define(`Set_CA',`$1->arrConts[$2] = $3;')dnl
define(`Set_IA',`$1->arrConts[$2] = $3;')dnl
define(`Set_FA',`$1->arrConts[$2] = $3;')dnl
define(`Set_FAND',`setInFloatArrayND($1, $2->arrConts, $3);')dnl
define(`Get_CA',`$1 = $2->arrConts[$3];')dnl
define(`Get_FA',`$1 = $2->arrConts[$3];')dnl
define(`Get_FAND',`$1 = getFromFloatArrayND($2, $3->arrConts);')dnl
define(`Clear_CA',`$1->arrLen = 0;')dnl
define(`Add_CA',`oldAlloc = $1->arrConts; if(addToByteArray($1, $2)){Error(`Allocation fail.')} if(oldAlloc != $1->arrConts){removePointerFromKillArray(&(allAllocs.curLen), allAllocs.allEnts, oldAlloc); addToPointerArray(&allAllocs, $1->arrConts);}')dnl
define(`Add_IA',`oldAlloc = $1->arrConts; if(addToIntArray($1, $2)){Error(`Allocation fail.')} if(oldAlloc != $1->arrConts){removePointerFromKillArray(&(allAllocs.curLen), allAllocs.allEnts, oldAlloc); addToPointerArray(&allAllocs, $1->arrConts);}')dnl
define(`Rem_CA',`($1->arrLen)--;')dnl
define(`Call',`ifelse(`$1',`',`',`$1 = ')$2`('shift(shift($@,ErrorRet))`)'; if(*ErrorRet){freeAllPointers(allAllocs.curLen, allAllocs.allEnts); return ifdef(`_FuncRTypes',`0',`');}')dnl
define(`ParseInt',`Add_CA($2,nullChar) $1 = atoi($2->arrConts); Rem_CA($2)')dnl
define(`If_Start',`if($1){')dnl
define(`Else',`}else{')dnl
define(`Else_If',`}else if($1){')dnl
define(`If_End',`}')dnl
define(`While_Start',`while($1){')dnl
define(`While_End',`}')dnl
define(`Error',`*ErrorRet = "$1"; freeAllPointers(allAllocs.curLen, allAllocs.allEnts); return ifdef(`_FuncRTypes',`0',`');')dnl
define(`Return',`ifdef(`_FuncRTypes',`
ifelse(_FuncRTypes,`GPML_BoolArray',`doNotKill[0] = $1; doNotKill[1] = $1->arrConts; freeAllPointersExcept(allAllocs.curLen, allAllocs.allEnts, 2, doNotKill);',
_FuncRTypes,`ByteArray',`doNotKill[0] = $1; doNotKill[1] = $1->arrConts; freeAllPointersExcept(allAllocs.curLen, allAllocs.allEnts, 2, doNotKill);',
_FuncRTypes,`IntArray',`doNotKill[0] = $1; doNotKill[1] = $1->arrConts; freeAllPointersExcept(allAllocs.curLen, allAllocs.allEnts, 2, doNotKill);',
_FuncRTypes,`FloatArray',`doNotKill[0] = $1; doNotKill[1] = $1->arrConts; freeAllPointersExcept(allAllocs.curLen, allAllocs.allEnts, 2, doNotKill);',
_FuncRTypes,`BoolArrayND',`doNotKill[0] = $1; doNotKill[1] = $1->arrLen; doNotKill[2] = $1->arrConts; freeAllPointersExcept(allAllocs.curLen, allAllocs.allEnts, 3, doNotKill);',
_FuncRTypes,`ByteArrayND',`doNotKill[0] = $1; doNotKill[1] = $1->arrLen; doNotKill[2] = $1->arrConts; freeAllPointersExcept(allAllocs.curLen, allAllocs.allEnts, 3, doNotKill);',
_FuncRTypes,`IntArrayND',`doNotKill[0] = $1; doNotKill[1] = $1->arrLen; doNotKill[2] = $1->arrConts; freeAllPointersExcept(allAllocs.curLen, allAllocs.allEnts, 3, doNotKill);',
_FuncRTypes,`FloatArrayND',`doNotKill[0] = $1; doNotKill[1] = $1->arrLen; doNotKill[2] = $1->arrConts; freeAllPointersExcept(allAllocs.curLen, allAllocs.allEnts, 3, doNotKill);',
`freeAllPointers(allAllocs.curLen, allAllocs.allEnts);')
free(allAllocs.allEnts); return $1;',`freeAllPointers(allAllocs.curLen, allAllocs.allEnts); free(allAllocs.allEnts); return;')')dnl

