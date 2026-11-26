#include "tree_core.h"

#include "tree_defs.h"
#include "tree_dump.h"
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

/* ---- Разрушение узла ---- */
static void destroy_node(Node* n) {
    if (!n) return;
    destroy_node(n->left);
    destroy_node(n->right);
    n = NULL;
}

/* ---- Заполнение буфера '\0' ---- */
void fill_poison(Tree* t, size_t from) {
    for (size_t i = from; i < t->buf_cap; i++) t->buffer[i] = '\0';
}

/* ---- Инициализация дерева ---- */
void tree_init(Tree* t TREE_PLACE_IN) {
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));

    t->root     = NULL;
    t->size     = 0;
    t->buf_used = 0;
    t->buf_cap  = 800;
    t->buffer   = calloc(1, t->buf_cap);
    fill_poison(t, t->buf_used);
    t->var_table->capacity = MAX_VARS;
    t->var_table->size     = 0;
    t->var_table->vars     = calloc(MAX_VARS, sizeof(Variable));

#ifdef TREE_DEBUG
    t->name     = name;
    t->function = function;
    t->file     = file;
    t->line     = line;
#endif

    tree_err_t err = tree_verify(t);
    ASSERT_MSG(err == TREE_OK, tree_strerror(err));
}

/* ---- Освобождает все узлы - обход снизу вверх ---- */
void tree_destroy(Tree* t) {
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));

    destroy_node(t->root);
    t->root = NULL;
    t->size = 0;
    free(t->buffer);
    free(t->var_table);
#ifdef TREE_DEBUG
    t->file     = NULL;
    t->function = NULL;
    t->line     = NULL;
    t->name     = NULL;
#endif
}

/* ---- Вставка узла ---- */
tree_err_t tree_insert(Tree* t, int key, int type, NodeElem data) {
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));

    if (type >= NUMBER_OF_TYPES) return TREE_WRONG_NODE_TYPE;

    Node* parent = NULL;
    Node** pp    = &t->root;
    while (*pp) {
        parent = *pp;
        if (key == parent->key) return TREE_DUPLICATE;
        pp = (key < parent->key) ? &parent->left : &parent->right;
    }

    Node* n = malloc(sizeof(*n));
    if (!n) return TREE_NO_MEM;

    if (type == STRING_TYPE) {
        while (t->buf_used + sizeof(NodeElem) > t->buf_cap) {
            tree_err_t err = resize_up(t);
            if (err != TREE_OK) {
                return err;
            }
        }

        char* buf_end = t->buffer + t->buf_used + 1;
        data.string_data ? memcpy(buf_end, data.string_data, sizeof(NodeElem)) : memcpy(buf_end, "POISON", sizeof(NodeElem));
        data.string_data = buf_end;
    }

    if (type == VAR_TYPE) {
        while (t->buf_used + sizeof(NodeElem) > t->buf_cap) {
            tree_err_t err = resize_up(t);
            if (err != TREE_OK) {
                return err;
            }
        }

        if (t->var_table->size == t->var_table) {
            return TREE_VARS_OVERFLOW;
        }

        if (data.var_data > t->var_table->capacity || data.var_data < 0) {
            return TREE_VAR_WRONG_FORMAT;
        }
    }

    if (type == OP_TYPE && (data.op_data < 0 || data.op_data > NUMBER_OF_TYPES)) {
        return TREE_WRONG_OP_CODE;
    }

    if (type == NON_TYPE) {
        data.string_data = NULL;
    }

    *n  = (Node){.key = key, .type = type, .data = data, .head = parent};
    *pp = n;
    t->size++;

    ASSERT_DUMP(tree_verify(t) == TREE_OK, tree_dump_hook, t);
    return TREE_OK;
}

tree_err_t resize_up(Tree* t) {
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));

    size_t new_cap = 2 * t->buf_cap + 1;
    if (new_cap > TREE_MAX_BUF_CAP) return TREE_NO_MEM;

    char* temp = realloc(t->buffer, new_cap);
    if (temp == NULL) return TREE_NO_MEM;

    t->buffer  = temp;
    t->buf_cap = new_cap;

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
    ASSERT_MSG(t != NULL, tree_strerror(TREE_NULL_PTR));
    ASSERT_MSG(n != NULL, tree_strerror(NODE_NULL_PTR));
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