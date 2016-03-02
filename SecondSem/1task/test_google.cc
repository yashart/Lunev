#include "binary_tree.h"
#include <gtest/gtest.h>

class TestSimpleTree : public ::testing::Test
{
protected:
	void SetUp()
	{
		tree 		= TreeCtor(5, NULL);
		treeLeft 	= TreeInsert(tree, 3);
		treeLeft2 	= TreeInsert(tree, 2);
		treeRight 	= TreeInsert(tree, 6);
		treeRight2	= TreeInsert(tree, 8);
	}
	void TearDown()
	{
		TreeDtor(tree);
	}
	Tree* tree;
	Tree* treeLeft;
	Tree* treeRight;
	Tree* treeLeft2;
	Tree* treeRight2;
};

TEST_F(TestSimpleTree, BadLeftOk)
{
	treeLeft->parent = NULL;
	errno = TreeOk(tree);
	EXPECT_EQ(errno, ECHILD);
	treeLeft->parent = tree;
}

TEST_F(TestSimpleTree, BadRightOk)
{
	treeRight->parent = NULL;
	errno = TreeOk(tree);
	EXPECT_EQ(errno, ECHILD);
	treeRight->parent = tree;
}

TEST_F(TestSimpleTree, BadRightOkCicle)
{
	treeLeft2->parent = NULL;
	errno = TreeOk(tree);
	EXPECT_EQ(errno, ECHILD);
	treeLeft2->parent = treeLeft;
}

TEST_F(TestSimpleTree, BadLeftOkCycle)
{
	treeRight2->parent = NULL;
	errno = TreeOk(tree);
	EXPECT_EQ(errno, ECHILD);
	treeRight2->parent = treeRight;
}

TEST_F(TestSimpleTree, CheckData) 
{
	EXPECT_EQ(tree->data, 5);
}

TEST_F(TestSimpleTree, DumpTest)
{
	FILE* logfile = fopen("logfile.txt", "wr");
	TreeDump(tree, logfile);
	fclose(logfile);
}

TEST_F(TestSimpleTree, SaveInFile)
{
	FILE* logfile = fopen("logfile.txt", "wr");
	TreeSaveInFile(tree, logfile);
	fclose(logfile);
}

TEST(NullPtr, DtorTest)
{
	TreeDtor(NULL);
	EXPECT_EQ(errno, EFAULT);
}

TEST(NullPtr, DumpTestTree)
{
	TreeDump(NULL, NULL);
	EXPECT_EQ(errno, EFAULT);
}

TEST(NullPtr, DumpTestLogfile)
{
	TreeDump((Tree*)5, NULL);
	EXPECT_EQ(errno, EBADF);
}

TEST(NullPtr, SaveTestTree)
{
	TreeSaveInFile(NULL, NULL);
	EXPECT_EQ(errno, EFAULT);
}

TEST(NullPtr, SaveTestLogfile)
{
	TreeSaveInFile((Tree*)5, NULL);
	EXPECT_EQ(errno, EBADF);
}

TEST(DumpTest, ErrorDump)
{
	errorDump(0, __PRETTY_FUNCTION__, __LINE__, __FILE__);
}

int main(int argc, char **argv) 
{
	::testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}