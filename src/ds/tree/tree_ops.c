#include "tree_ops.h"

#include "tree_defs.h"

/* заменяет поддерево u поддеревом v */
void transplant(Tree* t, Node* u, Node* v) {
    if (!u->head)
        t->root = v; /* u был корнем дерева */
    else if (u == u->head->left)
        u->head->left = v;
    else
        u->head->right = v;

    if (v) v->head = u->head;
}

/* ----- используется для печати узла типа union ------ */
void print_data(FILE* o, const Node* n, const VarTable* table) {
    switch (n->type) {
        case INT_TYPE:
            fprintf(o, "%d", n->data.int_data);
            break;
        case DOUBLE_TYPE:
            fprintf(o, "%lf", n->data.double_data);
            break;
        case STRING_TYPE:
            fprintf(o, "%s", n->data.string_data ? n->data.string_data : "(null)");
            break;
        case CHAR_TYPE:
            fprintf(o, "%c", n->data.char_data);
            break;
        case NUM_TYPE:
            fprintf(o, "%lf", n->data.double_data);
            break;
        case VAR_TYPE:
            fprintf(o, "%s", table->vars[n->data.var_data].name ? table->vars[n->data.var_data].name : "(null)");
            break;
        case OP_TYPE:
            printf_op_data(o, "%s", op_info[n->data.op_data].operation_ord);
        default:
            fprintf(o, "POISON");
            break;
    }
}