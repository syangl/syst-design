#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool()
{
	int i;
	for (i = 0; i < NR_WP; i++)
	{
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP *new_wp()
{
	if (free_ == NULL)
		return NULL;
	WP *free_head, *head_tail;
	free_head = free_; // get one free node
	head_tail = head;  // head_tail is the first in used nodes
	free_ = free_->next;
	free_head->next = NULL;
	if (head_tail == NULL)
		head = free_head;
	else
	{
		while (head_tail->next != NULL)
		{
			head_tail = head_tail->next;
		}
		head_tail->next = free_head; // link free_head on used nodes linklist as tail
	}
	return free_head;
}

void free_wp(WP *wp)
{
	WP *h;
	h = head;
	if (h == wp)
	{
		head = wp->next;
	}
	else
	{ // find wp
		while (h != NULL && h->next != wp)
		{
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
void print_w()
{
	WP *h = head;
	if (h == NULL){
		printf("No watchpoint was set.\n");
	}
	while (h != NULL){
		printf("[Watchpoint %d]\tExpr: %s\tValue: %d\n", h->NO, h->exprs, h->val);
		h = h->next;
	}
}

// check watchpoints
void check_wp(bool *check_flag)
{
	WP *h = head;
	while (h != NULL)
	{
		bool tmp = true;
		uint32_t ans = expr(h->exprs, &tmp);
		if (ans != h->val)
		{
			printf("[Watchpoint %d]\tExpr: %s\tOriginValue: %d\tNewValue: %d\n", h->NO, h->exprs, h->val, ans);
			h->val = ans;
			*check_flag = true;
		}
		h = h->next;
	}
}

// delete watchpoints
WP *delete_wp(int id, bool *success){
	WP *h = head;
	while (h != NULL && h->NO != id){
		h = h->next;
	}
	if (h == NULL){
		*success = false;
	}
	return h;
}