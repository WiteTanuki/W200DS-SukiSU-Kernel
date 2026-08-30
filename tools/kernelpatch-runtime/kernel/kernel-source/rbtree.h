#ifndef _LINUX_RBTREE_H
#define _LINUX_RBTREE_H

struct rb_node
{
 unsigned long rb_parent_color;
#define RB_RED 0
#define RB_BLACK 1
 struct rb_node *rb_right;
 struct rb_node *rb_left;
} __attribute__((aligned(sizeof(long))));


struct rb_root {
	struct rb_node *rb_node;
};

#endif