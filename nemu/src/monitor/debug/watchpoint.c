#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp() {
	if (free_ == NULL) return NULL;
	WP *f_top, *h_tail;
	f_top = free_; // get one free node
	h_tail = head; // h_tail is the first in used nodes
	free_ = free_ -> next;
	f_top -> next = NULL;
	if (h_tail == NULL) head = f_top;
	else {
		while (h_tail -> next != NULL){
			h_tail = h_tail -> next;
		}
		h_tail -> next = f_top; // link f_top on used nodes linklist as tail
	}
	return f_top;
}
