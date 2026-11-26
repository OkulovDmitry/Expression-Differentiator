#pragma once

typedef struct Tree Tree;
typedef struct Node Node;

void transplant(Tree* t, Node* u, Node* v);
void print_data(FILE* o, const Node* n, const VarTable* table);