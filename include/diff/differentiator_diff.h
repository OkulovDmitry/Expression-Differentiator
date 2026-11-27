#pragma once

#include <stddef.h>

#include "tree_errors.h"
#include "tree_fwd.h"

tree_err_t tree_diff(const Tree* t, size_t var_idx, Tree* result);