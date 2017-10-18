/*
 * Copyright 2017 Patrick O. Perry.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <check.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../src/utf8lite.h"
#include "../src/text.h"
#include "testutil.h"


void setup_text(void)
{
	setup();
}


void teardown_text(void)
{
	teardown();
}


int is_valid_json(const char *str)
{
	struct utf8lite_textscan scan;
	size_t n = strlen(str);
	int err = utf8lite_textscan(&scan, (const uint8_t *)str, n,
				    UTF8LITE_TEXT_UNESCAPE);
	return !err;
}


int is_valid_raw(const char *str)
{
	struct utf8lite_textscan scan;
	size_t n = strlen(str);
	int err = utf8lite_textscan(&scan, (const uint8_t *)str, n, 0);
	return !err;
}


const char *unescape(const struct utf8lite_text *text)
{
	struct utf8lite_text_iter it;
	size_t n = UTF8LITE_TEXT_SIZE(text);
	uint8_t *buf = alloc(n + 1);
	uint8_t *ptr = buf;

	utf8lite_text_iter_make(&it, text);
	while (utf8lite_text_iter_advance(&it)) {
		utf8lite_encode_utf8(it.current, &ptr);
	}
	*ptr = '\0';
	return (const char *)buf;
}


START_TEST(test_valid_text)
{
	ck_assert(is_valid_json("hello world"));
	ck_assert(is_valid_json("escape: \\n\\r\\t"));
	ck_assert(is_valid_json("unicode escape: \\u0034"));
	ck_assert(is_valid_json("surrogate pair: \\uD834\\uDD1E"));
	ck_assert(is_valid_json("B\\u0153uf Bourguignon"));
}
END_TEST


START_TEST(test_invalid_text)
{
	ck_assert(!is_valid_json("invalid utf-8 \xBF"));
	ck_assert(!is_valid_json("invalid utf-8 \xC2\x7F"));
	ck_assert(!is_valid_json("invalid escape \\a"));
	ck_assert(!is_valid_json("missing escape \\"));
	ck_assert(!is_valid_json("ends early \\u007"));
	ck_assert(!is_valid_json("non-hex value \\u0G7F"));
	ck_assert(!is_valid_json("\\uD800 high surrogate"));
	ck_assert(!is_valid_json("\\uDBFF high surrogate"));
	ck_assert(!is_valid_json("\\uD800\\uDC0G invalid hex"));
	ck_assert(!is_valid_json("\\uDC00 low surrogate"));
	ck_assert(!is_valid_json("\\uDFFF low surrogate"));
	ck_assert(!is_valid_json("\\uD84 incomplete"));
	ck_assert(!is_valid_json("\\uD804\\u2603 invalid low"));
}
END_TEST


START_TEST(test_valid_raw)
{
	ck_assert(is_valid_raw("invalid escape \\a"));
	ck_assert(is_valid_raw("missing escape \\"));
	ck_assert(is_valid_raw("ends early \\u007"));
	ck_assert(is_valid_raw("non-hex value \\u0G7F"));
	ck_assert(is_valid_raw("\\uD800 high surrogate"));
	ck_assert(is_valid_raw("\\uDBFF high surrogate"));
	ck_assert(is_valid_raw("\\uD800\\uDC0G invalid hex"));
	ck_assert(is_valid_raw("\\uDC00 low surrogate"));
	ck_assert(is_valid_raw("\\uDFFF low surrogate"));
	ck_assert(is_valid_raw("\\uD84 incomplete"));
	ck_assert(is_valid_raw("\\uD804\\u2603 invalid low"));
	ck_assert(is_valid_raw("B\xC5\x93uf Bourguignon"));
}
END_TEST


START_TEST(test_invalid_raw)
{
	ck_assert(!is_valid_raw("invalid utf-8 \xBF"));
	ck_assert(!is_valid_raw("invalid utf-8 \xC2\x7F"));
}
END_TEST


START_TEST(test_unescape_escape)
{
	ck_assert_str_eq(unescape(JS("\\\\")), "\\");
	ck_assert_str_eq(unescape(JS("\\/")), "/");
	ck_assert_str_eq(unescape(JS("\\\"")), "\"");
	ck_assert_str_eq(unescape(JS("\\b")), "\b");
	ck_assert_str_eq(unescape(JS("\\f")), "\f");
	ck_assert_str_eq(unescape(JS("\\n")), "\n");
	ck_assert_str_eq(unescape(JS("\\r")), "\r");
	ck_assert_str_eq(unescape(JS("\\t")), "\t");
}
END_TEST


START_TEST(test_unescape_raw)
{
	ck_assert_str_eq(unescape(S("\\\\")), "\\\\");
	ck_assert_str_eq(unescape(S("\\/")), "\\/");
	ck_assert_str_eq(unescape(S("\\\"")), "\\\"");
	ck_assert_str_eq(unescape(S("\\b")), "\\b");
	ck_assert_str_eq(unescape(S("\\f")), "\\f");
	ck_assert_str_eq(unescape(S("\\n")), "\\n");
	ck_assert_str_eq(unescape(S("\\r")), "\\r");
	ck_assert_str_eq(unescape(S("\\t")), "\\t");
}
END_TEST


START_TEST(test_unescape_utf16)
{
	ck_assert_str_eq(unescape(JS("\\u2603")), "\xE2\x98\x83");
	ck_assert_str_eq(unescape(JS("\\u0024")), "\x24");
	ck_assert_str_eq(unescape(JS("\\uD801\\uDC37")), "\xF0\x90\x90\xB7");
	ck_assert_str_eq(unescape(JS("\\uD852\\uDF62")), "\xF0\xA4\xAD\xA2");

}
END_TEST


static struct utf8lite_text_iter iter;

static void start (const struct utf8lite_text *text)
{
	utf8lite_text_iter_make(&iter, text);
}

static int next(void)
{
	if (utf8lite_text_iter_advance(&iter)) {
		return (int)iter.current;
	} else {
		return -1;
	}
}


static int has_next()
{
	return utf8lite_text_iter_can_advance(&iter);
}


static int prev(void)
{
	if (utf8lite_text_iter_retreat(&iter)) {
		return (int)iter.current;
	} else {
		return -1;
	}
}


static int has_prev()
{
	return utf8lite_text_iter_can_retreat(&iter);
}


START_TEST(test_iter_empty)
{
	start(JS(""));
	ck_assert(!has_next());
	ck_assert(!has_prev());

	ck_assert_int_eq(next(), -1);
	ck_assert(!has_next());
	ck_assert(!has_prev());

	ck_assert_int_eq(prev(), -1);
	ck_assert(!has_next());
	ck_assert(!has_prev());

	start(JS(""));
	ck_assert_int_eq(prev(), -1);
	ck_assert_int_eq(next(), -1);

	start(JS(""));
	ck_assert_int_eq(next(), -1);
	ck_assert_int_eq(next(), -1);
	ck_assert_int_eq(prev(), -1);
	ck_assert_int_eq(prev(), -1);

	start(JS(""));
	ck_assert_int_eq(prev(), -1);
	ck_assert_int_eq(prev(), -1);
	ck_assert_int_eq(next(), -1);
	ck_assert_int_eq(next(), -1);
}
END_TEST


START_TEST(test_iter_single)
{
	start(JS("a"));
	ck_assert(has_next());
	ck_assert(!has_prev());

	ck_assert_int_eq(next(), 'a');
	ck_assert(!has_next());
	ck_assert(!has_prev());

	ck_assert_int_eq(prev(), -1);
	ck_assert(has_next());
	ck_assert(!has_prev());

	ck_assert_int_eq(next(), 'a');
	ck_assert(!has_next());
	ck_assert(!has_prev());

	ck_assert_int_eq(next(), -1);
	ck_assert(!has_next());
	ck_assert(has_prev());

	ck_assert_int_eq(prev(), 'a');
	ck_assert(!has_next());
	ck_assert(!has_prev());

	ck_assert_int_eq(prev(), -1);
	ck_assert(has_next());
	ck_assert(!has_prev());
}
END_TEST


START_TEST(test_iter_ascii)
{
	start(JS("abba zabba"));

	// forward
	ck_assert_int_eq(prev(), -1);
	ck_assert_int_eq(next(), 'a');
	ck_assert_int_eq(next(), 'b');
	ck_assert_int_eq(next(), 'b');
	ck_assert_int_eq(next(), 'a');
	ck_assert_int_eq(next(), ' ');
	ck_assert_int_eq(next(), 'z');
	ck_assert_int_eq(next(), 'a');
	ck_assert_int_eq(next(), 'b');
	ck_assert_int_eq(next(), 'b');
	ck_assert_int_eq(next(), 'a');
	ck_assert_int_eq(next(), -1);
	ck_assert_int_eq(next(), -1);

	// backward
	ck_assert_int_eq(prev(), 'a');
	ck_assert_int_eq(prev(), 'b');
	ck_assert_int_eq(prev(), 'b');
	ck_assert_int_eq(prev(), 'a');
	ck_assert_int_eq(prev(), 'z');
	ck_assert_int_eq(prev(), ' ');
	ck_assert_int_eq(prev(), 'a');
	ck_assert_int_eq(prev(), 'b');
	ck_assert_int_eq(prev(), 'b');
	ck_assert_int_eq(prev(), 'a');
	ck_assert_int_eq(prev(), -1);
	ck_assert_int_eq(prev(), -1);
}
END_TEST


START_TEST(test_iter_bidi1)
{
	start(JS("abc"));
	ck_assert_int_eq(next(), 'a');
	ck_assert_int_eq(next(), 'b');
	ck_assert_int_eq(prev(), 'a');
	ck_assert_int_eq(next(), 'b');
	ck_assert_int_eq(prev(), 'a');
	ck_assert_int_eq(next(), 'b');
	ck_assert_int_eq(next(), 'c');
	ck_assert_int_eq(next(), -1);
}
END_TEST


START_TEST(test_iter_bidi2)
{
	start(JS("ab"));
	ck_assert_int_eq(next(), 'a');
	ck_assert_int_eq(prev(), -1);
	ck_assert_int_eq(next(), 'a');
	ck_assert_int_eq(next(), 'b');
	ck_assert_int_eq(next(), -1);
	ck_assert_int_eq(prev(), 'b');
	ck_assert_int_eq(prev(), 'a');
	ck_assert_int_eq(prev(), -1);
	ck_assert_int_eq(next(), 'a');
}
END_TEST


START_TEST(test_iter_utf8)
{
	start(JS("\xE2\x98\x83 \xF0\x9F\x99\x82 \xC2\xA7\xC2\xA4"));

	ck_assert_int_eq(next(), 0x2603); // \xE2\x98\x83
	ck_assert_int_eq(next(), ' ');
	ck_assert_int_eq(next(), 0x1F642); // \xF0\x9F\x99\x82
	ck_assert_int_eq(next(), ' ');
	ck_assert_int_eq(next(), 0xA7); // \xC2\xA7
	ck_assert_int_eq(next(), 0xA4); // \xC2\xA4
	ck_assert_int_eq(next(), -1);

	ck_assert_int_eq(prev(), 0xA4);
	ck_assert_int_eq(prev(), 0xA7);
	ck_assert_int_eq(prev(), ' ');
	ck_assert_int_eq(prev(), 0x1F642);
	ck_assert_int_eq(prev(), ' ');
	ck_assert_int_eq(prev(), 0x2603);
	ck_assert_int_eq(prev(), -1);
}
END_TEST


START_TEST(test_iter_escape)
{
	start(JS("nn\\\\\\n\\nn\\\\n"));

	ck_assert_int_eq(next(), 'n');
	ck_assert_int_eq(next(), 'n');
	ck_assert_int_eq(next(), '\\');
	ck_assert_int_eq(next(), '\n');
	ck_assert_int_eq(next(), '\n');
	ck_assert_int_eq(next(), 'n');
	ck_assert_int_eq(next(), '\\');
	ck_assert_int_eq(next(), 'n');
	ck_assert_int_eq(next(), -1);

	ck_assert_int_eq(prev(), 'n');
	ck_assert_int_eq(prev(), '\\');
	ck_assert_int_eq(prev(), 'n');
	ck_assert_int_eq(prev(), '\n');
	ck_assert_int_eq(prev(), '\n');
	ck_assert_int_eq(prev(), '\\');
	ck_assert_int_eq(prev(), 'n');
	ck_assert_int_eq(prev(), 'n');
	ck_assert_int_eq(prev(), -1);
}
END_TEST


START_TEST(test_iter_uescape)
{
	start(JS("\\u2603 \\uD83D\\uDE42 \\u00a7\\u00a4"));

	ck_assert_int_eq(next(), 0x2603);
	ck_assert_int_eq(next(), ' ');
	ck_assert_int_eq(next(), 0x1F642);
	ck_assert_int_eq(next(), ' ');
	ck_assert_int_eq(next(), 0xA7);
	ck_assert_int_eq(next(), 0xA4);
	ck_assert_int_eq(next(), -1);

	ck_assert_int_eq(prev(), 0xA4);
	ck_assert_int_eq(prev(), 0xA7);
	ck_assert_int_eq(prev(), ' ');
	ck_assert_int_eq(prev(), 0x1F642);
	ck_assert_int_eq(prev(), ' ');
	ck_assert_int_eq(prev(), 0x2603);
	ck_assert_int_eq(prev(), -1);

}
END_TEST

struct type {
	const char *string;
	uint32_t value;
	size_t attr;
};

#define ESC UTF8LITE_TEXT_ESC_BIT
#define UTF8 UTF8LITE_TEXT_UTF8_BIT

START_TEST(test_iter_random)
{
	const struct type types[] = {
		// escaped
		{ "\\\"", '\"', ESC },
		{ "\\\\", '\\', ESC },
		{ "\\/", '/', ESC },
		{ "\\b", '\b', ESC },
		{ "\\f", '\f', ESC },
		{ "\\n", '\n', ESC },
		{ "\\r", '\r', ESC },
		{ "\\t", '\t', ESC },

		// not escaped
		{ "\"", '\"', 0 },
		{ "/", '/', 0 },
		{ "b", 'b', 0 },
		{ "f", 'f', 0 },
		{ "n", 'n', 0 },
		{ "r", 'r', 0 },
		{ "t", 't', 0 },
		{ "u", 'u', 0 },

		// 2-byte UTF-8
		{ "\xC2\xA7", 0xA7, UTF8 },
		{ "\\u00a7", 0xA7, ESC|UTF8 },

		// 3-byte UTF-8
		{ "\xE2\x98\x83", 0x2603, UTF8 },
		{ "\\u2603", 0x2603, ESC|UTF8 },

		// 4-byte UTF-8
		{ "\xE2\x98\x83", 0x2603, UTF8 },
		{ "\\uD83D\\uDE42", 0x1F642, ESC|UTF8 }
	};
	unsigned ntype = sizeof(types) / sizeof(types[0]);

	struct utf8lite_textscan scan;
	struct utf8lite_text text;
	struct utf8lite_text_iter iter;
	uint8_t buffer[1024 * 12];
	int toks[1024];
	int ntok_max = 1024 - 1;
	const uint8_t *ptr;
	int ntok;
	size_t len, size, attr;
	int i, id;

	srand(_i);

	ntok = (337 * (_i))  % ntok_max;
	size = 0;
	attr = 0;
	for (i = 0; i < ntok; i++) {
		id = rand() % ntype;
		toks[i] = id;

		len = strlen(types[id].string);
		memcpy(buffer + size, types[id].string, len);
		size += len;
		attr |= types[id].attr;
	}

	ptr = buffer;
	ck_assert(!utf8lite_textscan(&scan, ptr, size, UTF8LITE_TEXT_UNESCAPE));
	text = scan.text;
	ck_assert(UTF8LITE_TEXT_SIZE(&text) == size);
	ck_assert(UTF8LITE_TEXT_BITS(&text) == attr);

	utf8lite_text_iter_make(&iter, &text);
	ck_assert(!utf8lite_text_iter_retreat(&iter));
	ck_assert(iter.ptr == ptr);

	// forward iteration
	for (i = 0; i < ntok; i++) {
		ck_assert(utf8lite_text_iter_can_advance(&iter));
		ck_assert(utf8lite_text_iter_advance(&iter));
		if (i > 0) {
			ck_assert(utf8lite_text_iter_can_retreat(&iter));
		}

		id = toks[i];
		ck_assert_uint_eq(iter.current, types[id].value);
		ck_assert_uint_eq(iter.attr, types[id].attr);

		len = strlen(types[id].string);
		ptr += len;
		ck_assert(iter.ptr == ptr);
	}

	ck_assert(!utf8lite_text_iter_can_advance(&iter));
	ck_assert(!utf8lite_text_iter_advance(&iter));
	ck_assert(!utf8lite_text_iter_advance(&iter));
	ck_assert(iter.ptr == ptr);

	// reverse iteration
	while (i-- > 0) {
		ck_assert(utf8lite_text_iter_can_retreat(&iter));
		ck_assert(utf8lite_text_iter_retreat(&iter));
		if (i != ntok - 1) {
			ck_assert(utf8lite_text_iter_can_advance(&iter));
		}

		id = toks[i];
		ck_assert_uint_eq(iter.current, types[id].value);
		ck_assert_uint_eq(iter.attr, types[id].attr);

		ck_assert(iter.ptr == ptr);
		len = strlen(types[id].string);
		ptr -= len;
	}

	ck_assert(!utf8lite_text_iter_can_retreat(&iter));
	ck_assert(!utf8lite_text_iter_retreat(&iter));
	ck_assert(!utf8lite_text_iter_retreat(&iter));
	ck_assert(iter.ptr == ptr);
}
END_TEST


Suite *text_suite(void)
{
	Suite *s;
	TCase *tc;

	s = suite_create("text");

	tc = tcase_create("validation");
        tcase_add_checked_fixture(tc, setup_text, teardown_text);
	tcase_add_test(tc, test_valid_text);
	tcase_add_test(tc, test_invalid_text);
	tcase_add_test(tc, test_valid_raw);
	tcase_add_test(tc, test_invalid_raw);
	suite_add_tcase(s, tc);

	tc = tcase_create("unescaping");
        tcase_add_checked_fixture(tc, setup_text, teardown_text);
	tcase_add_test(tc, test_unescape_escape);
	tcase_add_test(tc, test_unescape_raw);
	tcase_add_test(tc, test_unescape_utf16);
	suite_add_tcase(s, tc);

	tc = tcase_create("iteration");
        tcase_add_checked_fixture(tc, setup_text, teardown_text);
	tcase_add_test(tc, test_iter_empty);
	tcase_add_test(tc, test_iter_single);
	tcase_add_test(tc, test_iter_ascii);
	tcase_add_test(tc, test_iter_bidi1);
	tcase_add_test(tc, test_iter_bidi2);
	tcase_add_test(tc, test_iter_utf8);
	tcase_add_test(tc, test_iter_escape);
	tcase_add_test(tc, test_iter_uescape);
	tcase_add_loop_test(tc, test_iter_random, 0, 50);
	suite_add_tcase(s, tc);

	return s;
}


int main(void)
{
	int nfail;
	Suite *s;
	SRunner *sr;

	s = text_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	nfail = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (nfail == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
