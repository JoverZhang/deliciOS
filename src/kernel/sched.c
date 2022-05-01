// SPDX-License-Identifier: GPL-2.0-only

#include "kernel/sched.h"
#include "kernel/kernel.h"
#include "kernel/isr.h"
#include "lib/priority_queue.h"
#include "mm/mm.h"

#define DEFAULT_STACK     0x2000000
#define DEFAULT_PRIORITY  100

// this is first task
Task boot_task = {
    .pid = 1,
    .counter = DEFAULT_PRIORITY,
    .priority = DEFAULT_PRIORITY,
    .state = TASK_RUNNING,
    .stack = DEFAULT_STACK,
    .ts.rbp = DEFAULT_STACK,
    .ts.rsp = DEFAULT_STACK,
    .ts.rip = 0,
};

// task node use for task queue.
// and pointer to current task (like linux)
static TaskNode *current_node;

// TaskNode comparator
static i32 task_node_cmp(TaskNode *a, TaskNode *b);
// task queue
PRIORITY_QUEUE(TaskQueue, TaskNode, NR_TASKS, task_node_cmp) task_queue;

/**
 * TODO(jover): process switching using kernel stack
 *   The following code is so bad.
 */
#define switch_to(next) do {        \
  /* save context */                \
  prev->ts.rax = reg->rax;          \
  prev->ts.rcx = reg->rcx;          \
  prev->ts.rdx = reg->rdx;          \
  prev->ts.rbx = reg->rbx;          \
  prev->ts.rsi = reg->rsi;          \
  prev->ts.rdi = reg->rdi;          \
  prev->ts.rip = reg->rip;          \
  prev->ts.rbp = reg->rbp;          \
  prev->ts.rsp = reg->user_rsp;     \
  prev->ts.r8 = reg->r8;            \
  prev->ts.r9 = reg->r9;            \
  prev->ts.r10 = reg->r10;          \
  prev->ts.r11 = reg->r11;          \
  prev->ts.r12 = reg->r12;          \
  prev->ts.r13 = reg->r13;          \
  prev->ts.r14 = reg->r14;          \
  prev->ts.r15 = reg->r15;          \
  /* change context */              \
  reg->rax = (next)->ts.rax;        \
  reg->rcx = (next)->ts.rcx;        \
  reg->rdx = (next)->ts.rdx;        \
  reg->rbx = (next)->ts.rbx;        \
  reg->rsi = (next)->ts.rsi;        \
  reg->rdi = (next)->ts.rdi;        \
  reg->rip = (next)->ts.rip;        \
  reg->rbp = (next)->ts.rbp;        \
  reg->user_rsp = (next)->ts.rsp;   \
  reg->r8 = (next)->ts.r8;          \
  reg->r9 = (next)->ts.r9;          \
  reg->r10 = (next)->ts.r10;        \
  reg->r11 = (next)->ts.r11;        \
  reg->r12 = (next)->ts.r12;        \
  reg->r13 = (next)->ts.r13;        \
  reg->r14 = (next)->ts.r14;        \
  reg->r15 = (next)->ts.r15;        \
} while (0)

// This is a very simple scheduler function.
// Its average time complexity is O(logN),
// because it uses "big-endian heap" for task index.
public void schedule(Registers *reg) {
  Task *prev = current;
  // decrease counter of current process
  prev->counter--;

  // recalculate priority of process counter
  remove_at(&task_queue, current_node->index);
  offer(&task_queue, current_node);

  // get first task (maximum counter in the queue)
  TaskNode *next_n = peek(&task_queue);
  Task *next = next_n->task;

  // reset all tasks counter when the counter is used up.
  // copy from linux 0.11
  if (!next->counter) {
    for (i32 i = 0; i < task_queue.size; i++) {
      TaskNode *node = task_queue.queue[i];
      Task *p = node->task;
      p->counter = (p->counter >> 1) + p->priority;
    }
  }

  if (next == prev) return;

  current_node = next_n;
  switch_to(next);
}

public int sys_pause(Registers *reg) {
  current->state = TASK_STOPPED;
  schedule(reg);
  return 0;
}

public Task *find_empty_process(void) {
  Task *task = (Task *) kmalloc(sizeof(Task));
  TaskNode *node = kmalloc(sizeof(TaskNode));
  node->task = task;
  offer(&task_queue, node);
  return task;
}

public void wakeup_process(Task *task) {
  task->state = TASK_RUNNING;
}

public void task_print(Task *t) {
  printk("===print task===\n");
  printk("pid: %u, state: %d\n", t->pid, t->state);
  printk("counter: %u, priority: %d\n", t->counter, t->priority);
  printk("stack_start: 0x%08x, rip: 0x%08x\n", t->stack, t->ts.rip);
  printk("rax: 0x%08x, rcx: 0x%08x, rdx: 0x%08x, rbx: 0x%08x\n",
         t->ts.rax, t->ts.rcx, t->ts.rdx, t->ts.rbx);
  printk("rsi: 0x%08x, rdi: 0x%08x, rsp: 0x%08x, rbp: 0x%08x\n",
         t->ts.rsi, t->ts.rdi, t->ts.rsp, t->ts.rbp);
  printk("================\n");
}

public Task *get_current() {
  return current_node->task;
}

public void sched_init(void) {
  current_node = (TaskNode *) kmalloc(sizeof(TaskNode));
  current_node->task = &boot_task;
  offer(&task_queue, current_node);
}

static i32 task_node_cmp(TaskNode *a, TaskNode *b) {
  if (a->task->counter < b->task->counter) return 1;
  else if (a->task->counter > b->task->counter) return -1;
  return 0;
}
