/* libnih
 *
 * test_io.c - test suite for nih/io.c
 *
 * Copyright © 2006 Scott James Remnant <scott@netsplit.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <nih/test.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/io.h>
#include <nih/logging.h>
#include <nih/error.h>
#include <nih/errors.h>


static int watcher_called = 0;
static void *last_data = NULL;
static NihIoWatch *last_watch = NULL;
static NihIoEvents last_events = 0;

static void
my_watcher (void *data, NihIoWatch *watch, NihIoEvents events)
{
	watcher_called++;
	last_data = data;
	last_watch = watch;
	last_events = events;
}

void
test_add_watch (void)
{
	NihIoWatch *watch;
	int         fds[2];

	/* Check that we can add a watch on a file descriptor and that the
	 * structure is properly filled in and placed in a list.
	 */
	TEST_FUNCTION ("nih_io_add_watch");
	pipe (fds);
	watch = nih_io_add_watch (NULL, fds[0], NIH_IO_READ,
				  my_watcher, &watch);

	TEST_ALLOC_SIZE (watch, sizeof (NihIoWatch));
	TEST_EQ (watch->fd, fds[0]);
	TEST_EQ (watch->events, NIH_IO_READ);
	TEST_EQ_P (watch->watcher, my_watcher);
	TEST_EQ_P (watch->data, &watch);

	nih_list_free (&watch->entry);

	close (fds[0]);
	close (fds[1]);
}


void
test_select_fds (void)
{
	NihIoWatch *watch1, *watch2, *watch3;
	fd_set      readfds, writefds, exceptfds;
	int         nfds, fds[2];

	/* Check that the select file descriptor sets are correctly filled
	 * based on a set of watches we add.
	 */
	TEST_FUNCTION ("nih_io_select_fds");
	pipe (fds);
	watch1 = nih_io_add_watch (NULL, fds[0], NIH_IO_READ,
				   my_watcher, &watch1);
	watch2 = nih_io_add_watch (NULL, fds[1], NIH_IO_WRITE,
				   my_watcher, &watch2);
	watch3 = nih_io_add_watch (NULL, fds[0], NIH_IO_EXCEPT,
				   my_watcher, &watch3);

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);
	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);

	TEST_EQ (nfds, MAX (fds[0], fds[1]) + 1);
	TEST_TRUE (FD_ISSET (fds[0], &readfds));
	TEST_FALSE (FD_ISSET (fds[0], &writefds));
	TEST_TRUE (FD_ISSET (fds[0], &exceptfds));
	TEST_FALSE (FD_ISSET (fds[1], &readfds));
	TEST_TRUE (FD_ISSET (fds[1], &writefds));
	TEST_FALSE (FD_ISSET (fds[1], &exceptfds));

	nih_list_free (&watch1->entry);
	nih_list_free (&watch2->entry);
	nih_list_free (&watch3->entry);

	close (fds[0]);
	close (fds[1]);
}

void
test_handle_fds (void)
{
	NihIoWatch    *watch1, *watch2, *watch3;
	fd_set         readfds, writefds, exceptfds;
	int            fds[2];

	TEST_FUNCTION ("nih_io_handle_fds");
	pipe (fds);
	watch1 = nih_io_add_watch (NULL, fds[0], NIH_IO_READ,
				   my_watcher, &watch1);
	watch2 = nih_io_add_watch (NULL, fds[1], NIH_IO_WRITE,
				   my_watcher, &watch2);
	watch3 = nih_io_add_watch (NULL, fds[0], NIH_IO_EXCEPT,
				   my_watcher, &watch3);

	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	/* Check that something watching a file descriptor for readability
	 * is called, with the right arguments passed; and that another
	 * watch on the same file descriptor for different events is not
	 * called.
	 */
	TEST_FEATURE ("with select for read");
	watcher_called = 0;
	last_data = NULL;
	last_watch = NULL;
	last_events = 0;
	FD_SET (fds[0], &readfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_EQ (watcher_called, 1);
	TEST_EQ (last_events, NIH_IO_READ);
	TEST_EQ_P (last_watch, watch1);
	TEST_EQ_P (last_data, &watch1);


	/* Check that something watching a file descriptor for an exception
	 * is called, and that the watch on the same descriptor for reading
	 * is not called.
	 */
	TEST_FEATURE ("with select for exception");
	watcher_called = 0;
	last_data = NULL;
	last_watch = NULL;
	last_events = 0;
	FD_ZERO (&readfds);
	FD_SET (fds[0], &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_EQ (watcher_called, 1);
	TEST_EQ (last_events, NIH_IO_EXCEPT);
	TEST_EQ_P (last_watch, watch3);
	TEST_EQ_P (last_data, &watch3);


	/* Check that nothing is called if the file descriptor and events
	 * being polled don't match anything.
	 */
	TEST_FEATURE ("with unwatched select");
	watcher_called = 0;
	FD_ZERO (&exceptfds);
	FD_SET (fds[1], &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_EQ (watcher_called, 0);


	nih_list_free (&watch1->entry);
	nih_list_free (&watch2->entry);
	nih_list_free (&watch3->entry);

	close (fds[0]);
	close (fds[1]);
}


void
test_buffer_new (void)
{
	NihIoBuffer *buf;

	/* Check that we can create a new empty buffer, and that the
	 * structure members are correct.
	 */
	TEST_FUNCTION ("nih_io_buffer_new");
	buf = nih_io_buffer_new (NULL);

	TEST_ALLOC_SIZE (buf, sizeof (NihIoBuffer));
	TEST_EQ_P (buf->buf, NULL);
	TEST_EQ (buf->size, 0);
	TEST_EQ (buf->len, 0);

	nih_free (buf);
}

void
test_buffer_resize (void)
{
	NihIoBuffer *buf;

	TEST_FUNCTION ("nih_io_buffer_resize");
	buf = nih_io_buffer_new (NULL);

	/* Check that we can resize a NULL buffer; we ask for half a page
	 * and expect to get a full page allocated as a child of the buffer
	 * itself.
	 */
	TEST_FEATURE ("with empty buffer and half increase");
	nih_io_buffer_resize (buf, BUFSIZ / 2);

	TEST_ALLOC_PARENT (buf->buf, buf);
	TEST_ALLOC_SIZE (buf->buf, BUFSIZ);
	TEST_EQ (buf->size, BUFSIZ);
	TEST_EQ (buf->len, 0);


	/* Check that we can increase the size by a full page, and not
	 * have anything change because there's no space used yet.
	 */
	TEST_FEATURE ("with empty but alloc'd buffer and full increase");
	nih_io_buffer_resize (buf, BUFSIZ);

	TEST_ALLOC_SIZE (buf->buf, BUFSIZ);
	TEST_EQ (buf->size, BUFSIZ);


	/* Check that we can increase the size beyond a full page, and
	 * get another page of allocated space.
	 */
	TEST_FEATURE ("with empty but alloc'd buffer and larger increase");
	nih_io_buffer_resize (buf, BUFSIZ + BUFSIZ / 2);

	TEST_ALLOC_SIZE (buf->buf, BUFSIZ * 2);
	TEST_EQ (buf->size, BUFSIZ * 2);


	/* Check that we can drop the size of an allocated but empty buffer
	 * back to zero and have the buffer freed.
	 */
	TEST_FEATURE ("with alloc'd buffer and no data");
	nih_io_buffer_resize (buf, 0);

	TEST_EQ (buf->size, 0);
	TEST_EQ_P (buf->buf, NULL);


	/* Check that asking for a page more space when we claim to be
	 * using half a page gives us a full two pages of space.
	 */
	TEST_FEATURE ("with part-full buffer and increase");
	buf->len = BUFSIZ / 2;
	nih_io_buffer_resize (buf, BUFSIZ);

	TEST_ALLOC_SIZE (buf->buf, BUFSIZ * 2);
	TEST_EQ (buf->size, BUFSIZ * 2);
	TEST_EQ (buf->len, BUFSIZ / 2);


	/* Check that asking for an increase smaller than the difference
	 * between the buffer size and length has no effect.
	 */
	TEST_FEATURE ("with no change");
	buf->len = BUFSIZ + BUFSIZ / 2;
	nih_io_buffer_resize (buf, 80);

	TEST_ALLOC_SIZE (buf->buf, BUFSIZ * 2);
	TEST_EQ (buf->size, BUFSIZ * 2);
	TEST_EQ (buf->len, BUFSIZ + BUFSIZ / 2);

	nih_free (buf);
}

void
test_buffer_pop (void)
{
	NihIoBuffer *buf;
	char        *str;
	size_t       len;

	TEST_FUNCTION ("nih_io_buffer_pop");
	buf = nih_io_buffer_new (NULL);
	nih_io_buffer_push (buf, "this is a test of the buffer code", 33);


	/* Check that we can pop some bytes out of a buffer, and have a
	 * NULL-terminated string returned that is allocated with nih_alloc.
	 * The buffer should be shrunk appropriately and moved up.
	 */
	TEST_FEATURE ("with full buffer");
	len = 14;
	str = nih_io_buffer_pop (NULL, buf, &len);

	TEST_EQ (len, 14);
	TEST_ALLOC_SIZE (str, 15);
	TEST_EQ (str[14], '\0');
	TEST_EQ_STR (str, "this is a test");

	TEST_EQ (buf->len, 19);
	TEST_EQ_MEM (buf->buf, " of the buffer code", 19);

	nih_free (str);


	/* Check that we can empty the buffer and the buffer is freed. */
	TEST_FEATURE ("with request to empty buffer");
	len = 19;
	str = nih_io_buffer_pop (NULL, buf, &len);

	TEST_EQ (len, 19);
	TEST_ALLOC_SIZE (str, 20);
	TEST_EQ (str[19], '\0');
	TEST_EQ_STR (str, " of the buffer code");

	TEST_EQ (buf->len, 0);
	TEST_EQ (buf->size, 0);
	TEST_EQ_P (buf->buf, NULL);

	nih_free (str);


	/* Check that we can request more data than is in the buffer.
	 * We should get everything's there, and len should be updated to
	 * indicate the shortfall.
	 */
	TEST_FEATURE ("with request for more than buffer size");
	nih_io_buffer_push (buf, "another test", 12);

	len = 20;
	str = nih_io_buffer_pop (NULL, buf, &len);

	TEST_EQ (len, 12);
	TEST_ALLOC_SIZE (str, 13);
	TEST_EQ (str[12], '\0');
	TEST_EQ_STR (str, "another test");

	TEST_EQ (buf->len, 0);
	TEST_EQ (buf->size, 0);
	TEST_EQ_P (buf->buf, NULL);

	nih_free (str);


	nih_free (buf);
}

void
test_buffer_shrink (void)
{
	NihIoBuffer *buf;

	TEST_FUNCTION ("nih_io_buffer_shrink");
	buf = nih_io_buffer_new (NULL);
	nih_io_buffer_push (buf, "this is a test of the buffer code", 33);


	/* Check that we can shrink the buffer by a small number of bytes. */
	TEST_FEATURE ("with full buffer");
	nih_io_buffer_shrink (buf, 14);

	TEST_EQ (buf->len, 19);
	TEST_EQ_MEM (buf->buf, " of the buffer code", 19);


	/* Check that we can empty the buffer and the buffer is freed. */
	TEST_FEATURE ("with request to empty buffer");
	nih_io_buffer_shrink (buf, 19);

	TEST_EQ (buf->len, 0);
	TEST_EQ (buf->size, 0);
	TEST_EQ_P (buf->buf, NULL);


	/* Check that we can shrink the buffer by more bytes than its length
	 * and just end up freeing it.
	 */
	TEST_FEATURE ("with request larger than buffer size");
	nih_io_buffer_push (buf, "another test", 12);

	nih_io_buffer_shrink (buf, 20);

	TEST_EQ (buf->len, 0);
	TEST_EQ (buf->size, 0);
	TEST_EQ_P (buf->buf, NULL);

	nih_free (buf);
}

void
test_buffer_push (void)
{
	NihIoBuffer *buf;

	TEST_FUNCTION ("nih_io_buffer_push");
	buf = nih_io_buffer_new (NULL);

	/* Check that we can push data into an empty buffer, which will
	 * store it in the buffer.
	 */
	TEST_FEATURE ("with empty buffer");
	nih_io_buffer_push (buf, "test", 4);

	TEST_ALLOC_SIZE (buf->buf, BUFSIZ);
	TEST_EQ (buf->size, BUFSIZ);
	TEST_EQ (buf->len, 4);
	TEST_EQ_MEM (buf->buf, "test", 4);


	/* Check that we can push more data into that buffer, which will
	 * append it to the data already there.
	 */
	TEST_FEATURE ("with data in the buffer");
	nih_io_buffer_push (buf, "ing the buffer code", 14);

	TEST_ALLOC_SIZE (buf->buf, BUFSIZ);
	TEST_EQ (buf->size, BUFSIZ);
	TEST_EQ (buf->len, 18);
	TEST_EQ_MEM (buf->buf, "testing the buffer code", 18);

	nih_free (buf);
}


void
test_message_new (void)
{
	NihIoMessage *msg;

	/* Check that we can create a new empty message, that doesn't appear
	 * in any list and with the structure and msghdr members correct.
	 */
	TEST_FUNCTION ("nih_io_message_new");
	msg = nih_io_message_new (NULL);

	TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
	TEST_LIST_EMPTY (&msg->entry);
	TEST_EQ_P (msg->addr, NULL);
	TEST_EQ (msg->addrlen, 0);
	TEST_ALLOC_SIZE (msg->msg_buf, sizeof (NihIoBuffer));
	TEST_ALLOC_PARENT (msg->msg_buf, msg);
	TEST_ALLOC_SIZE (msg->ctrl_buf, sizeof (NihIoBuffer));
	TEST_ALLOC_PARENT (msg->ctrl_buf, msg);

	nih_free (msg);
}

void
test_message_push_control (void)
{
	NihIoMessage   *msg;
	struct cmsghdr *cmsg;
	struct ucred    cred;
	int             ret, value;

	TEST_FUNCTION ("nih_io_message_push_control");
	msg = nih_io_message_new (NULL);

	/* Check that we can add a control message to a message that doesn't
	 * yet have a control buffer.  The control buffer should be a child
	 * of the message, and contain the complete aligned cmsg.
	 */
	TEST_FEATURE ("with empty message");
	value = 0;
	ret = nih_io_message_push_control (msg, SOL_SOCKET, SCM_RIGHTS,
					   sizeof (int), &value);

	TEST_ALLOC_PARENT (msg->ctrl_buf->buf, msg->ctrl_buf);
	TEST_ALLOC_SIZE (msg->ctrl_buf->buf, BUFSIZ);

	TEST_EQ (msg->ctrl_buf->len, CMSG_SPACE (sizeof (int)));

	cmsg = (struct cmsghdr *)msg->ctrl_buf->buf;

	TEST_EQ (cmsg->cmsg_level, SOL_SOCKET);
	TEST_EQ (cmsg->cmsg_type, SCM_RIGHTS);
	TEST_EQ (cmsg->cmsg_len, CMSG_LEN (sizeof (int)));
	TEST_EQ_MEM (CMSG_DATA (cmsg), &value, sizeof (int));


	/* Check that we can append more control data onto the end of an
	 * existing message.  The buffer should include both messages.
	 */
	TEST_FEATURE ("with existing control data");
	cred.pid = cred.uid = cred.gid = 1;
	ret = nih_io_message_push_control (msg, SOL_SOCKET, SCM_CREDENTIALS,
					   sizeof (cred), &cred);

	TEST_ALLOC_PARENT (msg->ctrl_buf->buf, msg->ctrl_buf);
	TEST_ALLOC_SIZE (msg->ctrl_buf->buf, BUFSIZ);

	TEST_EQ (msg->ctrl_buf->len, (CMSG_SPACE (sizeof (int))
				      + CMSG_SPACE (sizeof (cred))));

	cmsg = (struct cmsghdr *)(msg->ctrl_buf->buf
				  + CMSG_SPACE (sizeof (int)));

	TEST_EQ (cmsg->cmsg_level, SOL_SOCKET);
	TEST_EQ (cmsg->cmsg_type, SCM_CREDENTIALS);
	TEST_EQ (cmsg->cmsg_len, CMSG_LEN (sizeof (cred)));
	TEST_EQ_MEM (CMSG_DATA (cmsg), &cred, sizeof (cred));

	nih_free (msg);
}

void
test_message_recv (void)
{
	NihError           *err;
	NihIoMessage       *msg;
	struct sockaddr_un  addr0, addr1;
	size_t              addr0len, addr1len, len;
	struct msghdr       msghdr;
	struct iovec        iov[1];
	char                buf[BUFSIZ * 2], cbuf[CMSG_SPACE(sizeof (int))];
	struct cmsghdr     *cmsg;
	int                 fds[2], *fdptr;

	TEST_FUNCTION ("nih_io_message_recv");
	socketpair (PF_UNIX, SOCK_DGRAM, 0, fds);

	msghdr.msg_name = NULL;
	msghdr.msg_namelen = 0;
	msghdr.msg_iov = iov;
	msghdr.msg_iovlen = 1;
	msghdr.msg_control = NULL;
	msghdr.msg_controllen = 0;
	msghdr.msg_flags = 0;

	iov[0].iov_base = buf;
	iov[0].iov_len = sizeof (buf);

	/* Check that we can receive a message from a socket with just
	 * text, and no control data.  The message structure should be
	 * allocated and filled properly.
	 */
	TEST_FEATURE ("with no control data");
	memcpy (buf, "test", 4);
	iov[0].iov_len = 4;

	sendmsg (fds[0], &msghdr, 0);

	len = 4;
	msg = nih_io_message_recv (NULL, fds[1], &len);

	TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
	TEST_LIST_EMPTY (&msg->entry);

	TEST_EQ (len, 4);
	TEST_EQ (msg->msg_buf->len, 4);
	TEST_EQ_MEM (msg->msg_buf->buf, "test", 4);

	nih_free (msg);


	/* Check that we can receive a message that contains control data,
	 * and that it's put in the structure.
	 */
	TEST_FEATURE ("with control data");
	msghdr.msg_control = cbuf;
	msghdr.msg_controllen = sizeof (cbuf);

	cmsg = CMSG_FIRSTHDR (&msghdr);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN (sizeof (int));

	fdptr = (int *)CMSG_DATA (cmsg);
	memcpy (fdptr, &fds[0], sizeof (int));

	msghdr.msg_controllen = cmsg->cmsg_len;

	sendmsg (fds[0], &msghdr, 0);

	len = 4;
	msg = nih_io_message_recv (NULL, fds[1], &len);

	TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
	TEST_LIST_EMPTY (&msg->entry);

	TEST_EQ (len, 4);
	TEST_EQ (msg->msg_buf->len, 4);
	TEST_EQ_MEM (msg->msg_buf->buf, "test", 4);

	cmsg = (struct cmsghdr *)msg->ctrl_buf->buf;
	TEST_EQ (cmsg->cmsg_level, SOL_SOCKET);
	TEST_EQ (cmsg->cmsg_type, SCM_RIGHTS);
	TEST_EQ (cmsg->cmsg_len, CMSG_LEN (sizeof (int)));

	nih_free (msg);

	msghdr.msg_control = NULL;
	msghdr.msg_controllen = 0;


	/* Check that we get the NIH_IO_MESSAGE_TRUNCATED error if we try
	 * and get fewer bytes than are sent in the message.
	 */
	TEST_FEATURE ("with message that will be truncated");
	memset (buf, ' ', BUFSIZ * 2);
	iov[0].iov_len = BUFSIZ * 2;

	sendmsg (fds[0], &msghdr, 0);

	len = 4;
	msg = nih_io_message_recv (NULL, fds[1], &len);

	TEST_EQ_P (msg, NULL);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_IO_MESSAGE_TRUNCATED);
	nih_free (err);


	/* Check that we get an empty message and len is zero if we try and
	 * receive from a socket when the remote end is closed.
	 */
	TEST_FEATURE ("with remote end closed");
	close (fds[0]);
	close (fds[1]);

	socketpair (PF_UNIX, SOCK_STREAM, 0, fds);
	close (fds[0]);

	len = 4;
	msg = nih_io_message_recv (NULL, fds[1], &len);

	TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
	TEST_LIST_EMPTY (&msg->entry);

	TEST_EQ (len, 0);
	TEST_EQ (msg->msg_buf->len, 0);

	nih_free (msg);

	close (fds[1]);


	/* Check that we can receive a message from a non-specific source
	 * over an unconnected socket.
	 */
	TEST_FEATURE ("with unconnected sockets");
	addr0.sun_family = AF_UNIX;
	addr0.sun_path[0] = '\0';

	addr0len = offsetof (struct sockaddr_un, sun_path) + 1;
	addr0len += snprintf (addr0.sun_path + 1, sizeof (addr0.sun_path) - 1,
			      "/com/netsplit/libnih/test_io/%d.0", getpid ());

	fds[0] = socket (PF_UNIX, SOCK_DGRAM, 0);
	bind (fds[0], (struct sockaddr *)&addr0, addr0len);

	addr1.sun_family = AF_UNIX;
	addr1.sun_path[0] = '\0';

	addr1len = offsetof (struct sockaddr_un, sun_path) + 1;
	addr1len += snprintf (addr1.sun_path + 1, sizeof (addr1.sun_path) - 1,
			      "/com/netsplit/libnih/test_io/%d.1", getpid ());

	fds[1] = socket (PF_UNIX, SOCK_DGRAM, 0);
	bind (fds[1], (struct sockaddr *)&addr1, addr1len);

	msghdr.msg_name = (struct sockaddr *)&addr1;
	msghdr.msg_namelen = addr1len;

	memcpy (buf, "test", 4);
	iov[0].iov_len = 4;

	sendmsg (fds[0], &msghdr, 0);

	len = 4;
	msg = nih_io_message_recv (NULL, fds[1], &len);

	TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
	TEST_LIST_EMPTY (&msg->entry);

	TEST_EQ (msg->msg_buf->len, 4);
	TEST_EQ_MEM (msg->msg_buf->buf, "test", 4);

	TEST_EQ (msg->addrlen, addr0len);
	TEST_EQ (msg->addr->sa_family, PF_UNIX);
	TEST_EQ_MEM (((struct sockaddr_un *)msg->addr)->sun_path,
		     addr0.sun_path,
		     addr0len - offsetof (struct sockaddr_un, sun_path));

	nih_free (msg);

	close (fds[0]);
	close (fds[1]);


	/* Check that we get an error if the socket is closed.
	 */
	TEST_FEATURE ("with closed socket");
	len = 4;
	msg = nih_io_message_recv (NULL, fds[1], &len);

	TEST_EQ_P (msg, NULL);

	err = nih_error_get ();
	TEST_EQ (err->number, EBADF);
	nih_free (err);
}

void
test_message_send (void)
{
	NihError           *err;
	NihIoMessage       *msg;
	struct sockaddr_un  addr;
	size_t              addrlen;
	struct msghdr       msghdr;
	struct iovec        iov[1];
	char                buf[BUFSIZ], cbuf[CMSG_SPACE(sizeof (int))];
	struct cmsghdr     *cmsg;
	ssize_t             len;
	int                 fds[2], ret, *fdptr;

	TEST_FUNCTION ("nih_io_message_send");
	socketpair (PF_UNIX, SOCK_DGRAM, 0, fds);

	msghdr.msg_name = NULL;
	msghdr.msg_namelen = 0;
	msghdr.msg_iov = iov;
	msghdr.msg_iovlen = 1;
	msghdr.msg_control = NULL;
	msghdr.msg_controllen = 0;
	msghdr.msg_flags = 0;

	iov[0].iov_base = buf;
	iov[0].iov_len = sizeof (buf);

	/* Check that we can send a message down a socket with just the
	 * ordinary text, and no control data.
	 */
	TEST_FEATURE ("with no control data");
	msg = nih_io_message_new (NULL);
	nih_io_buffer_push (msg->msg_buf, "test", 4);

	ret = nih_io_message_send (msg, fds[0]);

	TEST_EQ (ret, 0);

	len = recvmsg (fds[1], &msghdr, 0);

	TEST_EQ (len, 4);
	TEST_EQ_MEM (buf, "test", 4);


	/* Check that we can include control message information in the
	 * message, and have it come out the other end.
	 */
	TEST_FEATURE ("with control data");
	nih_io_buffer_resize (msg->ctrl_buf, CMSG_SPACE (sizeof (int)));

	cmsg = (struct cmsghdr *)msg->ctrl_buf->buf;
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN (sizeof (int));

	fdptr = (int *)CMSG_DATA (cmsg);
	memcpy (fdptr, &fds[0], sizeof (int));

	msg->ctrl_buf->len = cmsg->cmsg_len;

	ret = nih_io_message_send (msg, fds[0]);

	TEST_EQ (ret, 0);

	msghdr.msg_control = cbuf;
	msghdr.msg_controllen = sizeof (cbuf);

	len = recvmsg (fds[1], &msghdr, 0);

	TEST_EQ (len, 4);
	TEST_EQ_MEM (buf, "test", 4);

	cmsg = CMSG_FIRSTHDR (&msghdr);
	TEST_EQ (cmsg->cmsg_level, SOL_SOCKET);
	TEST_EQ (cmsg->cmsg_type, SCM_RIGHTS);
	TEST_EQ (cmsg->cmsg_len, CMSG_LEN (sizeof (int)));

	close (fds[0]);
	close (fds[1]);


	/* Check that we can send a message to a specific destination over
	 * an unconnected socket.
	 */
	TEST_FEATURE ("with unconnected sockets");
	addr.sun_family = AF_UNIX;
	addr.sun_path[0] = '\0';

	addrlen = offsetof (struct sockaddr_un, sun_path) + 1;
	addrlen += snprintf (addr.sun_path + 1, sizeof (addr.sun_path) - 1,
			     "/com/netsplit/libnih/test_io/%d", getpid ());

	fds[0] = socket (PF_UNIX, SOCK_DGRAM, 0);
	fds[1] = socket (PF_UNIX, SOCK_DGRAM, 0);
	bind (fds[1], (struct sockaddr *)&addr, addrlen);

	msg->addr = (struct sockaddr *)&addr;
	msg->addrlen = addrlen;

	msg->ctrl_buf->len = 0;

	ret = nih_io_message_send (msg, fds[0]);

	TEST_EQ (ret, 0);

	msghdr.msg_control = NULL;
	msghdr.msg_controllen = 0;

	len = recvmsg (fds[1], &msghdr, 0);

	TEST_EQ (len, 4);
	TEST_EQ_MEM (buf, "test", 4);

	nih_free (msg);

	close (fds[0]);
	close (fds[1]);


	/* Check that we get an error if the socket is closed. */
	msg = nih_io_message_new (NULL);
	nih_io_buffer_push (msg->msg_buf, "test", 4);

	ret = nih_io_message_send (msg, fds[0]);

	TEST_LT (ret, 0);

	err = nih_error_get ();
	TEST_EQ (err->number, EBADF);
	nih_free (err);

	nih_free (msg);
}


static int read_called = 0;
static int close_called = 0;
static int error_called = 0;
static NihError *last_error = NULL;
static const char *last_str = NULL;
static size_t last_len = 0;

static void
my_reader (void       *data,
	   NihIo      *io,
	   const char *str,
	   size_t      len)
{
	if (! data)
		nih_io_close (io);

	read_called++;
	last_data = data;
	last_str = str;
	last_len = len;
}

static void
my_close_handler (void  *data,
		  NihIo *io)
{
	last_data = data;
	close_called++;
}

static void
my_error_handler (void  *data,
		  NihIo *io)
{
	last_data = data;
	last_error = nih_error_get ();
	error_called++;
}

void
test_reopen (void)
{
	NihIo            *io;
	int               fds[2];
	struct sigaction  oldact;

	TEST_FUNCTION ("nih_io_reopen");
	pipe (fds);

	/* Check that we can create a stream mode NihIo structure from an
	 * existing file descriptor; the structure should be correctly
	 * populated and assigned an NihIoWatch.  The file descriptor
	 * should be altered so that it is non-blocking.
	 */
	TEST_FEATURE ("with stream mode");
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    my_reader, my_close_handler, my_error_handler,
			    &io);

	TEST_ALLOC_SIZE (io, sizeof (NihIo));
	TEST_ALLOC_PARENT (io->send_buf, io);
	TEST_ALLOC_SIZE (io->send_buf, sizeof (NihIoBuffer));
	TEST_ALLOC_PARENT (io->recv_buf, io);
	TEST_ALLOC_SIZE (io->recv_buf, sizeof (NihIoBuffer));
	TEST_EQ (io->type, NIH_IO_STREAM);
	TEST_EQ_P (io->reader, my_reader);
	TEST_EQ_P (io->close_handler, my_close_handler);
	TEST_EQ_P (io->error_handler, my_error_handler);
	TEST_EQ_P (io->data, &io);
	TEST_FALSE (io->shutdown);
	TEST_EQ_P (io->close, NULL);

	TEST_ALLOC_PARENT (io->watch, io);
	TEST_EQ (io->watch->fd, fds[0]);
	TEST_EQ (io->watch->events, NIH_IO_READ);
	TEST_TRUE (fcntl (fds[0], F_GETFL) & O_NONBLOCK);

	nih_free (io);


	/* Check that we can create a message mode NihIo structure from an
	 * existing file descriptor; the structure should be correctly
	 * populated and assigned an NihIoWatch.  The file descriptor
	 * should be altered so that it is non-blocking.
	 */
	TEST_FEATURE ("with message mode");
	io = nih_io_reopen (NULL, fds[0], NIH_IO_MESSAGE,
			    my_reader, my_close_handler, my_error_handler,
			    &io);

	TEST_ALLOC_SIZE (io, sizeof (NihIo));
	TEST_ALLOC_PARENT (io->send_q, io);
	TEST_ALLOC_SIZE (io->send_q, sizeof (NihList));
	TEST_ALLOC_PARENT (io->recv_q, io);
	TEST_ALLOC_SIZE (io->recv_q, sizeof (NihList));
	TEST_EQ (io->type, NIH_IO_MESSAGE);
	TEST_EQ_P (io->reader, my_reader);
	TEST_EQ_P (io->close_handler, my_close_handler);
	TEST_EQ_P (io->error_handler, my_error_handler);
	TEST_EQ_P (io->data, &io);
	TEST_FALSE (io->shutdown);
	TEST_EQ_P (io->close, NULL);

	TEST_ALLOC_PARENT (io->watch, io);
	TEST_EQ (io->watch->fd, fds[0]);
	TEST_EQ (io->watch->events, NIH_IO_READ);
	TEST_TRUE (fcntl (fds[0], F_GETFL) & O_NONBLOCK);

	nih_free (io);


	close (fds[0]);
	close (fds[1]);


	/* Check that the SIGPIPE signal will now be ignored */
	sigaction (SIGPIPE, NULL, &oldact);
	TEST_EQ (oldact.sa_handler, SIG_IGN);
}


static int free_called;

static int
destructor_called (void *ptr)
{
	free_called++;

	return 0;
}

void
test_shutdown (void)
{
	NihIo  *io;
	int     fds[2];
	fd_set  readfds, writefds, exceptfds;

	TEST_FUNCTION ("nih_io_shutdown");
	pipe (fds);
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    NULL, NULL, NULL, NULL);
	nih_io_buffer_push (io->recv_buf, "some data", 9);

	free_called = 0;
	nih_alloc_set_destructor (io, destructor_called);

	/* Check that shutting down a socket with data in the buffer
	 * merely marks it as shutdown and neither closes the socket or
	 * frees the structure.
	 */
	TEST_FEATURE ("with data in the buffer");
	nih_io_shutdown (io);

	TEST_TRUE (io->shutdown);
	TEST_FALSE (free_called);
	TEST_GE (fcntl (fds[0], F_GETFD), 0);


	/* Check that handling the data in the buffer, emptying it, causes
	 * the shutdown socket to be closed and the structure to be freed.
	 */
	TEST_FEATURE ("with data being handled");
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);
	FD_SET (fds[0], &readfds);
	nih_io_buffer_shrink (io->recv_buf, 9);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (free_called);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);

	close (fds[1]);


	/* Check that shutting down a socket with no data in the buffer
	 * results in it being immediately closed and freed.
	 */
	TEST_FEATURE ("with no data in the buffer");
	pipe (fds);
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    NULL, NULL, NULL, NULL);

	free_called = 0;
	nih_alloc_set_destructor (io, destructor_called);

	nih_io_shutdown (io);

	TEST_TRUE (free_called);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);

	close (fds[1]);
}

void
test_close (void)
{
	NihIo *io;
	int    fds[2], lazy_close;

	TEST_FUNCTION ("nih_io_close");

	/* Check that closing an open file descriptor doesn't call the error
	 * handler, and just closes the fd and frees the structure.
	 */
	TEST_FEATURE ("with open file descriptor");
	pipe (fds);
	error_called = 0;
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    NULL, NULL, my_error_handler, &io);

	free_called = 0;
	nih_alloc_set_destructor (io, destructor_called);

	nih_io_close (io);

	TEST_FALSE (error_called);
	TEST_TRUE (free_called);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);

	close (fds[1]);


	/* Check that closing a file descriptor that's already closed
	 * results in the error handler being called with an EBADF system
	 * error and the data pointer, followed by the structure being
	 * freed.
	 */
	TEST_FEATURE ("with closed file descriptor");
	pipe (fds);
	error_called = 0;
	last_data = NULL;
	last_error = NULL;
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    NULL, NULL, my_error_handler, &io);

	free_called = 0;
	nih_alloc_set_destructor (io, destructor_called);

	close (fds[0]);
	nih_io_close (io);

	TEST_TRUE (error_called);
	TEST_EQ (last_error->number, EBADF);
	TEST_EQ_P (last_data, &io);
	TEST_TRUE (free_called);

	nih_free (last_error);

	close (fds[1]);


	/* Check that closing the file descriptor during a watcher function
	 * (when io->close is non-NULL) just causes TRUE to be stored in
	 * that variable.
	 */
	TEST_FEATURE ("with close flag variable set");
	pipe (fds);
	error_called = 0;
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    NULL, NULL, my_error_handler, &io);

	free_called = 0;
	nih_alloc_set_destructor (io, destructor_called);

	lazy_close = FALSE;
	io->close = &lazy_close;

	nih_io_close (io);

	TEST_TRUE (lazy_close);
	TEST_FALSE (error_called);
	TEST_FALSE (free_called);
	TEST_EQ (fcntl (fds[0], F_GETFD), 0);

	nih_free (io);

	close (fds[0]);
	close (fds[1]);
}

void
test_watcher (void)
{
	NihIo  *io;
	int     fds[2];
	fd_set  readfds, writefds, exceptfds;
	FILE   *output;

	TEST_FUNCTION ("nih_io_watcher");

	/* Check that data to be read on a socket watched by NihIo ends up
	 * in the receive buffer, and results in the reader function being
	 * called with the right arguments.
	 */
	TEST_FEATURE ("with data to read");
	pipe (fds);
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    my_reader, my_close_handler, my_error_handler,
			    &io);

	write (fds[1], "this is a test", 14);

	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);
	FD_SET (fds[0], &readfds);

	read_called = 0;
	last_data = NULL;
	last_str = NULL;
	last_len = 0;

	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (read_called);
	TEST_EQ_P (last_data, &io);
	TEST_EQ_P (last_str, io->recv_buf->buf);
	TEST_EQ (last_len, io->recv_buf->len);
	TEST_EQ (io->recv_buf->len, 14);
	TEST_EQ_MEM (io->recv_buf->buf, "this is a test", 14);


	/* Check that the reader function is called again when more data
	 * comes in, and that the buffer contains both sets of data.
	 */
	TEST_FEATURE ("with more data to read");
	write (fds[1], " of the callback code", 19);

	read_called = 0;
	last_data = NULL;
	last_str = NULL;
	last_len = 0;

	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (read_called);
	TEST_EQ_P (last_data, &io);
	TEST_EQ_P (last_str, io->recv_buf->buf);
	TEST_EQ (last_len, io->recv_buf->len);
	TEST_EQ (io->recv_buf->len, 33);
	TEST_EQ_MEM (io->recv_buf->buf, "this is a test of the callback code",
		     33);


	/* Check that the reader function can call nih_io_close(), resulting
	 * in the structure being closed once it has finished the watcher
	 * function.
	 */
	TEST_FEATURE ("with close called in reader");
	io->data = NULL;

	free_called = 0;
	nih_alloc_set_destructor (io, destructor_called);

	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (free_called);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);

	close (fds[1]);


	/* Check that the reader function is also closed when the remote end
	 * has been closed; along with the close function.
	 */
	TEST_FEATURE ("with remote end closed");
	pipe (fds);
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    my_reader, my_close_handler, my_error_handler,
			    &io);

	nih_io_buffer_push (io->recv_buf,
			    "this is a test of the callback code", 33);

	read_called = 0;
	close_called = 0;
	last_data = NULL;
	last_str = NULL;
	last_len = 0;

	close (fds[1]);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (read_called);
	TEST_TRUE (close_called);
	TEST_EQ_P (last_data, &io);
	TEST_EQ_P (last_str, io->recv_buf->buf);
	TEST_EQ (last_len, io->recv_buf->len);
	TEST_EQ (io->recv_buf->len, 33);
	TEST_EQ_MEM (io->recv_buf->buf, "this is a test of the callback code",
		     33);


	/* Check that the reader function and error handler are called if
	 * the local end gets closed.  The error should be EBADF.
	 */
	TEST_FEATURE ("with local end closed");
	read_called = 0;
	error_called = 0;
	last_data = NULL;
	last_str = NULL;
	last_len = 0;
	last_error = NULL;

	close (fds[0]);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (error_called);
	TEST_EQ (last_error->number, EBADF);
	TEST_TRUE (read_called);
	TEST_EQ_P (last_data, &io);
	TEST_EQ_P (last_str, io->recv_buf->buf);
	TEST_EQ (last_len, io->recv_buf->len);
	TEST_EQ (io->recv_buf->len, 33);
	TEST_EQ_MEM (io->recv_buf->buf, "this is a test of the callback code",
		     33);

	nih_free (last_error);
	nih_free (io);


	/* Check that if the remote end closes and there's no close handler,
	 * the file descriptor is closed and the structure freed.
	 */
	TEST_FEATURE ("with no close handler");
	pipe (fds);
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    my_reader, NULL, NULL, &io);

	free_called = 0;
	nih_alloc_set_destructor (io, destructor_called);

	FD_ZERO (&readfds);
	FD_SET (fds[0], &readfds);

	close (fds[1]);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (free_called);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);


	/* Check that if the local end closes and there's no error handler
	 * that the structure is freed.
	 */
	TEST_FEATURE ("with no error handler");
	pipe (fds);
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    my_reader, NULL, NULL, &io);

	free_called = 0;
	nih_alloc_set_destructor (io, destructor_called);

	FD_ZERO (&readfds);
	FD_SET (fds[0], &readfds);

	nih_log_set_priority (NIH_LOG_FATAL);
	close (fds[0]);
	close (fds[1]);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);
	nih_log_set_priority (NIH_LOG_DEBUG);

	TEST_TRUE (free_called);

	FD_ZERO (&readfds);


	/* Check that data in the send buffer is written to the file
	 * descriptor if it's pollable for writing.  Once the data has been
	 * written, the watch should no longer be checking for writability.
	 */
	TEST_FEATURE ("with data to write");
	output = tmpfile ();
	io = nih_io_reopen (NULL, fileno (output), NIH_IO_STREAM,
			    NULL, my_close_handler, my_error_handler, &io);

	nih_io_printf (io, "this is a test\n");

	FD_SET (fileno (output), &writefds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	rewind (output);

	TEST_FILE_EQ (output, "this is a test\n");
	TEST_FILE_END (output);

	TEST_EQ (io->send_buf->len, 0);
	TEST_EQ (io->send_buf->size, 0);
	TEST_EQ_P (io->send_buf->buf, NULL);

	TEST_FALSE (io->watch->events & NIH_IO_WRITE);


	/* Check that we can write more data and that is send out to the
	 * file descriptor as well.
	 */
	TEST_FEATURE ("with more data to write");
	nih_io_printf (io, "so is this\n");
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	rewind (output);

	TEST_FILE_EQ (output, "this is a test\n");
	TEST_FILE_EQ (output, "so is this\n");
	TEST_FILE_END (output);

	TEST_EQ (io->send_buf->len, 0);
	TEST_EQ (io->send_buf->size, 0);
	TEST_EQ_P (io->send_buf->buf, NULL);

	TEST_FALSE (io->watch->events & NIH_IO_WRITE);

	fclose (output);


	/* Check that an attempt to write to a closed file results in the
	 * error handler being called.
	 */
	TEST_FEATURE ("with closed file");
	error_called = 0;
	last_data = NULL;
	last_error = NULL;

	nih_io_printf (io, "this write fails\n");
	FD_SET (fds[0], &readfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (error_called);
	TEST_EQ (last_error->number, EBADF);
	TEST_EQ_P (last_data, &io);

	nih_free (last_error);

	nih_free (io);
}


void
test_read_message (void)
{
	NihIo        *io;
	NihIoMessage *msg, *ptr;

	TEST_FUNCTION ("nih_io_read_message");
	io = nih_io_reopen (NULL, 0, NIH_IO_MESSAGE, NULL, NULL, NULL, NULL);

	msg = nih_io_message_new (io);
	nih_io_buffer_push (msg->msg_buf, "this is a test", 14);
	nih_list_add (io->recv_q, &msg->entry);

	/* Check that we can read a message in the NihIo receive queue,
	 * the message returned should be the same message we queued and
	 * should be reparented as well as removed from the queue.
	 */
	TEST_FEATURE ("with message in queue");
	ptr = nih_io_read_message (NULL, io);

	TEST_EQ_P (ptr, msg);
	TEST_ALLOC_PARENT (msg, NULL);
	TEST_LIST_EMPTY (&msg->entry);
	TEST_LIST_EMPTY (io->recv_q);

	nih_free (msg);


	/* Check that we get NULL when the receive queue is empty. */
	TEST_FEATURE ("with empty queue");
	msg = nih_io_read_message (NULL, io);

	TEST_EQ_P (msg, NULL);

	nih_free (io);
}

void
test_send_message (void)
{
	NihIo        *io;
	NihIoMessage *msg1, *msg2;

	TEST_FUNCTION ("nih_io_send_message");
	io = nih_io_reopen (NULL, 0, NIH_IO_MESSAGE, NULL, NULL, NULL, NULL);


	/* Check that we can send a message into the empty send queue, it
	 * should be added directly to the send queue, and not changed or
	 * reparented, etc.
	 */
	TEST_FEATURE ("with empty send queue");
	msg1 = nih_io_message_new (NULL);
	nih_io_buffer_push (msg1->msg_buf, "this is a test", 14);

	nih_io_send_message (io, msg1);

	TEST_EQ_P (io->send_q->next, &msg1->entry);
	TEST_ALLOC_PARENT (msg1, NULL);


	/* Check that we can send a message when there's already one in
	 * the send queue, it should be appended to the queue.
	 */
	TEST_FEATURE ("with message already in send queue");
	msg2 = nih_io_message_new (NULL);
	nih_io_buffer_push (msg2->msg_buf, "this is a test", 14);

	nih_io_send_message (io, msg2);

	TEST_EQ_P (io->send_q->next, &msg1->entry);
	TEST_EQ_P (io->send_q->prev, &msg2->entry);

	nih_free (msg1);
	nih_free (msg2);
	nih_free (io);
}


void
test_read (void)
{
	NihIo  *io;
	char   *str;
	size_t  len;

	TEST_FUNCTION ("nih_io_read");
	io = nih_io_reopen (NULL, 0, NIH_IO_STREAM, NULL, NULL, NULL, NULL);
	nih_io_buffer_push (io->recv_buf, "this is a test of the io code", 29);


	/* Check that we can read data in the NihIo receive buffer, and the
	 * data is returned NULL-terminated, allocated with nih_alloc and
	 * removed from the front of the receive buffer itself.
	 */
	TEST_FEATURE ("with full buffer");
	len = 14;
	str = nih_io_read (NULL, io, &len);

	TEST_EQ (len, 14);
	TEST_ALLOC_SIZE (str, 15);
	TEST_EQ (str[14], '\0');
	TEST_EQ_STR (str, "this is a test");
	TEST_EQ (io->recv_buf->len, 15);
	TEST_EQ_MEM (io->recv_buf->buf, " of the io code", 15);

	nih_free (str);


	/* Check that we can empty all of the data from the NihIo receive
	 * buffer, which results in the buffer being freed.
	 */
	TEST_FEATURE ("with request to empty buffer");
	len = 15;
	str = nih_io_read (NULL, io, &len);

	TEST_EQ (len, 15);
	TEST_ALLOC_SIZE (str, 16);
	TEST_EQ (str[15], '\0');
	TEST_EQ_STR (str, " of the io code");
	TEST_EQ (io->recv_buf->len, 0);
	TEST_EQ (io->recv_buf->size, 0);
	TEST_EQ_P (io->recv_buf->buf, NULL);

	nih_free (str);


	/* Check that we can request more data than is in the buffer, and
	 * get a short read with len updated.
	 */
	TEST_FEATURE ("with larger request than buffer");
	nih_io_buffer_push (io->recv_buf, "another test", 12);

	len = 20;
	str = nih_io_read (NULL, io, &len);

	TEST_EQ (len, 12);
	TEST_ALLOC_SIZE (str, 13);
	TEST_EQ (str[12], '\0');
	TEST_EQ_STR (str, "another test");
	TEST_EQ (io->recv_buf->len, 0);
	TEST_EQ (io->recv_buf->size, 0);
	TEST_EQ_P (io->recv_buf->buf, NULL);

	nih_free (str);

	nih_free (io);
}

void
test_write (void)
{
	NihIo *io;

	TEST_FUNCTION ("nih_io_write");
	io = nih_io_reopen (NULL, 0, NIH_IO_STREAM, NULL, NULL, NULL, NULL);

	/* Check that we can write data into the NihIo send buffer, the
	 * buffer should contain the data and be a page in size.  The
	 * watch should also now be looking for writability.
	 */
	TEST_FEATURE ("with empty buffer");
	nih_io_write (io, "test", 4);

	TEST_ALLOC_SIZE (io->send_buf->buf, BUFSIZ);
	TEST_EQ (io->send_buf->size, BUFSIZ);
	TEST_EQ (io->send_buf->len, 4);
	TEST_EQ_MEM (io->send_buf->buf, "test", 4);
	TEST_TRUE (io->watch->events & NIH_IO_WRITE);


	/* Check that we can write more data onto the end of the NihIo
	 * send buffer, which increases its size.
	 */
	TEST_FEATURE ("with data in the buffer");
	nih_io_write (io, "ing the io code", 10);

	TEST_EQ (io->send_buf->len, 14);
	TEST_EQ_MEM (io->send_buf->buf, "testing the io", 14);

	nih_free (io);
}

void
test_get (void)
{
	NihIo *io;
	char  *str;

	TEST_FUNCTION ("nih_io_get");
	io = nih_io_reopen (NULL, 0, NIH_IO_STREAM, NULL, NULL, NULL, NULL);
	nih_io_buffer_push (io->recv_buf, "some data\n", 10);
	nih_io_buffer_push (io->recv_buf, "and another line\n", 17);
	nih_io_buffer_push (io->recv_buf, "incomplete", 10);

	/* Check that we can take data from the front of a buffer up until
	 * the first embedded new line (which isn't returned), and have the
	 * buffer shuffled up.
	 */
	TEST_FEATURE ("with full buffer");
	str = nih_io_get (NULL, io, "\n");

	TEST_ALLOC_SIZE (str, 10);
	TEST_EQ_STR (str, "some data");

	nih_free (str);


	/* Check that we can read up to the next line line. */
	TEST_FEATURE ("with part-full buffer");
	str = nih_io_get (NULL, io, "\n");

	TEST_ALLOC_SIZE (str, 17);
	TEST_EQ_STR (str, "and another line");

	nih_free (str);


	/* Check that NULL is returned if the data in the buffer doesn't
	 * contain the delimeter or a NULL terminator.
	 */
	TEST_FEATURE ("with incomplete line in buffer");
	str = nih_io_get (NULL, io, "\n");

	TEST_EQ_P (str, NULL);


	/* Check that a NULL terminator is sufficient to return the data
	 * in the buffer, which should now be empty.
	 */
	TEST_FEATURE ("with null-terminated string in buffer");
	nih_io_buffer_push (io->recv_buf, "\0", 1);
	str = nih_io_get (NULL, io, "\n");

	TEST_ALLOC_SIZE (str, 11);
	TEST_EQ_STR (str, "incomplete");

	TEST_EQ (io->recv_buf->len, 0);

	nih_free (str);

	nih_free (io);
}

void
test_printf (void)
{
	NihIo *io;

	TEST_FUNCTION ("nih_io_printf");
	io = nih_io_reopen (NULL, 0, NIH_IO_STREAM, NULL, NULL, NULL, NULL);

	/* Check that we can write a line of formatted data into the send
	 * buffer, which should be written without a NULL terminator.
	 * The watch should also look for writability.
	 */
	TEST_FEATURE ("with empty buffer");
	nih_io_printf (io, "this is a %d %s test\n", 4, "format");

	TEST_ALLOC_SIZE (io->send_buf->buf, BUFSIZ);
	TEST_EQ (io->send_buf->size, BUFSIZ);
	TEST_EQ (io->send_buf->len, 24);
	TEST_EQ_MEM (io->send_buf->buf, "this is a 4 format test\n", 24);
	TEST_TRUE (io->watch->events & NIH_IO_WRITE);


	/* Check that we can append a further line of formatted data into
	 * the send buffer
	 */
	TEST_FEATURE ("with data in the buffer");
	nih_io_printf (io, "and this is %s line\n", "another");

	TEST_EQ (io->send_buf->len, 49);
	TEST_EQ_MEM (io->send_buf->buf,
		     "this is a 4 format test\nand this is another line\n",
		     49);

	nih_free (io);
}


void
test_set_nonblock (void)
{
	int fds[2];

	/* Check that we can trivially mark a socket to be non-blocking. */
	TEST_FUNCTION ("nih_io_set_nonblock");
	pipe (fds);
	nih_io_set_nonblock (fds[0]);

	TEST_TRUE (fcntl (fds[0], F_GETFL) & O_NONBLOCK);

	close (fds[0]);
	close (fds[1]);
}

void
test_set_cloexec (void)
{
	int fds[2];

	/* Check that we can trivially mark a socket to be closed on exec. */
	TEST_FUNCTION ("nih_io_set_cloexec");
	pipe (fds);
	nih_io_set_cloexec (fds[0]);

	TEST_TRUE (fcntl (fds[0], F_GETFD) & FD_CLOEXEC);

	close (fds[0]);
	close (fds[1]);
}

void
test_get_family (void)
{
	int fd;

	TEST_FUNCTION ("nih_io_get_family");

	/* Check that we can obtain the family of a UNIX socket. */
	fd = socket (PF_UNIX, SOCK_STREAM, 0);

	TEST_EQ (nih_io_get_family (fd), PF_UNIX);

	close (fd);


	/* Check that we can obtain the family of an IPv4 socket. */
	fd = socket (PF_INET, SOCK_STREAM, 0);

	TEST_EQ (nih_io_get_family (fd), PF_INET);

	close (fd);


	/* Check that we can obtain the family of an IPv6 socket. */
	fd = socket (PF_INET6, SOCK_STREAM, 0);

	TEST_EQ (nih_io_get_family (fd), PF_INET6);

	close (fd);


	/* Check that we get -1 on error. */
	TEST_LT (nih_io_get_family (fd), 0);
}


int
main (int   argc,
      char *argv[])
{
	test_add_watch ();
	test_select_fds ();
	test_handle_fds ();
	test_buffer_new ();
	test_buffer_resize ();
	test_buffer_pop ();
	test_buffer_shrink ();
	test_buffer_push ();
	test_message_new ();
	test_message_push_control ();
	test_message_recv ();
	test_message_send ();
	test_reopen ();
	test_shutdown ();
	test_close ();
	test_watcher ();
	test_read_message ();
	test_send_message ();
	test_read ();
	test_write ();
	test_get ();
	test_printf ();
	test_set_nonblock ();
	test_set_cloexec ();
	test_get_family ();

	return 0;
}
