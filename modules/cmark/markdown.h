/**************************************************************************/
/*  markdown.h                                                            */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef MARKDOWN_H
#define MARKDOWN_H

#include "core/object/ref_counted.h"
#include "core/variant/type_info.h"

struct cmark_node;
class RichTextLabel;

class Markdown : public RefCounted {
	GDCLASS(Markdown, RefCounted);

public:
	enum OptionsMask {
		KEEP_TEXT = 1 << 0,
		SOFT_BREAK_AS_HARD_BREAK = 1 << 1,
		PARSE_FOOTNOTES = 1 << 2,
	};

	constexpr static const BitField<OptionsMask> DEFAULT_OPTIONS = KEEP_TEXT | PARSE_FOOTNOTES;

protected:
	BitField<OptionsMask> options;

	cmark_node *root = nullptr;

	String text;

	const char *buffer = nullptr;
	size_t len = -1;

	static void _bind_methods();

	String _parse_literal(char *p_literal) const;

	void _append_block_quote(RichTextLabel *p_rich_text_label, bool p_is_entry) const;
	void _append_code_block(RichTextLabel *p_rich_text_label, String p_content) const;
	void _append_heading(RichTextLabel *p_rich_text_label, int p_level, bool p_is_entry) const;
	void _append_indent(RichTextLabel *p_rich_text_label, int p_level = 1) const;
	void _append_list_item(RichTextLabel *p_rich_text_label, int p_index, int p_offset, bool p_is_entry, bool p_is_tight, bool p_is_ordered) const;
	void _append_thematic_break(RichTextLabel *p_rich_text_label) const;

	int _find_index(cmark_node *p_parent, cmark_node *p_child) const;
	bool _has_lower_sibling_of_type(cmark_node *p_cur, LocalVector<int> p_sibling_types, bool p_immediate = false) const;
	bool _has_parent_of_type(cmark_node *p_cur, LocalVector<int> p_parent_types, bool p_immediate = false) const;
	bool _has_upper_sibling_of_type(cmark_node *p_cur, LocalVector<int> p_sibling_types, bool p_immediate = false) const;
	bool _is_rtl(RichTextLabel *p_rich_text_label) const;
	String _prefix_ordered_list_item(RichTextLabel *p_rich_text_label, int p_index) const;

	char *_to_html() const;
	char *_to_xml() const;

    int _get_cmark_options() const;

public:
	static Ref<Markdown> parse_string(const String &p_markdown, BitField<OptionsMask> p_options = DEFAULT_OPTIONS);

	Ref<Markdown> parse(const String &p_markdown);

	void set_options(BitField<OptionsMask> p_options);
	BitField<OptionsMask> get_options() const;

	String get_parsed_text() const;

	RichTextLabel *render_into(RichTextLabel *p_rich_text_label) const;

	String to_html() const;
	String to_xml() const;

	Markdown(BitField<OptionsMask> p_options = DEFAULT_OPTIONS) {
		options = p_options;
	}
	~Markdown();
};

VARIANT_BITFIELD_CAST(Markdown::OptionsMask);

#endif // MARKDOWN_H