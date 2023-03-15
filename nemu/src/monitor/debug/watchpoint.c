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
	WP *free_head, *head_tail;
	free_head = free_; // get one free node
	head_tail = head; // head_tail is the first in used nodes
	free_ = free_ -> next;
	free_head -> next = NULL;
	if (head_tail == NULL) head = free_head;
	else {
		while (head_tail -> next != NULL){
			head_tail = head_tail -> next;
		}
		head_tail -> next = free_head; // link free_head on used nodes linklist as tail
	}
	return free_head;
}

void free_wp(WP *wp){
	WP *f_head, *h;
	f_head = free_;
	h = head;
	if (h == wp){
		head = wp->next;
	}
	else{ // find wp
		while (h != NULL && h->next != wp){
			h = h->next;
		}
		h->next = h->next->next; // h->next is wp
	}
	// free wp
	wp->next = free_;
	free_ = wp;
	wp->exprs[0] = '\0';
	wp->val = 0;
}

//info w print
void print_w(){
	// todo
}