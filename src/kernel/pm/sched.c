/*
 * Copyright(C) 2011-2016 Pedro H. Penna   <pedrohenriquepenna@gmail.com>
 *              2015-2016 Davidson Francis <davidsondfgl@hotmail.com>
 *
 * This file is part of Nanvix.
 *
 * Nanvix is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nanvix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nanvix. If not, see <http://www.gnu.org/licenses/>.
 */

#include <nanvix/clock.h>
#include <nanvix/const.h>
#include <nanvix/hal.h>
#include <nanvix/pm.h>
#include <signal.h>

/**
 * @brief Schedules a process to execution.
 * 
 * @param proc Process to be scheduled.
 */
PUBLIC void sched(struct process *proc)
{
	proc->state = PROC_READY;
	proc->counter = 0;
}

/**
 * @brief Stops the current running process.
 */
PUBLIC void stop(void)
{
	curr_proc->state = PROC_STOPPED;
	sndsig(curr_proc->father, SIGCHLD);
	yield();
}

PUBLIC void resume(struct process *proc)
{	
	/* Resume only if process has stopped. */
	if (proc->state == PROC_STOPPED)
		sched(proc);
}

PUBLIC void yield(void)
{
	struct process *p;    /* Working process.     */
	struct process *next; /* Next process to run. */

	// if every active process had the opportunity to run, canBeElect of every process is set to 1
	static int hasToReset;
	static unsigned highestTime;

	//if every process has executed its quantums, then every process can be elected again
	hasToReset=1;

	for(p = FIRST_PROC; p <= LAST_PROC; p++)
	{
		if(p->canBeElect==1)
		{
			hasToReset=0;
		}
	}

	if(hasToReset==1){
		for(p = FIRST_PROC; p <= LAST_PROC; p++)
		{
			p->canBeElect=1;
		}
	}



	//find the process which has used the CPU the longest
	for (p = FIRST_PROC; p <= LAST_PROC; p++)
	{
		//on ignore IDLE
		if (p == IDLE)
			continue;

		if(highestTime<=(p->utime+p->ktime))
		{
			highestTime=p->utime+p->ktime;
		}
	}


	/* Re-schedule process for execution. */
	if (curr_proc->state == PROC_RUNNING)
	{
		sched(curr_proc);
	}

	//if the process didnt execute every of his quantums
	if(curr_proc->nbQuantum>0 && curr_proc->state==PROC_READY)
	{
		next=curr_proc;
		curr_proc->nbQuantum--;
	}
	else
	{
		/* Choose a process to run next. */
		next = IDLE;

		for (p = FIRST_PROC; p <= LAST_PROC; p++)
		{
			/* Skip non-ready process. */
			if (p->state != PROC_READY)
				continue;

			/* If next is IDLE, set next to p */
			if (next == IDLE)
			{
				next = p;
				continue;
			}

			/* Process with higher priority found. */
			if (p->canBeElect==1)
			{
				next = p;
				p->canBeElect=0;
				curr_proc->nbQuantum=((highestTime)+1)/((p->utime+p->ktime)+1);
				break;
			}
		}
	}

	/* Switch to next process. */
	next->state = PROC_RUNNING;
	next->counter = PROC_QUANTUM;
	switch_to(next);
}
