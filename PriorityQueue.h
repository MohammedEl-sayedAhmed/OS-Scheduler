#ifndef PRIORITY_QUEUE_H_INCLUDED
#define PRIORITY_QUEUE_H_INCLUDED

// C code to implement Priority Queue 
// using Linked List 
#include <stdio.h> 
#include <stdlib.h> 
  
// PNode 
typedef struct PNode { 
    void *data; 
    // Lower values indicate higher priority 
    int priority; 
    struct PNode* next; 
  
} PNode; 

// Function to Create A New Node 
PNode* newNode(void *d, int p) 
{ 
    PNode* temp = (PNode*)malloc(sizeof(PNode)); 
    temp->data = d; 
    temp->priority = p; 
    temp->next = NULL; 
  
    return temp; 
} 
  
// Return the value at head 
void* peek(PNode** head) 
{ 
    return (*head)->data; 
} 
// Function to check is list is empty 
int isEmpty(PNode** head) 
{ 
    return (*head) == NULL; 
} 

// Removes the element with the 
// highest priority form the list 
int pop(PNode** head, void *d) 
{ 
    if(isEmpty(head)) {
        return 0;
    }

    PNode* temp = *head; 
    (*head) = (*head)->next; 
//    d = temp->data;
    equate(temp->data, d);
    free(temp); 
    return 1;
}

// Function to push according to priority 
void push(PNode** head, void *d, int p) 
{ 
    PNode* start = (*head); 
  
    // Create new Node 
    PNode* temp = newNode(d, p); 
  
    // Special Case: The head of list has lesser 
    // priority than new node. So insert new 
    // node before head node and change head node. 
    if(isEmpty(head)) {
        (*head) = temp;
    }
    else {
        if ((*head)->priority > p) { 
    
            // Insert New Node before head 
            temp->next = *head; 
            (*head) = temp; 
        } 
        else { 
    
            // Traverse the list and find a 
            // position to insert new node 
            while (start->next != NULL && 
                start->next->priority < p) { 
                start = start->next; 
            } 
    
            // Either at the ends of the list 
            // or at required position 
            temp->next = start->next; 
            start->next = temp; 
        }
    } 
} 




#endif /* PRIORITY_QUEUE_H_INCLUDED */