#include "tree_errors.h"

#include "tree_defs.h"

/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* 3. Ошибки и проверка                                                                                                      */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */

const char* tree_strerror(tree_err_t err) {
    switch (err) {
        case TREE_OK:
            return "ok";
        case TREE_NULL_PTR:
            return "null tree pointer\n";
        case TREE_NO_MEM:
            return "out_of_memory (nodes)\n";
        case TREE_DUPLICATE:
            return "duplicate of key in tree\n";
        case TREE_WRONG_SIZE:
            return "tree have wrong size\n";
        case TREE_WRONG_HEAD:
            return "root have a parent\n";
        case NODE_LEFT_BROKEN:
            return "parent of left child is not this node\n";
        case NODE_RIGHT_BROKEN:
            return "parent of right child is not this node\n";
        case NODE_NULL_PTR:
            return "null node pointer\n";
        case TREE_NOT_FOUND:
            return "tree not found\n";
        case TREE_IO_ERROR:
            return "file processing failed\n";
        case TREE_WRONG_KEY:
            return "key is too big or small for his subtree\n";
        case TREE_DOT_CANT_OPEN:
            return "error running dot from the command line\n";
        case TREE_CORRYPTED:
            return "tree is not valid\n";
        case TREE_WRONG_NODE_TYPE:
            return "wrong type of data in node\n";
        case TREE_VARS_OVERFLOW:
            return "too many vars in tree\n";
        case TREE_VAR_WRONG_FORMAT:
            return "expected a var code, but it <0 or > 255\n";
        case TREE_WRONG_OP_CODE:
            return "operation code is not valid\n";
        case TREE_OP_WRONG_NUMBER_OF_CHILD:
            return "The number of children does not match the operation type\n";
        case TREE_VAR_TABLE_NOT_FOUND:
            return "tree has not a table with vars\n";
        case TREE_VARS_CORRYPTED:
            return "something went wrong with the vars table\n";
        case TREE_BUFFER_CORRYPTED:
            return "buffer is in wrong format\n";
        case NULL_PTR:
            return "null pointer in function\n";
        case DOUBLE_QUOTE_MISSING:
            return "a double quote is missing in the file\n";
        case NILL_MISSING:
            return "expected nil, but got another word\n";
        case ERROR_STACK_PUSH_IN_RPN:
            return "error in stack_push in rpn\n";
        case ERROR_RIGHT_CHILD_IN_RPN_NOT_FOUND:
            return "expected a right child on the stack while inserting a new node into the tree, but none was found\n";
        case ERROR_LEFT_CHILD_IN_RPN_NOT_FOUND:
            return "expected a left child on the stack while inserting a new node into the tree, but none was found\n";
        case ERROR_NULL_FILE:
            return "file with tree is empty\n";
        case ERROR_MISMATCHED_BRACKETS:
            return "unmatched opening or closing bracket\n";
        case ERROR_INVALID_NODE_CONTENT:
            return "Invalid node content in file\n";
        default:
            return "unknown error\n";
    }
}