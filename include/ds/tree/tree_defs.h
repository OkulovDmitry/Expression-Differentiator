#pragma once

/*
 * ВАЖНО: указатели data.string_data валидны только до следующего вызова
 * tree_insert (который может вызвать realloc и сдвинуть буфер).
 * Если нужно хранить строку долго — делайте strdup.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/* ------------------------- Режимы компиляции ----------------------------*/

#define TREE_DEBUG

#ifdef TREE_DEBUG
#    define TREE_PLACE_IN , const char *name, const char *function, const char *file, int line
#    define TREE_PLACE_OUT(l) , #l, __func__, __FILE__, __LINE__
#else
#    define TREE_PLACE_IN
#    define TREE_PLACE_OUT(l)
#endif

/* ------------------------------------------------------------------------- */

#define TREE_GV_MODE "ARCS"  // тип используемых стрелок в дампе дерева

#define MAX_VARS 200
#define MAX_VAR_LEN 20
#define VAR_POISON ((double)0x8BADF00DU)
#define TREE_MAX_BUF_CAP 100000

#define MAX_NODE_LEN 1000
#define MAX_OF_NODES 1000

/* -------------------------------- Определение структуры дерева -------------------------------- */

typedef enum {
    // типы для обычного дерева - используется для акинатора например
    NON_TYPE    = 0,
    INT_TYPE    = 1,
    DOUBLE_TYPE = 2,
    STRING_TYPE = 3,
    CHAR_TYPE   = 4,
    // типы для дифференциатора
    NUM_TYPE = 5,
    VAR_TYPE = 6,
    OP_TYPE  = 7,
    NUMBER_OF_TYPES,
} NodeType;

typedef struct {
    char name[MAX_VAR_LEN];  // имя переменной - указатель на позицию начала в едином буфере
    double value;            // в будущем для построения графиков и вычисления производной в точке
} Variable;

typedef struct VarTable {
    Variable* vars;
    size_t size;      // сколько записано переменных
    size_t capacity;  // сколько вмещается переменных
} VarTable;

typedef enum {
    OP_ADD = 0,
    OP_SUB = 1,
    OP_MUL = 2,
    OP_DIV = 3,
    OP_SIN = 4,
    OP_COS = 5,
    OP_LN  = 6,
    OP_POW = 7,
    NUMBER_OF_OPERATIONS,
} OpType;

typedef struct OpInfo {
    char* operation_ord;
    char* operation_tex;
    int priority;
    int min_children;  // 1 для унарных и 2 для бинарных
    int max_children;  // 1 для sin условно и 2 для бинарных и 2 для +/-
} OpInfo;

extern const OpInfo op_info[NUMBER_OF_OPERATIONS];

typedef union NodeElem {
    // обычный элемент дерева
    int int_data;
    double double_data;
    char* string_data;
    char char_data;
    // элемент дерева дифференциатора
    double num_data;
    size_t var_data;  // индекс в VarTable
    OpType op_data;
} NodeElem;

typedef struct Node {
    int key;
    int type;
    NodeElem data;
    struct Node* left;
    struct Node* right;
    struct Node* head;
} Node;

typedef struct Tree {
    Node* root;
    size_t size;
    char* buffer;
    size_t buf_used;
    size_t buf_cap;
    // для дифференциатора
    VarTable* var_table;
#ifdef TREE_DEBUG
    const char* name;
    const char* function;
    const char* file;
    int line;
#endif
} Tree;

/* ---------------------------------------------------------------------------------------------------------*/