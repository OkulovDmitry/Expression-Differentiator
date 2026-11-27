#pragma once

#include <stdio.h>

#include "tree_fwd.h"

void tree_renumber(Tree* t);
void transplant(Tree* t, Node* u, Node* v);
void print_data(FILE* o, const Node* n, const VarTable* table);