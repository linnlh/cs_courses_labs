/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "sdb.h"

#define NR_WP 32
#define NR_EXPR 128

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char expr[128];
  word_t val;

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    wp_pool[i].val = 0;
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* find_wp(int no) {
  WP* p = head;
  while(p != NULL && p->NO != no)
    p = p->next;

  return p;
}

WP* allocate_wp() {
  if(free_ == NULL) {
    Log("There is no free watchpoint in the pool.");
    assert(0);
  }

  WP* node = free_;
  free_ = free_->next;
  assert(free_ != NULL);

  node->next = head;
  head = node;

  return node;
}

void free_wp(WP *wp) {
  memset(wp->expr, 0, sizeof(wp->expr));
  wp->val = 0;

  if(wp == head) {
    head = wp->next;
  }
  else {
    WP* p = head;
    while(p->next != wp)
      p = p->next;
    
    p->next = wp->next;
  }

  wp->next = free_;
  free_ = wp;
}

void new_wp(char *expr) {
  WP* wp = allocate_wp();
  strcpy(wp->expr, expr);

  printf("watchpoint %d: %s\n", wp->NO, wp->expr);
}

void del_wp(int no) {
  WP* wp = find_wp(no);

  if(wp == NULL)
    Log("watchpoint %d is not exist.", no);
  else
    free_wp(wp);
}

void wp_display() {
  printf("Num            Type           What\n");
  WP* p = head;

  while(p != NULL) {
    printf("%-15dwatchpoint     %s\n", p->NO, p->expr);
    p = p->next;
  }
}

bool check_wp_is_changed() {
  bool is_changed = false;
  bool success;

  for(WP *p = head; p != NULL; p = p->next) {
    word_t val = expr(p->expr, &success);
    if(val != p->val) {
      printf("watchpoint %d: %s\n\n", p->NO, p->expr);
      printf("Old value = %lu\n", p->val);
      printf("New value = %lu\n", val);

      p->val = val;
      is_changed = true;
    }
  }

  return is_changed;
}
