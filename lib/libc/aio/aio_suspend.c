/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 * libc/aio/aio_suspend.c
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <sched.h>
#include <signal.h>
#include <aio.h>
#include <assert.h>
#include <errno.h>

#ifdef CONFIG_FS_AIO

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Variables
 ****************************************************************************/

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: aio_suspend
 *
 * Description:
 *   The aio_suspend() function suspends the calling thread until at least
 *   one of the asynchronous I/O operations referenced by the 'list' argument
 *   has completed, until a signal interrupts the function, or, if 'timeout'
 *   is not NULL, until the time interval specified by 'timeout' has passed.
 *   If any of the aiocb structures in the list correspond to completed
 *   asynchronous I/O operations (that is, the error status for the
 *   operation is not equal to EINPROGRESS) at the time of the call, the
 *   function returns without suspending the calling thread.
 *
 *   Each aiocb structure pointed to must have been used in initiating an
 *   asynchronous I/O request via aio_read(), aio_write(), or lio_listio().
 *   This array may contain NULL pointers, which are ignored. If this
 *   array contains pointers that refer to aiocb structures that have not
 *   been used in submitting asynchronous I/O, the effect is undefined.
 *
 * Input Parameters:
 *   list    - An array of pointers to asynchronous I/O control blocks.
 *   nent    - The number of elements in the array.
 *   aiocbp  - A pointer to an array
 *   timeout - If not NULL, this parameter is pointer to a timespec
 *             structure that determines a timeout on the operation.  If
 *             the time referred to timeout passes before any of the I/O
 *             operations referenced by list are completed, then
 *             aio_suspend() returns with an error.
 *
 * Returned Value:
 *   If the aio_suspend() function returns after one or more asynchronous
 *   I/O operations have completed, the function returns zero. Otherwise,
 *   the function returns a value of -1 and sets errno to indicate the
 *   error.  The application may determine which asynchronous I/O completed
 *   by scanning the associated error and return status using aio_error()
 *   and aio_return(), respectively.
 *
 *   The aio_suspend() function will fail if:
 *
 *     EAGAIN - No asynchronous I/O indicated in the list referenced by
 *              list completed in the time interval indicated by timeout.
 *     EINTR  - A signal interrupted the aio_suspend() function.
 *
 ****************************************************************************/

int aio_suspend(FAR const struct aiocb *const list[], int nent, FAR const struct timespec *timeout)
{
	sigset_t set;
	int ret;
	int i;

	DEBUGASSERT(list);

	/* Lock the scheduler so that no I/O events can complete on the worker
	 * thread until we set our wait set up.  Pre-emption will, of course, be
	 * re-enabled while we are waiting for the signal.
	 */

	sched_lock();

	/* Check each entry in the list.  Break out of the loop if any entry
	 * has completed.
	 */

	for (i = 0; i < nent; i++) {
		/* Check if the I/O has completed */

		if (list[i] && list[i]->aio_result != -EINPROGRESS) {
			/* Yes, return success */

			sched_unlock();
			return OK;
		}
	}

	/* Then wait for SIGPOLL.  On success sigtimedwait() will return the
	 * signal number that cause the error (SIGPOLL).  It will set errno
	 * appropriately for this function on errors.
	 *
	 * NOTE: If completion of the I/O causes other signals to be generated
	 * first, then this will wake up and return EINTR instead of success.
	 */

	sigemptyset(&set);
	sigaddset(&set, SIGPOLL);

	ret = sigtimedwait(&set, NULL, timeout);
	sched_unlock();
	return ret >= 0 ? OK : ERROR;
}

#endif							/* CONFIG_FS_AIO */
