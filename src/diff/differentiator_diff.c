#include "differentiator_diff.h"

#include <stdlib.h>

#include "string.h"
#include "tree_core.h"
#include "tree_defs.h"
#include "tree_ops.h"
#include "tree_validate.h"

static Node* new_num(Tree* t, double val, tree_err_t* err) {  // создать числовой узел
    Node* n = NULL;
    *err    = tree_alloc_node(t, NUM_TYPE, (NodeElem){.num_data = val}, &n);
    if (err) return NULL;
    return n;
}

static Node* clone_into(const Node* src, Tree* dst, tree_err_t* err) {  // клонировать поддерево src в dest
    if (!src) return NULL;

    Node* left = clone_into(src->left, dst, err);
    if (*err) return NULL;
    Node* right = clone_into(src->right, dst, err);
    if (*err) return NULL;

    NodeElem d = src->data;

    Node* n = NULL;
    *err    = tree_alloc_node(dst, src->type, d, &n);
    if (*err) return NULL;
    n->left = left;
    if (left) left->head = n;
    n->right = right;
    if (right) right->head = n;
    return n;
}

static tree_err_t copy_vartable(const VarTable* src, VarTable* dst) {  // копируем одну таблицу переменных в другую
    if (!src || !dst) return TREE_NULL_PTR;
    dst->size     = src->size;
    dst->capacity = src->capacity;
    dst->vars     = calloc(dst->capacity, sizeof(Variable));
    if (!dst->vars) return TREE_NO_MEM;
    memcpy(dst->vars, src->vars, dst->size * sizeof(Variable));
    return TREE_OK;
}

// функция дифференцирования одного узла
static Node* diff_node(const Node* n, Tree* result, size_t var_idx, tree_err_t* err) {
    if (!n) return NULL;

    switch (n->type) {
        case NUM_TYPE: {
            NodeElem d = {.num_data = 0.0};
            Node* n    = NULL;
            *err       = tree_alloc_node(result, NUM_TYPE, d, &n);
            if (*err != TREE_OK) return NULL;
            return n;
        }

        case VAR_TYPE: {
            double val = (n->data.var_data == var_idx) ? 1.0 : 0.0;
            NodeElem d = {.num_data = val};
            Node* n    = NULL;
            *err       = tree_alloc_node(result, NUM_TYPE, d, &n);
            return n;  // TODO: dsl ка нужна
        }

        case OP_TYPE: {
            OpType op = n->data.op_data;
            Node* u   = n->left;
            Node* v   = n->right;

            // Унарный операции
            if (op == OP_SIN || op == OP_COS || op == OP_LN) {
                // TODO: пока заглушка
                *err = TREE_OK;
                return NULL;
            }

            // Бинарные операции
            Node* u_prime = diff_node(u, result, var_idx, err);  // производная левого поддерева
            if (*err) return NULL;
            Node* v_prime = diff_node(v, result, var_idx, err);  // произо=водная правого поддерева
            if (*err) return NULL;

            if (op == OP_ADD) {
                // d/dx[u + v] = u' + v'
                NodeElem d = {.op_data = OP_ADD};
                Node* add  = NULL;
                *err       = tree_alloc_node(result, OP_TYPE, d, &add);
                if (*err) return NULL;
                add->left  = u_prime;
                add->right = v_prime;
                if (u_prime) u_prime->head = add;
                if (v_prime) v_prime->head = add;
                return add;
            }

            if (op == OP_SUB) {
                // d/dx[u - v] = u' - v'
                NodeElem d = {.op_data = OP_SUB};
                Node* sub  = NULL;
                *err       = tree_alloc_node(result, OP_TYPE, d, &sub);
                if (*err) return NULL;
                sub->left  = u_prime;
                sub->right = v_prime;
                if (u_prime) u_prime->head = sub;
                if (v_prime) v_prime->head = sub;
                return sub;
            }

            // TODO: Умножение, деление, степень
            *err = TREE_OK;
            return NULL;
        }

        default:
            *err = TREE_WRONG_NODE_TYPE;
            return NULL;
    }
}

tree_err_t tree_diff(const Tree* t, size_t var_idx, Tree* result) {
    if (!t || !result) return TREE_NULL_PTR;

    tree_err_t err = TREE_INIT(result);
    if (err != TREE_OK) return err;

    err = copy_vartable(t->var_table, result->var_table);
    if (err) {
        tree_destroy(result);
        return err;
    }

    result->root = diff_node(t->root, result, var_idx, &err);
    if (err != TREE_OK) {
        tree_destroy(result);
        return err;
    }
    if (!result->root) return TREE_CORRYPTED;

    tree_renumber(result);
    return tree_verify(result);
}
