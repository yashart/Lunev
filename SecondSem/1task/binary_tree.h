#ifndef TREE_H_INCLUDED
#define TREE_H_INCLUDED

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>


void errorDump(const int ERROR, const char* FUNCTION_NAME, 
				const int LINE, const char* FILE);

typedef int TreeData;

struct Tree
{
	TreeData data;
	Tree* parent;
	Tree* left;
	Tree* right;
};

Tree* 	TreeCtor 				(TreeData data, Tree* parent);
int 	TreeDtor				(Tree* tree);
int 	TreeOk					(Tree* tree);
void 	TreeDump				(Tree* tree, FILE* logfile);
int 	TreeSaveInFile			(Tree* tree, FILE* file);
Tree*	TreeReadFromFile		(FILE* file, Tree* parent);
Tree*	TreeNewLeft				(Tree* tree, TreeData data);
Tree*	TreeNewRight			(Tree* tree, TreeData data);

int 	TreeCheckBalance		(Tree* tree);

Tree* 	TreeSearch 				(Tree* tree, TreeData data);
Tree* 	TreeInsert 				(Tree* tree, TreeData data);
Tree*	TreeDelete				(Tree* tree, TreeData data);
Tree*	TreeFindMinimum 		(Tree* tree);

Tree*	TreeIteratorFirstElement(Tree* tree);
Tree* 	TreeIteratorNextElement	(Tree* tree);
int 	TreeIteratorLastElement	(Tree* tree);

#endif