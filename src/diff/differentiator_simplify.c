#include "differentiator_simplify.h"

#include <math.h>
#include <stdlib.h>

#include "string.h"
#include "tree_core.h"
#include "tree_defs.h"
#include "tree_ops.h"
#include "tree_validate.h"

static bool is_num(const Node* n) {
    return n && n->type == NUM_TYPE;
}

static bool is_zero(const Node* n) {
    return is_num(n) && n->data.num_data == 0.0;
}

static bool is_one(const Node* n) {
    return is_num(n) && n->data.num_data == 1.0;
}

// сравнение двух поддеревьев на структурное равенство
static bool nodes_equal(const Node* a, const Node* b) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    if (a->type != b->type) return false;

    switch (a->type) {
        case NUM_TYPE:
            return a->data.num_data == b->data.num_data;
        case VAR_TYPE:
            return a->data.var_data == b->data.var_data;
        case OP_TYPE:
            if (a->data.op_data != b->data.op_data) return false;
            return nodes_equal(a->left, b->left) && nodes_equal(a->right, b->right);
        default:
            return false;
    }
}

static void count_nodes(Node* n, size_t* count) {
    if (!n) return;
    (*count)++;
    count_nodes(n->left, count);
    count_nodes(n->right, count);
}

/* ----- Рекурсивное упрощение поддерева ----- */
// Упрощает поддерево, на которое указывает *node_ptr.
// Может заменить узел на другой (например, +(x,0) → x).
static tree_err_t simplify_node(Node** node_ptr, Tree* t) {
    if (!node_ptr || !*node_ptr) return TREE_OK;

    Node* n = *node_ptr;

    // сначала упрощаем детей
    if (n->left) {
        tree_err_t err = simplify_node(&n->left, t);
        if (err) return err;
    }
    if (n->right) {
        tree_err_t err = simplify_node(&n->right, t);
        if (err) return err;
    }

    // Теперь приступаем к упрощению текущего узла
    if (n->type == OP_TYPE) {
        OpType op = n->data.op_data;
        Node* u   = n->left;
        Node* v   = n->right;

        // Свертка констант - если оба ребенка числа, то вычисляем
        if (is_num(u) && is_num(v)) {
            double a      = u->data.num_data;
            double b      = v->data.num_data;
            double result = 0;
            bool can_fold = true;

            switch (op) {
                case OP_ADD:
                    result = a + b;
                    break;
                case OP_SUB:
                    result = a - b;
                    break;
                case OP_MUL:
                    result = a * b;
                    break;
                case OP_DIV:
                    if (b == 0) return TREE_OK;  // деление на ноль — не упрощаем
                    result = a / b;
                    break;
                case OP_POW:
                    result = pow(a, b);
                    break;
                default:
                    can_fold = false;
            }

            if (can_fold) {
                Node* new_node = calloc(1, sizeof(Node));
                if (!new_node) return TREE_NO_MEM;

                new_node->type          = NUM_TYPE;
                new_node->data.num_data = result;
                new_node->key           = n->key;
                new_node->head          = n->head;

                // Заменяем узел
                if (n->head) {
                    if (n->head->left == n)
                        n->head->left = new_node;
                    else if (n->head->right == n)
                        n->head->right = new_node;
                } else {
                    t->root = new_node;  // был корень
                }

                tree_node_destroy(n);
                *node_ptr = new_node;
                return TREE_OK;
            }
        }

        // Нейтральные элементы
        if (op == OP_ADD) {
            if (is_zero(u)) {
                // 0 + v -> v (заменяем весь узел n на v)
                if (n->head) {
                    if (n->head->left == n)
                        n->head->left = v;
                    else if (n->head->right == n)
                        n->head->right = v;
                } else {
                    t->root = v;
                }
                v->head  = n->head;
                n->left  = NULL;  // отцепляем детей, чтобы tree_node_destroy не уничтожил v
                n->right = NULL;
                tree_node_destroy(n);
                *node_ptr = v;
                return TREE_OK;  // v уже был упрощён, повторный вызов не нужен
            }
            if (is_zero(v)) {
                // u + 0 → u
                if (n->head) {
                    if (n->head->left == n)
                        n->head->left = u;
                    else if (n->head->right == n)
                        n->head->right = u;
                } else {
                    t->root = u;
                }
                u->head  = n->head;
                n->left  = NULL;
                n->right = NULL;
                tree_node_destroy(n);
                *node_ptr = u;
                return TREE_OK;
            }
        }

        if (op == OP_MUL) {
            if (is_zero(u) || is_zero(v)) {
                // 0 * v = 0, u * 0 = 0
                Node* zero = calloc(1, sizeof(Node));
                if (!zero) return TREE_NO_MEM;
                zero->type          = NUM_TYPE;
                zero->data.num_data = 0.0;
                zero->key           = n->key;
                zero->head          = n->head;

                if (n->head) {
                    if (n->head->left == n)
                        n->head->left = zero;
                    else if (n->head->right == n)
                        n->head->right = zero;
                } else {
                    t->root = zero;
                }

                n->left  = NULL;
                n->right = NULL;
                tree_node_destroy(n);
                *node_ptr = zero;
                return TREE_OK;
            }
            if (is_one(u)) {
                // 1 * v -> v
                if (n->head) {
                    if (n->head->left == n)
                        n->head->left = v;
                    else if (n->head->right == n)
                        n->head->right = v;
                } else {
                    t->root = v;
                }
                v->head  = n->head;
                n->left  = NULL;
                n->right = NULL;
                tree_node_destroy(n);
                *node_ptr = v;
                return TREE_OK;
            }
            if (is_one(v)) {
                // u * 1 → u
                if (n->head) {
                    if (n->head->left == n)
                        n->head->left = u;
                    else if (n->head->right == n)
                        n->head->right = u;
                } else {
                    t->root = u;
                }
                u->head  = n->head;
                n->left  = NULL;
                n->right = NULL;
                tree_node_destroy(n);
                *node_ptr = u;
                return TREE_OK;
            }
        }

        if (op == OP_SUB) {
            if (is_zero(v)) {
                // u - 0 → u
                if (n->head) {
                    if (n->head->left == n)
                        n->head->left = u;
                    else if (n->head->right == n)
                        n->head->right = u;
                } else {
                    t->root = u;
                }
                u->head  = n->head;
                n->left  = NULL;
                n->right = NULL;
                tree_node_destroy(n);
                *node_ptr = u;
                return TREE_OK;
            }
            if (nodes_equal(u, v)) {
                // u - u → 0
                Node* zero = calloc(1, sizeof(Node));
                if (!zero) return TREE_NO_MEM;
                zero->type          = NUM_TYPE;
                zero->data.num_data = 0.0;
                zero->key           = n->key;
                zero->head          = n->head;

                if (n->head) {
                    if (n->head->left == n)
                        n->head->left = zero;
                    else if (n->head->right == n)
                        n->head->right = zero;
                } else {
                    t->root = zero;
                }

                n->left  = NULL;
                n->right = NULL;
                tree_node_destroy(n);
                *node_ptr = zero;
                return TREE_OK;
            }
        }

        if (op == OP_DIV) {
            if (is_one(v)) {
                // u / 1 → u
                if (n->head) {
                    if (n->head->left == n)
                        n->head->left = u;
                    else if (n->head->right == n)
                        n->head->right = u;
                } else {
                    t->root = u;
                }
                u->head  = n->head;
                n->left  = NULL;
                n->right = NULL;
                tree_node_destroy(n);
                *node_ptr = u;
                return TREE_OK;
            }
        }
    }

    return TREE_OK;
}

tree_err_t tree_simplify(Tree* t) {
    if (!t || !t->root) return TREE_OK;

    tree_err_t err = simplify_node(&t->root, t);
    if (err) return err;

    // Пересчитываем size после упрощения
    t->size = 0;
    count_nodes(t->root, &t->size);

    tree_renumber(t);

    return TREE_OK;
}