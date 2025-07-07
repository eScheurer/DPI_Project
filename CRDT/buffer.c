//
// Created by Enya Scheurer on 07/07/2025.
//
#include <stdlib.h>

#include "buffer.h"

Buffer* CreateBuffer() {
    Buffer* buffer = malloc(sizeof(Buffer));
    buffer->head = NULL;
    buffer->tail = NULL;
    return buffer;
}

void AddToBuffer(Buffer* buffer, Node* node) {
    BufferNode* newNode = malloc(sizeof(BufferNode));
    newNode->node = node;
    newNode->next = NULL;
    buffer->tail->next = newNode;
    buffer->tail = newNode;
}

void removeFromBuffer(Buffer* buffer, BufferNode* node) {
    BufferNode* current = buffer->head;
    BufferNode* previous = NULL;

    while (current) {
        if (current == node) {
            if (previous) {
                previous->next = current->next;
            } else {
                buffer->head = current->next;
            }
            free(current);
            break;
        }
    }
}

void checkBuffer(Buffer* buffer) {
    BufferNode** current = &(buffer->head);

    while (*current) {
        Node* node = (*current)->node;
        ID parent = node->parentID;
        if (findNode(parent, root)) {
            insert(node);
            removeFromBuffer(buffer, node);
        }
        current = &((*current)->next);
    }
}