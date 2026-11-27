#pragma once

#include <stddef.h>

#include "tree_errors.h"
#include "tree_fwd.h"

// Упрощение дерева снизу вверх
tree_err_t tree_simplify(Tree* t);