#pragma once

#include "tree_fwd.h"

/* ----------------------------- обработка ошибок ---------------------------------*/

typedef enum {
    TREE_OK = 0,  //
    // Предупреждения 0...99
    NULL_PTR           = 7,  //
    TREE_NULL_PTR      = 1,  //
    NODE_NULL_PTR      = 2,  //
    TREE_NOT_FOUND     = 3,  //
    TREE_NO_MEM        = 4,  //
    TREE_IO_ERROR      = 5,  //
    TREE_DOT_CANT_OPEN = 6,  //
    // Ошибки структуры 100...199
    TREE_DUPLICATE                = 100,  //
    TREE_WRONG_KEY                = 101,  //
    TREE_WRONG_SIZE               = 102,  //
    TREE_WRONG_HEAD               = 103,  //
    NODE_LEFT_BROKEN              = 104,  //
    NODE_RIGHT_BROKEN             = 105,  //
    TREE_CORRYPTED                = 106,  //
    TREE_WRONG_NODE_TYPE          = 107,  //
    TREE_VARS_OVERFLOW            = 108,  //
    TREE_VAR_WRONG_FORMAT         = 109,  //
    TREE_WRONG_OP_CODE            = 110,  //
    TREE_OP_WRONG_NUMBER_OF_CHILD = 111,  //
    TREE_VAR_TABLE_NOT_FOUND      = 112,  //
    TREE_VARS_CORRYPTED           = 113,  //
    TREE_BUFFER_CORRYPTED         = 114,  //
    // ошибки чтения файла 200+
    DOUBLE_QUOTE_MISSING               = 200,  //
    NILL_MISSING                       = 201,  //
    ERROR_STACK_PUSH_IN_RPN            = 202,  //
    ERROR_RIGHT_CHILD_IN_RPN_NOT_FOUND = 203,  //
    ERROR_LEFT_CHILD_IN_RPN_NOT_FOUND  = 204,  //
    ERROR_NULL_FILE                    = 205,  //
    ERROR_MISMATCHED_BRACKETS          = 206,  //
    ERROR_INVALID_NODE_CONTENT         = 207,  //

} tree_err_t;

const char* tree_strerror(tree_err_t err);

/* --------------------------------------------------------------------------- */