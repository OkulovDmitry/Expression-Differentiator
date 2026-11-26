#pragma once

typedef struct Node Node;
typedef struct Tree Tree;

/* ---- Дополнительные функции ---- */
Node* tree_min(Node* n);                          // поиск минимального узла (самый левый)
Node* tree_max(Node* n);                          // поиск максималного узла (самый правый)
int tree_height(const Node* n);                   // нахождение высоты дерева
int bfs(const Tree* t);                           // нахождение ширины дерева - максимальное число узлов на одном уровне