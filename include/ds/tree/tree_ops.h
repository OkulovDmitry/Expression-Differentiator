#pragma once

#include "tree_fwd.h"

void transplant(Tree* t, Node* u, Node* v);
void print_data(FILE* o, const Node* n, const VarTable* table);