//
// Created by maria on 5/8/17.

#include <string.h>
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include "list_utils.h"
#define KEY_LEN 8
extern int opt_yield;

void SortedList_insert(SortedList_t* list, SortedListElement_t* element){
    if (element->key == NULL || list->key != NULL){ //assuming, of course, that list->key is 0x00000000 as required...
        fprintf(stderr, "The key given isn't valid, or the list is not a head pointer\n");
        exit(EXIT_FAILURE); //this isn't a list error per se, but it *is* extremely unlikely, and is illegal as per: spec
    }
    //SortedListElement_t* item = list;
    if (!list->next){
        list->next = element;
        element->prev = list;
        element->next = NULL;
        return;
    }
    for (SortedListElement_t* t = list->next; t; t = t->next){
        //t->prev should always be a valid pointer (the first pointer is after the overall list pointer)
        if ((t->prev && t != t->prev->next) || (t->next && t!= t->next->prev)) {
            //(prev is valid AND links are broken) OR (next is valid AND links are broken)
            exit(EXIT_LIST_FAILURE);
        }
        if (memcmp(element->key, t->key, KEY_LEN) < 0 || !t->next) { //if element->key > t->key, >1 (keep iterating until it's >=
            element->prev = t->prev;
            element->next = t;
            t->prev->next = element;
            t->prev = element;
            return;
        }
    }
}

int SortedList_delete(SortedListElement_t *element){
    if (!element){
        return 0;
    }
    if (element->prev){
        if (element->prev->next != element){
            return 1;
        }
        element->prev->next = element->next;
    }
    if (opt_yield & DELETE_YIELD){
        sched_yield();
    }
    if (element->next){
        if (element->next->prev != element){
            //I would free the element but, the entire list is garbage now so meh
            return 1;
        }
        element->next->prev = element->prev;
    }
    //free(element);
    return 0;
}

SortedListElement_t* SortedList_lookup(SortedList_t *list, const char* key){
    //SortedListElement_t* temp = list;
    for (list=list->next; list; list=list->next){
        if (memcmp(key, list->key, KEY_LEN) == 0){
            return list;
        }
    }
    return NULL;
}

int SortedList_length(SortedList_t *list){
    int len = 0;
    for (SortedListElement_t* item = list->next; item; item=item->next){
        if (item->next){
            if (item->next->prev != item){
                return -1;
            }
        }
        if (item->prev && !item->prev->key){
            if (item->prev->next != item){
                return -1;
            }
        }
        ++len;
    }
    return len;
}