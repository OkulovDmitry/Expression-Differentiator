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

// Рекурсивное упрощение поддерева
static tree_err_t simplify_node(Node* n) {
    if (!n) return TREE_OK;

    // сначала упрощаем детей
    if (n->left) {
        tree_err_t err = simplify_node(n->left);
        if (err) return err;
    }
    if (n->right) {
        tree_err_t err = simplify_node(n->right);
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
            double b      = u->data.num_data;
            double result = 0;

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
                    return TREE_OK;
            }
            n->type          = NUM_TYPE;
            n->data.num_data = result;
            // Освобождение детей - пока утечка
            n->left  = NULL;
            n->right = NULL;
            return TREE_OK;
        }

        // Нейтральные элементы
        if (op == OP_ADD) {
            if (is_num(u) && u->data.num_data == 0.0) {
                // TODO: 0 + v -> v (нужно заменить узел на v, но это какой то пиздец)
            }
            if (is_num(v) && v->data.num_data == 0.0) {
                // u * 0 -> 0
            }
        }

        if (op == OP_MUL) {
            if (is_num(u) && u->data.num_data == 0.0) {
                // 0 * v -> 0
                n->type          = NUM_TYPE;
                n->data.num_data = 0.0;
                n->left          = NULL;
                n->right         = NULL;
                return TREE_OK;
            }
            if (is_num(v) && v->data.num_data == 0.0) {
                // u * 0 -> 0
                n->type          = NUM_TYPE;
                n->data.num_data = 0.0;
                n->left          = NULL;
                n->right         = NULL;
                return TREE_OK;
            }
            if (is_num(u) && u->data.num_data == 1.0) {
                // TODO: 1 * v -> v
            }
            if (is_num(v) && v->data.num_data == 1.0) {
                // TODO: u * 1 -> u
            }
        }
    }

    return TREE_OK;
}

tree_err_t tree_simplify(Tree* t) {
    if (!t || !t->root) return TREE_OK;
    return simplify_node(t->root);
}