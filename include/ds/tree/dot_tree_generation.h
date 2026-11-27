#pragma once

#include "tree_errors.h"

#include "tree_fwd.h"

#include <stdio.h>

#define GV_USED_FILL "#2F5D8A"  // в data
#define GV_USED_BORDER "#6CA6E0"
#define GV_FREE_FILL "#2B3036"  // свободная ячейка
#define GV_FREE_BORDER "#6E7681"
#define GV_HDR_FILL "#5e4592"  // корень
#define GV_HDR_BORDER "#B392F0"
#define GV_RIGHT "#F0883E"       // правый ребенок
#define GV_LEFT "#187a06"        // левый ребенок
#define GV_FREE_CHAIN "#8B949E"  // non_type
#define GV_FREE_PTR "#D29922"    // указатель  на non_type
#define GV_BAD "#F85149"         // рассогласование

tree_err_t node_dump_graph(const Tree* t, const Node* n, int* qur_num, FILE* out);
tree_err_t arrow_dump_graph(const Node* n, int* qur_num, FILE* out);