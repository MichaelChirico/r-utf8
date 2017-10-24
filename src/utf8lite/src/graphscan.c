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
#include <stdio.h>
#include "private/graphbreak.h"
#include "utf8lite.h"

#define GRAPH_BREAK_NONE -1

void utf8lite_graphscan_make(struct utf8lite_graphscan *scan,
			  const struct utf8lite_text *text)
{
	scan->text = *text;
	scan->text_attr = text->attr & ~UTF8LITE_TEXT_SIZE_MASK;
	utf8lite_text_iter_make(&scan->iter, text);
	utf8lite_graphscan_reset(scan);
}


#define NEXT() \
	do { \
		scan->current.attr |= scan->attr; \
		scan->ptr = scan->iter_ptr; \
		scan->code = scan->iter.current; \
		scan->attr = scan->iter.attr; \
		scan->prop = scan->iter_prop; \
		scan->iter_ptr = scan->iter.ptr; \
		if (utf8lite_text_iter_advance(&scan->iter)) { \
			scan->iter_prop = graph_break(scan->iter.current); \
		} else { \
			scan->iter_prop = GRAPH_BREAK_NONE; \
		} \
	} while (0)


void utf8lite_graphscan_reset(struct utf8lite_graphscan *scan)
{
	scan->current.ptr = 0;
	scan->current.attr = 0;
	scan->type = UTF8LITE_GRAPH_NONE;

	utf8lite_text_iter_reset(&scan->iter);
	scan->ptr = scan->iter.ptr;

	if (utf8lite_text_iter_advance(&scan->iter)) {
		scan->code = scan->iter.current;
		scan->attr = scan->iter.attr;
		scan->prop = graph_break(scan->code);

		scan->iter_ptr = scan->iter.ptr;
		if (utf8lite_text_iter_advance(&scan->iter)) {
			scan->iter_prop = graph_break(scan->iter.current);
		} else {
			scan->iter_prop = GRAPH_BREAK_NONE;
		}
	} else {
		scan->code = 0;
		scan->attr = 0;
		scan->prop = GRAPH_BREAK_NONE;
		scan->iter_ptr = NULL;
		scan->iter_prop = GRAPH_BREAK_NONE;
	}
}


int utf8lite_graphscan_advance(struct utf8lite_graphscan *scan)
{
	scan->current.ptr = (uint8_t *)scan->ptr;
	scan->current.attr = 0;

Start:

	// Break at the start and end of text, unless the text is empty.
        if (scan->prop < 0) {
                // GB2: Any + eot
                goto Break;
        }

	switch ((enum graph_break_prop)scan->prop) {
	case GRAPH_BREAK_CR:
		if (scan->iter_prop == GRAPH_BREAK_LF) {
			// Do not break within CRLF
			// GB3: CR * LF
			NEXT();
		}

		// Otherwise break after controls
		// GB4: (Control | CR | LF) +
		NEXT();
		goto Break;

	case GRAPH_BREAK_CONTROL:
	case GRAPH_BREAK_LF:
		// Break after controls
		// GB4: (Newline | LF) +
		NEXT();
		goto Break;

	case GRAPH_BREAK_L:
		NEXT();
		goto L;

	case GRAPH_BREAK_LV:
	case GRAPH_BREAK_V:
		NEXT();
		goto V;

	case GRAPH_BREAK_LVT:
	case GRAPH_BREAK_T:
		NEXT();
		goto T;

	case GRAPH_BREAK_PREPEND:
		NEXT();
		goto Prepend;

	case GRAPH_BREAK_E_BASE:
	case GRAPH_BREAK_E_BASE_GAZ:
		NEXT();
		goto E_Base;

	case GRAPH_BREAK_ZWJ:
		if (scan->iter_prop == GRAPH_BREAK_GLUE_AFTER_ZWJ) {
			// Do not break within emoji zwj sequences
			// GB11: ZWJ * (Glue_After_Zwj | EBG)
			NEXT();
			NEXT();
			goto Break;
		} else if (scan->iter_prop == GRAPH_BREAK_E_BASE_GAZ) {
			// GB11: ZWJ * (Glue_After_Zwj | EBG)
			NEXT();
			NEXT();
			goto E_Base;
		}
		NEXT();
		goto MaybeBreak;

	case GRAPH_BREAK_REGIONAL_INDICATOR:
		NEXT();
		goto Regional_Indicator;

	case GRAPH_BREAK_OTHER:
	case GRAPH_BREAK_E_MODIFIER:
	case GRAPH_BREAK_EXTEND:
	case GRAPH_BREAK_GLUE_AFTER_ZWJ:
	case GRAPH_BREAK_SPACINGMARK:
		NEXT();
		goto MaybeBreak;
	}

	assert(0 && "unhandled word break property");
	return 0;

L:
	// GB6: Do not break Hangul syllable sequences.
	switch (scan->prop) {
	case GRAPH_BREAK_L:
		NEXT();
		goto L;

	case GRAPH_BREAK_V:
	case GRAPH_BREAK_LV:
		NEXT();
		goto V;

	case GRAPH_BREAK_LVT:
		NEXT();
		goto T;

	default:
		goto MaybeBreak;
	}

V:
	// GB7: Do not break Hangul syllable sequences.
	switch (scan->prop) {
	case GRAPH_BREAK_V:
		NEXT();
		goto V;

	case GRAPH_BREAK_T:
		NEXT();
		goto T;

	default:
		goto MaybeBreak;
	}

T:
	// GB8: Do not break Hangul syllable sequences.
	switch (scan->prop) {
	case GRAPH_BREAK_T:
		NEXT();
		goto T;

	default:
		goto MaybeBreak;
	}

Prepend:
	switch (scan->prop) {
	case GRAPH_BREAK_CONTROL:
	case GRAPH_BREAK_CR:
	case GRAPH_BREAK_LF:
		// GB5: break before controls
		goto Break;
	default:
		// GB9b: do not break after Prepend characters.
		goto Start;
	}

E_Base:
	// GB9:  Do not break before extending characters
	// GB10: Do not break within emoji modifier sequences
	while (scan->prop == GRAPH_BREAK_EXTEND) {
		NEXT();
	}
	if (scan->prop == GRAPH_BREAK_E_MODIFIER) {
		NEXT();
	}
	goto MaybeBreak;

Regional_Indicator:
	// Do not break within emoji flag sequences. That is, do not break
	// between regional indicator (RI) symbols if there is an odd number
	// of RI characters before the break point
	if (scan->prop == GRAPH_BREAK_REGIONAL_INDICATOR) {
		// GB12/13: [^RI] RI * RI
		NEXT();
	}
	goto MaybeBreak;

MaybeBreak:
	switch (scan->prop) {
	// GB9: Do not break before extending characters or ZWJ.
	case GRAPH_BREAK_EXTEND:
	case GRAPH_BREAK_ZWJ:
	// GB9a: Do not break before SpacingMark [extended grapheme clusters]
	case GRAPH_BREAK_SPACINGMARK:
		NEXT();
		goto MaybeBreak;

	default:
		goto Break;
	}

Break:
	scan->current.attr |= (size_t)(scan->ptr - scan->current.ptr);
	return (scan->ptr == scan->current.ptr) ? 0 : 1;
}
