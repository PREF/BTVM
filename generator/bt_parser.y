%name BTParser

%include {
    #include <cassert>
    #include "btvm/vm/vm_functions.h"
    #include "btvm/btvm.h"
    #include "bt_lexer.h"
}

%extra_argument { BTVM* btvm      }
%default_type   { Node*           }
%token_type     { BTLexer::Token* }
%token_prefix   BTT_

%syntax_error {
    VMUnused(yymajor); // Silence compiler warnings
    btvm->syntaxError(TOKEN->value, TOKEN->line);
}

%stack_overflow {
    btvm->error("Stack overflow");
}

%type stm_list             { NodeList*        }
%type var_list             { NodeList*        }
%type var_list_no_assign   { NodeList*        }
%type case_stms            { NodeList*        }
%type enum_def             { NodeList*        }
%type struct_stms          { NodeList*        }
%type params               { NodeList*        }
%type decls                { NodeList*        }
%type expr                 { NodeList*        }
%type args_decl            { NodeList*        }
%type custom_vars          { NodeList*        }
%type custom_var_decl      { NodeList*        }
%type scalar               { NScalarType*     }
%type string               { NType*           }
%type character            { NType*           }
%type datetime             { NType*           }
%type param                { NArgument*       }
%type var                  { NVariable*       }
%type var_no_assign        { NVariable*       }
%type custom_var           { NCustomVariable* }
%type block                { NBlock*          }
%type enum_type            { NType*           }
%type type                 { NType*           }
%type id                   { NIdentifier*     }
%type sign                 { int              }

program               ::= decls(B). { btvm->loadAST(new NBlock(*B)); delete B; }

decls(A)              ::= decls decl(B). { A->push_back(B); }
decls(A)              ::= decl(B).       { A = new NodeList(); A->push_back(B); }

decl(A)               ::= func_decl(B).    { A = B; }
decl(A)               ::= struct_decl(B).  { A = B; }
decl(A)               ::= union_decl(B).   { A = B; }
decl(A)               ::= enum_decl(B).    { A = B; }
decl(A)               ::= var_decl(B).     { A = B; }
decl(A)               ::= typedef_decl(B). { A = B; }
decl(A)               ::= stm(B).          { A = B; }

// =======================================================
//                   Function Declaration
// =======================================================

func_decl(A)          ::= type(B) id(C) O_ROUND params(D) C_ROUND            block(E). { A = new NFunction(B, C, *D, E); delete D; }
func_decl(A)          ::= type(B) id(C) O_ROUND           C_ROUND            block(D). { A = new NFunction(B, C, D); }

params(A)             ::= params COMMA param(B). { A->push_back(B); }
params(A)             ::= param(B).              { A = new NodeList(); A->push_back(B); }
params(A)             ::= VOID.                  { A = new NodeList(); }

param(A)              ::= type(B) BIN_AND id(C) array(D). { A = new NArgument(B, C, D); A->by_reference = true; }
param(A)              ::= type(B)         id(C) array(D). { A = new NArgument(B, C, D); }
param(A)              ::= id(B)   BIN_AND id(C) array(D). { A = new NArgument(B, C, D); A->by_reference = true; }
param(A)              ::= id(B)           id(C) array(D). { A = new NArgument(B, C, D); }

// =======================================================
//                   Type Declaration
// =======================================================

typedef_decl(A)       ::= TYPEDEF type(B) id(C) array(D) custom_var_decl(E) SEMICOLON. { A = new NTypedef(B, C, *E); B->size = D; delete E; }

struct_decl(A)        ::= STRUCT id(B) args_decl(C) O_CURLY struct_stms(D) C_CURLY custom_var_decl(E) SEMICOLON. { A = new NStruct(B, *C, *D, *E); delete C; delete D; delete E; }

union_decl(A)         ::= UNION id(B) args_decl(C) O_CURLY struct_stms(D) C_CURLY custom_var_decl(E) SEMICOLON. { A = new NUnion(B, *C, *D, *E); delete C; delete D; delete E; }

enum_decl(A)          ::= ENUM enum_type(B) id(C) O_CURLY enum_def(D) C_CURLY SEMICOLON. { A = new NEnum(C, *D, B); delete D; }

custom_var_decl(A)    ::= LT custom_vars(B) GT. { A = B; }
custom_var_decl(A)    ::= .                     { A = new NodeList(); }

args_decl(A)          ::= O_ROUND params(B) C_ROUND. { A = B; }
args_decl(A)          ::= .                          { A = new NodeList(); }

// =======================================================
//                 Variable Declaration
// =======================================================

var_decl(A)           ::= CONST   type(B)       var(C)           var_list(D)                              SEMICOLON. { C->type = B; C->is_const = true; C->names = *D; A = C; delete D; }
var_decl(A)           ::= CONST   id(B)         var(C)           var_list(D)                              SEMICOLON. { C->type = new NType(B); C->is_const = true; C->names = *D; A = C; delete D; }
var_decl(A)           ::= LOCAL   type(B)       var(C)           var_list(D)                              SEMICOLON. { C->type = B; C->is_local = true; C->names = *D; A = C; delete D; }
var_decl(A)           ::= LOCAL   id(B)         var(C)           var_list(D)                              SEMICOLON. { C->type = new NType(B); C->is_local = true; C->names = *D; A = C; delete D; }
var_decl(A)           ::=         type(B)       var_no_assign(C) var_list_no_assign(D) custom_var_decl(E) SEMICOLON. { C->type = B; C->custom_vars = *E; C->names = *D; A = C; delete D; delete E; }
var_decl(A)           ::=         id(B)         var_no_assign(C) var_list_no_assign(D) custom_var_decl(E) SEMICOLON. { C->type = new NType(B); C->names = *D; C->custom_vars = *E; A = C; delete D; delete E; }

var(A)                ::= id(B) array(C).                  { A = new NVariable(B, C); }
var(A)                ::= id(B) array(C) ASSIGN  op_if(D). { A = new NVariable(B, C); A->value = D; }

var_no_assign(A)      ::= id(B) array(C).                { A = new NVariable(B, C); }
var_no_assign(A)      ::= id(B) COLON number(C).         { A = new NVariable(B, NULL, C); }
var_no_assign(A)      ::= id(B) O_ROUND expr(C) C_ROUND. { A = new NVariable(B, NULL); A->constructor = *C; delete C; }
var_no_assign(A)      ::= COLON number(C).               { A = new NVariable(C); }

array(A)              ::= O_SQUARE expr(B) C_SQUARE. { A = new NBlock(*B); delete B; }
array(A)              ::= O_SQUARE         C_SQUARE. { A = new NBlock(); }
array(A)              ::= .                          { A = NULL; }

var_list(A)           ::= var_list COMMA var(B). { A->push_back(B); }
var_list(A)           ::= COMMA var(B).          { A = new NodeList(); A->push_back(B); }
var_list(A)           ::= .                      { A = new NodeList(); }

var_list_no_assign(A) ::= var_list_no_assign COMMA var_no_assign(B). { A->push_back(B); }
var_list_no_assign(A) ::= COMMA var_no_assign(B).                    { A = new NodeList(); A->push_back(B); }
var_list_no_assign(A) ::= .                                          { A = new NodeList(); }

// =======================================================
//                     Enumerations
// =======================================================

enum_def(A)           ::= enum_def COMMA enum_val(B). { A->push_back(B); }
enum_def(A)           ::= enum_val(B).                { A = new NodeList(); A->push_back(B); }

enum_val(A)           ::= id(B) ASSIGN number(C). { A = new NEnumValue(B, C); }
enum_val(A)           ::= id(B).                  { A = new NEnumValue(B); }

enum_type(A)          ::= LT type(B) GT.                { A = B; }
enum_type(A)          ::= .                             { A = NULL; }

// =======================================================
//                     Types
// =======================================================

custom_vars(A)        ::= custom_vars COMMA custom_var(B). { A->push_back(B); }
custom_vars(A)        ::= custom_var(B).                   { A = new NodeList(); A->push_back(B); }

custom_var(A)         ::= IDENTIFIER(B) ASSIGN id(C).      { A = new NCustomVariable(B->value, C); }
custom_var(A)         ::= IDENTIFIER(B) ASSIGN literal(C). { A = new NCustomVariable(B->value, C); }

type(A)               ::= sign(B) scalar(C).                                        { if(B != -1) C->is_signed = B; A = C; }
type(A)               ::= sign character(B).                                        { A = B; }
type(A)               ::= string(B).                                                { A = B; }
type(A)               ::= datetime(B).                                              { A = B; }
type(A)               ::= STRUCT              id(B) args_decl(C) O_CURLY struct_stms(D) C_CURLY. { A = new NStruct(B, *C, *D); delete C; delete D; }
type(A)               ::= UNION               id(B) args_decl(C) O_CURLY struct_stms(D) C_CURLY. { A = new NUnion(B, *C, *D); delete C; delete D; }
type(A)               ::= ENUM   enum_type(B) id(C)              O_CURLY enum_def(D)    C_CURLY. { A = new NEnum(C, *D, B); delete D; }
type(A)               ::= STRUCT                    args_decl(B) O_CURLY struct_stms(C) C_CURLY. { A = new NStruct(*B, *C); delete B; delete C; }
type(A)               ::= UNION                     args_decl(B) O_CURLY struct_stms(C) C_CURLY. { A = new NUnion(*B, *C); delete B; delete C; }
type(A)               ::= ENUM   enum_type(B)                    O_CURLY enum_def(C)    C_CURLY. { A = new NEnum(*C, B); delete C; }
type(A)               ::= VOID(B).                                                  { A = new NType(B->value); }
type(A)               ::= BOOL(B).                                                  { A = new NBooleanType(B->value); }

sign(A)               ::= UNSIGNED. { A =  0; }
sign(A)               ::= SIGNED.   { A =  1; }
sign(A)               ::= .         { A = -1; }

string(A)             ::= WSTRING(B).   { A = new NStringType(B->value); }
string(A)             ::= STRING(B).    { A = new NStringType(B->value); }
string(A)             ::= character(B). { A = B; }

character(A)          ::= CHAR(B).   { A = new NStringType(B->value); }
character(A)          ::= WCHAR(B).  { A = new NStringType(B->value); }

datetime(A)           ::= TIME(B).     { A = new NTime(B->value); }
datetime(A)           ::= DOSDATE(B).  { A = new NDosDate(B->value); }
datetime(A)           ::= DOSTIME(B).  { A = new NDosTime(B->value); }
datetime(A)           ::= OLETIME(B).  { A = new NOleTime(B->value); }
datetime(A)           ::= FILETIME(B). { A = new NFileTime(B->value); }

scalar(A)             ::= BYTE(B).   { A = new NScalarType(B->value, 8); }
scalar(A)             ::= UCHAR(B).  { A = new NScalarType(B->value, 8);  A->is_signed = false; }
scalar(A)             ::= UBYTE(B).  { A = new NScalarType(B->value, 8);  A->is_signed = false; }
scalar(A)             ::= SHORT(B).  { A = new NScalarType(B->value, 16); }
scalar(A)             ::= USHORT(B). { A = new NScalarType(B->value, 16); A->is_signed = false; }
scalar(A)             ::= INT32(B).  { A = new NScalarType(B->value, 32); }
scalar(A)             ::= UINT32(B). { A = new NScalarType(B->value, 32); A->is_signed = false; }
scalar(A)             ::= INT64(B).  { A = new NScalarType(B->value, 64); }
scalar(A)             ::= UINT64(B). { A = new NScalarType(B->value, 64); A->is_signed = false; }
scalar(A)             ::= HFLOAT(B). { A = new NScalarType(B->value, 16); A->is_fp = true; }
scalar(A)             ::= FLOAT(B).  { A = new NScalarType(B->value, 32); A->is_fp = true; }
scalar(A)             ::= DOUBLE(B). { A = new NScalarType(B->value, 64); A->is_fp = true; }

// =======================================================
//                      Statements
// =======================================================

struct_stms(A)        ::= struct_stms stm(B). { A->push_back(B); }
struct_stms(A)        ::= stm(B).             { A = new NodeList(); A->push_back(B); }
struct_stms(A)        ::= .                   { A = new NodeList(); }

stm(A)                ::= IF O_ROUND expr(B) C_ROUND stm(C).                                   { A = new NConditional(new NBlock(*B), C); delete B; }
stm(A)                ::= IF O_ROUND expr(B) C_ROUND then_stm(C) ELSE stm(D).                  { A = new NConditional(new NBlock(*B), C, D); delete B; }
stm(A)                ::= WHILE O_ROUND expr(B) C_ROUND stm(C).                                { A = new NWhile(new NBlock(*B), C); delete B; }
stm(A)                ::= FOR O_ROUND arg(B) SEMICOLON arg(C) SEMICOLON arg(D) C_ROUND stm(E). { A = new NFor(B, C, D, E); }
stm(A)                ::= normal_stm(B).                                                       { A = B; }

then_stm(A)           ::= IF O_ROUND expr(B) C_ROUND then_stm(C) ELSE then_stm(D).                  { A = new NConditional(new NBlock(*B), C, D); delete B; }
then_stm(A)           ::= WHILE O_ROUND expr(B) C_ROUND then_stm(C).                                { A = new NWhile(new NBlock(*B), C); delete B; }
then_stm(A)           ::= FOR O_ROUND arg(B) SEMICOLON arg(C) SEMICOLON arg(D) C_ROUND then_stm(E). { A = new NFor(B, C, D, E); }
then_stm(A)           ::= normal_stm(B).                                                            { A = B; }

normal_stm(A)         ::= DO stm(B) WHILE O_ROUND expr(C) C_ROUND SEMICOLON.           { A = new NDoWhile(B, new NBlock(*C)); delete C; }
normal_stm(A)         ::= SWITCH O_ROUND expr(B) C_ROUND O_CURLY case_stms(C) C_CURLY. { A = new NSwitch(new NBlock(*B), *C); delete B, delete C; }
normal_stm(A)         ::= var_decl(B).                                                 { A = B; }
normal_stm(A)         ::= block(B).                                                    { A = B; }
normal_stm(A)         ::= expr(B) SEMICOLON.                                           { A = new NBlock(*B); delete B; }
normal_stm(A)         ::= BREAK SEMICOLON.                                             { A = new NVMState(VMState::Break); }
normal_stm(A)         ::= CONTINUE SEMICOLON.                                          { A = new NVMState(VMState::Continue); }
normal_stm(A)         ::= RETURN expr(B) SEMICOLON.                                    { A = new NReturn(new NBlock(*B)); delete B; }
normal_stm(A)         ::= SEMICOLON.                                                   { A = new NBlock(); }

arg(A)                ::= expr(B).                 { A = new NBlock(*B); delete B; }
arg(A)                ::= .                        { A = new NBlock(); }

case_stms(A)          ::= case_stms case_stm(B). { A->push_back(B); }
case_stms(A)          ::= case_stm(B).           { A = new NodeList(); A->push_back(B); }
case_stms(A)          ::= .                      { A = new NodeList(); }

case_stm(A)           ::= CASE value(B) COLON stm_list(C). { A = new NCase(B, new NBlock(*C)); delete C; }
case_stm(A)           ::= DEFAULT COLON stm_list(B).       { A = new NCase(new NBlock(*B)); delete B; }

block(A)              ::= O_CURLY stm_list(B) C_CURLY. { A = new NBlock(*B); delete B; }

stm_list(A)           ::= stm_list stm(B). { A->push_back(B); }
stm_list(A)           ::= stm(B).          { A = new NodeList(); A->push_back(B); }
stm_list(A)           ::= .                { A = new NodeList(); }

// =======================================================
//           C's 15 level of operator precedence
// =======================================================

expr(A)               ::= expr COMMA op_assign(B). { A->push_back(B); }
expr(A)               ::= op_assign(B).            { A = new NodeList(); A->push_back(B); }

op_assign(A)          ::= op_if(B) ASSIGN(C)     op_assign(D). { A = new NBinaryOperator(B, C->value, D); }
op_assign(A)          ::= op_if(B) ADD_ASSIGN(C) op_assign(D). { A = new NBinaryOperator(B, C->value, D); }
op_assign(A)          ::= op_if(B) SUB_ASSIGN(C) op_assign(D). { A = new NBinaryOperator(B, C->value, D); }
op_assign(A)          ::= op_if(B) MUL_ASSIGN(C) op_assign(D). { A = new NBinaryOperator(B, C->value, D); }
op_assign(A)          ::= op_if(B) DIV_ASSIGN(C) op_assign(D). { A = new NBinaryOperator(B, C->value, D); }
op_assign(A)          ::= op_if(B) XOR_ASSIGN(C) op_assign(D). { A = new NBinaryOperator(B, C->value, D); }
op_assign(A)          ::= op_if(B) AND_ASSIGN(C) op_assign(D). { A = new NBinaryOperator(B, C->value, D); }
op_assign(A)          ::= op_if(B) OR_ASSIGN(C)  op_assign(D). { A = new NBinaryOperator(B, C->value, D); }
op_assign(A)          ::= op_if(B) LS_ASSIGN(C)  op_assign(D). { A = new NBinaryOperator(B, C->value, D); }
op_assign(A)          ::= op_if(B) RS_ASSIGN(C)  op_assign(D). { A = new NBinaryOperator(B, C->value, D); }
op_assign(A)          ::= op_if(B).                            { A = B; }

op_if(A)              ::= op_or(B) QUESTION op_if(C) COLON op_if(D). { A = new NConditional(B, C, D); }
op_if(A)              ::= op_or(B).                                  { A = B; }

op_or(A)              ::= op_or(B) LOG_OR(C) op_and(D). { A = new NBinaryOperator(B, C->value, D); }
op_or(A)              ::= op_and(B).                    { A = B; }

op_and(A)             ::= op_and(B) LOG_AND(C) op_binor(D). { A = new NBinaryOperator(B, C->value, D); }
op_and(A)             ::= op_binor(B).                      { A = B; }

op_binor(A)           ::= op_binor(B) BIN_OR(C) op_binxor(D). { A = new NBinaryOperator(B, C->value, D); }
op_binor(A)           ::= op_binxor(B).                       { A = B; }

op_binxor(A)          ::= op_binxor(B) BIN_XOR(C) op_binand(D). { A = new NBinaryOperator(B, C->value, D); }
op_binxor(A)          ::= op_binand(B).                         { A = B; }

op_binand(A)          ::= op_binand(B) BIN_AND(C) op_equate(D). { A = new NBinaryOperator(B, C->value, D); }
op_binand(A)          ::= op_equate(B).                         { A = B; }

op_equate(A)          ::= op_equate(B) EQ(C) op_compare(D). { A = new NCompareOperator(B, D, C->value); }
op_equate(A)          ::= op_equate(B) NE(C) op_compare(D). { A = new NCompareOperator(B, D, C->value); }
op_equate(A)          ::= op_compare(B).                    { A = B; }

op_compare(A)         ::= op_compare(B) LT(C) op_shift(D). { A = new NCompareOperator(B, D, C->value); }
op_compare(A)         ::= op_compare(B) GT(C) op_shift(D). { A = new NCompareOperator(B, D, C->value); }
op_compare(A)         ::= op_compare(B) LE(C) op_shift(D). { A = new NCompareOperator(B, D, C->value); }
op_compare(A)         ::= op_compare(B) GE(C) op_shift(D). { A = new NCompareOperator(B, D, C->value); }
op_compare(A)         ::= op_shift(B).                     { A = B; }

op_shift(A)           ::= op_shift(B) LSL(C) op_add(D). { A = new NBinaryOperator(B, C->value, D); }
op_shift(A)           ::= op_shift(B) LSR(C) op_add(D). { A = new NBinaryOperator(B, C->value, D); }
op_shift(A)           ::= op_add(B).                    { A = B; }

op_add(A)             ::= op_add(B) ADD(C) op_mult(D). { A = new NBinaryOperator(B, C->value, D); }
op_add(A)             ::= op_add(B) SUB(C) op_mult(D). { A = new NBinaryOperator(B, C->value, D); }
op_add(A)             ::= op_mult(B).                  { A = B; }

op_mult(A)            ::= op_mult(B) MUL(C) op_unary(D). { A = new NBinaryOperator(B, C->value, D); }
op_mult(A)            ::= op_mult(B) DIV(C) op_unary(D). { A = new NBinaryOperator(B, C->value, D); }
op_mult(A)            ::= op_mult(B) MOD(C) op_unary(D). { A = new NBinaryOperator(B, C->value, D); }
op_mult(A)            ::= op_unary(B).                   { A = B; }

op_unary(A)           ::= LOG_NOT(B) op_unary(C).              { A = new NUnaryOperator(B->value, C, true); }
op_unary(A)           ::= BIN_NOT(B) op_unary(C).              { A = new NUnaryOperator(B->value, C, true); }
op_unary(A)           ::= SUB(B) op_unary(C).                  { A = new NUnaryOperator(B->value, C, true); }
op_unary(A)           ::= INC(B) op_unary(C).                  { A = new NUnaryOperator(B->value, C, true); }
op_unary(A)           ::= DEC(B) op_unary(C).                  { A = new NUnaryOperator(B->value, C, true); }
op_unary(A)           ::= op_unary(B) INC(C).                  { A = new NUnaryOperator(C->value, B, false); }
op_unary(A)           ::= op_unary(B) DEC(C).                  { A = new NUnaryOperator(C->value, B, false); }
op_unary(A)           ::= O_ROUND type(B) C_ROUND op_unary(C). { A = new NCast(B, C); }
op_unary(A)           ::= SIZEOF O_ROUND type(B)      C_ROUND. { A = new NSizeOf(B); }
op_unary(A)           ::= SIZEOF O_ROUND op_assign(B) C_ROUND. { A = new NSizeOf(B); }
op_unary(A)           ::= op_pointer(B).                       { A = B; }

op_pointer(A)         ::= op_pointer(B) DOT id(C).                 { A = new NDotOperator(B, C); }
op_pointer(A)         ::= op_pointer(B) O_SQUARE expr(C) C_SQUARE. { A = new NIndexOperator(B, new NBlock(*C)); delete C; }
op_pointer(A)         ::= value(B).                                { A = B; }

// =======================================================
//                  Basic Declarations
// =======================================================

value(A)              ::= O_ROUND expr(B) C_ROUND.       { A = new NBlock(*B); delete B; }
value(A)              ::= id(B) O_ROUND expr(C) C_ROUND. { A = new NCall(B, *C); delete C; }
value(A)              ::= id(B) O_ROUND C_ROUND.         { A = new NCall(B); }
value(A)              ::= id(B).                         { A = B; }
value(A)              ::= literal(B).                    { A = B; }

literal(A)            ::= LITERAL_STRING(B). { A = new NString(B->value); }
literal(A)            ::= boolean(B).        { A = B; }
literal(A)            ::= number(B).         { A = B; }

number(A)             ::= LITERAL_OCT(B).  { A = new NInteger(VMFunctions::string_to_number(B->value, 8)); }
number(A)             ::= LITERAL_DEC(B).  { A = new NInteger(VMFunctions::string_to_number(B->value, 10)); }
number(A)             ::= LITERAL_HEX(B).  { A = new NInteger(VMFunctions::string_to_number(B->value, 16)); }
number(A)             ::= LITERAL_REAL(B). { A = new NReal(VMFunctions::string_to_number(B->value)); }

boolean(A)            ::= TRUE.            { A = new NBoolean(true); }
boolean(A)            ::= FALSE.           { A = new NBoolean(false); }

id(A)                 ::= IDENTIFIER(B). { A = new NIdentifier(B->value); }
