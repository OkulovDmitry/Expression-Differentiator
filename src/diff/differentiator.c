#include "differentiator.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "my_assert.h"
#include "tree_defs.h"

/*
Принцип работы:
  Парсер юзает рекурсивный спуск, где каждая функция
  отвечает за свой уровень приоритета операций:
  
    parse_expr()   - обрабатывает + и - (низший приоритет, левоассоциативность
    вся формула целиком как сумма или разность отдельных слагаемых - A + B - C)

    parse_term()   - обрабатывает * и / (средний приоритет,
    отдельный кусок между + и -, состоящий из произведения или частного A / B * C)

    parse_unary()  - обрабатывает унарный +/- (знак + или - перед выражением у которого только один операнд)

    parse_power()  - возведение в степень которое вычисляется раньше умножения и деления (высший приоритет, правоассоциативность)
    parse_factor() - обрабатывает числа, переменные, функции, скобки - вообщем как бы неделимые части

    По сути unary над power чтобы -x^2 дало -(x^2), а не (-x)^2
    А вpower справа unary чтобы работало 2^-3
  
    Когда парсер видит "x^2 + sin(x) - 1", он:
    1. Вызывает parse_expr() -> тот вызывает parse_term()
    2. parse_term() вызывает parse_unary() -> parse_power() -> parse_factor()
    3. parse_factor() читает "x", возвращает узел
    4. parse_power() видит "^", вызывает parse_unary() -> parse_factor() -> читает "2"
    5. parse_power() создаёт узел ^(x, 2), возвращает в parse_term()
    6. parse_term() не видит "*" или "/", возвращает ^(x, 2) в parse_expr()
    7. parse_expr() видит "+", вызывает parse_term() для правой части
    8. ...
  
    Как итог - скобки нужны только для явного изменения порядка
*/

typedef struct {
    char* pos;  // текущая позиция в t->buffer
    Tree* tree;
    tree_err_t err;  // первая возникшая ошибка
} Parser;

static void skip_ws(Parser* p) {
    while (*p->pos == ' ' || *p->pos == '\n' || *p->pos == '\r' || *p->pos == '\t')
        p->pos++;
}

static bool is_unary_operation(char* op) {
    ASSERT_MSG(op != NULL, tree_strerror(NULL_PTR));

    for (int i = 0; i < NUMBER_OF_OPERATIONS; i++) {
        if (!strcmp(op, op_info[i].operation_ord) && op_info[i].is_unary) return true;
    }
    return false;
}

/* ----- var_name_ok - проверяет валидность имени переменной ----- */
// имя начинается на _ или букву, далее буквы и цифры и _
static bool var_name_ok(const char* name) {
    ASSERT_MSG(name != NULL, strerror(NULL_PTR));

    if (!(isalpha((unsigned char)name[0])) && name[0] != '_') return false;
    for (const char* p = name; *p; p++)
        if (!(isalnum((unsigned char)*p)) && *p != '_') return false;

    return true;
}

static tree_err_t resize_up_var_table(VarTable* table) {
    ASSERT_MSG(table != NULL, strerror(NULL_PTR));

    size_t new_cap = 2 * table->capacity + 1;
    if (new_cap > MAX_VARS) return TREE_VARS_OVERFLOW;

    VarTable* temp = realloc(table->vars, new_cap);
    if (temp == NULL) return TREE_VARS_OVERFLOW;

    table->vars     = temp;
    table->capacity = new_cap;

    fill_vartable_poison(table, table->size);

    return TREE_OK;
}

static bool ensure_var_exists(const char* var, VarTable* table, size_t* var_pos) {
    ASSERT_MSG(var != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(table != NULL, tree_strerror(NULL_PTR));

    for (size_t i = 0; i < table->size; i++) {
        if (!strcmp(var, (table->vars[i]).name)) {  // переменная уже существует в таблице переменных
            var_pos = i;
            return true;
        }
    }
    if (var_name_ok(var)) {  // переменная не существует но ее имя подразумевает новую переменную в VarTable
        if (table->size == table->capacity) {
            tree_err_t err = resize_up_var_table(table);
            if (err != TREE_OK) return err;
        }

        snprintf(table->vars[table->size].name, (size_t)(MAX_VAR_LEN), var);
        var_pos = table->size;
        table->size++;
        return true;
    }

    return false;
}

// первым может идти + или -, далее только цифры
static bool is_const(const char* word) {
    char* endptr;
    double constanta = strtod(word, &endptr);
    if (endptr != '\0') return false;
    return true;
}

static void create_node(Node* n, int key, NodeType type, NodeElem elem_data, Node* head, Node* left, Node* right) {
    n->key   = key;
    n->type  = type;
    n->data  = elem_data;
    n->left  = left;
    n->right = right;
}

/* ----------------------- рекурсивная обработка узлов из файла ----------------------- */
static tree_err_t diff_nodes_take(char** pos, char* word, Tree* t, Node* node, char** node_data, size_t* tree_size) {
    ASSERT_MSG(pos != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(word != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));

    (*pos)++;
    int c = *pos;  // то есть остановились в прошлый раз на \0 или на (
    diff_skip_whitespace(pos);

    int len = 0;
    if (c != '(') {  // значит должна быть операция
        node_data = pos;
        while (c != ' ' && c != ')' && c != '(' && c != '\n' && c != '\r' && c != '\t' && c != EOF && len < MAX_NODE_LEN - 1) {
            word[len++] = (char)c;
            (*pos)++;
            c = *pos;
        }
        word[len] = '\0';
        ASSERT_MSG(strcmp("nil", word) == 0, strerror(NILL_MISSING));
        return;
    } else {  // перед нами левое поддерево унарной операции или переменная или константа
        (*pos)++;
        c = *pos;  // скипнули (
        diff_skip_whitespace(pos);
        c = *pos;
        while (c != ' ' && c != ')' && c != '(' && c != '\n' && c != '\r' && c != '\t' && c != EOF && len < MAX_NODE_LEN - 1) {
            word[len++] = (char)c;
            (*pos)++;
            c = *pos;
        }
        word[len] = '\0';
        diff_skip_whitespace(pos);
        size_t var_pos;
        OpType op_type;

        if (is_unary_operation(word)) {  // создание нового узла с правым ребенком и рекурсиный спуск в правое поддерево
            NodeElem elem_data;

        } else if (ensure_var_exists(word, t->var_table, &var_pos)) {  // создать узел переменную без детей
            NodeElem elem_data;
            elem_data.var_data = var_pos;
            Node* new_node     = calloc(1, sizeof(Node));
            create_node(new_node, 0, VAR_TYPE, elem_data, node, NULL, NULL);
            return TREE_OK;
        } else if (is_const(word)) {  // создать узел кнстанту без детец
            double constanta = strtod(word, NULL);
            NodeElem elem_data;
            elem_data.num_data = constanta;
            Node* new_node     = calloc(1, sizeof(Node));
            create_node(new_node, 0, NUM_TYPE, elem_data, node, NULL, NULL);
            return TREE_OK;

        } else {
            return ERROR_INVALID_NODE_CONTENT;
        }
    }
}

tree_err_t diff_read(Tree* t, const char* name) {
    ASSERT(name != NULL);
    FILE* in = fopen(name, "r");
    ASSERT(in != NULL);
    ASSERT_MSG(getline(t->buffer, t->buf_used, in) > 0, strerror(ERROR_NULL_FILE));
    fclose(in);
    if (strlen(t->buffer) > t->buf_cap) t->buf_cap = t->buf_used;

    char** pos = &(t->buffer[0]);

    char word[MAX_NODE_LEN];
    char** node_data = pos;

    diff_nodes_take(pos, word, t, t->root, node_data, t->size);

    tree_renumber(t);
    tree_err_t verify_err = tree_verify(t);
    ASSERT_MSG(verify_err == TREE_OK, strerror(verify_err));

    return TREE_OK;
}