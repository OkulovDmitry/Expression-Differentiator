#include "tree_core.h"

#include <stdlib.h>

#include "my_assert.h"
#include "tree_defs.h"
#include "tree_dump.h"
#include "tree_ops.h"
#include "tree_traverse.h"
#include "tree_validate.h"

/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* 4. Создание, вставка, удаление                                                                                            */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */
/* ========================================================================================================================= */

static void destroy_node(Node* n) {
    if (!n) return;
    destroy_node(n->left);
    destroy_node(n->right);
    free(n);
}

/* ---- Разрушение узла ---- */
void tree_node_destroy(Node* n) {
    if (!n) return;
    destroy_node(n->left);
    destroy_node(n->right);
    free(n);
}

/* ---- Заполнение буфера '\0' ---- */
void fill_poison(Tree* t, size_t from) {
    for (size_t i = from; i < t->buf_cap; i++) t->buffer[i] = '\0';
}

/* ---- Заполнение части таблицы меток POISON ---- */
void fill_vartable_poison(VarTable* table, size_t from) {
    ASSERT_MSG(table != NULL, strerror(NULL_PTR));

    for (size_t i = from; i < table->capacity; i++) {
        snprintf((table->vars[i]).name, (size_t)(MAX_VAR_LEN), "POISON%u", (unsigned int)i);
        (table->vars[i]).value = VAR_POISON;
    }
}

/* ---- Инициализация дерева ---- */
tree_err_t tree_init(Tree* t TREE_PLACE_IN) {
    if (!t) return TREE_NULL_PTR;

    t->root     = NULL;
    t->size     = 0;
    t->buf_used = 0;
    t->buf_cap  = 800;
    t->buffer   = calloc(1, t->buf_cap);
    if (!t->buffer) return TREE_NO_MEM;
    fill_poison(t, t->buf_used);

    t->var_table = calloc(1, sizeof(VarTable));
    if (!t->var_table) {
        free(t->buffer);
        t->buffer = NULL;
        return TREE_NO_MEM;
    }

    t->var_table->capacity = MAX_VARS;
    t->var_table->size     = 0;
    t->var_table->vars     = calloc(MAX_VARS, sizeof(Variable));

    if (!t->var_table->vars) {
        free(t->var_table);
        t->var_table = NULL;
        free(t->buffer);
        t->buffer = NULL;
        return TREE_NO_MEM;
    }
    fill_vartable_poison(t->var_table, 0);

#ifdef TREE_DEBUG
    t->name     = name;
    t->function = function;
    t->file     = file;
    t->line     = line;
#endif

    return tree_verify(t);
}

/* ---- Освобождает все узлы - обход снизу вверх ---- */
void tree_destroy(Tree* t) {
    if (!t) return;
    tree_node_destroy(t->root);
    t->root = NULL;
    t->size = 0;
    free(t->buffer);
    t->buffer   = NULL;
    t->buf_cap  = 0;
    t->buf_used = 0;
    if (t->var_table) {
        free(t->var_table->vars);
        free(t->var_table);
        t->var_table = NULL;
    }
#ifdef TREE_DEBUG
    t->file     = NULL;
    t->function = NULL;
    t->line     = 0;
    t->name     = NULL;
#endif
}

/* ---- Вставка узла ---- */
tree_err_t tree_insert(Tree* t, int key, int type, NodeElem data, Node** out_node) {
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));

    if (type >= NUMBER_OF_TYPES) return TREE_WRONG_NODE_TYPE;

    Node* parent = NULL;
    Node** pp    = &t->root;
    while (*pp) {
        parent = *pp;
        if (key == parent->key) return TREE_DUPLICATE;
        pp = (key < parent->key) ? &parent->left : &parent->right;
    }

    Node* n = calloc(1, sizeof(*n));
    if (!n) return TREE_NO_MEM;

    if (type == STRING_TYPE) {
        const char* src = data.string_data ? data.string_data : "POISON";
        size_t need     = strlen(src) + 1;
        while (t->buf_used + need > t->buf_cap) {
            tree_err_t err = resize_up(t);
            if (err != TREE_OK) {
                free(n);
                return err;
            }
        }
        char* dst = t->buffer + t->buf_used;
        memcpy(dst, src, need);
        t->buf_used += need;
        data.string_data = dst;
    }

    if (type == VAR_TYPE) {
        if (t->var_table->size >= t->var_table->capacity) {
            free(n);
            return TREE_VARS_OVERFLOW;
        }

        if (data.var_data >= t->var_table->size) {
            free(n);
            return TREE_VAR_WRONG_FORMAT;
        }
    }

    if (type == OP_TYPE && (data.op_data < 0 || data.op_data >= NUMBER_OF_OPERATIONS)) {
        free(n);
        return TREE_WRONG_OP_CODE;
    }

    if (type == NON_TYPE) {
        data.string_data = NULL;
    }

    *n  = (Node){.key = key, .type = type, .data = data, .head = parent};
    *pp = n;
    t->size++;

    ASSERT_DUMP(tree_verify(t) == TREE_OK, tree_dump_hook, t);
    if (out_node) *out_node = n;
    return TREE_OK;
}

static void update_string_ptrs(Node* n, ptrdiff_t delta) {
    if (!n) return;
    if (n->type == STRING_TYPE && n->data.string_data) {
        n->data.string_data += delta;
    }
    update_string_ptrs(n->left, delta);
    update_string_ptrs(n->right, delta);
}

tree_err_t resize_up(Tree* t) {
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));

    size_t new_cap = 2 * t->buf_cap + 1;
    if (new_cap > TREE_MAX_BUF_CAP) return TREE_NO_MEM;

    char* old_buffer = t->buffer;
    char* temp       = realloc(t->buffer, new_cap);
    if (!temp) return TREE_NO_MEM;

    t->buffer  = temp;
    t->buf_cap = new_cap;

    ptrdiff_t delta = t->buffer - old_buffer;
    if (delta != 0) {
        update_string_ptrs(t->root, delta);
    }  // тут я перевешиваю все указатели на строки

    fill_poison(t, t->buf_used);

    tree_verify(t);
    return TREE_OK;
}

/* ---- Поиск узла ---- */
Node* tree_find(const Tree* t, int key) {
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));

    Node* cur = t->root;
    while (cur && key != cur->key) {
        cur = (key < cur->key) ? cur->left : cur->right;
    }
    return cur;
}

/* ---- Удаление узла ---- */
tree_err_t tree_remove_node(Tree* t, Node* n) {
    if (!t || !n) return TREE_NULL_PTR;

    if (!n->left)
        transplant(t, n, n->right); /* нет детей или только правый */
    else if (!n->right)
        transplant(t, n, n->left);    /* только левый */
    else {                            /* 2 ребенка */
        Node* s = tree_min(n->right); /* преемник как минимум правого поддерева */

        if (s->head != n) {             /* преемник глебже n->right */
            transplant(t, s, s->right); /* вырезаем его и поднимаем правого ребенка */
            s->right       = n->right;  /* к преемнику цепляем правое поддерево */
            s->right->head = s;
        }
        transplant(t, n, s); /* преемник встает на место n */
        s->left       = n->left;
        s->left->head = s;
    }
    /* Кароче у s точно нет левого ребенка, тк он самый левый в правом поддереве. 
   Если s не прямой правый ребенок n, то мы его вырезаем из текущего места - transplant(t, s, s->right); - ну то есть пихаем его правую часть
   на его место. 
   Далее правому ребенку s отдаем все правод поддерево n. А хэдером для правого поддерева назначаем s.
   Ну и теперь можно независимо от случая ставить s на место n.
   */

    free(n);
    t->size--;
    ASSERT_DUMP(tree_verify(t) == TREE_OK, tree_dump_hook, t);
    return TREE_OK;
}

/* Поиск узла по key и его удаление */
tree_err_t tree_remove(Tree* t, int key) {
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));

    Node* n = tree_find(t, key);
    if (!n) return TREE_NOT_FOUND;

    return tree_remove_node(t, n);
}