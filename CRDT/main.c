//
// Created by Enya Scheurer on 02/07/2025.
//


#include <stdio.h>
#include "crdt.h"

int main() {
    Node* n1 = createNode("A", "", "Enya", "Hello!");
    insert(n1);

    Node* n2 = createNode("B", "A", "Luis", "Hi Alice!");
    insert(n2);

    Node* n3 = createNode("C", "A", "Jannick", "Hey!");
    insert(n3);

    Node* n4 = createNode("D", "B", "Leila", "How are you?");
    insert(n4);

    printf("Current message tree:\n");
    printTree(root, 0);

    freeNodes(root); // Am Ende alles freigeben
    return 0;
}
