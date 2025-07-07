//
// Created by Enya Scheurer on 01/07/2025.
//

// CRDT nach RGA
#ifndef CRDT_H
#define CRDT_H

typedef struct ID {
    char* clientID;
    int seqNum;
} ID;

typedef struct Node {
    ID msgID;
    ID parentID;
    char* sender;
    char* text;
    struct Node* child;
    struct Node* sibling;
} Node;

Node* createNode(const char* msgID, const char* parentID, const char* sender, const char* text);
Node* findNode(const char* ID, Node* current);
void insert(Node* node);
void insertSorted(Node** head, Node* newNode);
void printTree(Node* node, int depth);
void freeNodes(Node* node);

extern Node* root;

#endif

