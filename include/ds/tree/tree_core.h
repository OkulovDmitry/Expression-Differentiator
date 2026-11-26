#pragma once

#include "tree_errors.h"

/* ------------------------- Режимы компиляции ----------------------------*/

#define TREE_DEBUG

#ifdef TREE_DEBUG
#    define TREE_PLACE_IN , const char *name, const char *function, const char *file, int line
#    define TREE_PLACE_OUT(l) , #l, __func__, __FILE__, __LINE__
#else
#    define TREE_PLACE_IN
#    define TREE_PLACE_OUT(l)
#endif

/* ------------------------------------------------------------------------- */

typedef union NodeElem NodeElem;

/* ---- создание и удаление ---- */
void tree_init(Tree* t TREE_PLACE_IN);
#define TREE_INIT(t) tree_init((t)TREE_PLACE_OUT(t))
void tree_destroy(Tree* t);

/* ---- вставка/удаление узла за O(n) ---- */
tree_err_t tree_insert(Tree* t, int key, int type, NodeElem data);
Node* tree_find(const Tree* t, int key);
tree_err_t tree_remove_node(Tree* t, Node* n);
tree_err_t tree_remove(Tree* t, int key);