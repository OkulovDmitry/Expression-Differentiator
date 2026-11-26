#pragma once

#include "tree_fwd.h"
#include "tree_errors.h"

#define GV_BG "#1B1F24"         // фон
#define GV_TEXT "#E6EDF3"       // текст

/* ---- Инициализация html файла с истроией дерева ----*/
void tree_log_init();

/* --------------- Определение всех дампов ------------------ */
void tree_dump_hook(const char* tree, void* ctx);

void rpn_tree_dump(const char* name, const Tree* t);
tree_err_t tree_dump_graph_at(const char* base_name, const Tree* tree,
                              const char* call_file, int call_line, const char* call_func);
#define tree_dump_graph(base, tree) tree_dump_graph_at((base), (tree), __FILE__, __LINE__, __func__)