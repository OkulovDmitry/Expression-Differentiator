/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* 6. Чтение из файла                                                                                                        */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */

#include "tree_io.h"

#include <stdio.h>

#include "stack.h"
#include "tree_defs.h"
#include "tree_errors.h"

/*============================================================================================================================
  ============================================================================================================================
  ============================================================================================================================
  ============================================================================================================================
                                                      RPN чтение                                                            
  ============================================================================================================================
  ============================================================================================================================
  ============================================================================================================================
  ============================================================================================================================*/

/* ----------------------- обработка узла из файла ----------------------- */
tree_err_t rpn_node_take(FILE* in, char* word, int* num_open, int* num_close, int* is_nil) {
    ASSERT_MSG(in != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(word != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(num_open != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(num_close != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(is_nil != NULL, tree_strerror(NULL_PTR));

    int c = fgetc(in);  // то есть остановились в прошлый раз на \0

    while (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
        c = fgetc(in);
    }

    while (c == ')') {
        (*num_close)++;
        c = fgetc(in);
    }

    while (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
        c = fgetc(in);
    }

    while (c == '(') {
        c = fgetc(in);
        (*num_open)++;
    }

    while (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
        c = fgetc(in);
    }

    int len = 0;
    *is_nil = 0;
    if (c == '"') {
        c = fgetc(in);
        while (c != '"' && c != EOF && len < MAX_NODE_LEN - 1) {
            word[len++] = (char)c;
            c           = fgetc(in);
        }
        if (c != '"') return DOUBLE_QUOTE_MISSING;
        word[len] = '\0';
    } else {
        while (c != ' ' && c != ')' && c != '(' && c != '\n' && c != '\r' && c != EOF && len < MAX_NODE_LEN - 1) {
            word[len++] = (char)c;
            c           = fgetc(in);
        }
        word[len] = '\0';
        if (strcmp("nil", word) != 0) return NILL_MISSING;

        *is_nil = 1;

        if (c != EOF) ungetc(c, in);
    }

    return TREE_OK;
}

/* --------------------------------- запись узла из файла в стек --------------------------------- */
tree_err_t rpn_node_add_from_file(FILE* in, my_stack_t* nodes, int* num_open, int* num_close) {
    ASSERT_MSG(in != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(nodes != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(num_open != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(num_close != NULL, tree_strerror(NULL_PTR));

    char word[MAX_NODE_LEN];
    int is_nil;
    tree_err_t err = node_take(in, word, num_open, num_close, &is_nil);
    if (err != TREE_OK) return TREE_IO_ERROR;
    if (is_nil) {
        if (stack_push(nodes, NULL) != STACK_OK) return ERROR_STACK_PUSH_IN_RPN;
        return node_add_from_file(in, nodes, num_open, num_close);
    }

    Elem_t right_child;
    Elem_t left_child;
    if (stack_pop(nodes, &right_child) != STACK_OK) return ERROR_RIGHT_CHILD_IN_RPN_NOT_FOUND;
    if (stack_pop(nodes, &left_child) != STACK_OK) return ERROR_LEFT_CHILD_IN_RPN_NOT_FOUND;

    Node* new_node = malloc(sizeof(Node));
    if (!new_node) return TREE_NO_MEM;
    new_node->left             = left_child;
    new_node->right            = right_child;
    new_node->head             = NULL;
    new_node->key              = 0;
    new_node->type             = STRING_TYPE;
    new_node->data.string_data = strdup(word);
    if (right_child) right_child->head = new_node;
    if (left_child) left_child->head = new_node;

    stack_push(nodes, new_node);
    int c = fgetc(in);
    if (c != ')') {
        printf("Expected ')', got '%c' (ASCII %d) after word '%s'\n", c, c, word);
        return TREE_CORRYPTED;
    }
    (*num_close)++;

    c = fgetc(in);
    while (c == ')' || c == ' ' || c == '\n' || c == '\r' || c == '\t') {
        if (c == ')') (*num_close)++;
        c = fgetc(in);
    }

    if (c == EOF) return TREE_OK;
    ungetc(c, in);
    return node_add_from_file(in, nodes, num_open, num_close);
}

/* ---------------------- главная функция обработки файла с rpn записью дерева ------------------------ */
tree_err_t read_rpn_tree(Tree* t, const char* name) {
    ASSERT(name != NULL);
    FILE* in = fopen(name, "r");
    ASSERT(in != NULL);

    int c = fgetc(in);
    if (c != '(') {
        printf("Expected '(', got '%c' (ASCII %d) in begin of file %s'\n", c, c, name);
        return TREE_CORRYPTED;
    }

    int num_open  = 1;
    int num_close = 0;

    my_stack_t nodes;
    err_t stack_err = stack_ctor(&nodes, MAX_NODE_LEN);
    if (stack_err != STACK_OK) return TREE_CORRYPTED;

    tree_err_t err = node_add_from_file(in, &nodes, &num_open, &num_close);
    if (err != TREE_OK) return err;

    stack_err = stack_pop(&nodes, &(t->root));
    if (stack_err != STACK_OK) return TREE_CORRYPTED;

    t->size = num_open;
    fclose(in);

    tree_renumber(t);
    tree_err_t verify_err = tree_verify(t);
    ASSERT_MSG(verify_err == TREE_OK, strerror(verify_err));

    return TREE_OK;
}

/*============================================================================================================================
  ============================================================================================================================
  ============================================================================================================================
  ============================================================================================================================
                                                      Инфиксное чтение                                                            
  ============================================================================================================================
  ============================================================================================================================
  ============================================================================================================================
  ============================================================================================================================*/

void skip_whitespace(char** pos) {
    int c = *pos;

    while (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
        (*pos)++;
        c = *pos;
    }
}

/* ----------------------- рекурсивная обработка узлов из файла ----------------------- */
static void infix_nodes_take(char** pos, char* word, Tree* t, Node* node, char** node_data, size_t* tree_size) {
    ASSERT_MSG(pos != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(word != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));

    (*pos)++;
    int c = *pos;  // то есть остановились в прошлый раз на \0 или на (
    skip_whitespace(pos);

    int len = 0;
    if (c != '(') {  // должно быть nil
        node_data = pos;
        while (c != ' ' && c != ')' && c != '(' && c != '\n' && c != '\r' && c != '\t' && c != EOF && len < MAX_NODE_LEN - 1) {
            word[len++] = (char)c;
            (*pos)++;
            c = *pos;
        }
        word[len] = '\0';
        ASSERT_MSG(strcmp("nil", word) == 0, strerror(NILL_MISSING));
        return;
    } else {  // значит новый узел
        Node* new_node = сalloc(1, sizeof(Node));
        infix_node_take(pos, word, t, new_node->left, node_data, tree_size);  // указатель остановится на \0 - заменили кавычку левого ребенка на \0
        (*pos)++;
        c = *pos;
        skip_whitespace(pos);
        c = *pos;
        ASSERT_MSG(c == '"', strerror(DOUBLE_QUOTE_MISSING));
        (*pos)++;
        c         = *pos;
        node_data = pos;
        while (c != ' ' && c != ')' && c != '(' && c != '\n' && c != '\r' && c != '\t' && c != EOF && len < MAX_NODE_LEN - 1) {
            word[len++] = (char)c;
            (*pos)++;
            c = *pos;
        }
        word[len] = '\0';
        ASSERT_MSG(c == '"', strerror(DOUBLE_QUOTE_MISSING));
        *pos                       = '\0';  // перезапись второй " на \0 в буфере
        new_node->data.string_data = *node_data;
        infix_node_take(pos, word, t, new_node->right, node_data, tree_size);
        (*pos)++;
        c = *pos;
        ASSERT_MSG(c == ')', strerror(ERROR_MISMATCHED_BRACKETS));
        new_node->head = node;
        new_node->type = STRING_TYPE;
        new_node->key  = 0;
        (*tree_size)++;
        return;
    }
}

tree_err_t infix_read(Tree* t, const char* name) {
    ASSERT(name != NULL);
    FILE* in = fopen(name, "r");
    ASSERT(in != NULL);
    ASSERT_MSG(getline(t->buffer, t->buf_used, in) > 0, strerror(ERROR_NULL_FILE));
    fclose(in);
    if (strlen(t->buffer) > t->buf_cap) t->buf_cap = t->buf_used;

    char** pos = &(t->buffer[0]);

    int c = *pos;
    if (c != '(') {
        printf("Expected '(', got '%c' (ASCII %d) in begin of file %s'\n", c, c, name);
        return TREE_CORRYPTED;
    }

    char word[MAX_NODE_LEN];
    char** node_data = pos;

    infix_nodes_take(pos, word, t, t->root, node_data, t->size);
    t->var_table = NULL;

    tree_renumber(t);
    tree_err_t verify_err = tree_verify(t);
    ASSERT_MSG(verify_err == TREE_OK, strerror(verify_err));

    return TREE_OK;
}