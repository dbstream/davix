#include <davix/semaphore.h>
#include <davix/panic.h>

int sem_try_wait(struct semaphore *sem)
{
	int irqflag = spin_acquire_irq(&sem->lock);

	int ret;
	if(sem->count) {
		sem->count--;
		ret = 1;
	} else {
		ret = 0;
	}

	spin_release_irq(&sem->lock, irqflag);

	return ret;
}

void sem_wait(struct semaphore *sem)
{
	if(sem_try_wait(sem))
		return;

	panic("sem_wait(): blocking unimplemented.\n");
}

void sem_signal(struct semaphore *sem)
{
	int irqflag = spin_acquire_irq(&sem->lock);
	sem->count++;
	spin_release_irq(&sem->lock, irqflag);
}
