#include "differentiator.h"

int main() {
    tree_log_init();

    Tree t = {0};
    assert_set_hook(tree_dump_hook, &t);  // любой упавший ассерт сбросит дерево в лог
    tree_err_t err = TREE_INIT(&t);
    ASSERT_MSG(err == TREE_OK, tree_strerror(err));

    err = diff_read(&t, "Differentiator_database/first_expr.txt");
    printf("diff_read returned: %d\n", (int)err);
    ASSERT_MSG(err == TREE_OK, tree_strerror(err));

    tree_dump_graph_at("ORIGINAL", &t, __FILE__, __LINE__, "main");

    Tree t_first_diff = {0};
    err               = TREE_INIT(&t_first_diff);
    ASSERT_MSG(err == TREE_OK, tree_strerror(err));

    assert_set_hook(tree_dump_hook, &t_first_diff);
    err = tree_diff(&t, 0, &t_first_diff);
    err = tree_diff(&t, 0, &t_first_diff);
    printf("tree_diff returned: %d\n", (int)err);
    ASSERT_MSG(err == TREE_OK, tree_strerror(err));

    tree_dump_graph_at("DERIVATIVE", &t_first_diff, __FILE__, __LINE__, "main");

    tree_destroy(&t);
    tree_destroy(&t_first_diff);

    system("start chrome http://localhost:8000/log.html");
}