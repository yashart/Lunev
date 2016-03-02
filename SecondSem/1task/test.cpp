#include "binary_tree.h"


void checkCtorDtorOk();
void checkTreeDumpSaveInFileReadFromFile();
void checkTreeNewLeftRightBalance();
void checkTreeSearch();
void checkTreeInsert();
void checkTreeDelete();
void checkIterator();

int main()
{
	checkCtorDtorOk();
	checkTreeDumpSaveInFileReadFromFile();
	checkTreeNewLeftRightBalance();
	checkTreeSearch();
	checkTreeInsert();
	checkTreeDelete();
	checkIterator();
	sleep(5);
	perror("\n\n\n\nSUCCESS!!!\n\n\n\n\n\n");
	return 0;
}

void checkCtorDtorOk()
{
	Tree* tree = TreeCtor(4, NULL);
	TreeInsert(tree, 3);
	TreeInsert(tree, 5);
	TreeDtor(tree);
	//Bad Dtor
	if(fork() == 0)
	{
		tree = NULL;
		TreeDtor(tree);
		exit(0);
	}
	wait();
	//BadOk
	if(fork() == 0)
	{
		tree = TreeCtor(4, NULL);
		TreeNewLeft(tree, 3);
		TreeNewRight(tree, 5);
		tree->left->parent = tree->right;
		TreeOk(tree);
		exit(0);
	}
	wait();
	if(fork() == 0)
	{
		tree = TreeCtor(4, NULL);
		TreeNewLeft(tree, 3);
		TreeNewRight(tree, 5);
		tree->right->parent = tree->right;
		TreeOk(tree);
		exit(0);
	}
	wait();
}

void checkTreeDumpSaveInFileReadFromFile()
{
	Tree* treeIns = TreeCtor(5, NULL);
	TreeInsert(treeIns, 3);
	TreeInsert(treeIns, 8);
	TreeInsert(treeIns, 5);
	TreeInsert(treeIns, 9);
	TreeInsert(treeIns, 6);
	TreeInsert(treeIns, 1);
	TreeInsert(treeIns, 1);
	TreeInsert(treeIns, 2);

	//BadTree
	if(fork()  == 0)
	{
		TreeDump(NULL, NULL);
		exit(0);
	}
	wait();
	if(fork()  == 0)
	{
		TreeDump(treeIns, NULL);
		exit(0);
	}
	wait();
	if(fork()  == 0)
	{
		TreeSaveInFile(NULL, NULL);
		exit(0);
	}
	wait();
	if(fork()  == 0)
	{
		TreeSaveInFile(treeIns, NULL);
		exit(0);
	}
	wait();
	
	FILE* logfile = fopen("logfile.txt", "wr");
	FILE* logfile2 = fopen("logfile2.txt", "wr");
	TreeDump(treeIns, logfile);
	TreeSaveInFile(treeIns, logfile2);
	TreeDtor(treeIns);
	fclose(logfile);
	fclose(logfile2);
	logfile2 = fopen("logfile2.txt", "r");
	treeIns = TreeReadFromFile(logfile2, NULL);
	TreeDtor(treeIns);
	//BadFile
	if(fork()  == 0)
	{
		TreeReadFromFile(NULL, NULL);
		exit(0);
	}
	wait();
	fclose(logfile2);
}

void checkTreeNewLeftRightBalance()
{

	//Bad Add
	if(fork()  == 0)
	{
		TreeNewLeft(NULL, 5);
		exit(0);
	}
	wait();
	if(fork()  == 0)
	{
		TreeNewRight(NULL, 5);
		exit(0);
	}
	wait();
	Tree* treeIns = TreeCtor(5, NULL);
	TreeInsert(treeIns, 3);
	TreeInsert(treeIns, 8);
	TreeInsert(treeIns, 5);
	TreeInsert(treeIns, 9);
	TreeInsert(treeIns, 6);
	TreeInsert(treeIns, 1);
	TreeInsert(treeIns, 1);
	Tree* treeEnd = TreeInsert(treeIns, 2);
	TreeCheckBalance(treeIns);
	//Bad Balance
	if(fork()  == 0)
	{
		TreeNewLeft(treeEnd, 100);
		TreeCheckBalance(treeIns);
		exit(0);
	}
	wait();
	if(fork()  == 0)
	{
		TreeNewRight(treeEnd, 0);
		TreeCheckBalance(treeIns);
		exit(0);
	}
	wait();
	TreeDtor(treeIns);
}

void checkTreeSearch()
{
	Tree* treeIns = TreeCtor(5, NULL);
	TreeInsert(treeIns, 3);
	TreeInsert(treeIns, 8);
	TreeInsert(treeIns, 5);
	TreeInsert(treeIns, 9);
	TreeInsert(treeIns, 6);
	Tree* treeEnd = TreeInsert(treeIns, 1);
	//BadArgs
	if(fork() == 0)
	{
		TreeSearch(NULL, 5);
		exit(0);
	}
	wait();
	if(fork() == 0)
	{
		TreeNewLeft(treeEnd, 100);
		TreeSearch(treeIns, 0);
		exit(0);
	}
	wait();
	TreeSearch(treeIns, 2);
	TreeDtor(treeIns);
}

void checkTreeInsert()
{
	Tree* treeIns = TreeCtor(5, NULL);
	TreeInsert(treeIns, 3);
	TreeInsert(treeIns, 8);
	TreeInsert(treeIns, 5);
	TreeInsert(treeIns, 9);
	Tree* treeEnd = TreeInsert(treeIns, 6);
	TreeInsert(treeIns, 1);
	//BadArgs:
	if(fork() == 0)
	{
		TreeInsert(NULL, 10);
		exit(0);
	}
	wait();
	if(fork() == 0)
	{
		TreeNewLeft(treeEnd, 100);
		TreeInsert(treeIns, 10);
		exit(0);
	}
	wait();
	TreeDtor(treeIns);
}

void checkTreeDelete()
{
	Tree* treeIns = TreeCtor(5, NULL);
	TreeInsert(treeIns, 3);
	TreeInsert(treeIns, 8);
	TreeInsert(treeIns, 5);
	TreeInsert(treeIns, 9);
	Tree* treeEnd = TreeInsert(treeIns, 6);
	TreeInsert(treeIns, 1);
	//BadArgs:
	if(fork() == 0)
	{
		TreeDelete(NULL, 10);
		exit(0);
	}
	wait();
	if(fork() == 0)
	{
		TreeNewLeft(treeEnd, 100);
		TreeDelete(treeIns, 10);
		exit(0);
	}
	wait();

	treeIns = TreeDelete(treeIns, 1);
	treeIns = TreeDelete(treeIns, 5);
	treeIns = TreeDelete(treeIns, 8);
	treeIns = TreeDelete(treeIns, 9);
	treeIns = TreeDelete(treeIns, 9);
	treeIns = TreeDelete(treeIns, 3);
	treeIns = TreeDelete(treeIns, 6);
}

void checkIterator()
{
	Tree* treeIns = TreeCtor(5, NULL);
	TreeInsert(treeIns, 3);
	TreeInsert(treeIns, 8);
	TreeInsert(treeIns, 5);
	TreeInsert(treeIns, 9);
	TreeInsert(treeIns, 6);
	TreeInsert(treeIns, 1);
	//Bad Args
	if(fork() == 0)
	{
		TreeIteratorFirstElement(NULL);
		exit(0);
	}
	wait();
	if(fork() == 0)
	{
		TreeIteratorNextElement(NULL);
		exit(0);
	}
	wait();
	if(fork() == 0)
	{
		TreeIteratorLastElement(NULL);
		exit(0);
	}
	wait();
	Tree* treeIter = TreeIteratorFirstElement(treeIns);
	while(treeIter != NULL)
    {
        TreeIteratorLastElement(treeIter);
        treeIter = TreeIteratorNextElement(treeIter);
    }
    TreeDtor(treeIns);
}