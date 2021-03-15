#pragma once
#include <iostream>
#include <string>
using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::to_string;

enum RedBlackColor { red, black };
class RedBlackNode
{
public:
	RedBlackColor color = black;
	RedBlackNode* left;
	RedBlackNode* right;
	RedBlackNode* parent;
	int key;

	RedBlackNode(RedBlackNode& nil, int newKey);
	RedBlackNode();
};

class RedBlackTree
{
public:
	RedBlackNode* nil = new RedBlackNode;
	RedBlackNode* root = nil;
	RedBlackTree();
	RedBlackNode* closest(int key);
	RedBlackNode* search(int key);
	RedBlackNode* search(int key, bool closest);
	void leftRotate(RedBlackNode& x);
	void rightRotate(RedBlackNode& x);
	RedBlackNode* binaryInsert(int newKey);
	RedBlackNode* insert(int newKey);
	RedBlackNode* minimum(RedBlackNode* x);
	RedBlackNode* maximum(RedBlackNode* x);
	RedBlackNode* predecessor(RedBlackNode* x);
	RedBlackNode* successor(RedBlackNode* x);
	void deleteNodeFixup(RedBlackNode* x);
	RedBlackNode* deleteNode(RedBlackNode* z);
};