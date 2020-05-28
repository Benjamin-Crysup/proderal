define(`Package_Start',`
#ifndef $1_H
#define $1_H 1

/**$2*/

#include "gpmatlan.h"

#ifdef __cplusplus
extern "C" {
#endif

divert(-1)
')dnl
define(`Package_End',`
divert(0)
#ifdef __cplusplus
}
#endif

#endif
divert(-1)
')dnl
define(`Function_Arg',`pushdef(`_FuncATypes',`$1')pushdef(`_FuncANames',`$2')pushdef(`_FuncADocs',`$3')')dnl
define(`Function_Ret',`define(`_FuncRTypes',`$1')define(`_FuncRDocs',`$2')')dnl
define(`_Function_Arg_Consume',`ifdef(`_FuncATypes',` * @param _FuncANames _FuncADocs
pushdef(`_FuncATypesB',_FuncATypes)pushdef(`_FuncANamesB',_FuncANames)popdef(`_FuncATypes')popdef(`_FuncANames')popdef(`_FuncADocs')_Function_Arg_Consume',`')')dnl
define(`_Function_Arg_Splat',`ifdef(`_FuncATypesB',`_FuncArgSep`GPML_'_FuncATypesB _FuncANamesB`'define(`_FuncArgSep',`, ')popdef(`_FuncATypesB')popdef(`_FuncANamesB')_Function_Arg_Splat')')dnl
define(`Function_Start',`
divert(0)
/**
 * $2
_Function_Arg_Consume * @param ErrorRet Returns an error message on error, null if not.
ifdef(`_FuncRTypes',` * @return _FuncRDocs
') */
ifdef(`_FuncRTypes',`GPML_`'_FuncRTypes',`void') $1(define(`_FuncArgSep',`')_Function_Arg_Splat`'_FuncArgSep`'const char** ErrorRet);
divert(-1)
')dnl
define(`Function_End',`')dnl
define(`Lit_C',`')dnl
define(`Lit_I',`')dnl
define(`Lit_F',`')dnl
define(`Var_B',`')dnl
define(`Var_C',`')dnl
define(`Var_I',`')dnl
define(`Var_F',`')dnl
define(`Var_CA',`')dnl
define(`Var_IA',`')dnl
define(`Var_FA',`')dnl
define(`Var_FAND',`')dnl
define(`Con_CI',`')dnl
define(`Con_IF',`')dnl
define(`Con_FC',`')dnl
define(`Op_AND_B',`')dnl
define(`Op_OR_B',`')dnl
define(`Op_EQ_C',`')dnl
define(`Op_GTE_C',`')dnl
define(`Op_LTE_C',`')dnl
define(`Op_EQ_I',`')dnl
define(`Op_NEQ_I',`')dnl
define(`Op_GT_I',`')dnl
define(`Op_LT_I',`')dnl
define(`Op_ADD_I',`')dnl
define(`Op_ADD_F',`')dnl
define(`Op_NEG_I',`')dnl
define(`Op_MUL_F',`')dnl
define(`Op_DIV_F',`')dnl
define(`Dim_FAND',`')dnl
define(`Len_CA',`')dnl
define(`Len_FA',`')dnl
define(`Len_FAND',`')dnl
define(`Set_C',`')dnl
define(`Set_I',`')dnl
define(`Set_F',`')dnl
define(`Set_CA',`')dnl
define(`Set_IA',`')dnl
define(`Set_FA',`')dnl
define(`Set_FAND',`')dnl
define(`Get_CA',`')dnl
define(`Get_FA',`')dnl
define(`Get_FAND',`')dnl
define(`Clear_CA',`')dnl
define(`Add_CA',`')dnl
define(`Add_IA',`')dnl
define(`Rem_CA',`')dnl
define(`Call',`')dnl
define(`ParseInt',`')dnl
define(`If_Start',`')dnl
define(`Else',`')dnl
define(`Else_If',`')dnl
define(`If_End',`')dnl
define(`While_Start',`')dnl
define(`While_End',`')dnl
define(`Error',`')dnl
define(`Return',`')dnl
