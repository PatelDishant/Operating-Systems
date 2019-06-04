#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/current.h>
#include <asm/ptrace.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <asm/unistd.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/syscalls.h>
#include "interceptor.h"


MODULE_DESCRIPTION("My kernel module");
MODULE_AUTHOR("Me");
MODULE_LICENSE("GPL");

//----- System Call Table Stuff ------------------------------------
/* Symbol that allows access to the kernel system call table */
extern void* sys_call_table[];

/* The sys_call_table is read-only => must make it RW before replacing a syscall */
void set_addr_rw(unsigned long addr) {

	unsigned int level;
	pte_t *pte = lookup_address(addr, &level);

	if (pte->pte &~ _PAGE_RW) pte->pte |= _PAGE_RW;

}

/* Restores the sys_call_table as read-only */
void set_addr_ro(unsigned long addr) {

	unsigned int level;
	pte_t *pte = lookup_address(addr, &level);

	pte->pte = pte->pte &~_PAGE_RW;

}
//-------------------------------------------------------------


//----- Data structures and bookkeeping -----------------------
/**
 * This block contains the data structures needed for keeping track of
 * intercepted system calls (including their original calls), pid monitoring
 * synchronization on shared data, etc.
 * It's highly unlikely that you will need any globals other than these.
 */

/* List structure - each intercepted syscall may have a list of monitored pids */
struct pid_list {
	pid_t pid;
	struct list_head list;
};


/* Store info about intercepted/replaced system calls */
typedef struct {

	/* Original system call */
	asmlinkage long (*f)(struct pt_regs);

	/* Status: 1=intercepted, 0=not intercepted */
	int intercepted;

	/* Are any PIDs being monitored for this syscall? */
	int monitored;
	/* List of monitored PIDs */
	int listcount;
	struct list_head my_list;
}mytable;

/* An entry for each system call */
mytable table[NR_syscalls+1];

/* Access to the table and pid lists must be synchronized */
spinlock_t pidlist_lock = SPIN_LOCK_UNLOCKED;
spinlock_t calltable_lock = SPIN_LOCK_UNLOCKED;
//-------------------------------------------------------------


//----------LIST OPERATIONS------------------------------------
/**
 * These operations are meant for manipulating the list of pids
 * Nothing to do here, but please make sure to read over these functions
 * to understand their purpose, as you will need to use them!
 */

/**
 * Add a pid to a syscall's list of monitored pids.
 * Returns -ENOMEM if the operation is unsuccessful.
 */
static int add_pid_sysc(pid_t pid, int sysc)
{
	struct pid_list *ple=(struct pid_list*)kmalloc(sizeof(struct pid_list), GFP_KERNEL);

	if (!ple)
		return -ENOMEM;

	INIT_LIST_HEAD(&ple->list);
	ple->pid=pid;

	list_add(&ple->list, &(table[sysc].my_list));
	table[sysc].listcount++;

	return 0;
}

/**
 * Remove a pid from a system call's list of monitored pids.
 * Returns -EINVAL if no such pid was found in the list.
 */
static int del_pid_sysc(pid_t pid, int sysc)
{
	struct list_head *i;
	struct pid_list *ple;

	list_for_each(i, &(table[sysc].my_list)) {

		ple=list_entry(i, struct pid_list, list);
		if(ple->pid == pid) {

			list_del(i);
			kfree(ple);

			table[sysc].listcount--;
			/* If there are no more pids in sysc's list of pids, then
			 * stop the monitoring only if it's not for all pids (monitored=2) */
			if(table[sysc].listcount == 0 && table[sysc].monitored == 1) {
				table[sysc].monitored = 0;
			}

			return 0;
		}
	}

	return -EINVAL;
}

/**
 * Remove a pid from all the lists of monitored pids (for all intercepted syscalls).
 * Returns -1 if this process is not being monitored in any list.
 */
static int del_pid(pid_t pid)
{
	struct list_head *i, *n;
	struct pid_list *ple;
	int ispid = 0, s = 0;

	for(s = 1; s < NR_syscalls; s++) {

		list_for_each_safe(i, n, &(table[s].my_list)) {

			ple=list_entry(i, struct pid_list, list);
			if(ple->pid == pid) {

				list_del(i);
				ispid = 1;
				kfree(ple);

				table[s].listcount--;
				/* If there are no more pids in sysc's list of pids, then
				 * stop the monitoring only if it's not for all pids (monitored=2) */
				if(table[s].listcount == 0 && table[s].monitored == 1) {
					table[s].monitored = 0;
				}
			}
		}
	}

	if (ispid) return 0;
	return -1;
}

/**
 * Clear the list of monitored pids for a specific syscall.
 */
static void destroy_list(int sysc) {

	struct list_head *i, *n;
	struct pid_list *ple;

	list_for_each_safe(i, n, &(table[sysc].my_list)) {

		ple=list_entry(i, struct pid_list, list);
		list_del(i);
		kfree(ple);
	}

	table[sysc].listcount = 0;
	table[sysc].monitored = 0;
}

/**
 * Check if two pids have the same owner - useful for checking if a pid
 * requested to be monitored is owned by the requesting process.
 * Remember that when requesting to start monitoring for a pid, only the
 * owner of that pid is allowed to request that.
 */
static int check_pid_from_list(pid_t pid1, pid_t pid2) {

	struct task_struct *p1 = pid_task(find_vpid(pid1), PIDTYPE_PID);
	struct task_struct *p2 = pid_task(find_vpid(pid2), PIDTYPE_PID);
	if(p1->real_cred->uid != p2->real_cred->uid)
		return -EPERM;
	return 0;
}

/**
 * Check if a pid is already being monitored for a specific syscall.
 * Returns 1 if it already is, or 0 if pid is not in sysc's list.
 */
static int check_pid_monitored(int sysc, pid_t pid) {

	struct list_head *i;
	struct pid_list *ple;

	list_for_each(i, &(table[sysc].my_list)) {

		ple=list_entry(i, struct pid_list, list);
		if(ple->pid == pid)
			return 1;

	}
	return 0;
}
//----------------------------------------------------------------

//----- Intercepting exit_group ----------------------------------
/**
 * Since a process can exit without its owner specifically requesting
 * to stop monitoring it, we must intercept the exit_group system call
 * so that we can remove the exiting process's pid from *all* syscall lists.
 */

/**
 * Stores original exit_group function - after all, we must restore it
 * when our kernel module exits.
 */
void (*orig_exit_group)(int);

/**
 * Our custom exit_group system call.
 *
 * TODO When a process exits, make sure to remove that pid from all lists.
 * The exiting process's PID can be retrieved using the current variable (current->pid).
 * Don't forget to call the original exit_group.
 */
void my_exit_group(int status) {
  // lock the pid lists, make sure to unlock after CS
  spin_lock(&pidlist_lock);
  // remove process from all lists
  del_pid(current->pid);
  // CS has ended
  spin_unlock(&pidlist_lock);
  // call original exit_group passing the status of the exit
  (*orig_exit_group)(status);
}
//----------------------------------------------------------------



/**
 * This is the generic interceptor function.
 * It should just log a message and call the original syscall.
 *
 * TODO: Implement this function.
 * - Check first to see if the syscall is being monitored for the current->pid.
 * - Recall the convention for the "monitored" flag in the mytable struct:
 *     monitored=0 => not monitored
 *     monitored=1 => some pids are monitored, check the corresponding my_list
 *     monitored=2 => all pids are monitored for this syscall
 * - Use the log_message macro, to log the system call parameters!
 *     Remember that the parameters are passed in the pt_regs registers.
 *     The syscall parameters are found (in order) in the
 *     ax, bx, cx, dx, si, di, and bp registers (see the pt_regs struct).
 *     (syscall), 1-6 passing paramteres
 * - Don't forget to call the original system call, so we allow processes to proceed as normal.
 */
asmlinkage long interceptor(struct pt_regs reg) {
  // check if the syscall is being monitored for current pid
  int monitored = check_pid_monitored((int)reg.ax, current->pid);
  // log message if being monitored
  if (monitored > 0) {
    log_message(current->pid, reg.ax, reg.bx, reg.cx, reg.dx, reg.si, reg.di, reg.bp);
  }
  table[reg.ax].f(reg);
}

long request_syscall_intercept(int syscall);
long request_syscall_release(int syscall);
long request_start_monitoring(int syscall, int pid);
long request_stop_monitoring(int syscall, int pid);

/**
 * My system call - this function is called whenever a user issues a MY_CUSTOM_SYSCALL system call.
 * When that happens, the parameters for this system call indicate one of 4 actions/commands:
 *      - REQUEST_SYSCALL_INTERCEPT to intercept the 'syscall' argument
 *      - REQUEST_SYSCALL_RELEASE to de-intercept the 'syscall' argument
 *      - REQUEST_START_MONITORING to start monitoring for 'pid' whenever it issues 'syscall'
 *      - REQUEST_STOP_MONITORING to stop monitoring for 'pid'
 *      For the last two, if pid=0, that translates to "all pids".
 *
 * TODO: Implement this function, to handle all 4 commands correctly.
 *
 ** - For each of the commands, check that the arguments are valid (-EINVAL):
 **   a) the syscall must be valid (not negative, not > NR_syscalls, and not MY_CUSTOM_SYSCALL itself)
 **   b) the pid must be valid for the last two commands. It cannot be a negative integer,
 **      and it must be an existing pid (except for the case when it's 0, indicating that we want
 **      to start/stop monitoring for "all pids").
 **      If a pid belongs to a valid process, then the following expression is non-NULL:
 **          pid_task(find_vpid(pid), PIDTYPE_PID)
 **- Check that the caller has the right permissions (-EPERM)
 * *     For the first two commands, we must be root (see the current_uid() macro).
 * *     For the last two commands, the following logic applies:
 * *       - is the calling process root? if so, all is good, no doubts about permissions.
 * *       - if not, then check if the 'pid' requested is owned by the calling process
 * *       - also, if 'pid' is 0 and the calling process is not root, then access is denied
 * *         (monitoring all pids is allowed only for root, obviously).
 * *     To determine if two pids have the same owner, use the helper function provided above in this file.
 * - Check for correct context of commands (-EINVAL):
 * *    a) Cannot de-intercept a system call that has not been intercepted yet.
 * *    b) Cannot stop monitoring for a pid that is not being monitored, or if the
 * *       system call has not been intercepted yet.
 * - Check for -EBUSY conditions:
 * *    a) If intercepting a system call that is already intercepted.
 * *    b) If monitoring a pid that is already being monitored.
 * - If a pid cannot be added to a monitored list, due to no memory being available,
 *   an -ENOMEM error code should be returned.
 *
 * - Make sure to keep track of all the metadata on what is being intercepted and monitored.
 *   Use the helper functions provided above for dealing with list operations.
 *
 * - Whenever altering the sys_call_table, make sure to use the set_addr_rw/set_addr_ro functions
 *   to make the system call table writable, then set it back to read-only.
 *   For example: set_addr_rw((unsigned long)sys_call_table);
 *   Also, make sure to save the original system call (you'll need it for 'interceptor' to work correctly).
 *
 * - Make sure to use synchronization to ensure consistency of shared data structures.
 *   Use the calltable_spinlock and pidlist_spinlock to ensure mutual exclusion for accesses
 *   to the system call table and the lists of monitored pids. Be careful to unlock any spinlocks
 *   you might be holding, before you exit the function (including error cases!).
 */
asmlinkage long my_syscall(int cmd, int syscall, int pid) {
  // check the syscall to make sure it passes
  if(0<syscall && syscall < NR_syscalls && syscall != MY_CUSTOM_SYSCALL){
    if(cmd == REQUEST_SYSCALL_RELEASE || cmd == REQUEST_SYSCALL_INTERCEPT){
      // user must be root to call these, uid will be 0
      if(current_uid() == 0){
        if(cmd == REQUEST_SYSCALL_RELEASE){
          // check if syscall is being currently intercepted
          if(table[syscall].intercepted != 0){
            request_syscall_release(syscall);
          }
          return -EINVAL;
        } else {
          // request_syscall_intercept
          // check if intercepting an already intercepted call
          if(table[syscall].intercepted != 1){
            request_syscall_intercept(syscall);
          }
          return -EBUSY;
        }
      }
      return -EPERM;
      }
  } else if (cmd == REQUEST_STOP_MONITORING || cmd == REQUEST_START_MONITORING){
    // check if current PID actually exists
    if (pid >= 0) {
      if (!pid_task(find_vpid(pid), PIDTYPE_PID)) {
        // check if user is root
        if((current_uid() == 0) || (check_pid_from_list((pid_t) pid, current->pid) == 0 && pid != 0)){
            // check if syscall has been intercepted and pid is being monitored for stoping monitoring
            if(cmd == REQUEST_STOP_MONITORING){
              if(table[syscall].intercepted != 0){
                if(table[syscall].monitored == 2 || check_pid_monitored(syscall, (pid_t)pid) != 0){
                  request_stop_monitoring(syscall, pid);
                }
              }
              return -EINVAL;
            } else {
              // request_start_monitoring
              // check if trying to monitor a monitored pid
              if(table[syscall].intercepted != 0){
                if(table[syscall].monitored == 2 || check_pid_monitored(syscall, (pid_t)pid) == 0){
                  request_start_monitoring(syscall, pid);
                }
              }
              return -EBUSY;
            }
        }
        return -EPERM;
      }
    }
  }
  return -EINVAL;
}

/*
This helper function replaces the system call syscall with an interceptor function.
*/
long request_syscall_intercept(int syscall){
  // lock the syscall table
  spin_lock(&calltable_lock);
  // lock my table
  spin_lock(&pidlist_lock);
  //rw for call Table
  set_addr_rw((unsigned long) sys_call_table);
  // store original syscall to my table
  table[syscall].f = sys_call_table[syscall];
  // change syscall to generic intercept function
  sys_call_table[syscall] = &interceptor;
  // set the intercepted flag to 1
  table[syscall].intercepted = 1;
  // set ro for call table
  set_addr_ro((unsigned long) sys_call_table);
  // unlock syscall table
  spin_unlock(&pidlist_lock);
  // unlock my table
  spin_unlock(&calltable_lock);
  return (long) 0;
}

/*
This helper function releases the interceptor
*/
long request_syscall_release(int syscall){
  // lock the system call table
  spin_lock(&calltable_lock);
  // lock table
  spin_lock(&pidlist_lock);
  // clear table list
  destroy_list(syscall);
  set_addr_rw((unsigned long) sys_call_table);
  // restore original system call
  sys_call_table[syscall]=table[syscall].f;
  // wipe the stored syscall function
  table[syscall].f = NULL;
  set_addr_ro((unsigned long) sys_call_table);
  // unlock table
  spin_unlock(&pidlist_lock);
  // unlock system call table
  spin_unlock(&calltable_lock);
  return (long) 0;
}

/*
This helper function starts monitoring a pid given an intercepted system call
*/
long request_start_monitoring(int syscall, int pid){
  int result = 0;
  // lock table
  spin_lock(&pidlist_lock);
  // check if pid = 0, then all pids to be monitored
  if (pid == 0){
    // check if monitored was 1 before, then clear the list
    if (table[syscall].monitored >= 1){
      destroy_list(syscall);
    }
    // set monitored to 2, no need to add any pids to the list (blacklist)
    table[syscall].monitored = 2;
  }
  // else only that given pid will be monitored
  else {
    if(table[syscall].monitored != 2){
      // check if monitoring everything already, in that case nothing should be done
      table[syscall].monitored = 1;
      result = add_pid_sysc((pid_t) pid, syscall);
    }
  }
  // unlock table
  spin_lock(&pidlist_lock);
  return (long) result;
}

/*
This helper function stops monitoring a given pid
*/
long request_stop_monitoring(int syscall, int pid){
  int result = 0;
  // lock table
  spin_lock(&pidlist_lock);
  // check if monitored is set to 2
  if(table[syscall].monitored == 2){
    // the pidlist there is a blacklist so add it to there
    result = (long) add_pid_sysc((pid_t) pid, syscall);
  } // else we remove normally
  else {
    result = (long) del_pid_sysc((pid_t) pid, syscall);
  }
  // unlock table
  spin_unlock(&pidlist_lock);
  return result;
}

/**
 *
 */
long (*orig_custom_syscall)(void);


/**
 * Module initialization.
 *
 * TODO: Make sure to:
 * - Hijack MY_CUSTOM_SYSCALL and save the original in orig_custom_syscall.
 * - Hijack the exit_group system call (__NR_exit_group) and save the original
 *   in orig_exit_group.
 * - Make sure to set the system call table to writable when making changes,
 *   then set it back to read only once done.
 * - Perform any necessary initializations for bookkeeping data structures.
 *   To initialize a list, use
 *        INIT_LIST_HEAD (&some_list);
 *   where some_list is a "struct list_head".
 * - Ensure synchronization as needed.
 */
static int init_function(void) {
  int ctr;
  // lock syscall table
  spin_lock(&calltable_lock);
  // set call table to writable
  set_addr_rw((unsigned long)sys_call_table);
  // set orig_custom_syscall to be the function at MY_CUSTOM_SYSCALL
  orig_custom_syscall = sys_call_table[MY_CUSTOM_SYSCALL];
  // replace call in table with the custom syscall
  sys_call_table[MY_CUSTOM_SYSCALL] = &my_syscall;
  // hijack the exit_group
  orig_exit_group = sys_call_table[__NR_exit_group];
  sys_call_table[__NR_exit_group] = &my_exit_group;
  // set call table to read only
  set_addr_ro((unsigned long)sys_call_table);
  // unlock the call table
  spin_unlock(&calltable_lock);
  // lock pidlists
  spin_lock(&pidlist_lock);
  // init all the list heads
  for(ctr = 0; ctr < NR_syscalls + 1 ; ctr ++){
    INIT_LIST_HEAD(&(table[ctr].my_list));
  }
  // unlock pidlist
  spin_unlock(&pidlist_lock);
	return 0;
}

/**
 * Module exits.
 *
 * TODO: Make sure to:
 * - Restore MY_CUSTOM_SYSCALL to the original syscall.
 * - Restore __NR_exit_group to its original syscall.
 * - Make sure to set the system call table to writable when making changes,
 *   then set it back to read only once done.
 * - Ensure synchronization, if needed.
 */
static void exit_function(void){
  // lock the call table
  spin_lock(&calltable_lock);
  // set the table to writeable
  set_addr_rw((unsigned long)sys_call_table);
  // set the original system calls in place
  sys_call_table[MY_CUSTOM_SYSCALL] = orig_custom_syscall;
  sys_call_table[__NR_exit_group] = orig_exit_group;
  // set the table to read only
  set_addr_ro((unsigned long)sys_call_table);
  // lock the call Table
  spin_unlock(&calltable_lock);
}

module_init(init_function);
module_exit(exit_function);
