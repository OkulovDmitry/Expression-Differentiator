#pragma once

#include "tree_errors.h"
#include "tree_fwd.h"

/* ---------------------- Принятия файла с постфиксным и инфиксным заданием дерева --------------------- */
tree_err_t read_rpn_tree(Tree* t, const char* name);