#pragma once

#include "tree_errors.h"

#include <stdio.h>

/* -------------------- Верификаторы узла и дерева ------------------- */
tree_err_t verify_node(Node* n, const int* lo, const int* hi, size_t* count, size_t limit, size_t var_cap);
tree_err_t tree_verify(const Tree* t);