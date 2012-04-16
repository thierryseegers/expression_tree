/*
	(C) Copyright Thierry Seegers 2010-2011. Distributed under the following license:

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

//!\cond .
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
//!\endcond

#include <functional>	// Eventually, that's all we'll need for all compilers.

#if (defined(__GNUG__) && (GCC_VERSION < 40500))
#include <tr1/functional>	// Needed for g++ up to 4.4.x.
#endif

#if (defined(__GNUG__) && (GCC_VERSION >= 40500)) || (defined(_MSC_VER) && (_MSC_VER >= 1700))
#define EXPRESSION_TREE_HAS_FUTURE
#include <future>
#endif

//!\cond .

// Here we define a tr1_ macro that will map to the right namespace depending on the compiler.
#if (defined(__GNUG__) && (GCC_VERSION < 40500)) || (defined(_MSC_VER) && (_MSC_VER < 1600))
#define tr1_ std::tr1	// For g++ up to 4.4.x or for VS 2008, the function class is in the std::tr1 namespace.
#else
#define tr1_ std		// For g++ 4.5.x and VS 2010, the function class is in the std namespace.
#endif

//!\endcond

namespace expression_tree
{

template<typename T, template<typename, typename> class CachingPolicy, class ThreadingPolicy>
class node;

namespace detail
{

//!\brief Operations are what branches perform on their children.
//!
//! Operations must take two Ts as arguments and return a T.
template<typename T>
struct operation
{
	typedef typename tr1_::function<T (const T&, const T&)> t;	//!< Template typedef trick.
};

}

#if defined(EXPRESSION_TREE_HAS_FUTURE)

//!\brief Performs parallel evaluation of a branch's children before applying its operation.
//!
//! This policy is available when \c EXPRESSION_TREE_HAS_FUTURE is defined.
//! It is dependent on the \c \<future\> header.
struct parallel
{
	//!\brief Spawns a parallel evaluation task for the left child and evaluates the right child on the current thread.
	template<typename T, template<typename, typename> class C, class E>
	static T evaluate(const typename detail::operation<T>::t& o, const node<T, C, E>& l, const node<T, C, E>& r)
	{
		std::future<T> f = std::async(&node<T, C, E>::evaluate, &l);

		// Let's not rely on any assumption of parameter evaluation order...
		T t = r.evaluate();

		f.wait(); // Workaround GCC bug: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=52988

		return o(f.get(), t);
	}
};

#endif

//!\brief Performs sequential evaluation of a branch's children before applying its operation.
struct sequential
{
	//!\brief Evaluates the right child and then the left child on the current thread.
	template<typename T, template<typename, typename> class C, class E>
	static T evaluate(const typename detail::operation<T>::t& o, const node<T, C, E>& l, const node<T, C, E>& r)
	{
		return o(l.evaluate(), r.evaluate());
	}
};

namespace detail
{

//!\brief Base class for the node class internal implementation.
template<typename T>
class node_impl
{
public:
	virtual ~node_impl() {}

	//!\brief Clones this object.
	//!
	//! Necessary for assignment operator of classes that will own concrete instances of this base class.
	virtual node_impl<T>* clone() const = 0;

	//!\brief Constness of the node.
	//!
	//! A leaf node is constant if its data is constant.
	//! A branch node is constant if all its leaf nodes are constant.
	virtual bool constant() const = 0;

	//!\brief All nodes must evaluate.
	//!
	//! A leaf will evaluate to itself.
	//! A branch will evaluate to its operation applied to its left and right children.
	virtual T evaluate() const = 0;
};

//!\brief Leaf class.
//!
//! This class stores a copy of its data.
template<typename T>
class leaf : public node_impl<T>
{
	const T value; //!< This node's value.

public:
	//!\brief Constructor.
	//!
	//!\param value The value of this node.
	leaf(const T& value) : value(value) {}

	//!\brief Copy constructor.
	leaf(const leaf<T>& other) : value(other.value) {}

	virtual ~leaf() {}

	//!\brief Clones this object.
	virtual leaf<T>* clone() const
	{
		return new leaf<T>(*this);
	}

	//! Because this classes stores a copy of its data, it is constant.
	virtual bool constant() const
	{
		return true;
	}

	//! Plainly return our value.
	virtual T evaluate() const
	{
		return value;
	}
};

//!\brief Leaf class specialized to T*.
//!
//! This class stores a pointer to data.
template<typename T>
class leaf<T*> : public node_impl<T>
{
	const T *p;	//!< This node's pointer to data.

public:
	//!\brief Constructor
	//!
	//!\param p Pointer to this node's value.
	leaf(const T* p) : p(p) {}

	//!\brief Copy constructor.
	leaf(const leaf<T*>& other) : p(other.p) {}

	virtual ~leaf() {}

	//!\brief Clones this object.
	virtual leaf<T*>* clone() const
	{
		return new leaf<T*>(*this);
	}

	//! Because this class stores a pointer to its data, it is not constant.
	virtual bool constant() const
	{
		return false;
	}

	//! Dereference our pointer.
	virtual T evaluate() const
	{
		return *p;
	}
};

//!\brief Branch class.
//!
//! This class stores an operation and two children nodes.
//! This default implementation does \a no caching optimization.
template<typename T, template<typename, typename> class CachingPolicy, class ThreadingPolicy>
class default_branch : public node_impl<T>
{
public:
	node<T, CachingPolicy, ThreadingPolicy> l;	//!< This branch's left child.
	node<T, CachingPolicy, ThreadingPolicy> r;	//!< This branch's right child.
	typename operation<T>::t f;	//!< Operation to be applied to this node's children.
	
	//!\brief Constructor.
	//!
	//!\param f The operation to apply to this branch's children,
	//!\param l This branch's left child.
	//!\param r This branch's right child.
	default_branch(const typename operation<T>::t& f, const node<T, CachingPolicy, ThreadingPolicy>& l, const node<T, CachingPolicy, ThreadingPolicy>& r) : f(f), l(l), r(r) {}

	//!\brief Copy constructor.
	default_branch(const default_branch<T, CachingPolicy, ThreadingPolicy>& other) : f(other.f), l(other.l), r(other.r) {}
	
	virtual ~default_branch() {}

	//!\brief Clones this object.
	virtual default_branch<T, CachingPolicy, ThreadingPolicy>* clone() const
	{
		return new default_branch<T, CachingPolicy, ThreadingPolicy>(*this);
	}

	//! The constness of a branch is determined by the constness of its children.
	virtual bool constant() const
	{
		return l.constant() && r.constant();
	}

	//! Evaluating a branch applies its operation on its children.
	virtual T evaluate() const
	{
		return ThreadingPolicy::evaluate(f, l, r);
	}

	//!\brief This branch's left child.
	virtual node<T, CachingPolicy, ThreadingPolicy>& left()
	{
		return l;
	}

	//!\brief This branch's right child.
	virtual node<T, CachingPolicy, ThreadingPolicy>& right()
	{
		return r;
	}

	//! This function is called when anyone of this branch's children is modified.
	//! This default implementation does nothing when that happens.
	virtual void grow()
	{
		return;
	}
};

}

template<typename T, class ThreadingPolicy>
struct no_caching;

//!\brief The tree's node class.
//!
//! This class stores a pointer to its implementation.
//! The implementation node's type is derived at runtime when it is assigned to.
template<typename T, template<typename, typename> class CachingPolicy = no_caching, class ThreadingPolicy = sequential>
class node
{
	detail::node_impl<T> *impl;		//!< Follows the pimpl idiom.
	node<T, CachingPolicy, ThreadingPolicy> *parent; //!< This node's parent. Ends up unused when no caching occurs.

public:
	//!\brief Default constructor.
	//!
	//!\param parent Pointer to this node's parent.
	node(node<T, CachingPolicy, ThreadingPolicy> *parent = 0) : impl(0), parent(parent) {}

	//!\brief Copy constructor.
	node(const node<T, CachingPolicy, ThreadingPolicy>& other) : impl(other.impl ? other.impl->clone() : 0), parent(other.parent) {}

	//!\brief Assignment operator.
	node<T, CachingPolicy, ThreadingPolicy>& operator=(const node<T, CachingPolicy, ThreadingPolicy>& other)
	{
		if(this != &other)
		{
			detail::node_impl<T>* c = other.impl->clone();

			if(impl)
			{
				delete impl;
			}

			impl = c;
			parent = other.parent;

			if(parent)
			{
				parent->grow();
			}
		}

		return *this;
	}

	virtual ~node()
	{
		if(impl)
		{
			delete impl;
		}
	}

	//!\brief Assign a value to this node.
	//!
	//! The assignment of a \c T designates this node as a leaf node.
	//! A leaf can still be changed to a branch by assigning an operation to it.
	node<T, CachingPolicy, ThreadingPolicy>& operator=(const T& t)
	{
		if(impl)
		{
			delete impl;
		}

		impl = new detail::leaf<T>(t);

		if(parent)
		{
			parent->grow();
		}

		return *this;
	}

	//!\brief Assign a pointer to this node.
	//!
	//! The assignment of a \c T* designates this node as a leaf node.
	//! A leaf can still be changed to a branch by assigning an operation to it.
	node<T, CachingPolicy, ThreadingPolicy>& operator=(const T* t)
	{
		if(impl)
		{
			delete impl;
		}

		impl = new detail::leaf<T*>(t);

		if(parent)
		{
			parent->grow();
		}

		return *this;
	}

	//!\brief Assign an operation to this node.
	//!
	//! The assignment of an operation designates this node as a branch.
	//! A branch can still be changed to a leaf by assigning data to it.
	node<T, CachingPolicy, ThreadingPolicy>& operator=(const typename detail::operation<T>::t& f)
	{
		if(impl)
		{
			delete impl;
		}

		// Create a new branch with the passed operation and two nodes with this node as their parent.
		impl = new typename CachingPolicy<T, ThreadingPolicy>::branch(f, node<T, CachingPolicy, ThreadingPolicy>(this), node<T, CachingPolicy, ThreadingPolicy>(this));

		if(parent)
		{
			parent->grow();
		}

		return *this;
	}

	//!\brief This node's left child.
	//!
	//! Note that if this node is a leaf node, behavior is undefined.
	node<T, CachingPolicy, ThreadingPolicy>& left()
	{
		return dynamic_cast<typename CachingPolicy<T, ThreadingPolicy>::branch*>(impl)->left();
	}

	//!\brief This node's right child.
	//!
	//! Note that if this node is a leaf node, behavior is undefined.
	node<T, CachingPolicy, ThreadingPolicy>& right()
	{
		return dynamic_cast<typename CachingPolicy<T, ThreadingPolicy>::branch*>(impl)->right();
	}

	//!\brief Constness of this node.
	bool constant() const
	{
		return impl ? impl->constant() : false;
	}

	//!\brief Evaluates the value of this node.
	T evaluate() const
	{
		return impl->evaluate();
	}

	//!\brief Called when this node is assigned to.
	//!
	//! Recursively notifies parent nodes of the growth that happened.
	void grow()
	{
		dynamic_cast<typename CachingPolicy<T, ThreadingPolicy>::branch*>(impl)->grow();

		if(parent)
		{
			parent->grow();
		}
	}
};

//!\brief Implementation of the CachingPolicy used by tree.
template<typename T, class ThreadingPolicy>
struct no_caching
{
	//!\brief Implementation of a branch class that performs no caching.
	//!
	//! This class performs no optimization.
	//! A non-caching branch will apply its operation on its children whenever it is evaluated.
	class branch : public detail::default_branch<T, expression_tree::no_caching, ThreadingPolicy>
	{
	public:
		//!\brief Default constructor.
		branch(const typename detail::operation<T>::t& f, const node<T, expression_tree::no_caching, ThreadingPolicy>& l, const node<T, expression_tree::no_caching, ThreadingPolicy>& r)
			: detail::default_branch<T, expression_tree::no_caching, ThreadingPolicy>(f, l, r) {}

		//!\brief Copy constructor.
		branch(const branch& o) : detail::default_branch<T, expression_tree::no_caching, ThreadingPolicy>(o) {}

		virtual ~branch() {}
	};
};

//!\brief Implementation of the CachingPolicy used by tree.
template<typename T, class ThreadingPolicy>
struct cache_on_evaluation
{
	//!\brief Implementation of a branch class that performs caching on evaluation.
	//!
	//! A caching-on-evaluation branch will apply its operation on its children when it is evaluated
	//! and cache that value if it is constant (e.g. if its children are of constant value).
	class branch : public detail::default_branch<T, expression_tree::cache_on_evaluation, ThreadingPolicy>
	{
		mutable bool cached;	//!< Whether the value of this node can be considered as cached.
		mutable T value;		//!< This node's value, if \c cached is \c true.

		using detail::default_branch<T, expression_tree::cache_on_evaluation, ThreadingPolicy>::f;
		using detail::default_branch<T, expression_tree::cache_on_evaluation, ThreadingPolicy>::l;
		using detail::default_branch<T, expression_tree::cache_on_evaluation, ThreadingPolicy>::r;
		using detail::default_branch<T, expression_tree::cache_on_evaluation, ThreadingPolicy>::constant;

	public:
		//!\brief Default constructor.
		branch(const typename detail::operation<T>::t& f, const node<T, expression_tree::cache_on_evaluation, ThreadingPolicy>& l, const node<T, expression_tree::cache_on_evaluation, ThreadingPolicy>& r)
			: detail::default_branch<T, expression_tree::cache_on_evaluation, ThreadingPolicy>(f, l, r), cached(false) {}

		//!\brief Copy constructor.
		branch(const branch& o) : detail::default_branch<T, expression_tree::cache_on_evaluation, ThreadingPolicy>(o), cached(o.cached), value(o.value) {}

		virtual ~branch() {}

		//! If the value of this branch has been cached already, return it.
		//! Otherwise, evaluate it and determine if this branch is constant.
		//! If it is, considered the value as cached to re-use later.
		virtual T evaluate() const
		{
			if(cached) return value;

			value = ThreadingPolicy::evaluate(f, l, r);

			if(constant())
			{
				cached = true;
			}

			return value;
		}

		//! When this branch grows (e.g. has its children modified), forget that the value was cached.
		virtual void grow()
		{
			cached = false;
		}
	};
};

//!\brief Implementation of the CachingPolicy used by tree.
template<typename T, class ThreadingPolicy>
struct cache_on_assignment
{
	//!\brief Implementation of a branch class that performs caching on assignment of its children.
	//!
	//! When a caching-on-assignment branch' children are assigned to, the branch checks whether its children
	//! are constant. If they are, it applies its operation on them and caches that value.
	class branch : public detail::default_branch<T, expression_tree::cache_on_assignment, ThreadingPolicy>
	{
		mutable bool cached;	//!< Whether the value of this node can be considered as cached.
		mutable T value;		//!< This node's value, if \c cached is \c true.

		using detail::default_branch<T, expression_tree::cache_on_assignment, ThreadingPolicy>::f;
		using detail::default_branch<T, expression_tree::cache_on_assignment, ThreadingPolicy>::l;
		using detail::default_branch<T, expression_tree::cache_on_assignment, ThreadingPolicy>::r;
		using detail::default_branch<T, expression_tree::cache_on_assignment, ThreadingPolicy>::constant;

	public:
		//!\brief Default constructor.
		branch(const typename detail::operation<T>::t& f, const node<T, expression_tree::cache_on_assignment, ThreadingPolicy>& l, const node<T, expression_tree::cache_on_assignment, ThreadingPolicy>& r)
			: detail::default_branch<T, expression_tree::cache_on_assignment, ThreadingPolicy>(f, l, r), cached(false) {}

		//!\brief Copy constructor.
		branch(const branch& o) : detail::default_branch<T, expression_tree::cache_on_assignment, ThreadingPolicy>(o), cached(o.cached), value(o.value) {}

		virtual ~branch() {}

		//! If the value of this branch has been cached already, return it.
		virtual T evaluate() const
		{
			if(cached) return value;

			return value = ThreadingPolicy::evaluate(f, l, r);
		}
		
		//! When this branch has its children modified, check if they are constant.
		//! If they are, perform the operation and cache the value.
		virtual void grow()
		{
			if(constant())
			{
				// If this node is constant, cache its value now.
				cached = true;
				value = ThreadingPolicy::evaluate(f, l, r);
			}
			else
			{
				// One or both children are not constant. This node is then not constant.
				cached = false;
			}
		}
	};
};

//!\brief Implements an expression tree.
//!
//!\param T The data type.
//!\param CachingPolicy Caching optimization policy to use. Choices are:
//! - no_caching: no caching optimization is performed.
//! - cache_on_evaluation: caching of branches' values is performed when they are evaluated.
//! - cache_on_assignment: caching of branches' values is performed when they are modified.
//!\param ThreadingPolicy Threading policy to use when evaluating a branch's children. Choices are:
//! - \link expression_tree::sequential sequential\endlink: evaluate children on after the after on a single thread.
//! - \link expression_tree::parallel parallel\endlink: evaluate children in parallel as hardware permits using \c std::async.
template<typename T, template<typename, typename> class CachingPolicy = no_caching, class ThreadingPolicy = sequential>
class tree : public node<T, CachingPolicy, ThreadingPolicy>
{
public:
	virtual ~tree() {}

	//!\brief This tree's root node.
	node<T, CachingPolicy, ThreadingPolicy>& root()
	{
		return *this;
	}
};

}

#endif

/*!
\file expression_tree.h
\brief The only file you need.
\author Thierry Seegers
\version 3.0

\mainpage expression_tree

 - \ref introduction
 - \ref considerations
 - \ref optimizations
  - \ref caching
   - \ref evaluation
   - \ref assignment
  - \ref multithreaded
 - \ref improvements
 - \ref sample
 - \ref license

\section introduction Introduction

An expression tree is a tree that stores data in its leaf nodes and operations in its branch nodes.
The tree's value can then be obtained by performing a postorder traversal, applying each branch's operation to its children.

For example, this tree evaluates to (2 * 1 + (2 - \a x)):

\code
(2 * l + r)
 /       \
1      (l - r)
        /   \
      2      x
\endcode

To build an expression tree with expression_tree::tree, you assign <a href="http://www.google.com/search?q=c%2B%2B+function+object">function objects</a> to branch nodes and either constant values or pointers to variables to leaf nodes.
When your tree is built, call its \link expression_tree::tree::evaluate evaluate \endlink member function to get its value.

\section considerations Technical considerations

This implementation:
 - is contained in a single header file.
 - uses templates heavily.
 - specializes the branches and the leaves to reduce space overhead.
 - is dependent on C++11's function<> class
 - can take advantage of multithreading hardware if the \c \<future\> header is available.
 - requires RTTI.
 - has little in the way of safety checks.
 - has been tested with GCC 4.2.1, GCC 4.5.0, GCC 4.6.3, VS 2008, VS 2010 and VS 11 Beta.
  - Note that, as of this writing, GCC up to version 4.7 does not implement std::async <a href="http://gcc.gnu.org/bugzilla/show_bug.cgi?id=51617">as one would expect</a>.
    Thus, \ref multithreaded "parallel evaluation" will only truly be parallel when the implementation is corrected.

In order to be evaluated, the tree must be correctly formed.
That is, all its branch nodes' children nodes must have been given a value.

\section optimizations Optimizations

Two policies are vailable as template parameters to expression_tree::tree.
The first enables caching the value of certain branches to avoid unnecessary evaluation.
The second enables multi-threaded evaluation.

\subsection caching Branch caching

Caching optimizations rely on the constness of branches.
Once a constant branch has been evaluated, it is not required to evaluate it again.
A branch is considered constant when all its children are constant, be they branches or leaves.
A leaf is considered constant when its value is assigned a constant value rather than a variable value (i.e. a pointer).

The first optimization caches a branch's value when a it is first evaluated.
The second, more aggressive optimziation, caches when a branch's children are assigned to.
The default policy is \link expression_tree::no_caching no_caching \endlink which performs no caching.

\subsubsection evaluation Caching-on-evaluation optimization

By instantiating a \link expression_tree::tree tree \endlink with its second template parameter set to 
\link expression_tree::cache_on_evaluation cache_on_evaluation \endlink, a tree's evaluation will be optimzed by caching-on-evaluation.
Caching on evaluation simply consists on remembering a branch's value at the time it is evaluated, if that branch is considered constant.

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

Because one of B<SUB>1</SUB> children is not constant, evaluating B<SUB>1</SUB> will always perform its operation on its two children.

\subsubsection assignment Caching-on-assignment optimization

By instantiating a \link expression_tree::tree tree \endlink with its second template parameter set to 
\link expression_tree::cache_on_assignment cache_on_assignment \endlink, a tree's evaluation will be optimzed by caching-on-assignment.
Caching-on-assignment means that when a branch's children nodes are assigned to, and if those children are constant, the branch's value is evaluated and cached.

Consider the following tree, where B<SUB>n</SUB> is a branch and C<SUB>n</SUB> is a constant value:

\code
  B1
 /  \
C1  B2
   /  \
  C2  C3
\endcode

Let's assume that that tree is constructed in the following order: B<SUB>1</SUB>, C<SUB>1</SUB>, B<SUB>2</SUB>, C<SUB>2</SUB>, C<SUB>3</SUB>.

Upon assignment of C<SUB>3</SUB>, B<SUB>2</SUB> will be found to be of constant value and be pre-evaluated.
This pre-evaluation will continue recursively up the tree for as long as a branch's both children are constant.
In this case, B<SUB>1</SUB> will also be pre-evaluated.

If C<SUB>1</SUB> had instead been a variable (e.g. \a x), only B<SUB>2</SUB> will have been pre-evaluated.
Evaluating B1 would then always perform its operation on its two children.

\subsubsection degenerate Degenerate case of caching-on-assignment optimization

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

Let's assume that that tree is constructed in the following order: B<SUB>1</SUB>, C<SUB>1</SUB>, B<SUB>2</SUB>, C<SUB>2</SUB>, ..., B<SUB>n</SUB>, C<SUB>n</SUB>, C<SUB>n+1</SUB>.

Before C<SUB>n+1</SUB> is assigned to, none of the tree has been pre-evaluated, given that none of its nodes' constness can be confirmed.
When C<SUB>n+1</SUB> is assigned to, B<SUB>n</SUB> will be found constant and be evaluated.
B<SUB>n-1</SUB> will also be found constant and be evaluated.
This will continue until entire tree is evaluated.
Thus, a single assignment can trigger the equivalent of \link expression_tree::tree::evaluate() evaluate() \endlink.

\subsection multithreaded Parallel evaluation

This optimization depends on the availability of C++11's \c \<future\> header.
It is enabled automatically if your toolchain supports it.
To benefit from parallel evaluation, a tree must be instantiated a \link expression_tree::tree tree \endlink with the \link expression_tree::parallel parallel \endlink policy class.

When enabled, a branch will evaluate one of its children on the current thread and its other child on a separate, hardware permitting.
The decision to actually spawn a seperate thread is left to \c std::async's implementation.
For large enough tree's the hardware limit will be reached and branches will start evaluating their children sequentially regardless of their threading policy.

The default policy is \link expression_tree::sequential sequential \endlink which evalutes the tree sequentially.

\section improvements Future improvements

 - I'll think of something. I can't help myself.

\section sample Sample code

\include examples/main.cpp

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
