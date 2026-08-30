
#ifndef _LINUX_PRIO_TREE_H
#define _LINUX_PRIO_TREE_H
/*
 * K&R 2nd ed. A8.3 somewhat obliquely hints that initial sequences of struct
 * fields with identical types should end up at the same location. We'll use
 * this until we can scrap struct raw_prio_tree_node.
 *
 * Note: all this could be done more elegantly by using unnamed union/struct
 * fields. However, gcc 2.95.3 and apparently also gcc 3.0.4 don't support this
 * language extension.
 */
struct raw_prio_tree_node {
	struct prio_tree_node	*left;
	struct prio_tree_node	*right;
	struct prio_tree_node	*parent;
};

#endif