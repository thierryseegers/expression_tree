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
		typedef typename tr1_::function<T (T, T)> t;	//!< Template typedef trick.
	};

	//!\brief The tree's node class.
	//!
	//! This class stores a pointer to its implementation.
	//! The implementation node's type is derived at runtime when it is assigned to.
	template<typename T, bool C, bool P>
	class node;

	//!\brief Base class for the node class internal implementation.
	template<typename T, bool C, bool P>
	class node_impl
	{
	public:
		virtual ~node_impl() {}

		//!\brief Clones this object.
		//!
		//! Necessary for assignment operator of classes that will own concrete instances of this base class.
		virtual node_impl<T, C, P>* clone() const = 0;

		//!\brief Constness of the node.
		//!
		//! A leaf node is constant if its data is constant.
		//! A branch node is constant if all its leaf nodes are constant.
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
	template<typename T, bool C, bool P>
	class leaf : public node_impl<T, C, P>
	{
		const T d;	//!< This node's data.

	public:
		//! Constructor.
		leaf(const T& d) : d(d) {}

		//! Copy constructor.
		leaf(const leaf<T, C, P>& o) : d(o.d) {}

		virtual ~leaf() {}

		//!\brief Clones this object.
		virtual leaf<T, C, P>* clone() const
		{
			return new leaf<T, C, P>(*this);
		}

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
	template<typename T, bool C, bool P>
	class leaf<T*, C, P> : public node_impl<T, C, P>
	{
		const T *p;		//!< This node's pointer to data.

	public:
		//! Constructor.
		leaf(const T* p) : p(p) {}

		//! Copy constructor.
		leaf(const leaf<T, C, P>& o) : p(o.p) {}

		virtual ~leaf() {}

		//!\brief Clones this object.
		virtual leaf<T*, C, P>* clone() const
		{
			return new leaf<T*, C, P>(*this);
		}

		//! Because this class stores a pointer to its data, it is not constant.
		virtual bool constant() const
		{
			return false;
		}

		virtual T evaluate() const
		{
			return *p;
		}
	};

	//!\brief Branch class.
	//!
	//! This class stores an operation and two children nodes.
	//! The default implementation does \a no caching optimization.
	template<typename T, bool C, bool P>
	class branch : public node_impl<T, C, P>
	{
		typename operation<T>::t f;	//!< Operation to be applied to this node's children.
		node<T, C, P> l, r;			//!< This node's children.

	public:
		//! Constructor.
		branch(const typename operation<T>::t& f) : f(f) {}

		//! Copy constructor.
		branch(const branch<T, C, P>& o) : f(o.f), l(o.l), r(o.r) {}

		virtual ~branch() {}

		//!\brief Clones this object.
		virtual branch<T, C, P>* clone() const
		{
			return new branch<T, C, P>(*this);
		}

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

		//!\brief This node's left child.
		node<T, C, P>& left()
		{
			return l;
		}

		//!\brief This node's right child.
		node<T, C, P>& right()
		{
			return r;
		}
	};

	//!\brief Template specialization of the branch class with the caching optimization.
	//!
	//! The time at which to cache the value depends on template parameter \c P.
	//! A branch will cache its value only if its children are constant.
	//! If \c P is \c false, caching is done at evaluation.
	//! If \c P is \c true, caching is done when its children are set or modified. It is pre-evaluated.
	template<typename T, bool P>
	class branch<T, true, P> : public node_impl<T, true, P>
	{
		typename operation<T>::t f;	//!< Operation to be applied to this node's children.
		node<T, true, P> l, r;		//!< This node's children.

		mutable bool c;				//!< Whether the value of this node is cached.
		mutable T v;				//!< This node's value, if \c c is \c true.

	public:
		//! Constructor.
		branch(const typename operation<T>::t& f, const node<T, true, P>& l, const node<T, true, P>& r) : f(f), l(l), r(r), c(false) {}

		//! Copy constructor.
		branch(const branch<T, true, P>& o) : f(o.f), l(o.l), r(o.r), c(o.c), v(o.v) {}

		virtual ~branch() {}
		
		//!\brief Clones this object.
		virtual branch<T, true, P>* clone() const
		{
			return new branch<T, true, P>(*this);
		}

		//! This constness of this node is dependent on the constness of its children.
		virtual bool constant() const
		{
			return l.constant() && r.constant();
		}

		//! If this node's value has been cached, return it.
		//! Else, evaluate this node's value and cache it if this node is constant.
		virtual T evaluate() const
		{
			if(c) return v;

			v = f(l.evaluate(), r.evaluate());

			if(constant())
			{
				c = true;
			}

			return v;
		};

		//! Called when children are added or modified.
		void grow()
		{
			if(P && constant())
			{
				// If pre-evaluation optimization is on and this node is constant, cache its value now.
				c = true;
				v = f(l.evaluate(), r.evaluate());
			}
			else
			{
				// One or both children are not constant. This node is then not constant.
				c = false;
			}
		}

		//!\brief This node's left child.
		node<T, true, P>& left()
		{
			return l;
		}

		//!\brief This node's right child.
		node<T, true, P>& right()
		{
			return r;
		}
	};

	//!\brief Node class.
	//!
	//! The default implementation does \a no caching optimization.
	template<typename T, bool C, bool P>
	class node
	{
		node_impl<T, C, P> *i;	//!< Follows the pimpl idiom.

	public:
		//! Constructor.
		node() : i(0) {}

		//! Copy constructor.
		node(const node<T, C, P>& o) : i(o.i->clone()) {}

		//! Assignment operator.
		node<T, C, P>& operator=(const node<T, C, P>& o)
		{
			if(this != &o)
			{
				node_impl<T, C, P>* c = o.i->clone();

				if(i)
				{
					delete i;
				}

				i = c;
			}

			return *this;
		}

		virtual ~node()
		{
			if(i)
			{
				delete i;
			}
		}

		//!\brief Assign a value to this node.
		//!
		//! The assignment of a \c T designates this node as a leaf node.
		//! A node can still become a branch node by assigning an operation to it.
		node<T, C, P>& operator=(const T& t)
		{
			if(i)
			{
				delete i;
			}

			i = new leaf<T, C, P>(t);

			return *this;
		}

		//!\brief Assign a pointer to this node.
		//!
		//! The assignment of a \c T* designates this node as a leaf node.
		//! A node can still become a branch node by assigning an operation to it.
		node<T, C, P>& operator=(const T* t)
		{
			if(i)
			{
				delete i;
			}

			i = new leaf<T*, C, P>(t);

			return *this;
		}

		//!\brief Assign an operation to this node.
		//!
		//! The assignment of an operation designates this node as a branch node.
		//! A node can still become a leaf node by assigning data to it.
		node<T, C, P>& operator=(const typename operation<T>::t& f)
		{
			if(i)
			{
				delete i;
			}

			i = new branch<T, C, P>(f);

			return *this;
		}

		//!\brief This node's left child.
		//!
		//! Note that if this node is a leaf node, behavior is undefined.
		node<T, C, P>& left()
		{
			return dynamic_cast<branch<T, C, P>*>(i)->left();
		}

		//!\brief This node's right child.
		//!
		//! Note that if this node is a leaf node, behavior is undefined.
		node<T, C, P>& right()
		{
			return dynamic_cast<branch<T, C, P>*>(i)->right();
		}

		//!\brief Evaluates the value of this node.
		T evaluate() const
		{
			return i->evaluate();
		}
	};

	//! Template specialization of the node class with the caching optimization.
	template<typename T, bool P>
	class node<T, true, P>
	{
		node_impl<T, true, P> *i;	//!< Follows the pimpl idiom.

		node<T, true, P> *p;		//!< Pointer to this node's parent.

	public:
		//! Constructor.
		node(node<T, true, P> *p = 0) : i(0), p(p) {}

		//! Copy constructor.
		node(const node<T, true, P>& o) : i(o.i ? o.i->clone() : 0), p(o.p) {}

		//! Assignment operator.
		node<T, true, P>& operator=(const node<T, true, P>& o)
		{
			if(this != &o)
			{
				node_impl<T, true, P>* c = o.i->clone();

				if(i)
				{
					delete i;
				}

				i = c;
				p = o.p;

				if(p)
				{
					p->grow();
				}
			}

			return *this;
		}

		virtual ~node()
		{
			if(i)
			{
				delete i;
			}
		}

		//!\brief Assign a value to this node.
		//!
		//! The assignment of a \c T designates this node as a leaf node.
		//! A node can still become a branch node by assigning an operation to it.
		node<T, true, P>& operator=(const T& t)
		{
			if(i)
			{
				delete i;
			}

			i = new leaf<T, true, P>(t);

			if(p)
			{
				p->grow();
			}

			return *this;
		}

		//!\brief Assign a pointer to this node.
		//!
		//! The assignment of a \c T* designates this node as a leaf node.
		//! A node can still become a branch node by assigning an operation to it.
		node<T, true, P>& operator=(const T* t)
		{
			if(i)
			{
				delete i;
			}

			i = new leaf<T*, true, P>(t);

			if(p)
			{
				p->grow();
			}

			return *this;
		}

		//!\brief Assign an operation to this node.
		//!
		//! The assignment of an operation designates this node as a branch node.
		//! A node can still become a leaf node by assigning data to it.
		node<T, true, P>& operator=(const typename operation<T>::t& f)
		{
			if(i)
			{
				delete i;
			}

			i = new branch<T, true, P>(f, node<T, true, P>(this), node<T, true, P>(this));

			if(p)
			{
				p->grow();
			}

			return *this;
		}

		//!\brief This node's left child.
		//!
		//! Note that if this node is a leaf node, behavior is undefined.
		node<T, true, P>& left()
		{
			return dynamic_cast<branch<T, true, P>*>(i)->left();
		}

		//!\brief This node's right child.
		//!
		//! Note that if this node is a leaf node, behavior is undefined.
		node<T, true, P>& right()
		{
			return dynamic_cast<branch<T, true, P>*>(i)->right();
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

		//!\brief Called when this node is assigned to and pre-evaluation optimization is enabled.
		//!
		//! Recursively pre-evaluates this branch and this its parents.
		//! If this function ends up being called when this node is a leaf,
		//! it is indicative of user error.
		void grow()
		{
			dynamic_cast<branch<T, true, P>*>(i)->grow();

			if(p)
			{
				p->grow();
			}
		}
	};
}

//!\brief Implements an expression tree.
//!
//!\param T The data type.
//!\param C Whether to cache the value of constant branches.
//!\param P If C is \c true, whether to pre-evaluate branches of constant value.
template<typename T, bool C = false, bool P = false>
class expression_tree : public detail::node<T, C, P>
{
public:
	typedef typename expression_tree<T, true, false> caching;		//!< Convenience typedef.
	typedef typename expression_tree<T, true, true> preevaluating;	//!< Convenience typedef.

	virtual ~expression_tree() {}

	//!\brief This tree's root node.
	detail::node<T, C, P>& root()
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

\li \ref introduction
\li \ref considerations
\li \ref optimizations
\li \ref improvements
\li \ref sample
\li \ref license

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
 - uses templates heavily.
 - specializes the branches and the leaves to reduce space overhead.
 - is dependent on C++0x's function<> class.
 - requires RTTI.
 - has little in the way of safety checks.
 - has been tested with g++ 4.2.1, VS 2008 and VS2010.
   Later versions of g++ probably do not require the extra \c <tr1/functional> header inclusion (may not hurt) or the \c std::tr1 macro (may hurt).

In order to be evaluated, the expression_tree must be correctly formed.
That is, all its branch nodes' children nodes must have been given a value.

\section optimizations Optimizations

Two different optimizations are available. Both cache the value of a constant branch.
One caches it when a branch is first evaluated. The other, when a branch's children are assigned to.

\subsection caching Caching-on-evaluation optimization

By instantiating an expression_tree with its second template parameter set to \c true and its third template parameter to \c false, 
a tree's evaluation will be optimzed by caching-on-evaluation.
Caching on evaluation simply consists on remembering a branch's value if that branch is considered constant.

Consider the following tree, where B<SUB>n</SUB> is a branch, C<SUB>n</SUB> is a constant value and x<SUB>n</SUB> is a variable value:

\code
  B1
 /  \
x1  B2
   /  \
  C2  C3
\endcode

When that tree is evaluated, B<SUB>2</SUB>'s value will be cached because its children are constant.
If C<SUB>2</SUB> and C<SUB>3</SUB> don't change (i.e. if they are not assigned a different constant value),
the second time the tree is evaluated, the evaluation of B<SUB>2</SUB> will return its cached value.
It will not perform its operation on its children again.

\subsection pre-evaluating Caching-on-modification optimization

By instantiating an expression_tree with its second and third template parameter set to \c true, 
a tree's evaluation will be optimzed by caching-on-modification, or pre-evaluation of a branch's value.
Pre-evaluation happens when a branch's children nodes are assigned to.
If all of a branch's childrens (branches or leaves) are constant, the branch's value is evaluated and cached.

Consider the following tree, where B<SUB>n</SUB> is a branch and C<SUB>n</SUB> is a constant value:

\code
  B1
 /  \
C1  B2
   /  \
  C2  C3
\endcode

If this tree is built in the following order: B<SUB>1</SUB>, C<SUB>1</SUB>, B<SUB>2</SUB>, C<SUB>2</SUB>, C<SUB>3</SUB>, 
upon assignment of C<SUB>3</SUB>, B<SUB>2</SUB> will be found to be of constant value and be pre-evaluated.
This pre-evaluation will continue recursively up the tree for as long as a branch's both children are constant.
In this case, B<SUB>1</SUB> will also be pre-evaluated.

If C<SUB>1</SUB> had instead been a variable (e.g. \a x), only B<SUB>2</SUB> will have been pre-evaluated.
B<SUB>1</SUB>, not having both its children constant, will be evaluated when expression_tree::evaluate() is called.

\subsubsection degenerate Degenerate case of caching-on-modification optimization

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

 - Multi-threaded evaluation perhaps? In the spirit of not having this header depend on third-party libraries, I'll wait until std::thread is more readily available.

\section sample Sample code

\code
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
    // expression_tree will do the right thing, and discard the previously cached value.

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
