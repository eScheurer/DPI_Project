//
// Created by Enya Scheurer on 01/07/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crdt.h"

Node* root = NULL; //initialize root

Node* createNode(const char* msgID, const char* parentID, const char* sender, const char* text, Node* child, Node* sibling) {
    Node* node = (Node*)malloc(sizeof(Node));
	if (!node) {
	return NULL;	// if malloc fails
	}
    node->msgID = strdup(msgID); //strdup kopiert den String
    node->parentID = strdup(parentID);
    node->sender = strdup(sender);
    node->text = strdup(text);
    node->child = child;
    node->sibling = sibling;
    return node;
}

Node* findNode(const char* ID, Node* current) {
    if (!current) {
        return NULL;
    }
    if (strcmp(current->msgID, ID) == 0) {
        return current;
    }
    Node* found = findNode(ID, current->child);
    if (found) {
        return found;
    }

    return findNode(ID, current->sibling);
}

void insertSorted(Node** head, Node* newNode) {
	// empty list or new node is smaller than head
	if (!*head || strcmp(newNode->msgID, (*head)->msgID) < 0) {
	newNode->sibling = *head;
	*head = newNode;
	return;
	}
	// insert inbetween or at end
	Node* current = *head;
	while (current->sibling && strcmp(newNode->msgID, current->sibling->msgID) >= 0) {
	current = current->sibling;
	}

	newNode->sibling = current->sibling;
	current->sibling = newNode;
}


void insert(Node* node) {
	if (!root){
		root = node;
		return;
	}
    // if node already in list
	if (findNode(node->msgID, root)) {
		printf("Node already in list\n");
		return;
	}
	// find parten
	Node* parent = findNode(node->parentID, root);
	if (!parent) {
		printf("Parent not found\n"); //TODO: bufferQueue thingy
		return;
	}

	insertSorted(&(parent->child), node);

	//free Node?!!!
}

void freeNode(Node* node) {
    if (!node) return;

    free(node->msgID);
    free(node->parentID);
    free(node->sender);
    free(node->text);

    freeNode(node->child);
    freeNode(node->sibling);
    free(node);
}
//TODO: freeTree?

void printTree(Node* node, int depth) {
    if (!node) return;

    for (int i = 0; i < depth; i++) {
        printf("  "); // Einrückung
    }
    printf("[%s] %s: %s\n", node->msgID, node->sender, node->text);

    printTree(node->child, depth + 1);
    printTree(node->sibling, depth);
}




