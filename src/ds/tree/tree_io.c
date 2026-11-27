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
                                                      Инфиксное чтение                                                            
  ============================================================================================================================
  ============================================================================================================================
  ============================================================================================================================
  ============================================================================================================================*/

void skip_whitespace(char** pos) {
    int c = **pos;

    while (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
        (*pos)++;
        c = **pos;
    }
}

/* ----------------------- рекурсивная обработка узлов из файла ----------------------- */
static tree_err_t infix_node_take(char** pos, char* word, Tree* t, Node** node, size_t* tree_size) {
    ASSERT_MSG(pos != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(word != NULL, tree_strerror(NULL_PTR));
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));

    skip_whitespace(pos);
    char c = **pos;

    if (c == '\0') {
        return TREE_CORRYPTED;  // конец файла не должен быть на данном этапе
    }

    int len = 0;

    if (c != '(') {  // должно быть nil
        while (c != ' ' && c != ')' && c != '(' && c != '\n' && c != '\r' && c != '\t' && c != '\0' && len < MAX_NODE_LEN - 1) {
            word[len++] = (char)c;
            (*pos)++;
            c = **pos;
        }
        word[len] = '\0';

        if (strcmp(word, "nil") == 0) {
            *node = NULL;  // лист дерева
            return TREE_OK;
        } else {
            return ERROR_INVALID_NODE_CONTENT;  // ожидали nil
        }
    } else {       // значит новый узел
        (*pos)++;  // пропускаем (
        skip_whitespace(pos);

        // Читаем левое поддерево
        Node* left_child = NULL;
        tree_err_t err   = infix_nodes_take(pos, word, t, &left_child, tree_size);
        if (err != TREE_OK) return err;

        skip_whitespace(pos);
        c = **pos;

        if (c == ' ' || c == '\t') {  // пропуск пробела меж левым ребенком и родителем
            (*pos)++;
            skip_whitespace(pos);
            c = **pos;
        }

        len = 0;
        while (c != ' ' && c != ')' && c != '(' && c != '\n' && c != '\r' && c != '\t' && c != '\0' && len < MAX_NODE_LEN - 1) {
            word[len++] = (char)c;
            (*pos)++;
            c = **pos;
        }
        word[len] = '\0';

        size_t need = strlen(word) + 1;
        while (t->buf_used + need > t->buf_cap) {
            err = resize_up(t);
            if (err != TREE_OK) return err;
        }
        char* dst = t->buffer + t->buf_used;
        memcpy(dst, word, need);
        t->buf_used += need;

        Node* new_node = calloc(1, sizeof(Node));
        if (!new_node) return TREE_NO_MEM;

        new_node->type             = STRING_TYPE;
        new_node->data.string_data = dst;
        new_node->key              = 0;
        new_node->left             = left_child;
        if (left_child) left_child->head = new_node;

        skip_whitespace(pos);
        c = **pos;

        if (c == ' ' || c == '\t') {  // пропуск пробела меж родителем и правым ребенком
            (*pos)++;
            skip_whitespace(pos);
            c = **pos;
        }

        // читаем правое поддерево
        Node* right_child = NULL;
        err               = infix_nodes_take(pos, word, t, &right_child, tree_size);
        if (err != TREE_OK) {
            free(new_node);
            return err;
        }

        new_node->right = right_child;
        if (right_child) right_child->head = new_node;

        skip_whitespace(pos);
        c = **pos;

        // Пропускаем закрывающую скобку
        if (c == ')') {
            (*pos)++;
        } else {
            free(new_node);
            return TREE_CORRYPTED;  // ожидали )
        }

        *node = new_node;
        (*tree_size)++;
        return TREE_OK;
    }
}

tree_err_t infix_read(Tree* t, const char* name) {
    if (!t || !name) return TREE_NULL_PTR;

    FILE* in = fopen(name, "r");
    if (!in) return ERROR_NULL_FILE;

    // Узнаём размер файла
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

    // Сбрасываем дерево до чтения
    tree_destroy(t);
    tree_err_t err = TREE_INIT(t);
    if (err != TREE_OK) {
        fclose(in);
        return err;
    }

    // Выделяем буфер под файл
    while ((size_t)sz + 1 > t->buf_cap) {
        err = resize_up(t);
        if (err != TREE_OK) {
            fclose(in);
            return err;
        }
    }

    // Читаем файл в буфер
    size_t got = fread(t->buffer, 1, (size_t)sz, in);
    fclose(in);

    t->buf_used = got;
    memset(t->buffer + got, 0, t->buf_cap - got);  // зануляем хвост

    char* pos = t->buffer[0];
    char word[MAX_NODE_LEN];
    size_t tree_size = 0;

    err = infix_nodes_take(&pos, word, t, &(t->root), &tree_size);
    if (err != TREE_OK) {
        tree_node_destroy(t->root);
        t->root = NULL;
        t->size = 0;
        return err;
    }
    t->size = tree_size;

    skip_whitespace(&pos);  // проверка на наличие мусора после
    if (*pos != '\0') {
        tree_node_destroy(t->root);
        t->root = NULL;
        t->size = 0;
        return TREE_CORRYPTED;
    }

    if (!t->root) {
        return ERROR_NULL_FILE;
    }

    tree_renumber(t);
    return tree_verify(t);
}