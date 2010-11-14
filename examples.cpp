#include "expression_tree.h"

#include <iostream>
#include <string>

using namespace std;

int main()
{
    // Create an int tree that will not pre-evaluate.
    expression_tree<int, false> etif;

    //        3

    etif.root() = 3;

    cout << etif.evaluate() << endl; // Prints "3".


    //  (2 * l + r)
    //   /       \
    //  1      (l - r)
    //          /   \
    //        2      3

    etif.root() = [](int i, int j){ return 2 * i + j; }; // Re-assigns root node from leaf to branch.
    etif.root().left() = 1;
    etif.root().right() = minus<int>();
    etif.root().right().left() = 2;
    etif.root().right().right() = 3;

    cout << etif.evaluate() << endl; // Prints "1" (2 * 1 + (2 - 3)).

    // Re-evaluate the tree.
    // Because it is a non-pre-evaluating tree, all nodes will be re-visited and all operations re-applied.
    cout << etif.evaluate() << endl;

    // Assign a variable to the rightmost leaf.
    int x = 1;
    etif.root().right().right() = &x;
	
    cout << etif.evaluate() << endl; // Prints "3" (2 * 1 + (2 - 1)).

    // Change the value of that variable and re-evaluate.
    x = 2;

    cout << etif.evaluate() << endl; // Prints "2" (2 * 1 + (2 - 2)).


    // Create a string tree that will pre-evaluate.
    expression_tree<string, true> etst;
    
    //    (l + r)
    //  /         \
    // s        (l + r)
    //           /   \
    //         " "  "tree"

    string s = "expression";

    etst.root() = plus<string>();
    etst.root().left() = &s;
    etst.root().right() = plus<string>();
    etst.root().right().left() = string(" ");
    etst.root().right().right() = string("tree");

    cout << etst.evaluate() << endl; // Prints "expression tree".

    // Change the value of variable s and re-evaluate.
    // Because this is a pre-evaluating tree, constant branches will not be re-evaluated.
    // For this tree, it means the concatenation of " " and "tree" will not be performed again.
    s = "apple";

    cout << etst.evaluate() << endl; // Prints "apple tree".

    
    // Misues.

    expression_tree<float, true> crash;
    //crash.evaluate(); // The tree is empty.

    crash.root() = divides<float>();
    //crash.evaluate(); // This tree cannot be evaluated, its root node is an operation with no data children to operate on.

    crash.root() = 2.;
    //crash.root().left() = divides<float>(); // Assigning to a data node's children is undefined behavior.

    return 0;
}
