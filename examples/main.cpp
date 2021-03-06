#include "expression_tree.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using namespace std;

int main()
{
	// Create an int tree that will not cache values.
	expression_tree::tree<int, expression_tree::no_caching> tinc;

	// Let's build the simplest of trees, a singe leaf:
	//
	//		3

	tinc.root() = 3;

	cout << tinc.evaluate() << endl; // Prints "3".


	// Let's build a more complex tree:
	//
	//  (2 * l + r)
	//   /		 \
	//  1	  (l - r)
	//		   /   \
	//		  2		3

	tinc.root() = [](int i, int j){ return 2 * i + j; };

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
	//   /		 \
	//  1	  (l - r)
	//		   /   \
	//		  2		x

	// Assign a variable to the rightmost leaf.
	int x = 1;
	tinc.root().right().right() = &x;

	cout << tinc.evaluate() << endl; // Prints "3" (2 * 1 + (2 - 1)).

	// Change the value of that variable and re-evaluate.
	x = 2;

	cout << tinc.evaluate() << endl; // Prints "2" (2 * 1 + (2 - 2)).

	// Let's do the same thing but with a callable instead.
	//
	//  (2 * l + r)
	//   /		 \
	//  1	  (l - r)
	//		   /   \
	//		  2		f()
	
	// Assign a callable to the rightmost leaf.
	x = 1;
	tinc.root().right().right() = [&x]{ return x; };
	
	cout << tinc.evaluate() << endl; // Prints "3" (2 * 1 + (2 - 1)).
	
	// Change the value of that variable and re-evaluate.
	x = 2;
	
	cout << tinc.evaluate() << endl; // Prints "2" (2 * 1 + (2 - 2)).
	

	// Create a string tree with caching-on-evaluation optimization.
	expression_tree::tree<string, expression_tree::cache_on_evaluation> tsce;
	
	// Let's build this tree:
	//
	//	(l + r)
	//	 /	 \
	//	s	(l + r)
	//		 /   \
	//	   " "	 "tree"

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
	//		/   \
	//	   2   (l + r)
	//			/   \
	//		   3	 4

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
	//  /	\
	// y	 2

	int y = 2;

	tice.root() = plus<int>();
	tice.left() = &y;
	tice.right() = 2;

	cout << tice.evaluate() << endl; // Prints "4" (y + 2).

	// Then we build on it (using its own nodes!):
	//
	//	 (l + r)
	//	  /	   \
	// (l + r)  2
	//  /   \
	// y	 2

	expression_tree::node<int, expression_tree::cache_on_evaluation> n = tice.root();

	tice.left() = n;

	// Let's make it even bigger:
	//
	//	 (l + r)
	//	  /   \
	// (l + r)  (l + r)
	//  /   \	 /   \
	// y	 2  y	  2

	tice.right() = n;

	cout << tice.evaluate() << endl; // Prints "8" ((y + 2) + (y + 2)).

	// Demonstration of parallel evaluation.
	//
	// We build two trees with the exact same morphology, one to be evaluated sequentially and another, parallely.
	// Branches operations are to stay busy for one second. This simulates expensive computations on values.
	// Leaves have no value of consequence for this example.
	//
	//			  busy 1s				// Level 1
	//			 /	   \
	//	  busy 1s		 busy 1s		// Level 2
	//	  /		\		 /	   \
	// busy 1s busy 1s busy 1s busy 1s	// Level 3
	//  /   \   /   \   /   \   /   \ 
	// 0	 0 0	 0 0	 0 0	 0	// Level 4 (instantaneous evaluation)
	//
	// It should take 7 seconds to evaluate the sequential tree, but less time to evaluate the parallel one.
	// If one's hardware can execute two threads in parallel, level 3 can be evaluated in two seconds and level 2 in one second.
	// If one's hardware can execute four threads in parallel, level 3 and level 2 can each be evaluated in one second.

	// Here's our branch operation. It stays busy doing "work" for one second.
	auto busy_1_sec = [](const nullptr_t&, const nullptr_t&)
	{
		auto then = chrono::high_resolution_clock::now();
		
		while((chrono::high_resolution_clock::now() - then) < 1s)
		{
			short i = 0; while(++i);
		}
		
		return nullptr;
	};

	// The sequential tree.
	expression_tree::tree<nullptr_t, expression_tree::no_caching, expression_tree::sequential> tnncl;
	
	tnncl.root() = busy_1_sec;
	tnncl.left() = busy_1_sec;
	tnncl.left().left() = busy_1_sec;
	tnncl.left().left().left() = nullptr;
	
	tnncl.left().left().right() = tnncl.left().left().left();
	tnncl.left().right() = tnncl.left().left();
	tnncl.right() = tnncl.left();

	auto then = chrono::high_resolution_clock::now();
	tnncl.evaluate();
	cout << "Sequential tree evaluated in " << chrono::duration<float>(chrono::high_resolution_clock::now() - then).count() << " seconds.\n";	// 7 seconds.

	// The parallel tree.
	expression_tree::tree<nullptr_t, expression_tree::no_caching, expression_tree::parallel> tnncp;
	tnncp.root() = busy_1_sec;
	tnncp.left() = busy_1_sec;
	tnncp.left().left() = busy_1_sec;
	tnncp.left().left().left() = nullptr;

	tnncp.left().left().right() = tnncp.left().left().left();
	tnncp.left().right() = tnncp.left().left();
	tnncp.right() = tnncp.left();

	then = chrono::high_resolution_clock::now();
	tnncp.evaluate();
	cout << "Parallel tree evaluated in " << chrono::duration<float>(chrono::high_resolution_clock::now() - then).count() << " seconds.\n";  // 3 seconds on my computer.


	// Misues.

	expression_tree::tree<float> crash;
	//crash.evaluate(); // The tree is empty.

	crash.root() = divides<float>();
	//crash.evaluate(); // This tree cannot be evaluated, its root node is an operation with no data children to operate on.

	crash.root() = 2.;
	//crash.root().left() = divides<float>(); // Assigning to a leaf node's children is undefined behavior.

	return 0;
}
