/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * Copyright (C) 2018 Institute of Computing
 * Technology, CAS Author : Han Shukai (email :
 * hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * Changelog: 2019-8 Reimplement queue.h.
 * Provide Linux-style doube-linked list instead of original
 * uprevendable Queue implementation. Luming
 * Wang(wangluming@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * */

#ifndef INCLUDE_LIST_H_
#define INCLUDE_LIST_H_

#include <type.h>

// double-linked list
//   TODO: use your own list design!!
typedef struct list_node
{    
    struct list_node *prev, *next;
    int   priority;
} list_node_t;

typedef list_node_t list_head;


#define LIST_HEAD(name) struct list_node name = {&(name), &(name)}

#define list_entry(ptr, type, member)                      \
    container_of(ptr, type, member)

#define container_of(ptr, type, member)                    \
    ({                                                     \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);   \
    (type *)( (char *)__mptr -  offsetof(type,member) );   \
    })

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

static inline void init_list(list_head *list){
    list->prev=list;
    list->next=list;
}


static inline void add_element(list_node_t *node,list_node_t *head){
    (head->prev)->next=node;
    node->prev=head->prev;
    node->next=head;
    head->prev=node;
}

static inline void delete_element(list_node_t *node){
    if (node->next!=NULL&&node->prev!=NULL) {
        (node->prev)->next=node->next;
        (node->next)->prev=node->prev;
        node->next=NULL;
        node->prev=NULL;
    }
}

static inline int is_empty(list_head *head)
{
    if(head==head->prev){
        return 1;
    }else
        return 0;
}

static inline void add_element_priority(list_node_t *node, list_node_t *head){
    list_node_t *current_node = head->next;
    list_node_t *next_node;
    if(current_node==head){

        add_element(node,head);
        return;
    }
    while (current_node!=head){
        next_node = current_node->next;
        if (current_node->priority <= node->priority){
            add_element(node,current_node);
            return;
        }
        current_node = next_node;
    }
    add_element(node,head);     
}

#endif
