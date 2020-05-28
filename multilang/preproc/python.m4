define(`_Current_Tab',`0')define(`_Make_Indent',`ifelse($1,0,`',`	_Make_Indent(eval($1 - 1))')')dnl
define(`_Output_Line',`divert(0)_Make_Indent(_Current_Tab)$1
divert(-1)')dnl
define(`Package_Start',`
"""$2"""
import gpmatlan
divert(-1)
')dnl
define(`Package_End',`')dnl
define(`Function_Arg',`pushdef(`_FuncATypes',`$1')pushdef(`_FuncANames',`$2')pushdef(`_FuncADocs',`$3')')dnl
define(`Function_Ret',`define(`_FuncRTypes',`$1')define(`_FuncRDocs',`$2')')dnl
define(`_ArgListFlip',`ifdef(`_FuncATypes',`pushdef(`_FuncATypesB',_FuncATypes)pushdef(`_FuncANamesB',_FuncANames)pushdef(`_FuncADocsB',_FuncADocs)popdef(`_FuncATypes')popdef(`_FuncANames')popdef(`_FuncADocs')_ArgListFlip',`')')dnl
define(`_Function_Arg_Splat',`ifdef(`_FuncATypesB',`pushdef(`_FuncATypesC',_FuncATypesB)pushdef(`_FuncANamesC',_FuncANamesB)pushdef(`_FuncADocsC',_FuncADocsB)_FuncArgSep`'_FuncANamesB`'define(`_FuncArgSep',`, ')popdef(`_FuncATypesB')popdef(`_FuncANamesB')popdef(`_FuncADocsB')_Function_Arg_Splat',`')')dnl
define(`_Function_Arg_Consume',`ifdef(`_FuncATypesC',`		@param _FuncANamesC _FuncADocsC
popdef(`_FuncATypesC')popdef(`_FuncANamesC')popdef(`_FuncADocsC')_Function_Arg_Consume',`')')dnl
define(`Function_Start',`
divert(0)
def $1(define(`_FuncArgSep',`')_ArgListFlip`'_Function_Arg_Splat):
	"""
		$2
_Function_Arg_Consume`'ifdef(`_FuncRTypes',`		@return _FuncRDocs',`dnl')
	"""
define(`_Current_Tab',`1')divert(-1)
')dnl
define(`Function_End',`divert(0)
divert(-1)define(`_Current_Tab',`0')')dnl
define(`Lit_C',`_Output_Line(`$1 = $2')')dnl
define(`Lit_I',`_Output_Line(`$1 = $2')')dnl
define(`Lit_F',`_Output_Line(`$1 = $2')')dnl
define(`Var_B',`_Output_Line(`$1 = None')')dnl
define(`Var_C',`_Output_Line(`$1 = None')')dnl
define(`Var_I',`_Output_Line(`$1 = None')')dnl
define(`Var_F',`_Output_Line(`$1 = None')')dnl
define(`Var_CA',`_Output_Line(`$1 = bytearray([0 for _ in range(0, $2)])')')dnl
define(`Var_IA',`_Output_Line(`$1 = [0 for _ in range(0, $2)]')')dnl
define(`Var_FA',`_Output_Line(`$1 = [0.0 for _ in range(0, $2)]')')dnl
define(`Var_FAND',`_Output_Line(`$1 = gpmatlan.NDArray($2,0.0)')')dnl
define(`Con_CI',`_Output_Line(`$1 = 0x00FF & $2')')dnl
define(`Con_IF',`_Output_Line(`$1 = int($2)')')dnl
define(`Con_FC',`_Output_Line(`$1 = $2')')dnl
define(`Op_AND_B',`_Output_Line(`$1 = ($2 and $3)')')dnl
define(`Op_OR_B',`_Output_Line(`$1 = ($2 or $3)')')dnl
define(`Op_EQ_C',`_Output_Line(`$1 = ($2 == $3)')')dnl
define(`Op_GTE_C',`_Output_Line(`$1 = ($2 >= $3)')')dnl
define(`Op_LTE_C',`_Output_Line(`$1 = ($2 <= $3)')')dnl
define(`Op_EQ_I',`_Output_Line(`$1 = ($2 == $3)')')dnl
define(`Op_NEQ_I',`_Output_Line(`$1 = ($2 != $3)')')dnl
define(`Op_GT_I',`_Output_Line(`$1 = ($2 > $3)')')dnl
define(`Op_LT_I',`_Output_Line(`$1 = ($2 < $3)')')dnl
define(`Op_ADD_I',`_Output_Line(`$1 = ($2 + $3)')')dnl
define(`Op_ADD_F',`_Output_Line(`$1 = ($2 + $3)')')dnl
define(`Op_NEG_I',`_Output_Line(`$1 = -($2)')')dnl
define(`Op_MUL_F',`_Output_Line(`$1 = ($2 * $3)')')dnl
define(`Op_DIV_F',`_Output_Line(`$1 = ($2 / $3)')')dnl
define(`Dim_FAND',`_Output_Line(`$1 = len($2.dimList)')')dnl
define(`Len_CA',`_Output_Line(`$1 = `len'($2)')')dnl
define(`Len_FA',`_Output_Line(`$1 = `len'($2)')')dnl
define(`Len_FAND',`_Output_Line(`$1 = $2.dimList[$3]')')dnl
define(`Set_C',`_Output_Line(`$1 = $2')')dnl
define(`Set_I',`_Output_Line(`$1 = $2')')dnl
define(`Set_F',`_Output_Line(`$1 = $2')')dnl
define(`Set_CA',`_Output_Line(`$1[$2] = $3')')dnl
define(`Set_IA',`_Output_Line(`$1[$2] = $3')')dnl
define(`Set_FA',`_Output_Line(`$1[$2] = $3')')dnl
define(`Set_FAND',`_Output_Line(`$1[$2] = $3')')dnl
define(`Get_CA',`_Output_Line(`$1 = $2[$3]')')dnl
define(`Get_FA',`_Output_Line(`$1 = $2[$3]')')dnl
define(`Get_FAND',`_Output_Line(`$1 = $2[$3]')')dnl
define(`Clear_CA',`_Output_Line(`$1.clear()')')dnl
define(`Add_CA',`_Output_Line(`$1.append($2)')')dnl
define(`Add_IA',`_Output_Line(`$1.append($2)')')dnl
define(`Rem_CA',`_Output_Line(`$1.pop()')')dnl
define(`Call',`_Output_Line(`ifelse(`$1',`',`',`$1 = ')$2`('shift(shift($@))`)'')')dnl
define(`ParseInt',`_Output_Line(`$1 = int($2.decode("utf-8"))')')dnl
define(`If_Start',`divert(0)_Make_Indent(_Current_Tab)if $1:
define(`_Current_Tab',eval(_Current_Tab + 1))_Make_Indent(_Current_Tab)pass
divert(-1)')dnl
define(`Else',`define(`_Current_Tab',eval(_Current_Tab - 1))
divert(0)_Make_Indent(_Current_Tab)else:
define(`_Current_Tab',eval(_Current_Tab + 1))_Make_Indent(_Current_Tab)pass
divert(-1)')dnl
define(`Else_If',`define(`_Current_Tab',eval(_Current_Tab - 1))
divert(0)_Make_Indent(_Current_Tab)elif $1:
define(`_Current_Tab',eval(_Current_Tab + 1))_Make_Indent(_Current_Tab)pass
divert(-1)')dnl
define(`If_End',`define(`_Current_Tab',eval(_Current_Tab - 1))')dnl
define(`While_Start',`divert(0)_Make_Indent(_Current_Tab)while $1:
define(`_Current_Tab',eval(_Current_Tab + 1))_Make_Indent(_Current_Tab)pass
divert(-1)')dnl
define(`While_End',`define(`_Current_Tab',eval(_Current_Tab - 1))')dnl
define(`Error',`_Output_Line(`raise ValueError("$1")')')dnl
define(`Return',`_Output_Line(`return $1')')dnl

