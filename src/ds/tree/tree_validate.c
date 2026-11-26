#include <tree_validate.h>

#include "tree_defs.h"

/* ---------------------------- верификатор отдельного узла ---------------------------*/
tree_err_t verify_node(Node* n, const int* lo, const int* hi, size_t* count, size_t limit, size_t var_size) {
    if (!n) return TREE_OK;
    if (++*count > limit) return TREE_WRONG_SIZE;
    if (n->type < 0 || n->type >= NUMBER_OF_TYPES) return TREE_WRONG_NODE_TYPE;
    if (n->type == STRING_TYPE && n->data.string_data == NULL) return TREE_WRONG_NODE_TYPE;
    if (n->type == OP_TYPE) {
        OpType op = n->data.op_data;
        if ((int)op < 0 || op >= NUMBER_OF_OPERATIONS) return TREE_WRONG_OP_CODE;
        int kids = (n->left ? 1 : 0) + (n->right ? 1 : 0);
        if (kids < op_info[op].min_children || kids > op_info[op].max_children)
            return TREE_OP_WRONG_NUMBER_OF_CHILD;
        if (op_info[op].max_children == 1 && n->left != NULL)
            return TREE_OP_WRONG_NUMBER_OF_CHILD;
    }
    if (n->type == VAR_TYPE && n->data.var_data >= var_size) return TREE_VAR_WRONG_FORMAT;
    if (lo && n->key <= *lo) return TREE_WRONG_KEY;
    if (hi && n->key >= *hi) return TREE_WRONG_KEY;

    if (n->left && n->left->head != n) return NODE_LEFT_BROKEN;
    if (n->right && n->right->head != n) return NODE_RIGHT_BROKEN;

    tree_err_t err = verify_node(n->left, lo, &n->key, count, limit, var_size);
    if (err != TREE_OK) return err;
    return verify_node(n->right, &n->key, hi, count, limit, var_size);
}

/* --------------------------- верификатор всего дерева ------------------------- */
tree_err_t tree_verify(const Tree* t) {
    if (!t) return TREE_NULL_PTR;
    if (t->var_table == NULL) return TREE_VAR_TABLE_NOT_FOUND;
    size_t count = 0;
    if (t->root && t->root->head != NULL) return TREE_WRONG_HEAD;
    tree_err_t tree_err = verify_node(t->root, NULL, NULL, &count, t->size, t->var_table->size);
    if (tree_err != TREE_OK) return tree_err;
    if (count != t->size) return TREE_WRONG_SIZE;
    count = 0;
    if (t->size != 0 && t->buffer == NULL) return TREE_BUFFER_CORRYPTED;
    if (t->buf_cap > TREE_MAX_BUF_CAP) return TREE_BUFFER_CORRYPTED;
    if (t->buf_used > t->buf_cap) return TREE_BUFFER_CORRYPTED;
    for (size_t i = t->buf_used; i < t->buf_cap; i++) {
        if (t->buffer[i] != '\0') return TREE_BUFFER_CORRYPTED;
    }
    return TREE_OK;
}