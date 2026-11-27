#include <dot_tree_generation.h>
#include <string.h>

#include "tree_defs.h"
#include "tree_ops.h"

/* ------------------------------ построение одной стрелки в dot ------------------------------- */
static void gv_edge(FILE* o, const char* mode, int from, const char* fport, int to, const char* tport,
                    const char* color, const char* attrs) {
    if (attrs == NULL) {
        fprintf(o, "  cell%d -> cell%d [color=\"%s\"];\n", from, to, color);
        return;
    }
    const char* sep = attrs[0] ? ", " : "";
    if (strcmp(mode, "ARCS") == 0)
        fprintf(o, "  cell%d:%s -> cell%d:%s [color=\"%s\"%s%s];\n",
                from, fport, to, tport, color, sep, attrs);
    else {
        fprintf(o, "  cell%d -> cell%d [color=\"%s\"%s%s];\n", from, to, color, sep, attrs);
    }
}

/* ----------------------------- построение всех стрелок для текущей ячейки в dot -------------------------- */
tree_err_t arrow_dump_graph(const Node* n, int* qur_num, FILE* out) {
    if (n == NULL) return TREE_OK;
    if (*qur_num > 4 * MAX_OF_NODES) return TREE_CORRYPTED;
    int parent = (*qur_num)++;

    if (n->left != NULL) {
        int left_child = *qur_num;
        arrow_dump_graph(n->left, qur_num, out);

        if (n->left->type == NON_TYPE && n->left->head == n)  // не уверен есть ли тут какие то проблемные случаи
            gv_edge(out, TREE_GV_MODE, parent, "w", left_child, "n", GV_FREE_CHAIN, "style=dashed, headclip=true");
        else if (n->left->head == n)  // left стрелка обычная или красная при несоглассованности
            gv_edge(out, TREE_GV_MODE, parent, "w", left_child, "n", GV_LEFT, "headclip=true");
        else
            gv_edge(out, TREE_GV_MODE, parent, "w", left_child, "n", GV_BAD, "penwidth=4, headclip=true");
    }

    for (int i = 0; i < 3; i++) {
        int invis_child = (*qur_num)++;
        gv_edge(out, TREE_GV_MODE, parent, "s", invis_child, "n", GV_FREE_CHAIN, "style=invis, weigh=10000, headclip=true");  // невидимый ребенок - его рисуем в любом случае
    }

    if (n->right != NULL) {
        int right_child = *qur_num;
        arrow_dump_graph(n->right, qur_num, out);

        if (n->right->type == NON_TYPE && n->right->head == n)  // не уверен есть ли тут какие то проблемные случаи
            gv_edge(out, TREE_GV_MODE, parent, "e", right_child, "n", GV_FREE_CHAIN, "style=dashed, headclip=true");
        else if (n->right->head == n)  // right стрелка обычная или красная при несогласованности
            gv_edge(out, TREE_GV_MODE, parent, "e", right_child, "n", GV_RIGHT ":" GV_RIGHT, "headclip=true");
        else
            gv_edge(out, TREE_GV_MODE, parent, "e", right_child, "n", GV_BAD, "penwidth=4, headclip=true");
    }
    return TREE_OK;
}

/* --------------------------------------- дамп ячейки в dot ------------------------------------------ */
tree_err_t node_dump_graph(const Tree* t, const Node* n, int* qur_num, FILE* out) {
    if (n == NULL) return TREE_OK;
    if (*qur_num > 4 * MAX_OF_NODES) return TREE_CORRYPTED;

    if (*qur_num == 0)  // корень
    {
        fprintf(out, "  cell%d [fillcolor=\"" GV_HDR_FILL "\", color=\"" GV_HDR_BORDER
                     "\","
                     " label=\"ROOT\\n\\key: %d",
                *qur_num, n->key);
        fprintf(out, "\\ndata: ");
        print_data(out, n, t->var_table);

    } else {
        int is_free         = (n->type == NON_TYPE);
        int number_of_child = (n->left == NULL && n->right == NULL) ? 0 : ((n->right != NULL && n->left != NULL) ? 2 : 1);
        char style[256]     = "POISON";
        if (n->type == NUM_TYPE) snprintf(style, sizeof(style), "rounded");
        if (n->type == VAR_TYPE) snprintf(style, sizeof(style), "ellipse");
        if (n->type == OP_TYPE) snprintf(style, sizeof(style), "triangle");
        if (is_free) {
            fprintf(out, "  cell%d [fillcolor=\"%s\", color=\"%s\", style=\"%s,filled,dashed\", label=\"key: %d[%d]\\n\\n",
                    *qur_num,
                    GV_FREE_FILL,
                    GV_FREE_BORDER,
                    style,
                    n->key, number_of_child);
        } else {
            fprintf(out, "  cell%d [fillcolor=\"%s\", color=\"%s\", label=\"key: %d[%d]\\n\\n",
                    *qur_num,
                    GV_USED_FILL,
                    GV_USED_BORDER,
                    n->key, number_of_child);
        }
        print_data(out, n, t->var_table);
    }

    if (n->head != NULL)
        fprintf(out, "\\nhead: %d", n->head->key);
    else
        fprintf(out, "\\nhead: NO_HEAD");

    if (n->left != NULL)
        fprintf(out, "\\nleft_child: %d", n->left->key);
    else
        fprintf(out, "\\nleft_child: NO_LEFT_CHILD");

    if (n->right != NULL)
        fprintf(out, "\\nright_child: %d", n->right->key);
    else
        fprintf(out, "\\nright_child: NO_RIGHT_CHILD");
    fprintf(out, "\"];\n");

    (*qur_num)++;
    node_dump_graph(t, n->left, qur_num, out);
    for (int i = 0; i < 3; i++) {
        fprintf(out, "  cell%d [style=\"invis\", width=1, fixedsize=true];\n", *qur_num);  // средний невидимка
        (*qur_num)++;
    }
    node_dump_graph(t, n->right, qur_num, out);
    return TREE_OK;
}