#include <stdio.h>
#include <tree_dump.h>

#include "tree_defs.h"
#include "dot_tree_generation.h"

/* ------------------------ выбор функции дампа при падении assert -------------------- */
void tree_dump_hook(const char* name, void* ctx) {
    tree_dump_graph(name, (const Tree*)ctx);
}

/* ---------- дамп узла в обычный .txt файл --------- */
static void rpn_dump_node(FILE* o, const Node* n, size_t* budget) {
    if (!n) {
        fputs("nil", o);
        return;
    }

    if (*budget == 0) { /* цикл или битый size */
        fputs("...", o);
        return;
    }
    (*budget)--;

    fputc('(', o);
    dump_node(o, n->left, budget);
    fputc(' ', o);
    dump_node(o, n->right, budget);
    fprintf(o, " \"");
    print_data(o, n);
    fprintf(o, "\"");
    fprintf(o, ")");
}

/* -------------------- дамп дерева в .txt ----------------- */
void rpn_tree_dump(const char* name, const Tree* t) {
    ASSERT_MSG(name != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));

    FILE* o;
    if (strcmp(name, "stderr") == 0) {
        o = stderr;
    } else if (strcmp(name, "stdout") == 0) {
        o = stdout;
    } else {
        o = fopen(name, "w");
        ASSERT_MSG(o != NULL, tree_strerror(NULL_PTR));
    }

    size_t budget = t->size;
    //fprintf(o, "tree(size = %zu): ", t->size);
    dump_node(o, t->root, &budget);
    fputc('\n', o);

    if (o != stderr && o != stdout) fclose(o);
}

/* ---------------------------------------- дамп дерева в html с записью позиции где произошел вызов -------------------------------------*/
tree_err_t tree_dump_graph_at(const char* base_name, const Tree* tree,
                              const char* call_file, int call_line, const char* call_func) {
    if (tree == NULL || base_name == NULL) return TREE_NULL_PTR;

    char dot[256], png[256];
    snprintf(dot, sizeof(dot), "./logs/ot/%s_ortho.dot", base_name);
    snprintf(png, sizeof(png), "./logs/img/%s_ortho.png", base_name);
    tree_err_t err;

    static int dump_id = 0;
    dump_id++;

    FILE* out = fopen(dot, "w");
    if (out == NULL) return TREE_IO_ERROR;

    FILE* log_file = fopen("log.html", "a");
    if (log_file == NULL) {
        fclose(out);
        return TREE_IO_ERROR;
    }

    const char* mode_str = TREE_GV_MODE;
    fprintf(log_file, "<div class=\"dump-card\">\n");
    fprintf(log_file,
            "  <div class=\"meta\">Dump called from <span class=\"meta-func\">%s()</span> "
            "at <span class=\"meta-file\">%s:%d</span></div>\n",
            call_func, call_file, call_line);
#ifdef TREE_DEBUG
    fprintf(log_file,
            "  <div class=\"meta\">Tree <span class=\"meta-func\">%s</span> created in "
            "<span class=\"meta-func\">%s()</span> at <span class=\"meta-file\">%s:%d</span></div>\n",
            tree->name ? tree->name : "?",
            tree->function ? tree->function : "?",
            tree->file ? tree->file : "?",
            tree->line);
#endif  // печать места создания дерева

    const char* fail_file;
    const char* fail_func;
    const char* fail_expr;
    int fail_line;
    if (assert_get_place(&fail_file, &fail_line, &fail_func, &fail_expr)) {
        fprintf(log_file, "  <div class=\"error\"><b>ASSERT FAILED</b> at %s:%d in %s()<br>condition: <code>",
                fail_file, fail_line, fail_func);
        html_escape(log_file, fail_expr);
        fprintf(log_file, "</code></div>\n");
    }  // печать места падения

    fprintf(log_file, "  <div class=\"meta\"><strong>DUMP #%d</strong> | Mode: <span class=\"meta-func\">%s</span> | File: <span class=\"meta-file\">%s</span></div>\n",
            dump_id, mode_str, png);
    fprintf(log_file, "  <table class=\"stats-table\">\n");
    if (tree->root != NULL) {
        fprintf(log_file,
                "    <tr>"
                "<td class=\"var-name\">Size:</td><td class=\"var-val\">%zu</td>"
                "<td class=\"var-name\">Key of root:</td><td class=\"var-val\">%d</td>"
                "<td class=\"var-name\">Data of root:</td><td class=\"var-val\">",
                tree->size, tree->root->key);
        print_data(log_file, tree->root);
        fprintf(log_file, "</td>");
        if (tree->root->left != NULL) {
            fprintf(log_file,
                    "<td class=\"var-name\">Key of left child:</td><td class=\"var-val\">%d</td>", tree->root->left->key);
        }
        if (tree->root->right != NULL) {
            fprintf(log_file,
                    "<td class=\"var-name\">Key of right child:</td><td class=\"var-val\">%d</td>", tree->root->right->key);
        }
    } else {
        fprintf(log_file, "    <tr><td class=\"var-name\">Size:</td><td class=\"var-val\">0 (Empty tree)</td>\n");
    }
    fprintf(log_file, "</tr>\n");
    fprintf(log_file, "  </table>\n");

    // ============================ заголовок =========================
    fprintf(out, "digraph tree {\n");
    const char* splines = (strcmp(TREE_GV_MODE, "ORTHO") == 0) ? "ortho" : "true";
    double nodesep      = (strcmp(TREE_GV_MODE, "ORTHO") == 0) ? 2.0 : 1.6;
    double ranksep      = (strcmp(TREE_GV_MODE, "ORTHO") == 0) ? 2.5 : 1.5;
    fprintf(out, "  graph [dpi=110, bgcolor=\"" GV_BG
                 "\", splines=%s, pad=0.5,"
                 " nodesep=%.1f, ranksep=%.1f];\n",
            splines, nodesep, ranksep);
    fprintf(out, "  node  [shape=rectangle, style=\"rounded,filled\", fontcolor=\"" GV_TEXT
                 "\","
                 " fontsize=22, width=6.2, height=2.5, fixedsize=true, penwidth=3];\n");
    fprintf(out, "  edge  [penwidth=2, arrowsize=1.0];\n");

    //==================================================================================

    // ============================== ячейки ==========================
    int qur_num = 0;
    err         = node_dump_graph(tree->root, &qur_num, out);
    ASSERT(err == TREE_OK);

    // =========================== стрелки ============================
    qur_num = 0;
    err     = arrow_dump_graph(tree->root, &qur_num, out);
    ASSERT(err == TREE_OK);

    // =========== все ячейки строятся сверху вниз ================
    fprintf(out, "  { rank=TB;");
    for (size_t i = 0; i < tree->size; i++) fprintf(out, " cell%zu;", i);
    fprintf(out, " }\n");

    fprintf(out, "}\n");
    fclose(out);

    // ================================ запуск dot ===================================
    char cmd[1024] = {0};
    snprintf(cmd, sizeof(cmd), "dot -Tpng \"%s\" -o \"%s\"", dot, png);
    if (system(cmd) != 0) {
        fprintf(log_file, "  <div style=\"color: #e06c75; font-weight: bold;\">[ERROR] Graphviz generation failed!</div>\n");
        fprintf(log_file, "</div>\n");  // Закрываем dump-card
        fclose(log_file);
        return TREE_DOT_CANT_OPEN;
    }

    fprintf(log_file, "  <div class=\"graph-container\">\n");
    fprintf(log_file, "    <img src=\"%s\" alt=\"List State\">\n", png);
    fprintf(log_file, "  </div>\n");
    fprintf(log_file, "</div>\n");  // Закрываем dump-card

    fclose(log_file);

    return TREE_OK;
}

/* ----------------------- заменяет спец. символы html на безопасные текстовые коды - html-сущности ------------------------*/
static void html_escape(FILE* o, const char* s) {
    for (; *s; s++) {
        switch (*s) {
            case '&':
                fputs("&amp;", o);
                break;
            case '<':
                fputs("&lt;", o);
                break;
            case '>':
                fputs("&gt;", o);
                break;
            default:
                fputc(*s, o);
        }
    }
}

/* -------------------------- генерация заголовка html файла ------------------------------*/
void tree_log_init() {
    FILE* log = fopen("log.html", "w");
    if (!log) return;

    /* 1 строка - говорит браузеру чт осейчас будет современный HTML5 и его нужно читать по актуальным правилам
       meta - это для текста на русском
       4 строка - задает текст, который будет написан на самой вкладке в браузере рядом с иконкой
       далее в стайле задается правила внешнего вида для тегов:
           выбираем тег боди и пишем условия: цвета окружения текста отступы и прочее
           далее следующий тег - натсройка внешнего вида главного заголовка страницы и самой карточки дампа
           и следующий тег - натсройка дампа самой картинки
           и далее настройка текста внутри карточки  - где и почему вызван дамп, таблички с переменными окраска и прочее
           и завершающий блок - чтобы картинка не ломала верстку и читалась на темном фоне
       */

    fprintf(log,
            "<!DOCTYPE html>\n"
            "<html>\n<head>\n"
            " <meta charset=\"UTF-8\">\n"
            " <title>Tree Lifecycle Log</title>\n"
            "<style>\n"
            "   body {\n"
            "       font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;\n"
            "       background-color: #13161A;\n"
            "       color: #C9D1D9;\n"
            "       padding: 30px;\n"
            "       margin: 0;\n"
            "   }\n"
            "   h1 {\n"
            "       color: #F0F6FC;\n"
            "       border-bottom: 2px solid #30363D;\n"
            "       padding-bottom: 10px;\n"
            "   }\n"
            "   .dump-card {\n"
            "       background-color: #1E232A;\n"
            "       border: 1px solid #30363D;\n"
            "       border-left: 5px solid #58A6FF;\n"
            "       border-radius: 6px;\n"
            "       padding: 20px;\n"
            "       margin-bottom: 25px;\n"
            "       box-shadow: 0 4px 6px rgba(0,0,0,0.3);\n"
            "   }\n"
            "   .meta {\n"
            "       font-size: 0.95em;\n"
            "       color: #85c1e9;\n"
            "       margin-bottom: 10px;\n"
            "   }\n"
            "   .meta-func { color: #dcdcaa; font-weight: bold; }\n"
            "   .meta-file { color: #ce9178; }\n"
            "   .stats-table {\n"
            "       border-collapse: collapse;\n"
            "       margin: 10px;\n"
            "   }\n"
            "   .stats-table td {\n"
            "       padding: 4px 15px 4px 0;\n"
            "   }\n"
            "   .var-name { color: #9cdcfe; font-weight: bold; }\n"
            "   .var-val { color: #b5cea8; }\n"
            "    .graph-container {\n"
            "        background-color: #1B1F24;\n"
            "        border: 1px solid #30363D;\n"
            "        padding: 15px;\n"
            "        border-radius: 4px;\n"
            "        margin-top: 15px;\n"
            "        display: inline-block;\n"
            "        max-width: 100%%;\n"
            "        overflow-x: auto;\n"
            "    }\n"
            "    img {\n"
            "        max-height: 1000px;\n"
            "        display: block;\n"
            "    }\n"
            "       /* темный скроллбар */\n"
            "       ::-webkit-scrollbar {\n"
            "           width: 10px;\n"
            "           height: 10px;\n"
            "       }\n"
            "       ::-webkit-scrollbar-track {\n"
            "           background: #16191E;\n"
            "           border-radius: 4px;\n"
            "       }\n"
            "       ::-webkit-scrollbar-thumb {\n"
            "           background: #38414D;\n"
            "           border-radius: 4px;\n"
            "       }\n"
            "       ::-webkit-scrollbar-thumb:hover {\n"
            "           background: #555555;\n"
            "    }\n"
            "    .error {\n"
            "        background-color: #3a1d1d;\n"
            "        border-left: 5px solid #f85149;\n"
            "        color: #ffb4ae;\n"
            "        padding: 10px 15px;\n"
            "        margin: 15px 0;\n"
            "        border-radius: 4px;\n"
            "   }\n"
            "</style>\n"
            "</head>\n<body>\n<h1>Tree Execution History</h1>\n");
    fclose(log);
}