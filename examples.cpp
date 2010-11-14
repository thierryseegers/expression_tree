#include "expression_tree.h"

#include <iostream>
#include <string>

using namespace std;

int main()
{
    expression_tree<int> eti;

    //        3

    eti.root() = 3;

    cout << eti.evaluate() << endl;    	// Prints "3".


    //  (2 * l + r)
    //   /       \
    //  1      (l - r)
    //          /   \
    //        2      3

    eti.root() = [](int i, int j){ return 2 * i + j; };
    eti.root().left() = 1;
    eti.root().right() = minus<int>();
    eti.root().right().left() = 2;
    eti.root().right().right() = 3;

    cout << eti.evaluate() << endl;    	// Prints "1" (2 * 1 + (2 - 3)).


	int x = 1;
	eti.root().right().right() = &x;
	
	cout << eti.evaluate() << endl;		// Prints "3" (2 * 1 + (2 - 1)).

	x = 2;

	cout << eti.evaluate() << endl;		// Prints "2" (2 * 1 + (2 - 2)).


    //         (l + r)
    //       /         \
    // "expression"  (l + r)
    //                /   \
    //              " "  "tree"

    expression_tree<string> ets;
    ets.root() = plus<string>();
    ets.root().left() = string("expression");
    ets.root().right() = plus<string>();
    ets.root().right().left() = string(" ");
    ets.root().right().right() = string("tree");

    cout << ets.evaluate() << endl;    	// Prints "expression tree".


    expression_tree<float> crash;
    //crash.evaluate();    	// The tree is empty.

    crash.root() = divides<float>();
    //crash.evaluate();    	// This tree cannot be evaluated, its root node is an operation with no data children to operate on.

    crash.root() = 2.;
    //crash.root().left() = divides<float>();    // Assigning to a data node's children is undefined behavior.

    return 0;
}
