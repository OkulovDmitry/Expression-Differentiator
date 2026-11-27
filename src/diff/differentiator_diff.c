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
    if (*err) return NULL;
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

static Node* new_binop(Tree* t, OpType op, Node* left, Node* right, tree_err_t* err) {  // создать узел бинарной операции
    if (!left || !right) {
        *err = TREE_CORRYPTED;
        return NULL;
    }
    Node* n = NULL;
    *err    = tree_alloc_node(t, OP_TYPE, (NodeElem){.op_data = op}, &n);
    if (*err) return NULL;
    n->left     = left;
    left->head  = n;
    n->right    = right;
    right->head = n;
    return n;
}

static Node* new_unop(Tree* t, OpType op, Node* arg, tree_err_t* err) {  // создать узел унаной операции
    if (!arg) {
        *err = TREE_CORRYPTED;
        return NULL;
    }
    Node* n = NULL;
    *err    = tree_alloc_node(t, OP_TYPE, (NodeElem){.op_data = op}, &n);
    if (*err) return NULL;
    n->right  = arg;
    arg->head = n;
    return n;
}

static Node* new_negate(Tree* t, Node* arg, tree_err_t* err) {
    return new_unop(t, OP_SUB, arg, err);  // унарный минус = OP_SUB с left = NULL
}

// функция дифференцирования одного узла
static Node* diff_node(const Node* n, Tree* result, size_t var_idx, tree_err_t* err) {
    if (!n) return NULL;

    switch (n->type) {
        /* ---- Константа: d/dx[c] = 0 ---- */
        case NUM_TYPE: {
            return new_num(result, 0.0, err);
        }

        /* ---- Переменная: d/dx[x] = 1, d/dx[y] = 0 ---- */
        case VAR_TYPE: {
            double val = (n->data.var_data == var_idx) ? 1.0 : 0.0;
            return new_num(result, val, err);
        }

        /* ---- Операции ---- */
        case OP_TYPE: {
            OpType op = n->data.op_data;
            Node* u   = n->left;
            Node* v   = n->right;

            /* --- Унарные функции: аргумент в v (right), u == NULL --- */
            if (op == OP_SIN) {
                // d/dx[sin(v)] = cos(v) * v'
                Node* v_prime = diff_node(v, result, var_idx, err);
                if (*err) return NULL;
                Node* cos_v = new_unop(result, OP_COS, clone_into(v, result, err), err);
                if (*err) return NULL;
                return new_binop(result, OP_MUL, cos_v, v_prime, err);
            }
            if (op == OP_COS) {
                // d/dx[cos(v)] = -sin(v) * v'
                Node* v_prime = diff_node(v, result, var_idx, err);
                if (*err) return NULL;
                Node* sin_v   = new_unop(result, OP_SIN, clone_into(v, result, err), err);
                Node* neg_sin = new_negate(result, sin_v, err);
                if (*err) return NULL;
                return new_binop(result, OP_MUL, neg_sin, v_prime, err);
            }
            if (op == OP_LN) {
                // d/dx[ln(v)] = v' / v
                Node* v_prime = diff_node(v, result, var_idx, err);
                if (*err) return NULL;
                Node* v_copy = clone_into(v, result, err);
                if (*err) return NULL;
                return new_binop(result, OP_DIV, v_prime, v_copy, err);
            }

            /* --- Унарный минус: d/dx[-v] = -v' --- */
            if (u == NULL && op == OP_SUB) {
                Node* v_prime = diff_node(v, result, var_idx, err);
                if (*err) return NULL;
                return new_negate(result, v_prime, err);
            }

            /* --- Унарный плюс: d/dx[+v] = v' --- */
            if (u == NULL && op == OP_ADD) {
                return diff_node(v, result, var_idx, err);
            }

            // Бинарные операции
            Node* u_prime = diff_node(u, result, var_idx, err);  // производная левого поддерева
            if (*err) return NULL;
            Node* v_prime = diff_node(v, result, var_idx, err);  // произо=водная правого поддерева
            if (*err) return NULL;

            if (op == OP_ADD) {
                // d/dx[u + v] = u' + v'
                return new_binop(result, OP_ADD, u_prime, v_prime, err);
            }

            if (op == OP_SUB) {
                // d/dx[u - v] = u' - v'
                return new_binop(result, OP_SUB, u_prime, v_prime, err);
            }

            if (op == OP_MUL) {
                // d/dx[u * v] = u'*v + u*v'
                Node* term1 = new_binop(result, OP_MUL, u_prime, clone_into(v, result, err), err);
                if (*err) return NULL;
                Node* term2 = new_binop(result, OP_MUL, clone_into(u, result, err), v_prime, err);
                if (*err) return NULL;
                return new_binop(result, OP_ADD, term1, term2, err);
            }

            if (op == OP_DIV) {
                // d/dx[u / v] = (u'*v - u*v') / v^2
                Node* num1 = new_binop(result, OP_MUL, u_prime, clone_into(v, result, err), err);
                if (*err) return NULL;
                Node* num2 = new_binop(result, OP_MUL, clone_into(u, result, err), v_prime, err);
                if (*err) return NULL;
                Node* num = new_binop(result, OP_SUB, num1, num2, err);
                if (*err) return NULL;
                Node* v2 = new_binop(result, OP_POW, clone_into(v, result, err),
                                     new_num(result, 2.0, err), err);
                if (*err) return NULL;
                return new_binop(result, OP_DIV, num, v2, err);
            }

            if (op == OP_POW) {
                // Общий случай: (u^v)' = u^v * (v'*ln(u) + v*u'/u)
                Node* u_copy = clone_into(u, result, err);
                if (*err) return NULL;
                Node* v_copy = clone_into(v, result, err);
                if (*err) return NULL;

                // v' * ln(u)
                Node* ln_u = new_unop(result, OP_LN, clone_into(u, result, err), err);
                if (*err) return NULL;
                Node* t1 = new_binop(result, OP_MUL, v_prime, ln_u, err);
                if (*err) return NULL;

                // v * u' / u
                Node* vu = new_binop(result, OP_MUL, v_copy, u_prime, err);
                if (*err) return NULL;
                Node* t2 = new_binop(result, OP_DIV, vu, clone_into(u, result, err), err);
                if (*err) return NULL;

                // v'*ln(u) + v*u'/u
                Node* bracket = new_binop(result, OP_ADD, t1, t2, err);
                if (*err) return NULL;

                // u^v * (...)
                Node* uv = new_binop(result, OP_POW, u_copy, clone_into(v, result, err), err);
                if (*err) return NULL;
                return new_binop(result, OP_MUL, uv, bracket, err);
            }

            *err = TREE_WRONG_OP_CODE;
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
