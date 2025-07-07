//
// Created by Enya Scheurer on 07/07/2025.
//

#ifndef BUFFER_H
#define BUFFER_H
#include "crdt.h"

typedef struct BufferNode {
    Node* node;
    struct BufferNode* next;
} BufferNode;

typedef struct Buffer {
    BufferNode* head;
    BufferNode* tail;
} Buffer;

Buffer* CreateBuffer();
void AddToBuffer(Buffer* buffer, Node* node);
void removeFromBuffer(Buffer* buffer, Node* node);
void checkBuffer(Buffer* buffer);

#endif //BUFFER_H
