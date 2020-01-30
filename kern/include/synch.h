/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYNCH_H_
#define _SYNCH_H_

/*
 * Header file for synchronization primitives.
 */


#include <spinlock.h>

/*
 * Dijkstra-style semaphore.
 *
 * The name field is for easier debugging. A copy of the name is made
 * internally.
 */
struct semaphore {
	char *sem_name;
	struct wchan *sem_wchan;
	struct spinlock sem_lock;
	volatile unsigned sem_count;
};

struct semaphore *sem_create(const char *name, unsigned initial_count);
void sem_destroy(struct semaphore *);

/*
 * Operations (both atomic):
 *     P (proberen): decrement count. If the count is 0, block until
 *                   the count is 1 again before decrementing.
 *     V (verhogen): increment count.
 */
void P(struct semaphore *);
void V(struct semaphore *);


/*
 * Simple lock for mutual exclusion.
 *
 * When the lock is created, no thread should be holding it. Likewise,
 * when the lock is destroyed, no thread should be holding it.
 *
 * The name field is for easier debugging. A copy of the name is
 * (should be) made internally.
 */
struct lock {
        char *lk_name;
        HANGMAN_LOCKABLE(lk_hangman);   /* Deadlock detector hook. */
	/*
		a wait channel on which a lock waits if it is held by another lock
		a spinlock used to impemet the lock and usek in the wait channel

	*/
	struct wchan *lk_wchan;
	struct spinlock lk_lock;
	volatile struct thread* lk_currthread;
	volatile bool lk_isheld;
	volatile unsigned lk_count;
	

        // add what you need here
        // (don't forget to mark things volatile as needed)
};

struct lock *lock_create(const char *name);
void lock_destroy(struct lock *);

/*
 * Operations:
 *    lock_acquire - Get the lock. Only one thread can hold the lock at the
 *                   same time.
 *    lock_release - Free the lock. Only the thread holding the lock may do
 *                   this.
 *    lock_do_i_hold - Return true if the current thread holds the lock;
 *                   false otherwise.
 *
 * These operations must be atomic. You get to write them.
 */
void lock_acquire(struct lock *);
void lock_release(struct lock *);
bool lock_do_i_hold(struct lock *);


/*
 * Condition variable.
 *
 * Note that the "variable" is a bit of a misnomer: a CV is normally used
 * to wait until a variable meets a particular condition, but there's no
 * actual variable, as such, in the CV.
 *
 * These CVs are expected to support Mesa semantics, that is, no
 * guarantees are made about scheduling.
 *
 * The name field is for easier debugging. A copy of the name is
 * (should be) made internally.
 */

struct cv {
        
	char *cv_name;
	struct wchan* cv_wchan;
	struct spinlock cv_lock;
	// I really need more idea bout what it means
        // add what you need here
        // (don't forget to mark things volatile as needed)
};

struct cv *cv_create(const char *name);
void cv_destroy(struct cv *);

/*
 * Operations:
 *    cv_wait      - Release the supplied lock, go to sleep, and, after
 *                   waking up again, re-acquire the lock.
 *    cv_signal    - Wake up one thread that's sleeping on this CV.
 *    cv_broadcast - Wake up all threads sleeping on this CV.
 *
 * For all three operations, the current thread must hold the lock passed
 * in. Note that under normal circumstances the same lock should be used
 * on all operations with any particular CV.
 *
 * These operations must be atomic. You get to write them.
 */
void cv_wait(struct cv *cv, struct lock *lock);
void cv_signal(struct cv *cv, struct lock *lock);
void cv_broadcast(struct cv *cv, struct lock *lock);

/*
 * Reader-writer locks.
 *
 * When the lock is created, no thread should be holding it. Likewise,
 * when the lock is destroyed, no thread should be holding it.
 *
 * The name field is for easier debugging. A copy of the name is
 * (should be) made internally.
 */
/*
	Requirements
	- allow multiple threads in a cridtical section if their function is to read
	- readers can enter when no writer is holding the lock
	- writers can enter when no one neither readers or writers hold the lock
	- point no 2 can cause starvation
	- multiple readers can enter when the writer is waiting for a reader to exit
	- so change point no 2 to no writers inside or waiting
	- we'll need
		- a spinlock
		- number of writers waiting
		- try to use cv for each reader,writer
		- use wchan_broadcast instead of wake one
		- number of readers that are in 
		- booleans for whether its held
		- 

*/

struct rwlock {
        
	char *rwlock_name;	
	struct cv *rwlock_rcv;
	struct cv *rwlock_wcv;
	struct lock rwlock_sleep;		// chagned to a sleep lock because i s big enough to have a long context switch duration
	volatile bool rwlock_rin;
	volatile bool  rwlock_win;
	volatile unsigned int rwlock_rc;
	volatile unsigned int rwlock_rrc;	// read request count i.e the nnumber waiting
	volatile unsigned int rwlock_wrc;	// write request count
};

struct rwlock * rwlock_create(const char *);
void rwlock_destroy(struct rwlock *);

/*
 * Operations:
 *    rwlock_acquire_read  - Get the lock for reading. Multiple threads can
 *                          hold the lock for reading at the same time.
 *    rwlock_release_read  - Free the lock. 
 *    rwlock_acquire_write - Get the lock for writing. Only one thread can
 *                           hold the write lock at one time.
 *    rwlock_release_write - Free the write lock.
 *
 * These operations must be atomic. You get to write them.
 */

void rwlock_acquire_read(struct rwlock *);
void rwlock_release_read(struct rwlock *);
void rwlock_acquire_write(struct rwlock *);
void rwlock_release_write(struct rwlock *);

#endif /* _SYNCH_H_ */
