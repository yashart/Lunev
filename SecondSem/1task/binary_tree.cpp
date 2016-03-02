#include "binary_tree.h"

void errorDump(const int ERROR, const char* FUNCTION_NAME, 
				const int LINE, const char* FILE)
{
	printf("ERROR!!! %d in %s file in %s line: %d",
			 ERROR, FILE, FUNCTION_NAME, LINE);
	assert(0);
}

Tree* TreeCtor(TreeData data, Tree* parent)
{
	Tree* tree 		= (Tree*) calloc(1, sizeof(*tree));
	tree->data 		= data;
	tree->parent 	= parent;
	tree->left 		= NULL;
	tree->right 	= NULL;
	if(TreeOk(tree))
		errorDump(TreeOk(tree), __PRETTY_FUNCTION__, __LINE__, __FILE__);
	return tree;
}

int TreeOk(Tree* tree)
{
	if(tree == NULL)
		return EFAULT;
	if(((tree->left != NULL) && (tree->left->parent != tree))
		|| ((tree->right != NULL) && (tree->right->parent != tree)))
		return ECHILD;
	if((tree->left != 0)&&(TreeOk(tree->left) != 0))
		return ECHILD;
	if((tree->right != 0)&&(TreeOk(tree->right) != 0))
		return ECHILD;
	return 0;
}

int TreeDtor(Tree* tree)
{
	if(TreeOk(tree))
		errorDump(TreeOk(tree), __PRETTY_FUNCTION__, __LINE__, __FILE__);

	if(tree->left != NULL)
		TreeDtor(tree->left);
	if(tree->right != NULL)
		TreeDtor(tree->right);

	free(tree);
	return 0;
}

void TreeDump(Tree* tree, FILE* logfile)
{
	if(tree == NULL)
		errorDump(EFAULT, __PRETTY_FUNCTION__, __LINE__, __FILE__);
	if(logfile == NULL)
		errorDump(EFAULT, __PRETTY_FUNCTION__, __LINE__, __FILE__);

	fprintf(logfile, "(%d [%x]", tree->data, (int)(long)tree);
	if(tree->left != NULL)
		TreeDump(tree->left, logfile);
	if(tree->right != NULL)
		TreeDump(tree->right, logfile);
	fprintf(logfile, ")");
}

int TreeSaveInFile(Tree* tree, FILE* file)
{
	if(tree == NULL)
		errorDump(EFAULT, __PRETTY_FUNCTION__, __LINE__, __FILE__);
	if(file == NULL)
		errorDump(EBADF, __PRETTY_FUNCTION__, __LINE__, __FILE__);

	fprintf(file, "(%d", tree->data);
	if(tree->left != NULL)
		TreeSaveInFile(tree->left, file);
	else
		fprintf(file, "()");
	if(tree->right != NULL)
		TreeSaveInFile(tree->right, file);
	else
		fprintf(file, "()");
	fprintf(file, ")");

	return 0;
}

Tree* TreeReadFromFile(FILE* file, Tree* parent)
{
	if(file == NULL)
		errorDump(EFAULT, __PRETTY_FUNCTION__, __LINE__, __FILE__);

	TreeData data = 0;

	Tree* tree = NULL;

	if(fscanf(file, "(%d", &data) == 1)
	{
		tree = TreeCtor(data, parent);
		tree->left  = TreeReadFromFile(file, tree);
		tree->right = TreeReadFromFile(file, tree);
		fscanf(file, ")");
	}else
	{
		fscanf(file, ")");
	}
	return tree;
}

Tree* TreeNewLeft(Tree* tree, TreeData data)
{
	if(TreeOk(tree))
		errorDump(TreeOk(tree), __PRETTY_FUNCTION__, __LINE__, __FILE__);

	tree->left = TreeCtor(data, tree);
	return tree->left;
}

Tree* TreeNewRight(Tree* tree, TreeData data)
{
	if(TreeOk(tree))
		errorDump(TreeOk(tree), __PRETTY_FUNCTION__, __LINE__, __FILE__);

	tree->right = TreeCtor(data, tree);
	return tree->right;
}

int TreeCheckBalance(Tree* tree)
{
	if(TreeOk(tree))
		errorDump(TreeOk(tree), __PRETTY_FUNCTION__, __LINE__, __FILE__);
	
	if((tree->left != NULL) && (tree->data < tree->left->data))
		return 1;
	if((tree->right != NULL) && (tree->data > tree->right->data))
		return 1;

	if((tree->right != NULL) && (tree->left != NULL))
		return (TreeCheckBalance(tree->right) || TreeCheckBalance(tree->left));
	if(tree->right != NULL)
		return TreeCheckBalance(tree->right);
	if(tree->left != NULL)
		return TreeCheckBalance(tree->left);

	return 0;
}

Tree* TreeSearch(Tree* tree, TreeData data)
{
	if(TreeOk(tree))
		errorDump(TreeOk(tree), __PRETTY_FUNCTION__, __LINE__, __FILE__);
	if(TreeCheckBalance(tree))
		errorDump(EINVAL, __PRETTY_FUNCTION__, __LINE__, __FILE__);

	if(tree->data < data)
	{
		if(tree->right == NULL)
			return NULL;
		return TreeSearch(tree->right, data);
	}
	if(tree->data > data)
	{
		if(tree->left == NULL)
			return NULL;
		return TreeSearch(tree->left, data);
	}
	return tree;
}

Tree* TreeInsert(Tree* tree, TreeData data)
{
	if(TreeOk(tree))
		errorDump(TreeOk(tree), __PRETTY_FUNCTION__, __LINE__, __FILE__);
	if(TreeCheckBalance(tree))
		errorDump(EINVAL, __PRETTY_FUNCTION__, __LINE__, __FILE__);

	if(tree->data < data)
	{
		if(tree->right != NULL)
			return TreeInsert(tree->right, data);
		return TreeNewRight(tree, data);
	}

	if(tree->data == data)
		return tree;

	if(tree->left != NULL)
		return TreeInsert(tree->left, data);
	return TreeNewLeft(tree, data);
}

Tree* TreeDelete(Tree* tree, TreeData data)
{
	if(TreeOk(tree))
		errorDump(TreeOk(tree), __PRETTY_FUNCTION__, __LINE__, __FILE__);
	if(TreeCheckBalance(tree))
		errorDump(EINVAL, __PRETTY_FUNCTION__, __LINE__, __FILE__);

	Tree* treeFind 			= TreeSearch(tree, data);
	Tree* treeReturned 		= NULL;
	Tree* treeRightMinimum 	= NULL;
	if(treeFind == NULL)
		return tree;

	if(treeFind->right == NULL)
	{
		//treeFind hasn't continious
		if(treeFind->left == NULL)
		{
			if(treeFind->parent == NULL)
			{
				TreeDtor(treeFind);
				return NULL;
			}
			if(treeFind->parent->left == treeFind)
			{
				treeFind->parent->left = NULL;
				TreeDtor(treeFind);
				return tree;
			}
			if(treeFind->parent->right == treeFind)
			{
				treeFind->parent->right = NULL;
				TreeDtor(treeFind);
				return tree;
			}
			return tree;
		}
		//treeFind hasn't right node
		if(treeFind->parent == NULL)
		{
			treeReturned = treeFind->left;
			treeFind->left = NULL;
			TreeDtor(treeFind);
			treeReturned->parent = NULL;
			return treeReturned;
		}
		if(treeFind->parent->left == treeFind)
		{
			treeFind->parent->left = treeFind->left;
			treeFind->left->parent = treeFind->parent;
			treeFind->left = NULL;
			TreeDtor(treeFind);
			return tree;
		}
		if(treeFind->parent->right == treeFind)
		{
			treeFind->parent->right = treeFind->left;
			treeFind->left->parent = treeFind->parent;
			treeFind->left = NULL;
			TreeDtor(treeFind);
			return tree;
		}
	}
	//treeFind has right node
	treeRightMinimum = TreeFindMinimum(treeFind->right);
	treeFind->data = treeRightMinimum->data;
	if(treeRightMinimum->parent->right == treeRightMinimum)
	{
		treeRightMinimum->parent->right = treeRightMinimum->right;
		if(treeRightMinimum->right != NULL)
			treeRightMinimum->right->parent = treeRightMinimum->parent;
	}
	else if(treeRightMinimum->parent->left == treeRightMinimum)
	{
		treeRightMinimum->parent->left = treeRightMinimum->right;
		if(treeRightMinimum->right != NULL)
			treeRightMinimum->right->parent = treeRightMinimum->parent;
	}
	treeRightMinimum->right = NULL;
	TreeDtor(treeRightMinimum);
	return tree;
}

Tree* TreeFindMinimum(Tree* tree)
{
	if(TreeOk(tree))
		errorDump(TreeOk(tree), __PRETTY_FUNCTION__, __LINE__, __FILE__);

	if(tree->left == NULL)
		return tree;
	return TreeFindMinimum(tree->left);
}

Tree* TreeIteratorFirstElement(Tree* tree)
{
	if(TreeOk(tree))
		errorDump(TreeOk(tree), __PRETTY_FUNCTION__, __LINE__, __FILE__);

	return TreeFindMinimum(tree);
}

Tree* TreeIteratorNextElement(Tree* tree)
{	
	if(TreeOk(tree))
		errorDump(TreeOk(tree), __PRETTY_FUNCTION__, __LINE__, __FILE__);

	if(tree->right != NULL)
		return TreeFindMinimum(tree->right);

	Tree* previous = tree;
	do
	{
		previous = tree;
		tree = tree->parent;
	}while((tree != NULL) && (previous == tree->right));

	return tree;
}

int TreeIteratorLastElement(Tree* tree)
{
	if(TreeOk(tree))
		errorDump(TreeOk(tree), __PRETTY_FUNCTION__, __LINE__, __FILE__);

	Tree* rightTree = tree;
	while(rightTree->parent != NULL)
	{
		rightTree = rightTree->parent;
	}

	while(rightTree->right != NULL)
	{
		rightTree = rightTree->right;
	}
	
	if(rightTree == tree)
		return 1;
	return 0;
}