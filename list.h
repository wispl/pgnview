#ifndef LIST_H
#define LIST_H

#include <stddef.h>

// Doublely instrusive linked list from linux kernel
struct node {
	struct node *prev;
	struct node *next;
};

#define container_of(ptr, type, member) ({				\
 	void *__mptr = (void *)(ptr);					\
	((type *)(__mptr - offsetof(type, member))); })

#define list_entry(ptr, type, member) container_of(ptr, type, member)

#define list_for_each(pos, head) \
	for (pos = (head); pos != (head)->prev; pos = pos->next)

 #define list_for_each_entry(pos, head, member)                         	\
	for (pos = list_entry((head)->next, typeof(*pos), member);      	\
		&pos->member != (head);        					\
		pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, typeof(*pos), member))

static inline void list_init(struct node* head)
{
	head->next = head;
	head->prev = head;
}

static inline void list_add(struct node* head, struct node* new)
{
	new->next = head;
	new->prev = head->prev;
	head->prev->next = new;
	head->prev = new;
}

#endif
