#include "expression_tree.h"

#include <iostream>
#include <string>

using namespace std;

int main()
{
    // Create an int tree that will not cache values.
    expression_tree::tree<int, expression_tree::no_caching> tinc;

    // Let's build the simplest of trees, a singe leaf:
    //
    //        3

    tinc.root() = 3;

    cout << tinc.evaluate() << endl; // Prints "3".


    // Let's build a more complex tree:
    //
    //  (2 * l + r)
    //   /       \
    //  1      (l - r)
    //          /   \
    //        2      3

    tinc.root() = [](int i, int j){ return 2 * i + j; }; // Will change root node from leaf to branch.
    tinc.root().left() = 1;
    tinc.root().right() = minus<int>();
    tinc.root().right().left() = 2;
    tinc.root().right().right() = 3;

    cout << tinc.evaluate() << endl; // Prints "1" (2 * 1 + (2 - 3)).

    // Re-evaluate the tree.
    // Because it is a non-caching tree, all nodes will be re-visited and all operations re-applied.
    cout << tinc.evaluate() << endl;

    // Let's change the tree a bit. Make one leaf a variable:
    //
    //  (2 * l + r)
    //   /       \
    //  1      (l - r)
    //          /   \
    //        2      x

    // Assign a variable to the rightmost leaf.
    int x = 1;
    tinc.root().right().right() = &x;

    cout << tinc.evaluate() << endl; // Prints "3" (2 * 1 + (2 - 1)).

    // Change the value of that variable and re-evaluate.
    x = 2;

    cout << tinc.evaluate() << endl; // Prints "2" (2 * 1 + (2 - 2)).


    // Create a string tree with caching-on-evaluation optimization.
    expression_tree::tree<string, expression_tree::cache_on_evaluation> tsce;
    
    // Let's build this tree:
    //
    //    (l + r)
    //  /         \
    // s        (l + r)
    //           /   \
    //         " "  "tree"

    string s = "expression";

    tsce.root() = plus<string>();
    tsce.root().left() = &s;
    tsce.root().right() = plus<string>();
    tsce.root().right().left() = string(" ");
    tsce.root().right().right() = string("tree");

    cout << tsce.evaluate() << endl; // Prints "expression tree".

    // Change the value of variable s and re-evaluate.
    // Because this is a caching tree, constant branches will not be re-evaluated.
    // For this tree, it means the concatenation of " " and "tree" will not be performed again.
    s = "apple";

    cout << tsce.evaluate() << endl; // Prints "apple tree".

    // But hwat happens if we do change the value of one the leaves that had a constant value?
    // The tree will do the right thing, and discard the previously cached value.

    tsce.root().right().right() = string("pie");

    cout << tsce.evaluate() << endl; // Prints "apple pie".


    // Demonstration of a caching-on-modification tree with its degenerate case.

    // Let's build this tree:
    //
    //
    // (l + r)
    //  /   \
    // 1   (l + r)
    //      /   \
    //     2   (l + r)
    //          /   \
    //         3     4

    expression_tree::tree<int, expression_tree::cache_on_assignment> tica;

    tica.root() = plus<int>();
    tica.root().left() = 1;
    tica.root().right() = plus<int>();
    tica.root().right().left() = 2;
    tica.root().right().right() = plus<int>();
    tica.root().right().right().left() = 3;
    tica.root().right().right().right() = 4; // Right here, the entire tree will be pre-evaluated.

    cout << tica.evaluate() << endl; // Prints "10". Even the first time this tree is evaluated, its value is already cached!

    tica.root().right().right().right() = 5; // Again, the entire tree will be pre-evaluated.

    cout << tica.evaluate() << endl; // Prints "11".


    // Here's an example of copying a tree (or sub-tree) to another tree's node.
    expression_tree::tree<int, expression_tree::cache_on_evaluation> tice;

    // We first build a simple tree:
    //
    //  (l + r)
    //  /    \
    // y      2

    int y = 2;

    tice.root() = plus<int>();
    tice.left() = &y;
    tice.right() = 2;

    cout << tice.evaluate() << endl; // Prints "4" (y + 2).

    // Then we build on it (using its own nodes!):
    //
    //     (l + r)
    //      /   \
    // (l + r)   2
    //  /   \
    // y     2

    expression_tree::node<int, expression_tree::cache_on_evaluation> n = tice.root();

    tice.left() = n;

    // Let's make it even bigger:
    //
    //     (l + r)
    //      /   \
    // (l + r)  (l + r)
    //  /   \    /   \
    // y     2  y     2

    tice.right() = n;

    cout << tice.evaluate() << endl; // Prints "8" ((y + 2) + (y + 2)).


    // Misues.

    expression_tree::tree<float> crash;
    //crash.evaluate(); // The tree is empty.

    crash.root() = divides<float>();
    //crash.evaluate(); // This tree cannot be evaluated, its root node is an operation with no data children to operate on.

    crash.root() = 2.;
    //crash.root().left() = divides<float>(); // Assigning to a leaf node's children is undefined behavior.

    return 0;
}
