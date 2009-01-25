/* libnih
 *
 * test_com.netsplit.Nih.Test_proxy.c - test suite for auto-generated
 * proxy bindings.
 *
 * Copyright © 2008 Scott James Remnant <scott@netsplit.com>.
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

#include <dbus/dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/error.h>
#include <nih/errors.h>

#include <nih/dbus.h>

#include "com.netsplit.Nih.Test_proxy.h"
#include "com.netsplit.Nih.Test_impl.h"


void
test_method_dispatch (void)
{
	DBusConnection  *conn;
	NihDBusProxy    *proxy;
	NihError        *err;
	NihDBusError    *dbus_err;
	char            *output;
	uint8_t          byte_arg;
	int              boolean_arg;
	int16_t          int16_arg;
	uint16_t         uint16_arg;
	int32_t          int32_arg;
	uint32_t         uint32_arg;
	int64_t          int64_arg;
	uint64_t         uint64_arg;
	double           double_arg;
	int32_t         *int32_array;
	char           **str_array;
	size_t           array_len;
	int              ret;

	TEST_GROUP ("method dispatching");


	/* Check that we can make a D-Bus method call, passing in the
	 * expected arguments and receiving the expected arguments in the
	 * reply.
	 */
	TEST_FEATURE ("with valid argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_method (proxy, "test data", 0, &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "test data");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that if the method call returns a D-Bus error, the proxy
	 * call returns a negative number and raises the same D-Bus error.
	 */
	TEST_FEATURE ("with returned D-Bus error");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_method (proxy, "test data", 1, &output);

	TEST_LT (ret, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_DBUS_ERROR);
	TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

	dbus_err = (NihDBusError *)err;
	TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.IllegalValue");

	nih_free (dbus_err);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that in out of memory conditions, D-Bus automatically
	 * repeats the method call so we don't notice on the client side.
	 */
	TEST_FEATURE ("with out of memory error");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_method (proxy, "test data", 2, &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "test data");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an error unknown to D-Bus is turned into a generic
	 * failed error.
	 */
	TEST_FEATURE ("with unknown error");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_method (proxy, "test data", 3, &output);

	TEST_LT (ret, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_DBUS_ERROR);
	TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

	dbus_err = (NihDBusError *)err;
	TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

	nih_free (dbus_err);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that the fact the server implementation is asynchronous
	 * is hidden and the call blocks until the reply comes back anyway.
	 */
	TEST_FEATURE ("with valid argument to async call");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_async_method (proxy, "test data", 0, &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "test data");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an error returned from an asynchronous server-side
	 * call still comes back as an error.
	 */
	TEST_FEATURE ("with returned D-Bus error from async call");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_async_method (proxy, "test data", 1, &output);

	TEST_LT (ret, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_DBUS_ERROR);
	TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

	dbus_err = (NihDBusError *)err;
	TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.IllegalValue");

	nih_free (dbus_err);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that in out of memory conditions, D-Bus automatically
	 * repeats the method call so we don't notice on the client side
	 * even for async server-side calls.
	 */
	TEST_FEATURE ("with out of memory error from async call");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_async_method (proxy, "test data", 2, &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "test data");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an error unknown to D-Bus is turned into a generic
	 * failed error.
	 */
	TEST_FEATURE ("with unknown error from async call");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_async_method (proxy, "test data", 3, &output);

	TEST_LT (ret, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_DBUS_ERROR);
	TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

	dbus_err = (NihDBusError *)err;
	TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

	nih_free (dbus_err);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that a condition whereby the wrong arguments are returned
	 * from a method call results in a special illegal arguments error
	 * being returned.
	 */
	TEST_FEATURE ("with wrong argument type in reply");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_async_method (proxy, "test data", 4, &output);

	TEST_LT (ret, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
	nih_free (err);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that a condition whereby too many arguments are returned
	 * from a method call results in a special illegal arguments error
	 * being returned.
	 */
	TEST_FEATURE ("with too many arguments in reply");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_async_method (proxy, "test data", 5, &output);

	TEST_LT (ret, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
	nih_free (err);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that a condition whereby arguments are missing from the
	 * method call return results in a special illegal arguments error
	 * being returned.
	 */
	TEST_FEATURE ("with missing arguments in reply");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_async_method (proxy, "test data", 6, &output);

	TEST_LT (ret, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
	nih_free (err);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an input argument of Byte type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Byte input argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_byte_to_str (proxy, 65, &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "65");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an output argument of Byte type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Byte output argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	byte_arg = 0;

	ret = proxy_str_to_byte (proxy, "65", &byte_arg);

	TEST_EQ (ret, 0);

	TEST_EQ (byte_arg, 65);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an input argument of Boolean type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Boolean input argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_boolean_to_str (proxy, 1, &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "True");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an output argument of Boolean type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Boolean output argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	boolean_arg = TRUE;

	ret = proxy_str_to_boolean (proxy, "False", &boolean_arg);

	TEST_EQ (ret, 0);

	TEST_EQ (boolean_arg, FALSE);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an input argument of Int16 type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Int16 input argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_int16_to_str (proxy, 1701, &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "1701");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an output argument of Int16 type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Int16 output argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	int16_arg = 0;

	ret = proxy_str_to_int16 (proxy, "1701", &int16_arg);

	TEST_EQ (ret, 0);

	TEST_EQ (int16_arg, 1701);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an input argument of UInt16 type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with UInt16 input argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_uint16_to_str (proxy, 1701, &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "1701");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an output argument of UInt16 type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with UInt16 output argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	uint16_arg = 0;

	ret = proxy_str_to_uint16 (proxy, "1701", &uint16_arg);

	TEST_EQ (ret, 0);

	TEST_EQ (uint16_arg, 1701);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an input argument of Int32 type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Int32 input argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_int32_to_str (proxy, 1701, &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "1701");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an output argument of Int32 type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Int32 output argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	int32_arg = 0;

	ret = proxy_str_to_int32 (proxy, "1701", &int32_arg);

	TEST_EQ (ret, 0);

	TEST_EQ (int32_arg, 1701);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an input argument of UInt32 type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with UInt32 input argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_uint32_to_str (proxy, 1701, &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "1701");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an output argument of UInt32 type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with UInt32 output argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	uint32_arg = 0;

	ret = proxy_str_to_uint32 (proxy, "1701", &uint32_arg);

	TEST_EQ (ret, 0);

	TEST_EQ (uint32_arg, 1701);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an input argument of Int64 type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Int64 input argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_int64_to_str (proxy, 1701, &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "1701");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an output argument of Int64 type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Int64 output argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	int64_arg = 0;

	ret = proxy_str_to_int64 (proxy, "1701", &int64_arg);

	TEST_EQ (ret, 0);

	TEST_EQ (int64_arg, 1701);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an input argument of UInt64 type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with UInt64 input argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_uint64_to_str (proxy, 1701, &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "1701");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an output argument of UInt64 type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with UInt64 output argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	uint64_arg = 0;

	ret = proxy_str_to_uint64 (proxy, "1701", &uint64_arg);

	TEST_EQ (ret, 0);

	TEST_EQ (uint64_arg, 1701);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an input argument of Double type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Double input argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_double_to_str (proxy, 3.141592, &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "3.141592");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an output argument of Double type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Double output argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	double_arg = 0;

	ret = proxy_str_to_double (proxy, "3.141592", &double_arg);

	TEST_EQ (ret, 0);

	TEST_EQ (double_arg, 3.141592);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an input argument of ObjectPath type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with ObjectPath input argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_object_path_to_str (proxy, "/com/netsplit/Nih", &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "/com/netsplit/Nih");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an output argument of ObjectPath type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with ObjectPath output argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_str_to_object_path (proxy, "/com/netsplit/Nih", &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "/com/netsplit/Nih");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an input argument of Signature type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Signature input argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_signature_to_str (proxy, "a{sv}", &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "a{sv}");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an output argument of Signature type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Signature output argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_str_to_signature (proxy, "a{sv}", &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "a{sv}");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an input argument of Array type with Int32 members
	 * is dispatched correctly.
	 */
	TEST_FEATURE ("with Int32 Array input argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	int32_array = nih_alloc (NULL, sizeof (int32_t) * 6);
	int32_array[0] = 4;
	int32_array[1] = 8;
	int32_array[2] = 15;
	int32_array[3] = 16;
	int32_array[4] = 23;
	int32_array[5] = 42;
	array_len = 6;

	output = NULL;

	ret = proxy_int32_array_to_str (proxy, int32_array, array_len,
					&output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "4 8 15 16 23 42");

	nih_free (proxy);
	nih_free (int32_array);

	my_teardown (conn);


	/* Check that an output argument of Array type with Int32 members
	 * is marshalled correctly.
	 */
	TEST_FEATURE ("with Int32 Array output argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	int32_array = NULL;
	array_len = 0;

	ret = proxy_str_to_int32_array (proxy, "4 8 15 16 23 42",
					&int32_array, &array_len);

	TEST_EQ (ret, 0);

	TEST_NE_P (int32_array, NULL);
	TEST_ALLOC_PARENT (int32_array, proxy);
	TEST_EQ (array_len, 6);
	TEST_EQ (int32_array[0], 4);
	TEST_EQ (int32_array[1], 8);
	TEST_EQ (int32_array[2], 15);
	TEST_EQ (int32_array[3], 16);
	TEST_EQ (int32_array[4], 23);
	TEST_EQ (int32_array[5], 42);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an input argument of Array type with String members
	 * is dispatched correctly.
	 */
	TEST_FEATURE ("with String Array input argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	str_array = nih_alloc (NULL, sizeof (char *) * 5);
	str_array[0] = "this";
	str_array[1] = "is";
	str_array[2] = "a";
	str_array[3] = "test";
	str_array[4] = NULL;

	output = NULL;

	ret = proxy_str_array_to_str (proxy, str_array, &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "this is a test");

	nih_free (proxy);
	nih_free (str_array);

	my_teardown (conn);


	/* Check that an output argument of Array type with String members
	 * is marshalled correctly.
	 */
	TEST_FEATURE ("with String Array output argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	str_array = NULL;

	ret = proxy_str_to_str_array (proxy, "this is a test", &str_array);

	TEST_EQ (ret, 0);

	TEST_NE_P (str_array, NULL);
	TEST_ALLOC_PARENT (str_array, proxy);
	TEST_EQ_STR (str_array[0], "this");
	TEST_EQ_STR (str_array[1], "is");
	TEST_EQ_STR (str_array[2], "a");
	TEST_EQ_STR (str_array[3], "test");
	TEST_EQ_P (str_array[4], NULL);

	nih_free (proxy);

	my_teardown (conn);
}


int
main (int   argc,
      char *argv[])
{
	test_method_dispatch ();

	return 0;
}