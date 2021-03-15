#pragma once
#include <iostream>
#include <string>
#include "RedBlackTree.h"
using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::to_string;

RedBlackNode::RedBlackNode(RedBlackNode& nil, int newKey) : key(newKey), left(&nil), right(&nil), parent(&nil) {};
RedBlackNode::RedBlackNode() {};
RedBlackTree::RedBlackTree() : root(nil) {
	nil->left = nil;
	nil->right = nil;
	nil->parent = nil;
};
RedBlackNode* RedBlackTree::closest(int key)
{
	return search(key, true);
};
RedBlackNode* RedBlackTree::search(int key)
{
	return search(key, false);
};
RedBlackNode* RedBlackTree::search(int key, bool closest)
{
	RedBlackNode* x = root;
	RedBlackNode* lastGood = x;
	while (x != nil && key != x->key)
	{
		lastGood = x;
		if (key < x->key)
		{
			x = x->left;
		}
		else
		{
			x = x->right;
		}
	}
	if (closest && x == nil)
	{
		return lastGood;
	}
	else
	{
		return x;
	}
};
void RedBlackTree::leftRotate(RedBlackNode& x)
{
	RedBlackNode* y = x.right;
	x.right = y->left;
	if (y->left != nil)
	{
		y->left->parent = &x;
	}
	y->parent = x.parent;
	if (x.parent == nil)
	{
		root = y;
	}
	else
	{
		if (&x == x.parent->left)
		{
			x.parent->left = y;
		}
		else
		{
			x.parent->right = y;
		}
	}
	y->left = &x;
	x.parent = y;
};
void RedBlackTree::rightRotate(RedBlackNode& x)
{
	RedBlackNode* y = x.left;
	x.left = y->right;
	if (y->right != nil)
	{
		y->right->parent = &x;
	}
	y->parent = x.parent;
	if (x.parent == nil)
	{
		root = y;
	}
	else
	{
		if (&x == x.parent->right)
		{
			x.parent->right = y;
		}
		else
		{
			x.parent->left = y;
		}
	}
	y->right = &x;
	x.parent = y;
};
RedBlackNode* RedBlackTree::binaryInsert(int newKey)
{
	RedBlackNode* z = new RedBlackNode(*(nil), newKey); // delete later?
	RedBlackNode* y = nil;
	RedBlackNode* x = root;
	while (x != nil)
	{
		y = x;
		if (z->key < x->key)
		{
			x = x->left;
		}
		else
		{
			x = x->right;
		}
	}
	z->parent = y;
	if (y == nil)
	{
		root = z;
	}
	else {
		if (z->key < y->key)
		{
			y->left = z;
		}
		else
		{
			y->right = z;
		}
	}
	return z;
};
RedBlackNode* RedBlackTree::insert(int newKey)
{
	if (search(newKey, false) != nil)
	{
		return nil;
	}
	RedBlackNode* x = binaryInsert(newKey);
	RedBlackNode* newNode = x;
	RedBlackNode* y;
	x->color = red;
	while (x != root && x->parent->color == red)
	{
		if (x->parent == x->parent->parent->left)
		{
			y = x->parent->parent->right;
			if (y->color == red)
			{
				x->parent->color = black;
				y->color = black;
				x->parent->parent->color = red;
				x = x->parent->parent;
			}
			else
			{
				if (x == x->parent->right)
				{
					x = x->parent;
					leftRotate(*x);
				}
				x->parent->color = black;
				x->parent->parent->color = red;
				rightRotate(*(x->parent->parent));
			}
		}
		else
		{
			y = x->parent->parent->left;
			if (y->color == red)
			{
				x->parent->color = black;
				y->color = black;
				x->parent->parent->color = red;
				x = x->parent->parent;
			}
			else
			{
				if (x == x->parent->left)
				{
					x = x->parent;
					rightRotate(*x);
				}
				x->parent->color = black;
				x->parent->parent->color = red;
				leftRotate(*(x->parent->parent));
			}
		}
	}
	root->color = black;
	return newNode;
};
RedBlackNode* RedBlackTree::minimum(RedBlackNode* x)
{
	while (x->left != nil)
	{
		x = x->left;
	}
	return x;
};
RedBlackNode* RedBlackTree::maximum(RedBlackNode* x)
{
	while (x->right != nil)
	{
		x = x->right;
	}
	return x;
};
RedBlackNode* RedBlackTree::predecessor(RedBlackNode* x)
{
	RedBlackNode* y;
	if (x->left != nil)
	{
		return maximum(x->left);
	}
	y = x->parent;
	while (y != nil && x == y->left)
	{
		x = y;
		y = y->parent;
	}
	return y;
};
RedBlackNode* RedBlackTree::successor(RedBlackNode* x)
{
	RedBlackNode* y;
	if (x->right != nil)
	{
		return minimum(x->right);
	}
	y = x->parent;
	while (y != nil && x == y->right)
	{
		x = y;
		y = y->parent;
	}
	return y;
};
void RedBlackTree::deleteNodeFixup(RedBlackNode* x)
{
	RedBlackNode* w;
	while (x != root && x->color == black)
	{
		if (x == x->parent->left)
		{
			w = x->parent->right;
			if (w->color == red)
			{
				w->color = black;
				x->parent->color = red;
				leftRotate(*(x->parent));
				w = x->parent->right;
			}
			if (w->left->color == black && w->right->color == black)
			{
				w->color = red;
				x = x->parent;
			}
			else
			{
				if (w->right->color == black)
				{
					w->left->color = black;
					w->color = red;
					rightRotate(*w);
					w = x->parent->right;
				}
				w->color = x->parent->color;
				x->parent->color = black;
				w->right->color = black;
				leftRotate(*(x->parent));
				x = root;
			}
		}
		else
		{
			w = x->parent->left;
			if (w->color == red)
			{
				w->color = black;
				x->parent->color = red;
				rightRotate(*(x->parent));
				w = x->parent->left;
			}
			if (w->right->color == black && w->left->color == black)
			{
				w->color = red;
				x = x->parent;
			}
			else
			{
				if (w->left->color == black)
				{
					w->right->color = black;
					w->color = red;
					leftRotate(*w);
					w = x->parent->left;
				}
				w->color = x->parent->color;
				x->parent->color = black;
				w->left->color = black;
				rightRotate(*(x->parent));
				x = root;
			}
		}
	}
	x->color = black;
};
RedBlackNode* RedBlackTree::deleteNode(RedBlackNode* z)
{
	RedBlackNode* x;
	RedBlackNode* y;
	if (z == nil) {
		return nil;
	}
	if (z->left == nil || z->right == nil)
	{
		y = z;
	}
	else
	{
		y = successor(z);
	}
	if (y->left == nil)
	{
		x = y->right;
	}
	else
	{
		x = y->left;
	}
	x->parent = y->parent;
	if (y->parent == nil)
	{
		root = x;
	}
	else
	{
		if (y == y->parent->left)
		{
			y->parent->left = x;
		}
		else
		{
			y->parent->right = x;
		}
	}
	if (y != z)
	{
		z->key = y->key;
		// copy any other fields
	}
	if (y->color == black)
	{
		deleteNodeFixup(x);
	}
	delete y;
	return nil;
};