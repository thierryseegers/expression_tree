#include "expression_tree.h"

#include <iostream>
#include <string>

using namespace std;

int main()
{
    // Create an int tree that will not cache values.
    expression_tree<int, false> eti;

    // Let's build the simplest of trees, a singe leaf:
    //
    //        3

    eti.root() = 3;

    cout << eti.evaluate() << endl; // Prints "3".


    // Let's build a more complex tree:
    //
    //  (2 * l + r)
    //   /       \
    //  1      (l - r)
    //          /   \
    //        2      3

    eti.root() = [](int i, int j){ return 2 * i + j; }; // Will change root node from leaf to branch.
    eti.root().left() = 1;
    eti.root().right() = minus<int>();
    eti.root().right().left() = 2;
    eti.root().right().right() = 3;

    cout << eti.evaluate() << endl; // Prints "1" (2 * 1 + (2 - 3)).

    // Re-evaluate the tree.
    // Because it is a non-pre-evaluating tree, all nodes will be re-visited and all operations re-applied.
    cout << eti.evaluate() << endl;

    // Let's change the tree a bit. Make one leaf a variable:
    //
    //  (2 * l + r)
    //   /       \
    //  1      (l - r)
    //          /   \
    //        2      x

    // Assign a variable to the rightmost leaf.
    int x = 1;
    eti.root().right().right() = &x;

    cout << eti.evaluate() << endl; // Prints "3" (2 * 1 + (2 - 1)).

    // Change the value of that variable and re-evaluate.
    x = 2;

    cout << eti.evaluate() << endl; // Prints "2" (2 * 1 + (2 - 2)).


    // Create a string tree with caching-on-evaluation optimization.
    expression_tree<string, true, false> etsc;
    
    // Let's build this tree:
    //
    //    (l + r)
    //  /         \
    // s        (l + r)
    //           /   \
    //         " "  "tree"

    string s = "expression";

    etsc.root() = plus<string>();
    etsc.root().left() = &s;
    etsc.root().right() = plus<string>();
    etsc.root().right().left() = string(" ");
    etsc.root().right().right() = string("tree");

    cout << etsc.evaluate() << endl; // Prints "expression tree".

    // Change the value of variable s and re-evaluate.
    // Because this is a caching tree, constant branches will not be re-evaluated.
    // For this tree, it means the concatenation of " " and "tree" will not be performed again.
    s = "apple";

    cout << etsc.evaluate() << endl; // Prints "apple tree".

    // But hwat happens if we do change the value of one the leaves that had a constant values?
    // We do the right thing, and discard the previously cached value.

    etsc.root().right().right() = string("pie");

    cout << etsc.evaluate() << endl; // Prints "apple pie".


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

    expression_tree<int, true, true> etip;

    etip.root() = plus<int>();
    etip.root().left() = 1;
    etip.root().right() = plus<int>();
    etip.root().right().left() = 2;
    etip.root().right().right() = plus<int>();
    etip.root().right().right().left() = 3;
    etip.root().right().right().right() = 4; // Right here, the entire tree will be evaluated.

    cout << etip.evaluate() << endl; // Prints "10". Even the first time this tree is evaluated, its value is already cached!

    etip.root().right().right().right() = 5; // Again, the entire tree will be evaluated.

    cout << etip.evaluate() << endl; // Prints "11".


    // Here's an example of copying a tree (or sub-tree) to another tree's node.
    expression_tree<int, true> etic;

    // We first build a simple tree:
    //
    //  (l + r)
    //  /    \
    // y      2

    int y = 2;

    etic.root() = plus<int>();
    etic.left() = &y;
    etic.right() = 2;

    cout << etic.evaluate() << endl; // Prints "4" (y + 2).

    // Then we build on it (using its own nodes!):
    //
    //     (l + r)
    //      /   \
    // (l + r)   2
    //  /   \
    // y     2

    etic.left() = etic.root();

    // Let's make it even bigger:
    //
    //     (l + r)
    //      /   \
    // (l + r)  (l + r)
    //  /   \    /   \
    // y     2  y     2

    etic.right() = etic.left();

    cout << etic.evaluate() << endl; // Prints "8" ((y + 2) + (y + 2)).


    // Misues.

    expression_tree<float, true> crash;
    //crash.evaluate(); // The tree is empty.

    crash.root() = divides<float>();
    //crash.evaluate(); // This tree cannot be evaluated, its root node is an operation with no data children to operate on.

    crash.root() = 2.;
    //crash.root().left() = divides<float>(); // Assigning to a data node's children is undefined behavior.

    return 0;
}
