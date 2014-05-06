#include "expression_tree.h"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <functional>
#include <limits>
#include <string>

using namespace expression_tree;
using namespace std;

template<typename T, typename F>
void all_policies(F f)
{
    f(tree<T, no_caching, sequential>());
    f(tree<T, no_caching, parallel>());
    
    f(tree<T, cache_on_evaluation, sequential>());
    f(tree<T, cache_on_evaluation, parallel>());
    
    f(tree<T, cache_on_assignment, sequential>());
    f(tree<T, cache_on_assignment, parallel>());
}

auto leaf_kinds = [](auto&& tree)
{
	tree.root() = 0;
    REQUIRE(tree.evaluate() == 0);
	
	int x = 22;
	tree.root() = &x;
    REQUIRE(tree.evaluate() == x);
	
	x = numeric_limits<int>::max();
	tree.root() = [&x]{ return x; };
    REQUIRE(tree.evaluate() == x);
};

TEST_CASE("leaf_kinds", "Evaluate a single int leaf tree.")
{
    all_policies<int>(leaf_kinds);
}

auto node_copy = [](auto&& tree)
{
	tree.root() = [](int const& a, int const& b){ return a + b; };
	
	tree.left() = 1;
	
	tree.right() = tree.left();
	REQUIRE(tree.evaluate() == 2);
	
	tree.left() = tree.root();
	REQUIRE(tree.evaluate() == 3);
	
	tree.left() = tree.root();
	REQUIRE(tree.evaluate() == 4);
	
	tree.left().left() = tree.left().left().left();
	REQUIRE(tree.evaluate() == 3);

	tree.left() = tree.left().left();
	REQUIRE(tree.evaluate() == 2);

	tree.root() = tree.left();
	REQUIRE(tree.evaluate() == 1);
};

TEST_CASE("node_copy", "Grow and prune a tree by copying branches and leaves.")
{
    all_policies<int>(node_copy);
}

auto single_leaf_int = [](auto&& tree)
{
    tree.root() = 0;
    REQUIRE(tree.evaluate() == 0);

    tree.root() = 22;
    REQUIRE(tree.evaluate() == 22);

    tree.root() = numeric_limits<int>::max();
    REQUIRE(tree.evaluate() == numeric_limits<int>::max());
};

TEST_CASE("single_leaf_int", "Evaluate a single int leaf tree.")
{
    all_policies<int>(single_leaf_int);
}

auto single_leaf_string = [](auto&& tree)
{
    tree.root() = string();
    REQUIRE(tree.evaluate() == string());
    
    tree.root() = "hello";
    REQUIRE(tree.evaluate() == "hello");
};

TEST_CASE("single_leaf_string", "Evaluate a single string leaf tree.")
{
    all_policies<string>(single_leaf_string);
}

auto add_two_ints = [](auto&& tree)
{
	tree.root() = plus<int>();

	tree.root().left() = 0;
	tree.root().right() = 0;
	REQUIRE(tree.evaluate() == plus<int>()(0, 0));

	tree.root().left() = 2;
	tree.root().right() = 2;
	REQUIRE(tree.evaluate() == plus<int>()(2, 2));

	tree.root().left() = -1;
	tree.root().right() = 1;
	REQUIRE(tree.evaluate() == plus<int>()(-1, 1));

	tree.root().left() = numeric_limits<int>::max();
	tree.root().right() = numeric_limits<int>::max();
	REQUIRE(tree.evaluate() == plus<int>()(numeric_limits<int>::max(), numeric_limits<int>::max()));
};

TEST_CASE("add_two_ints", "Add two integers together.")
{
    all_policies<int>(add_two_ints);
}

auto add_four_ints = [](auto&& tree)
{
	tree.root() = plus<int>();

	tree.root().left() = plus<int>();
	tree.root().right() = plus<int>();
	tree.root().left().left() = 1;
	tree.root().left().right() = 2;
	tree.root().right().left() = 3;
	tree.root().right().right() = 4;
	REQUIRE(tree.evaluate() == plus<int>()(plus<int>()(1, 2), plus<int>()(3, 4)));

	tree.left() = 1;
	tree.right() = plus<int>();
	tree.right().left() = 2;
	tree.right().right() = plus<int>();
	tree.right().right().left() = 3;
	tree.right().right().right() = 4;
	REQUIRE(tree.evaluate() == plus<int>()(1, plus<int>()(2, plus<int>()(3, 4))));

	tree.right() = 1;
	tree.left() = plus<int>();
	tree.left().left() = 2;
	tree.left().right() = plus<int>();
	tree.left().right().left() = 3;
	tree.left().right().right() = 4;
	REQUIRE(tree.evaluate() == plus<int>()(plus<int>()(plus<int>()(3, 4), 2), 1));
};

TEST_CASE("add_four_ints", "Add four integers together.")
{
    all_policies<int>(add_four_ints);
}

auto add_two_strings = [](auto&& tree)
{
	tree.root() = plus<string>();

	tree.root().left() = string();
	tree.root().right() = string();
	REQUIRE(tree.evaluate() == plus<string>()("", ""));

	tree.root().left() = string(" ");
	tree.root().right() = string(" ");
	REQUIRE(tree.evaluate() == plus<string>()(" ", " "));

	tree.root().left() = string("apple ");
	tree.root().right() = string("pie");
	REQUIRE(tree.evaluate() == plus<string>()("apple ", "pie"));
};

TEST_CASE("add_two_strings", "Add two strings together.")
{
    all_policies<string>(add_two_strings);
}

auto add_four_strings = [](auto&& tree)
{
	tree.root() = plus<string>();

	tree.root().left() = plus<string>();
	tree.root().right() = plus<string>();
	tree.root().left().left() = string("Hello");
	tree.root().left().right() = string(", ");
	tree.root().right().left() = string("world");
	tree.root().right().right() = string("!");
	REQUIRE(tree.evaluate() == plus<string>()(plus<string>()("Hello", ", "), plus<string>()("world", "!")));

	tree.left() = string("Hello");
	tree.right() = plus<string>();
	tree.right().left() = string(", ");
	tree.right().right() = plus<string>();
	tree.right().right().left() = string("world");
	tree.right().right().right() = string("!");
	REQUIRE(tree.evaluate() == plus<string>()("Hello", plus<string>()(", ", plus<string>()("world", "!"))));

	tree.right() = string("!");
	tree.left() = plus<string>();
	tree.left().left() = plus<string>();
	tree.left().right() = string("world");
	tree.left().left().left() = string("Hello");
	tree.left().left().right() = string(", ");
	REQUIRE(tree.evaluate() == plus<string>()(plus<string>()(plus<string>()("Hello", ", "), "world"), "!"));
};

TEST_CASE("add_four_strings", "Add four strings together.")
{
    all_policies<string>(add_four_strings);
}