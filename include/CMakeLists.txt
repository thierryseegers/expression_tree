# This is somewhat of a "dummy" target to put all the headersof the expression_tree library in one place in the project structure rather than have all projects hold references to them.

add_custom_target(expression_tree
				  COMMAND cmake -E echo ""
				  SOURCES expression_tree.h)
