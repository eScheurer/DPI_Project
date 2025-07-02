//
// Created by Enya Scheurer on 02/07/2025.
//


#include <stdio.h>
#include "crdt.h"

int main() {
    Node* n1 = createNode("A", "", "Alice", "Hello!", NULL, NULL);
    insert(n1);

    Node* n2 = createNode("B", "A", "Bob", "Hi Alice!", NULL, NULL);
    insert(n2);

    Node* n3 = createNode("C", "A", "Charlie", "Hey!", NULL, NULL);
    insert(n3);

    Node* n4 = createNode("D", "B", "Dana", "How are you?", NULL, NULL);
    insert(n4);

    printf("Current message tree:\n");
    printTree(root, 0);

    freeNode(root); // Am Ende alles freigeben
    return 0;
}
