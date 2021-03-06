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

/*
 * Synchronization primitives.
 * The specifications of the functions are in synch.h.
 */

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <current.h>
#include <synch.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *name, unsigned initial_count)
{
	struct semaphore *sem;

	sem = kmalloc(sizeof(*sem));
	if (sem == NULL) {
		return NULL;
	}

	sem->sem_name = kstrdup(name);
	if (sem->sem_name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->sem_wchan = wchan_create(sem->sem_name);
	if (sem->sem_wchan == NULL) {
		kfree(sem->sem_name);
		kfree(sem);
		return NULL;
	}

	spinlock_init(&sem->sem_lock);
	sem->sem_count = initial_count;

	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&sem->sem_lock);
	wchan_destroy(sem->sem_wchan);
	kfree(sem->sem_name);
	kfree(sem);
}

void
P(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	KASSERT(curthread->t_in_interrupt == false);

	/* Use the semaphore spinlock to protect the wchan as well. */
	spinlock_acquire(&sem->sem_lock);
	while (sem->sem_count == 0) {
		/*
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the semaphore; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting. Apparently according to some
		 * textbooks semaphores must for some reason have
		 * strict ordering. Too bad. :-)
		 *
		 * Exercise: how would you implement strict FIFO
		 * ordering?
		 */
		wchan_sleep(sem->sem_wchan, &sem->sem_lock);
	}
	KASSERT(sem->sem_count > 0);
	sem->sem_count--;
	spinlock_release(&sem->sem_lock);
}

void
V(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	spinlock_acquire(&sem->sem_lock);

	sem->sem_count++;
	KASSERT(sem->sem_count > 0);
	wchan_wakeone(sem->sem_wchan, &sem->sem_lock);

	spinlock_release(&sem->sem_lock);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(*lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->lk_name = kstrdup(name);
	if (lock->lk_name == NULL) {
		kfree(lock);
		return NULL;
	}

	HANGMAN_LOCKABLEINIT(&lock->lk_hangman, lock->lk_name);
	

	// add stuff here as needed
	/* Here's a pattern that is usually followed in an OS
		- allocate the struct members incrementally 
		- if an error occurs during the allocation or init of a struct free all the previously allocated members
		- and then return NULL which will act as an error code
	*/
	lock->lk_wchan = wchan_create(lock->lk_name);
	if(lock->lk_wchan == NULL){
		kfree(lock->lk_name);
		kfree(lock);
		lock = NULL;	

	}
	spinlock_init(&lock->lk_lock);	// here you are sending the address of the date member lk_lock that lock points to the function takes a spinlock * parameter
	lock->lk_count  = 0;
	lock->lk_isheld = false;
	return lock;
}

void
lock_destroy(struct lock *lock)
{

	/*
		mutex behavoiur can be considered to be as follows
			- don't detroy when lock is held
			- don't destroy when threads are blocked waitingn for it. ***###***
			- the errors thrown here seem to be only due to implementing the above point
			- this sort ofwhat the doc for wait-channel says as well
		sources
			- cmu conflluence page
			- stack exchange
			- wait_channel header file in include
	*/
	KASSERT(lock != NULL);
	// KASSERT(wchan_isempty(lock->lk_wchan,&lock->lk_lock));
	KASSERT(lock->lk_isheld == false);
	
	// add stuff here as needed
	spinlock_cleanup(&lock->lk_lock);
	wchan_destroy(lock->lk_wchan);
	kfree(lock->lk_name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
	KASSERT(lock != NULL);
	/*
		never let a thread that is about to acquire a lock to be interrupted
			- if it is interrupted and the thread that interrupts tries to acquire the lock 
			- the lock will still be held by the interrupted lock but it can't be given up since the the interrupted lock doesnt give it up while interrupting
	*/
	KASSERT(curthread->t_in_interrupt == false);
	/* Call this (atomically) before waiting for a lock */
	HANGMAN_WAIT(&curthread->t_hangman, &lock->lk_hangman);

	// Write this

	//(void)lock;  // suppress warning until code gets written

	/* Call this (atomically) once the lock is acquired */
	spinlock_acquire(&lock->lk_lock);
	lock->lk_count++;
	while(lock->lk_isheld){
		wchan_sleep(lock->lk_wchan,&lock->lk_lock);
	}
	lock->lk_count--;
	//KASSERT(lock->lk_isheld == false);
	//KASSERT(lock->lk_currthread == NULL);
	lock->lk_currthread = curthread;
	lock->lk_isheld = true;
	spinlock_release(&lock->lk_lock);
	HANGMAN_ACQUIRE(&curthread->t_hangman, &lock->lk_hangman);
}

void
lock_release(struct lock *lock)
{
	/* Call this (atomically) when the lock is released */
	KASSERT(lock != NULL);
	KASSERT(lock_do_i_hold(lock));
	spinlock_acquire(&lock->lk_lock);
	lock->lk_isheld = false;
	lock->lk_currthread = NULL;
	wchan_wakeone(lock->lk_wchan,&lock->lk_lock);
	spinlock_release(&lock->lk_lock);
	HANGMAN_RELEASE(&curthread->t_hangman, &lock->lk_hangman);
	// Write this
	

	//(void)lock;  // suppress warning until code gets written
}

bool
lock_do_i_hold(struct lock *lock)
{
	// Write this
	KASSERT(lock != NULL);
	if (lock->lk_isheld ==  true && lock->lk_currthread == curthread)	{
		return true;
	}

	return false;
	//(void)lock;  // suppress warning until code gets written

	//return true; // dummy until code gets written
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(*cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->cv_name = kstrdup(name);
	if (cv->cv_name==NULL) {
		kfree(cv);
		return NULL;
	}
	
	cv->cv_wchan = wchan_create(cv->cv_name);
	if (cv->cv_wchan == NULL)	{
		kfree(cv->cv_name);
		kfree(cv);
		return NULL;
	}
	spinlock_init(&cv->cv_lock);
	return cv;
}

void
cv_destroy(struct cv *cv)
{
	KASSERT(cv != NULL);
	wchan_destroy(cv->cv_wchan);
	spinlock_cleanup(&cv->cv_lock);
	kfree(cv->cv_name);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	// Expected behaviour is to release the lock and go to sleep and after waking up again  try to reacquire lock
	/*
		The calling thread checks whether it holds the lock an then releases the lock and goes to sleep
		when it is woken up by the signal of some other thread it tries to acquire the lock
		TRIES
	*/
	KASSERT(cv != NULL);
	KASSERT(lock_do_i_hold(lock));
	spinlock_acquire(&cv->cv_lock);
	lock_release(lock);				// This operation of releasing and going to sleep musst be atomic
	wchan_sleep(cv->cv_wchan,&cv->cv_lock);
	spinlock_release(&cv->cv_lock);
	lock_acquire(lock);
}


void
cv_signal(struct cv *cv, struct lock *lock)
{
	// Write this
	//(void)cv;    // suppress warning until code gets written
	//void)lock;  // suppress warning until code gets written
	KASSERT(cv != NULL);
	KASSERT(lock_do_i_hold(lock));
	// All it does is is wake a thread on waiting on a cv
	spinlock_acquire(&cv->cv_lock);
	wchan_wakeone(cv->cv_wchan,&cv->cv_lock);
	spinlock_release(&cv->cv_lock);
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	// Write this
	//(void)cv;    // suppress warning until code gets written
	//void)lock;  // suppress warning until code gets written

	// It seems a bit confusing and I dont Understand how 
	// In MESA semantics the thread that signals hold the lock and waiting thread waits for lock
	// In hoare semantics the signallling thread gives up its lock and waits until the thread that acquires it completes after which the lock is returned 
	KASSERT(cv != NULL);
	KASSERT(lock_do_i_hold(lock));
	spinlock_acquire(&cv->cv_lock);
	wchan_wakeall(cv->cv_wchan,&cv->cv_lock);
	spinlock_release(&cv->cv_lock);
}


struct rwlock*
rwlock_create(const char *name){
	struct rwlock *rwlock;
	rwlock = kmalloc(sizeof(*rwlock));
	if (rwlock == NULL){
		return NULL;
	}
	rwlock->rwlock_name = kstrdup(name);
	if (rwlock->rwlock_name == NULL){
		kfree(rwlock);
		return NULL;
	}

	rwlock->rwlock_rcv = cv_create("rwlock_readercv");
	if (rwlock->rwlock_rcv == NULL){
		kfree(rwlock->rwlock_name);
		kfree(rwlock);
		return NULL;
	}
	rwlock->rwlock_wcv = cv_create("rwlock_writercv");
	if (rwlock->rwlock_wcv == NULL){
		cv_destroy(rwlock->rwlock_rcv);
		kfree(rwlock->rwlock_name);
		kfree(rwlock);
		return NULL;
	}

	//using sleeplock instead
	rwlock->rwlock_sleep = lock_create("rwlock_sleeplock");
	if (rwlock->rwlock_sleep ==  NULL){
		cv_destroy(rwlock->rwlock_wcv);
		cv_destroy(rwlock->rwlock_rcv);
		kfree(rwlock->rwlock_name);
		kfree(rwlock);
		return NULL;
	}
	rwlock->rwlock_rin = false;
	rwlock->rwlock_win = false;
	rwlock->rwlock_rc = 0;
	rwlock->rwlock_rrc = 0;
	rwlock->rwlock_wrc = 0;
	return rwlock;
}

void
rwlock_destroy(struct rwlock *rwlock){
	KASSERT(rwlock != NULL);
	KASSERT(rwlock->rwlock_rin == false);
	KASSERT(rwlock->rwlock_win == false);
	KASSERT(rwlock->rwlock_wrc == 0);
	KASSERT(rwlock->rwlock_rc == 0);
	cv_destroy(rwlock->rwlock_rcv);
	cv_destroy(rwlock->rwlock_wcv);
	kfree(rwlock->rwlock_name);
	lock_destroy(rwlock->rwlock_sleep);
	kfree(rwlock);
}

void 
rwlock_acquire_read(struct rwlock* rwlock){
	lock_acquire(rwlock->rwlock_sleep);
	rwlock->rwlock_rrc++;
	while(rwlock->rwlock_win == true || rwlock->rwlock_wrc > 0){	// wait while there is a writer in there or a waiting writer while a readder is in to prevent reader bias
		cv_wait(rwlock->rwlock_rcv,rwlock->rwlock_sleep);
	}
	rwlock->rwlock_rrc--;
	rwlock->rwlock_rin = true;
	rwlock->rwlock_rc++;
	lock_release(rwlock->rwlock_sleep);
}

/*
	Ous bias prevention strategy is
		- prefer waking writers when releasing a reader lock
		- prefer waking readers when writeresare releasing their writer lock
		- make readers go to sleep if there is a writer waiting to prevent starvation

*/


void 
rwlock_release_read(struct rwlock* rwlock){
	KASSERT(rwlock->rwlock_rin == true);
	lock_acquire(rwlock->rwlock_sleep);
	rwlock->rwlock_rc--;
	rwlock->rwlock_rin = false;
	if (rwlock->rwlock_rc == 0){
		//If all the readers have exited then let the writer that was waitin enter or wake up readers that were stuck
		if (rwlock->rwlock_wrc > 0){ 
			cv_signal(rwlock->rwlock_wcv,rwlock->rwlock_sleep);
		}
		
		else {
			cv_broadcast(rwlock->rwlock_rcv,rwlock->rwlock_sleep);
		}
	}
	lock_release(rwlock->rwlock_sleep);
}

void
rwlock_acquire_write(struct rwlock* rwlock){
	lock_acquire(rwlock->rwlock_sleep);
	rwlock->rwlock_wrc++;
	while(rwlock->rwlock_win == true || rwlock->rwlock_rc > 0){ //wait as long as there is another writer in or other readers are in
		cv_wait(rwlock->rwlock_wcv,rwlock->rwlock_sleep);
	}

	rwlock->rwlock_wrc--;
	rwlock->rwlock_win == true;
	lock_release(rwlock->rwlock_sleep);
}

void
rwlock_release_write(struct rwlock* rwlock){
	KASSERT(rwlock->rwlock_win == true);
	lock_acquire(rwlock->rwlock_sleep);
	rwlock->rwlock_win = false;
	if (rwlock->rwlock_rrc > 0){
		cv_broadcast(rwlock->rwlock_rcv,rwlock->rwlock_sleep);
	}
	else {
		cv_signal(rwlock->rwlock_wcv,rwlock->rwlock_sleep);
	}
	lock_release(rwlock->rwlock_sleep);
}



