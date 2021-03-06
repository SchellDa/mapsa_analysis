
#include "cfgparse.h"
#include "gtest/gtest.h"

using namespace core;

TEST(cfg_parser, parse_valid)
{
	CfgParse p;
	try {
		p.parse("a = abc123\nbratwurst=     test23\n\n");
	} catch(CfgParse::parse_error p) {
		ASSERT_TRUE(false) << "Parsing of valid config failed.";
	} catch(...) {
		ASSERT_TRUE(false) << "Some exception occured during parsing.";
	}
	EXPECT_EQ(p.getVariable("a"), "abc123") << "Variable has wrong content!";
	EXPECT_EQ(p.getVariable("bratwurst"), "test23") << "Variable has wrong content!";
	try {
		p.getVariable("currywurst");
		ASSERT_TRUE(false) << "Reading undefined variable did not cause exception!";
	} catch(...) {
	}
}

TEST(cfg_parser, empty_assignment) {
	CfgParse p;
	try {
		p.parse("abc = bratwurst\nempty =   \nanotherempty=");
	} catch(CfgParse::parse_error p) {
		ASSERT_TRUE(false) << "Parsing of valid config failed.";
	} catch(...) {
		ASSERT_TRUE(false) << "Some exception occured during parsing.";
	}
	EXPECT_EQ(p.getVariable("abc"), "bratwurst");
	EXPECT_EQ(p.getVariable("empty"), "");
	EXPECT_EQ(p.getVariable("anotherempty"), "");
}

TEST(cfg_parser, handle_comments)
{
	CfgParse p;
	try {
		p.parse("a = abc123 # This is a comment\n#Line with comment only.\nbratwurst=     test23\n\nemptywurst = # Haha fooled you!");
	} catch(CfgParse::parse_error p) {
		ASSERT_TRUE(false) << "Parsing of valid config failed. " << p.what();
	} catch(...) {
		ASSERT_TRUE(false) << "Some exception occured during parsing.";
	}
	EXPECT_EQ(p.getVariable("a"), "abc123") << "Variable has wrong content!";
	EXPECT_EQ(p.getVariable("bratwurst"), "test23") << "Variable has wrong content!";
	EXPECT_EQ(p.getVariable("emptywurst"), "") << "Variable has wrong content!";
}

TEST(cfg_parser, error_handling)
{
	CfgParse p;
	try {
		p.parse("= a");
		EXPECT_TRUE(false) << "Missing identifier not detected";
	} catch(CfgParse::parse_error p) {
	} catch(...) {
		EXPECT_TRUE(false) << "Some exception occured during parsing.";
	}
	try {
		p.parse("a == a");
		EXPECT_TRUE(false) << "Multiple =";
	} catch(CfgParse::parse_error p) {
	} catch(...) {
		EXPECT_TRUE(false) << "Some exception occured during parsing.";
	}
	try {
		p.parse("a = a =");
		EXPECT_TRUE(false) << "Multiple =";
	} catch(CfgParse::parse_error p) {
	} catch(...) {
		EXPECT_TRUE(false) << "Some exception occured during parsing.";
	}
	try {
		p.parse("a#= 25");
		EXPECT_TRUE(false) << "commented assignment";
	} catch(CfgParse::parse_error p) {
	} catch(...) {
		EXPECT_TRUE(false) << "Some exception occured during parsing.";
	}
}

TEST(cfg_parser, whitespace_values)
{	
	try {
		CfgParse p;
		p.parse("a = hallo welt!");
		EXPECT_EQ(p.getVariable("a"), "hallo welt!");
	} catch(std::runtime_error p) {
		EXPECT_TRUE(false) << "Some exception occured during parsing." << p.what();
	}
	try {
		CfgParse p;
		p.parse("  a b c  = hallo welt! # test comment");
		EXPECT_EQ(p.getVariable("a b c"), "hallo welt!");
	} catch(std::runtime_error p) {
		EXPECT_TRUE(false) << "Some exception occured during parsing." << p.what();
	}
}

TEST(cfg_parser, whitespace_subst)
{	
	try {
		CfgParse p;
		p.parse(" a bc = hallo welt! # Test\nb = noch mehr @a bc@\nendlich = @b@");
		EXPECT_EQ(p.getVariable("a bc"), "hallo welt!");
		EXPECT_EQ(p.getVariable("b"), "noch mehr hallo welt!");
		EXPECT_EQ(p.getVariable("endlich"), "noch mehr hallo welt!");
	} catch(std::runtime_error p) {
		EXPECT_TRUE(false) << "Some exception occured during parsing." << p.what();
	}
	try {
		CfgParse p;
		p.parse("  a b c  = hallo welt! # test comment");
		EXPECT_EQ(p.getVariable("a b c"), "hallo welt!");
	} catch(std::runtime_error p) {
		EXPECT_TRUE(false) << "Some exception occured during parsing." << p.what();
	}
}
int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

