#include "differentiator_read.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "my_assert.h"
#include "tree_core.h"
#include "tree_defs.h"
#include "tree_ops.h"
#include "tree_traverse.h"

/*
Принцип работы читалки:
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

/* ----- var_name_ok - проверяет валидность имени переменной ----- */
// имя начинается на _ или букву, далее буквы и цифры и _
static bool var_name_ok(const char* name) {
    ASSERT_MSG(name != NULL, strerror(NULL_PTR));

    if (!(isalpha((unsigned char)name[0])) && name[0] != '_') return false;
    for (const char* p = name; *p; p++)
        if (!(isalnum((unsigned char)*p)) && *p != '_') return false;

    return true;
}

static tree_err_t var_get_or_add(VarTable* table, const char* name, size_t* idx) {
    if (!table || !name || !idx) return TREE_NULL_PTR;

    for (size_t i = 0; i < table->size; i++) {
        if (strcmp(name, (table->vars[i]).name) == 0) {  // переменная уже существует в таблице переменных
            *idx = i = i;
            return TREE_OK;
        }
    }

    if (strlen(name) >= MAX_VAR_LEN) return TREE_VAR_WRONG_FORMAT;

    if (var_name_ok(name)) {  // переменная не существует но ее имя подразумевает новую переменную в VarTable
        if (table->size >= table->capacity) {
            size_t new_cap = table->capacity * 2 + 1;
            if (new_cap > MAX_VARS) new_cap = MAX_VARS;
            if (new_cap <= table->capacity) return TREE_VARS_OVERFLOW;
            Variable* tmp = realloc(table->vars, new_cap * sizeof(Variable));
            if (!tmp) return TREE_NO_MEM;
            table->vars     = tmp;
            table->capacity = new_cap;
            fill_vartable_poison(table, table->size);
        }

        snprintf(table->vars[table->size].name, (size_t)(MAX_VAR_LEN), name);
        table->vars[table->size].value = VAR_POISON;
        *idx                           = table->size;
        table->size++;
        return TREE_OK;
    }

    return TREE_VAR_WRONG_FORMAT;
}

static Node* create_new_node(Parser* p, NodeType type, NodeElem data, Node* left, Node* right) {
    Node* n = calloc(1, sizeof(Node));
    if (!n) {
        p->err = TREE_NO_MEM;
        tree_node_destroy(left);
        tree_node_destroy(right);
        return NULL;
    }
    n->key   = 0;
    n->type  = type;
    n->data  = data;
    n->left  = left;
    n->right = right;
    n->head  = NULL;
    if (left) left->head = n;
    if (right) right->head = n;
    p->tree->size++;
    return n;
}

static bool find_unary_func(const char* word, OpType* op) {
    for (int i = 0; i < NUMBER_OF_OPERATIONS; i++)
        if (op_info[i].max_children == 1 && strcmp(word, op_info[i].operation_ord) == 0) {
            *op = (OpType)i;
            return true;
        }
    return false;
}

static Node* parse_expr(Parser* p);

static Node* parse_factor(Parser* p) {
    skip_ws(p);
    char c = *p->pos;

    if (c == '(') {  // перед нами выражение в скобочках
        p->pos++;
        Node* inner = parse_expr(p);
        if (!inner) return NULL;
        skip_ws(p);
        if (*p->pos != ')') {
            p->err = TREE_CORRYPTED;
            tree_node_destroy(inner);
            return NULL;
        }
        p->pos++;
        return inner;
    }

    if (isdigit((unsigned char)c) || c == '.') {  // перед нами константа
        char* end = NULL;
        double v  = strtod(p->pos, &end);  // как оказалось strtod и числа .5 умеет разбирать
        if (end == p->pos) {
            p->err = ERROR_INVALID_NODE_CONTENT;
            return NULL;
        }
        p->pos     = end;
        NodeElem d = {.num_data = v};
        return create_new_node(p, NUM_TYPE, d, NULL, NULL);
    }

    if (isalpha((unsigned char)c) || c == '_') {  // перед нами должна быть переменная дальше бога нет
        char word[MAX_VAR_LEN];
        size_t len = 0;
        while (isalnum((unsigned char)*p->pos) || *p->pos == '_') {
            if (len + 1 >= MAX_VAR_LEN) {
                p->err = TREE_VAR_WRONG_FORMAT;
                return NULL;
            }
            word[len++] = *p->pos++;
        }
        word[len] = '\0';

        OpType op;
        if (find_unary_func(word, &op)) {
            skip_ws(p);
            if (*p->pos != '(') {
                p->err = TREE_CORRYPTED;
                return NULL;
            }  // sin/cos/... без ( - ошибка
            p->pos++;
            Node* arg = parse_expr(p);
            if (!arg) return NULL;
            skip_ws(p);
            if (*p->pos != ')') {
                p->err = TREE_CORRYPTED;
                tree_node_destroy(arg);
                return NULL;
            }
            p->pos++;
            NodeElem d = {.op_data = op};
            return create_new_node(p, OP_TYPE, d, NULL, arg);  // аргумент в right
        }

        size_t idx = 0;
        p->err     = var_get_or_add(p->tree->var_table, word, &idx);
        if (p->err != TREE_OK) return NULL;
        NodeElem d = {.var_data = idx};
        return create_new_node(p, VAR_TYPE, d, NULL, NULL);
    }

    p->err = ERROR_INVALID_NODE_CONTENT;
    return NULL;
}

static Node* parse_unary(Parser* p);

static Node* parse_power(Parser* p) {
    Node* base = parse_factor(p);  // это наше основание степени
    if (!base) return NULL;
    skip_ws(p);
    if (*p->pos != '^') return base;
    p->pos++;
    Node* _exp = parse_unary(p);  // это наща степень и мой любимый мувчик который позволит читать как 2^-2 так и обеспечит правоассоциативность
    if (!_exp) {
        tree_node_destroy(base);
        return NULL;
    }
    NodeElem d = {.op_data = OP_POW};
    return create_new_node(p, OP_TYPE, d, base, _exp);
}

static Node* parse_unary(Parser* p) {
    skip_ws(p);
    char c = *p->pos;
    if (c == '+' || c == '-') {
        p->pos++;
        Node* arg = parse_unary(p);
        if (!arg) return NULL;
        if (c == '+') return arg;  // унарный плюс бесполезен и не входит в дерево
        NodeElem d = {.op_data = OP_SUB};
        return create_new_node(p, OP_TYPE, d, NULL, arg);  // унарный минус - left = NULL
    }
    return parse_power(p);
}

static Node* parse_term(Parser* p) {
    Node* left = parse_unary(p);
    if (!left) return NULL;

    while (true) {
        skip_ws(p);
        char c = *p->pos;
        if (c != '*' && c != '/') break;
        p->pos++;
        Node* right = parse_unary(p);
        if (!right) {
            tree_node_destroy(left);
            return NULL;
        }
        NodeElem d = {.op_data = (c == '*') ? OP_MUL : OP_DIV};
        left       = create_new_node(p, OP_TYPE, d, left, right);
        if (!left) return NULL;
    }
    return left;
}

static Node* parse_expr(Parser* p) {
    Node* left = parse_term(p);
    if (!left) return NULL;

    while (true) {
        skip_ws(p);
        char c = *p->pos;
        if (c != '-' && c != '+') break;
        p->pos++;
        Node* right = parse_term(p);
        if (!right) {
            tree_node_destroy(left);
            return NULL;
        }
        NodeElem d = {.op_data = (c == '+') ? OP_ADD : OP_SUB};
        left       = create_new_node(p, OP_TYPE, d, left, right);
        if (!left) return NULL;
    }
    return left;
}

tree_err_t diff_read(Tree* t, const char* name) {
    if (!t || !name) return TREE_NULL_PTR;

    FILE* in = fopen(name, "rb");
    if (!in) return ERROR_NULL_FILE;

    if (fseek(in, 0, SEEK_END) != 0) {
        fclose(in);
        return TREE_IO_ERROR;
    }
    long sz = ftell(in);
    if (sz < 0) {
        fclose(in);
        return TREE_IO_ERROR;
    }
    rewind(in);
    if (sz == 0) {
        fclose(in);
        return ERROR_NULL_FILE;
    }
    if ((size_t)sz + 1 > TREE_MAX_BUF_CAP) {
        fclose(in);
        return TREE_BUFFER_CORRYPTED;
    }

    tree_destroy(t);
    tree_err_t err = TREE_INIT(t);
    if (err != TREE_OK) {
        fclose(in);
        return err;
    }

    while ((size_t)sz + 1 > t->buf_cap) {
        err = resize_up(t);
        if (err != TREE_OK) {
            fclose(in);
            return err;
        }
    }  // создаем буфер под файл

    size_t got = fread(t->buffer, 1, (size_t)sz, in);
    fclose(in);  // принимаем буфер

    t->buf_used = got;
    memset(t->buffer + got, 0, t->buf_cap - got);  // обнуляем хвост буфера

    Parser p = {.pos = t->buffer, .tree = t, .err = TREE_OK};
    t->root  = parse_expr(&p);

    if (p.err == TREE_OK) {
        skip_ws(&p);
        if (*p.pos != '\0') p.err = TREE_CORRYPTED;
        if (!t->root) p.err = ERROR_NULL_FILE;
    }
    if (p.err != TREE_OK) {
        tree_node_destroy(t->root);
        t->root = NULL;
        t->size = 0;
        return p.err;
    }

    tree_renumber(t);
    return tree_verify(t);
}