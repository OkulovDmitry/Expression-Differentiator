#pragma once

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

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
    char name[MAX_VAR_LEN];    // имя переменной - указатель на позицию начала в едином буфере
    double value;  // в будущем для построения графиков и вычисления производной в точке
} Variable;

typedef struct {
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
    bool is_unary;
} OpInfo;

const OpInfo op_info[NUMBER_OF_OPERATIONS] = {
    {.operation_ord = "+", .operation_tex = "+", .priority = 1, .is_unary = true},
    {.operation_ord = "-", .operation_tex = "-", .priority = 1, .is_unary = true},
    {.operation_ord = "*", .operation_tex = "\cdot", .priority = 2, .is_unary = true},
    {.operation_ord = "/", .operation_tex = "/", .priority = 2, .is_unary = true},
    {.operation_ord = "sin", .operation_tex = "\sin", .priority = 4, .is_unary = false},
    {.operation_ord = "cos", .operation_tex = "\cos", .priority = 4, .is_unary = false},
    {.operation_ord = "ln", .operation_tex = "\ln", .priority = 4, .is_unary = false},
    {.operation_ord = "^", .operation_tex = "^", .priority = 3, .is_unary = false},
};

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