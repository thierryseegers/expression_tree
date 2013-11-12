#include "expression_tree.h"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <functional>
#include <limits>
#include <string>

using namespace expression_tree;

TEST_CASE("add/int/2", "Add two integers together.")
{
	tree<int> t;

	t.root() = std::plus<int>();

	t.root().left() = 0;
	t.root().right() = 0;
	REQUIRE(t.evaluate() == std::plus<int>()(0, 0));

	t.root().left() = 2;
	t.root().right() = 2;
	REQUIRE(t.evaluate() == std::plus<int>()(2, 2));

	t.root().left() = -1;
	t.root().right() = 1;
	REQUIRE(t.evaluate() == std::plus<int>()(-1, 1));

	t.root().left() = std::numeric_limits<int>::max();
	t.root().right() = std::numeric_limits<int>::max();
	REQUIRE(t.evaluate() == std::plus<int>()(std::numeric_limits<int>::max(), std::numeric_limits<int>::max()));
}

TEST_CASE("add/int/4", "Add four integers together.")
{
	tree<int> t;

	t.root() = std::plus<int>();

	t.root().left() = std::plus<int>();
	t.root().right() = std::plus<int>();
	t.root().left().left() = 1;
	t.root().left().right() = 2;
	t.root().right().left() = 3;
	t.root().right().right() = 4;
	REQUIRE(t.evaluate() == std::plus<int>()(std::plus<int>()(1, 2), std::plus<int>()(3, 4)));

	t.left() = 1;
	t.right() = std::plus<int>();
	t.right().left() = 2;
	t.right().right() = std::plus<int>();
	t.right().right().left() = 3;
	t.right().right().right() = 4;
	REQUIRE(t.evaluate() == std::plus<int>()(1, std::plus<int>()(2, std::plus<int>()(3, 4))));

	t.right() = 1;
	t.left() = std::plus<int>();
	t.left().left() = 2;
	t.left().right() = std::plus<int>();
	t.left().right().left() = 3;
	t.left().right().right() = 4;
	REQUIRE(t.evaluate() == std::plus<int>()(std::plus<int>()(std::plus<int>()(3, 4), 2), 1));
}

TEST_CASE("add/string/2", "Add two std::strings together.")
{
	tree<std::string> t;

	t.root() = std::plus<std::string>();

	t.root().left() = std::string();
	t.root().right() = std::string();
	REQUIRE(t.evaluate() == std::plus<std::string>()("", ""));

	t.root().left() = std::string(" ");
	t.root().right() = std::string(" ");
	REQUIRE(t.evaluate() == std::plus<std::string>()(" ", " "));

	t.root().left() = std::string("apple ");
	t.root().right() = std::string("pie");
	REQUIRE(t.evaluate() == std::plus<std::string>()("apple ", "pie"));
}

TEST_CASE("add/string/4", "Add four std::strings together.")
{
	tree<std::string> t;

	t.root() = std::plus<std::string>();

	t.root().left() = std::plus<std::string>();
	t.root().right() = std::plus<std::string>();
	t.root().left().left() = std::string("Hello");
	t.root().left().right() = std::string(", ");
	t.root().right().left() = std::string("world");
	t.root().right().right() = std::string("!");
	REQUIRE(t.evaluate() == std::plus<std::string>()(std::plus<std::string>()("Hello", ", "), std::plus<std::string>()("world", "!")));

	t.left() = std::string("Hello");
	t.right() = std::plus<std::string>();
	t.right().left() = std::string(", ");
	t.right().right() = std::plus<std::string>();
	t.right().right().left() = std::string("world");
	t.right().right().right() = std::string("!");
	REQUIRE(t.evaluate() == std::plus<std::string>()("Hello", std::plus<std::string>()(", ", std::plus<std::string>()("world", "!"))));

	t.right() = std::string("!");
	t.left() = std::plus<std::string>();
	t.left().left() = std::plus<std::string>();
	t.left().right() = std::string("world");
	t.left().left().left() = std::string("Hello");
	t.left().left().right() = std::string(", ");
	REQUIRE(t.evaluate() == std::plus<std::string>()(std::plus<std::string>()(std::plus<std::string>()("Hello", ", "), "world"), "!"));
}