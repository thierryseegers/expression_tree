/*
	(C) Copyright Thierry Seegers 2010. Distributed under the following license:

	Boost Software License - Version 1.0 - August 17th, 2003

	Permission is hereby granted, free of charge, to any person or organization
	obtaining a copy of the software and accompanying documentation covered by
	this license (the "Software") to use, reproduce, display, distribute,
	execute, and transmit the Software, and to prepare derivative works of the
	Software, and to permit third-parties to whom the Software is furnished to
	do so, all subject to the following:

	The copyright notices in the Software and this entire statement, including
	the above license grant, this restriction and the following disclaimer,
	must be included in all copies of the Software, in whole or in part, and
	all derivative works of the Software, unless such copies or derivative
	works are solely in the form of machine-executable object code generated by
	a source language processor.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
	SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
	FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
	ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.
*/

#if !defined(EXPRESSION_TREE_H)
	 #define EXPRESSION_TREE_H

#include <functional>	// Eventually, that's all we'll need.

#if defined(__GNUG__)
#include <tr1/functional>	// Needed for g++ 4.2.x.
#endif

// As of g++ 4.2.x and VS 2008, the function class is in the std::tr1 namespace.
#if defined(__GNUG__) || (defined(_MSC_VER) && (_MSC_VER >= 1500 && _MSC_VER < 1600))
#define tr1_ std::tr1
#else
#define tr1_ std	// For vs 2010, the function class is in the std namespace.
#endif

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4355)
#endif

namespace detail
{
	//!\brief Operations must take two Ts as arguments and return a T.
	template<typename T>
	struct operation
	{
		typedef typename tr1_::function<T (T, T)> t;
	};

	//!\brief The tree's node class.
	//!
	//! This class stores a pointer to its implementation.
	//! The implementation node's type is derived at runtime when it is assigned to.
	template<typename T, bool P>
	class node;

	//!\brief Base class for the node class internal implementation.
	template<typename T, bool P>
	class node_impl
	{
	public:
		virtual ~node_impl() {}

		//!\brief Constness of the node.
		//!
		//! A leaf node is constant if its data is constant.
		//! A bracn node is constant if all its leaf nodes are constant.
		virtual bool constant() const = 0;

		//!\brief All nodes must evaluate.
		//!
		//! A leaf will evaluate to itself.
		//! A branch will evaluate to its operation applied to its children.
		virtual T evaluate() const = 0;
	};

	//!\brief Leaf class.
	//!
	//! This class stores a copy of its data.
	template<typename T, bool P>
	class leaf : public node_impl<T, P>
	{
		const T d;	//!< This node's data.

	public:
		leaf(const T& d) : d(d) {}

		virtual ~leaf() {}

		//! Because this classes stores a copy of its data, it is constant.
		virtual bool constant() const
		{
			return true;
		}

		virtual T evaluate() const
		{
			return d;
		}
	};

	//!\brief Leaf class specialized to T*.
	//!
	//! This class stores a pointer to data.
	template<typename T, bool P>
	class leaf<T*, P> : public node_impl<T, P>
	{
		const T *d;		//!< This node's pointer to data.

	public:
		leaf(const T* d) : d(d) {}

		virtual ~leaf() {}

		//! Because this class stores a pointer to its data, it is not constant.
		virtual bool constant() const
		{
			return false;
		}

		virtual T evaluate() const
		{
			return *d;
		}
	};

	//!\brief Branch class.
	//!
	//! This class stores an operation and two children nodes.
	template<typename T, bool P>
	class branch;

	//!\brief Branch class specialized to \a not pre-evaluate.
	template<typename T>
	class branch<T, false> : public node_impl<T, false>
	{
		typename operation<T>::t f;	//!< Operation to be applied to this node's children.
		node<T, false> l, r;		//!< This node's children.

	public:
		branch(const typename operation<T>::t& f) : f(f) {}

		virtual ~branch() {}

		//! This will not be called.
		virtual bool constant() const
		{
			return false;
		}

		//! Evaluating a branch node applies its operation on its children nodes.
		virtual T evaluate() const
		{
			return f(l.evaluate(), r.evaluate());
		};

		node<T, false>& left()
		{
			return l;
		}

		node<T, false>& right()
		{
			return r;
		}
	};

	//!\brief Branch class specialized to pre-evaluate.
	template<typename T>
	class branch<T, true> : public node_impl<T, true>
	{
		typename operation<T>::t f;	//!< Operation to be applied to this node's children.
		node<T, true> l, r;			//!< This node's children.

		branch *p;					//!< This branch's parent node.
		bool c;						//!< Whether this node is constant.
		T v;						//!< This node's value, if this node is constant.

	public:
		branch(const typename operation<T>::t& f, branch<T, true>* p = 0) : f(f), l(this), r(this), p(p), c(false) {}

		virtual ~branch() {}
		
		//! This constness of this node will be calculated whenever its children are set.
		virtual bool constant() const
		{
			return c;
		}

		//! If this node is constant, its values will have been pre-evaluated.
		virtual T evaluate() const
		{
			if(c) return v;

			return f(l.evaluate(), r.evaluate());
		};

		//! Called when children are added or modified.
		void grow()
		{
			if(l.constant() && r.constant())
			{
				// If both children are constant, this node is constant. Pre-evaluate its value.
				c = true;
				v = f(l.evaluate(), r.evaluate());
			}
			else
			{
				// One or both children are not constant. This node is then not constant.
				c = false;
			}

			// Recursively notify parent of growth.
			if(p)
			{
				p->grow();
			}
		}

		node<T, true>& left()
		{
			return l;
		}

		node<T, true>& right()
		{
			return r;
		}
	};

	//!\brief Node class specialized to \a not pre-evaluate.
	template<typename T>
	class node<T, false>
	{
		node_impl<T, false> *i;	//!< Follows the pimpl idiom.

	public:
		node() : i(0) {}

		virtual ~node()
		{
			if(i)
			{
				delete i;
			}
		}

		//!\brief Assign a value to this node.
		//!
		//! This designates this node as a leaf node.
		//! A node can still become a branch node by assigning an operation to it.
		node<T, false>& operator=(const T& t)
		{
			if(i)
			{
				delete i;
			}

			i = new leaf<T, false>(t);

			return *this;
		}

		//!\brief Assign a pointer to this node.
		//!
		//! This designates this node as a leaf node.
		//! A node can still become a branch node by assigning an operation to it.
		node<T, false>& operator=(const T* t)
		{
			if(i)
			{
				delete i;
			}

			i = new leaf<T*, false>(t);

			return *this;
		}

		//!\brief Assign an operation to this node.
		//!
		//! This designates this node as a branch node.
		//! A node can still become a leaf node by assigning data to it.
		node<T, false>& operator=(const typename operation<T>::t& f)
		{
			if(i)
			{
				delete i;
			}

			i = new branch<T, false>(f);

			return *this;
		}

		//!\brief This node's left child.
		//!
		//! Note that if this node is a leaf node, behavior is undefined.
		node<T, false>& left()
		{
			return dynamic_cast<branch<T, false>*>(i)->left();
		}

		//!\brief This node's right child.
		//!
		//! Note that if this node is a leaf node, behavior is undefined.
		node<T, false>& right()
		{
			return dynamic_cast<branch<T, false>*>(i)->right();
		}

		//!\brief Evaluates the value of this node.
		T evaluate() const
		{
			return i->evaluate();
		}
	};

	//! Node class specialized to pre-evaluate.
	template<typename T>
	class node<T, true>
	{
		node_impl<T, true> *i;	//!< Follows the pimpl idiom.

		branch<T, true> *parent;	//!< Pointer to this node's parent.

	public:
		node(branch<T, true> *parent = 0) : i(0), parent(parent) {}

		virtual ~node()
		{
			if(i)
			{
				delete i;
			}
		}

		//!\brief Assign a value to this node.
		//!
		//! This designates this node as a leaf node.
		//! A node can still become a branch node by assigning an operation to it.
		node<T, true>& operator=(const T& t)
		{
			if(i)
			{
				delete i;
			}

			i = new leaf<T, true>(t);

			if(parent)
			{
				parent->grow();
			}

			return *this;
		}

		//!\brief Assign a pointer to this node.
		//!
		//! This designates this node as a leaf node.
		//! A node can still become a branch node by assigning an operation to it.
		node<T, true>& operator=(const T* t)
		{
			if(i)
			{
				delete i;
			}

			i = new leaf<T*, true>(t);

			if(parent)
			{
				parent->grow();
			}

			return *this;
		}

		//!\brief Assign an operation to this node.
		//!
		//! This designates this node as a branch node.
		//! A node can still become a leaf node by assigning data to it.
		node<T, true>& operator=(const typename operation<T>::t& f)
		{
			if(i)
			{
				delete i;
			}

			i = new branch<T, true>(f, parent);

			if(parent)
			{
				parent->grow();
			}

			return *this;
		}

		//!\brief This node's left child.
		//!
		//! Note that if this node is a leaf node, behavior is undefined.
		node<T, true>& left()
		{
			return dynamic_cast<branch<T, true>*>(i)->left();
		}

		//!\brief This node's right child.
		//!
		//! Note that if this node is a leaf node, behavior is undefined.
		node<T, true>& right()
		{
			return dynamic_cast<branch<T, true>*>(i)->right();
		}

		//!\brief Constness of this node.
		bool constant() const
		{
			return i ? i->constant() : false;
		}

		//!\brief Evaluates the value of this node.
		T evaluate() const
		{
			return i->evaluate();
		}
	};
}

//!\brief Implements an expression tree.
//!
//!\param T The data type.
//!\param P Whether to pre-evaluate branches of constant value.
template<typename T, bool P>
class expression_tree : public detail::node<T, P>
{
public:
	virtual ~expression_tree() {}

	//!\brief This tree's root node.
	detail::node<T, P>& root()
	{
		return *this;
	}
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif

/*!
\mainpage expression_tree

\section introduction Introduction

An expression tree is a tree that stores data in its leaf nodes and operations in its branch nodes.
The tree's value can then be obtained by performing a preorder traversal, recursively applying the branch nodes's operations to their children.

For example, his tree evaluates to (2 * 1 + (2 - \a x)):

\code
(2 * l + r)
 /       \
1      (l - r)
        /   \
      2      x
\endcode

\section considerations Technical considerations

This implementation:
 - is contained in a single header file.
 - is templatized.
 - specializes the branches and the leaves to reduce space overhead.
 - is dependent on C++0x's function<> class.
 - requires RTTI.
 - has little in the way of safety checks.
 - has been tested with g++ 4.2.1, VS 2008 and VS2010.
   Later versions of g++ probably do not require the extra header inclusion (may not hurt) or the std::tr1 macro (may hurt).

In order to be evaluated, the expression_tree must be correctly formed.
That is, all its branch nodes' children nodes must have been given a value.

\section pre-evaluating Pre-evaluating optimization

By instantiating an expression_tree with its second template parameter set to \c true, 
evaluation will be optimzed by pre-evaluating a branch's value if all it childrens (branches or leaves) are constant.

For example:

\code
  B1
 /  \
C1  B2
   /  \
  C2  C3
\endcode

If this tree is built in the following order: B1, C1, B2, C2, C3, upon assignment of C3, B2 will be found to be of constant value and be pre-evaluated.
This pre-evaluation will continue recursively up the tree for as long as a branch's both children are constant.
In this case, B1 will also be pre-evaluated.

If C1 had instead been a variable (e.g. \a x), only B2 will have been pre-evaluated.
B1, not having both its children constant, will be evaluated when expression_tree::evaluate() is called.

\subsection degenerate Degenerate case

Given the following tree:

\code
  B1
 /  \
C1  B2
   /  \
  C2  B3
     /  \
    C3   B4
        / .
       C4  .
            .
            Bn
           /  \
          Cn  Cn+1
\endcode

Before C<SUB>n+1</SUB> is assigned to, none of the tree has been pre-evaluated, given that none of its nodes' constness can be confirmed.
When C<SUB>n+1</SUB> is assigned to, B<SUB>n</SUB> will be found constant and be evaluated.
B<SUB>n-1</SUB> will also be found constant and be evaluated.
This will continue until entire tree is evaluated.
Thus, a single assignment can trigger the equivalent of expression_tree::evaluate().

\section improvements Future improvements

 - Allow assignment of an entire tree to a node.

\section sample Sample code

\code
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
\endcode

\section license License

\verbatim
Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
\endverbatim

*/
