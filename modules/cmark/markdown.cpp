/**************************************************************************/
/*  markdown.cpp                                                          */
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

#include "core/error/error_macros.h"
#include "core/io/resource_loader.h"
#include "core/math/color.h"
#include "core/math/math_defs.h"
#include "core/math/rect2.h"
#include "core/math/rect2i.h"
#include "core/math/vector2.h"
#include "core/math/vector2i.h"
#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "core/os/memory.h"
#include "core/string/print_string.h"
#include "core/string/ustring.h"
#include "core/templates/local_vector.h"
#include "core/typedefs.h"
#include "scene/gui/rich_text_label.h"

#include "cmark-gfm_export.h"
#include "config.h"
#include "thirdparty/cmark-gfm/src/cmark-gfm.h"
#include "thirdparty/cmark-gfm/src/node.h"

#include "markdown.h"

#define COLOR(m_color_name) Color::from_string(m_color_name, Color::from_rgbe9995(0))

String Markdown::_parse_literal(char *literal) const {
	String parsed_literal = String::utf8(literal);
	memfree(literal);
	return parsed_literal;
}

void Markdown::_append_block_quote(RichTextLabel *p_rich_text_label, bool p_is_entry) const {
	if (p_is_entry) {
		p_rich_text_label->push_table(3, INLINE_ALIGNMENT_CENTER);
		p_rich_text_label->push_cell();
		p_rich_text_label->set_cell_row_background_color(COLOR("gray"), Color::hex(0x00000000));
		p_rich_text_label->pop();
		p_rich_text_label->push_cell();
		_append_indent(p_rich_text_label);
		p_rich_text_label->pop();
		p_rich_text_label->push_cell();
	} else {
		p_rich_text_label->pop();
		p_rich_text_label->pop();
		p_rich_text_label->add_newline();
	}
}

void Markdown::_append_code_block(RichTextLabel *p_rich_text_label, String p_content) const {
	p_rich_text_label->push_table(3, INLINE_ALIGNMENT_CENTER);
	p_rich_text_label->push_cell();
	p_rich_text_label->set_cell_row_background_color(COLOR("gray"), COLOR("gray"));
	_append_indent(p_rich_text_label);
	p_rich_text_label->pop();
	p_rich_text_label->push_cell();
	p_rich_text_label->set_cell_row_background_color(COLOR("gray"), COLOR("gray"));
	p_rich_text_label->push_mono();
	p_rich_text_label->add_text(p_content);
	p_rich_text_label->pop();
	p_rich_text_label->pop();
	p_rich_text_label->push_cell();
	p_rich_text_label->set_cell_row_background_color(COLOR("gray"), COLOR("gray"));
	_append_indent(p_rich_text_label);
	p_rich_text_label->pop();
	p_rich_text_label->pop();
	p_rich_text_label->add_newline();
}

void Markdown::_append_heading(RichTextLabel *p_rich_text_label, int p_level, bool p_is_entry) const {
	if (p_is_entry) {
		int font_size = -1;
		switch (p_level) {
			case 1: {
				font_size = p_rich_text_label->theme_cache.first_heading_font_size;
			} break;
			case 2: {
				font_size = p_rich_text_label->theme_cache.second_heading_font_size;
			} break;
			case 3: {
				font_size = p_rich_text_label->theme_cache.third_heading_font_size;
			} break;
			case 4: {
				font_size = p_rich_text_label->theme_cache.fourth_heading_font_size;
			} break;
			case 5: {
				font_size = p_rich_text_label->theme_cache.fifth_heading_font_size;
			} break;
			case 6: {
				font_size = p_rich_text_label->theme_cache.sixth_heading_font_size;
			} break;
			default: {
				ERR_PRINT("Markdown parser bug: Unknown heading level");
				break;
			}
		}
		if (p_level < 3) {
			p_rich_text_label->push_underline();
		}
		p_rich_text_label->push_font(p_rich_text_label->theme_cache.heading_font, font_size);
	} else {
		p_rich_text_label->pop();
		if (p_level < 3) {
			p_rich_text_label->pop();
			p_rich_text_label->add_newline();
		}
		p_rich_text_label->add_newline();
	}
}

void Markdown::_append_indent(RichTextLabel *p_rich_text_label, int p_level) const {
	for (int i = 0; i < p_rich_text_label->tab_size * p_level; i++) {
		p_rich_text_label->add_text(" ");
	}
}

void Markdown::_append_list_item(RichTextLabel *p_rich_text_label, int p_index, int p_offset, bool p_is_entry, bool p_is_tight, bool p_is_ordered) const {
	if (p_is_entry) {
		_append_indent(p_rich_text_label, p_offset + 1);

		String segment = p_is_ordered ? _prefix_ordered_list_item(p_rich_text_label, p_index) : U"•";
		if (_is_rtl(p_rich_text_label)) {
			_append_indent(p_rich_text_label);
			p_rich_text_label->add_text(segment);
		} else {
			p_rich_text_label->add_text(segment);
			_append_indent(p_rich_text_label);
		}
	} else {
		if (!p_is_tight) {
			p_rich_text_label->add_newline();
		}
	}
}

void Markdown::_append_thematic_break(RichTextLabel *p_rich_text_label) const {
	p_rich_text_label->add_newline();
	p_rich_text_label->push_paragraph(HORIZONTAL_ALIGNMENT_FILL);
	p_rich_text_label->push_table(1);
	p_rich_text_label->push_cell();
	p_rich_text_label->set_cell_row_background_color(COLOR("gray"), Color::hex(0x00000000));
	p_rich_text_label->set_cell_size_override(Size2i(1, 1), Size2i(1, 1));
	p_rich_text_label->add_text(" ");
	p_rich_text_label->pop();
	p_rich_text_label->pop();
	p_rich_text_label->pop();
	p_rich_text_label->add_newline();
}

int Markdown::_find_index(cmark_node *p_parent, cmark_node *p_child) const {
	int index = 1;
	cmark_node *child = cmark_node_first_child(p_parent);
	while (child != p_child && cmark_node_get_type(child) == cmark_node_get_type(p_child)) {
		child = cmark_node_next(child);
		if (child == nullptr) {
			break;
		}
		index++;
	}
	return index;
}

bool Markdown::_has_lower_sibling_of_type(cmark_node *p_cur, LocalVector<int> p_sibling_types, bool p_immediate) const {
	bool is_contained = false;
	cmark_node *sibling = cmark_node_next(p_cur);
	while (sibling != nullptr) {
		cmark_node_type sibling_type = cmark_node_get_type(sibling);
		if (p_sibling_types.find(sibling_type) != -1) {
			is_contained = true;
			break;
		}
		if (p_immediate) {
			break;
		}
		sibling = cmark_node_next(sibling);
	}

	return is_contained;
}

bool Markdown::_has_parent_of_type(cmark_node *p_cur, LocalVector<int> p_parent_types, bool p_immediate) const {
	bool is_contained = false;
	cmark_node *parent = cmark_node_parent(p_cur);
	while (parent != nullptr) {
		cmark_node_type parent_type = cmark_node_get_type(parent);
		if (p_parent_types.find(parent_type) != -1) {
			is_contained = true;
			break;
		}
		if (p_immediate) {
			break;
		}
		parent = cmark_node_parent(parent);
	}

	return is_contained;
}

bool Markdown::_has_upper_sibling_of_type(cmark_node *p_cur, LocalVector<int> p_sibling_types, bool p_immediate) const {
	bool is_contained = false;
	cmark_node *sibling = cmark_node_previous(p_cur);
	while (sibling != nullptr) {
		cmark_node_type sibling_type = cmark_node_get_type(sibling);
		if (p_sibling_types.find(sibling_type) != -1) {
			is_contained = true;
			break;
		}
		if (p_immediate) {
			break;
		}
		sibling = cmark_node_previous(sibling);
	}

	return is_contained;
}

bool Markdown::_is_rtl(RichTextLabel *p_rich_text_label) const {
	int line_idx = MIN(p_rich_text_label->main->lines.size() - 1, p_rich_text_label->current->line);
	return p_rich_text_label->main->lines[line_idx].text_buf->get_direction() == TextServer::DIRECTION_RTL;
}

// TODO: Allow user to choose between numbers and roman.
String Markdown::_prefix_ordered_list_item(RichTextLabel *p_rich_text_label, int p_index) const {
	String index_s = itos(p_index);
	if (p_rich_text_label->is_localizing_numeral_system()) {
		index_s = TS->format_number(index_s, p_rich_text_label->_find_language(p_rich_text_label->current));
	}
	return _is_rtl(p_rich_text_label) ? "." + index_s : index_s + ".";
}

char *Markdown::_to_html() const {
	return cmark_render_html(root, _get_cmark_options(), nullptr);
}

char *Markdown::_to_xml() const {
	return cmark_render_xml(root, _get_cmark_options());
}

int Markdown::_get_cmark_options() const {
	int _options = CMARK_OPT_DEFAULT;
	if (options.has_flag(OptionsMask::PARSE_FOOTNOTES)) {
		_options |= CMARK_OPT_FOOTNOTES;
	}
	return _options;
}

Ref<Markdown> Markdown::parse(const String &p_markdown) {
	if (p_markdown.is_empty()) {
		return this;
	}

	Vector<uint8_t> string_bytes = p_markdown.to_utf8_buffer();
	buffer = (const char *)string_bytes.ptr();
	len = string_bytes.size();
	root = cmark_parse_document(buffer, len, _get_cmark_options());

	if (options.has_flag(OptionsMask::KEEP_TEXT)) {
		text = p_markdown;
	}

	return this;
}

Ref<Markdown> Markdown::parse_string(const String &p_markdown, BitField<OptionsMask> p_options) {
	Ref<Markdown> mark;
	mark.instantiate();
	mark->options = p_options;
	return mark->parse(p_markdown);
}

void Markdown::set_options(BitField<OptionsMask> p_options) {
	options = p_options;
}

BitField<Markdown::OptionsMask> Markdown::get_options() const {
	return options;
}

String Markdown::get_parsed_text() const {
	return text;
}

RichTextLabel *Markdown::render_into(RichTextLabel *p_rich_text_label) const {
	// Our implementation takes advantage of the existing RichTextLabel logic to produce valid bbcode.
	// Ideally we would be able to do that without instantiating a Control node (RichTextLabel itself),
	// but because the Rich Text rendering logic is part of the RichTextLabel that's not currently possible.
	p_rich_text_label->set_process_internal(false);

	LocalVector<String> footnotes;
	int rendered_footnotes = 0;
	cmark_event_type ev_type = CMARK_EVENT_NONE;
	cmark_iter *iter = cmark_iter_new(root);
	if (iter != nullptr) {
		while (ev_type != CMARK_EVENT_DONE && (ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
			cmark_node *cur = cmark_iter_get_node(iter);
			cmark_node_type node_type = cmark_node_get_type(cur);
			String content = String::utf8(cmark_node_get_literal(cur));
			switch (node_type) {
				// Leaf Nodes (No Exit Event).
				case CMARK_NODE_HTML_BLOCK: {
					p_rich_text_label->add_text(content);
				} break;
				case CMARK_NODE_THEMATIC_BREAK: {
					_append_thematic_break(p_rich_text_label);
				} break;
				case CMARK_NODE_CODE_BLOCK: {
					_append_code_block(p_rich_text_label, content);
				} break;
				case CMARK_NODE_TEXT: {
					p_rich_text_label->add_text(content);
				} break;
				case CMARK_NODE_SOFTBREAK: {
					if (options.has_flag(OptionsMask::SOFT_BREAK_AS_HARD_BREAK)) {
						p_rich_text_label->add_newline();
					} else {
						p_rich_text_label->add_text(" ");
					}
				} break;
				case CMARK_NODE_LINEBREAK: {
					p_rich_text_label->add_newline();
				} break;
				case CMARK_NODE_CODE: {
					p_rich_text_label->push_mono();
					p_rich_text_label->add_text(" ");
					p_rich_text_label->push_bgcolor(COLOR("gray"));
					p_rich_text_label->add_text(content);
					p_rich_text_label->pop();
					p_rich_text_label->add_text(" ");
					p_rich_text_label->pop();
				} break;
				case CMARK_NODE_HTML_INLINE: {
				} break;

				// Non-leaf Nodes.
				case CMARK_NODE_CUSTOM_BLOCK:
					break;
				case CMARK_NODE_DOCUMENT:
					break;
				case CMARK_NODE_CUSTOM_INLINE:
					break;
				case CMARK_NODE_PARAGRAPH: {
					if (ev_type == CMARK_EVENT_EXIT) {
						p_rich_text_label->add_newline();
					} else {
						bool has_valid_parent = !_has_parent_of_type(cur, { CMARK_NODE_LIST, CMARK_NODE_ITEM, CMARK_NODE_BLOCK_QUOTE, CMARK_NODE_FOOTNOTE_DEFINITION }, true);
						bool has_valid_sibling = _has_upper_sibling_of_type(cur, { CMARK_NODE_PARAGRAPH }, true);
						if (has_valid_parent && has_valid_sibling) {
							p_rich_text_label->add_newline();
						}
					}
				} break;
				case CMARK_NODE_BLOCK_QUOTE: {
					if (ev_type == CMARK_EVENT_ENTER && _has_parent_of_type(cur, { CMARK_NODE_BLOCK_QUOTE })) {
						p_rich_text_label->add_newline();
					}
					_append_block_quote(p_rich_text_label, ev_type == CMARK_EVENT_ENTER);
				} break;
				case CMARK_NODE_LIST: {
					// Avoid adding newlines to nested lists
					if (cmark_node_previous(cur) != nullptr && !_has_parent_of_type(cur, { CMARK_NODE_LIST, CMARK_NODE_ITEM })) {
						p_rich_text_label->add_newline();
					}
				} break;
				case CMARK_NODE_ITEM: {
					cmark_node *parent = cmark_node_parent(cur);
					cmark_list_type _cmark_list_type = cmark_node_get_list_type(parent);

					bool is_tight = cmark_node_get_list_tight(parent);
					bool is_entry = ev_type == CMARK_EVENT_ENTER;
					bool is_ordered = _cmark_list_type == CMARK_ORDERED_LIST;
					int index = is_ordered ? _find_index(parent, cur) : -1;
					int offset = cur->as.list.marker_offset;

					_append_list_item(p_rich_text_label, index, offset, is_entry, is_tight, is_ordered);
				} break;
				case CMARK_NODE_HEADING: {
					if (ev_type == CMARK_EVENT_ENTER) {
						if (!_has_upper_sibling_of_type(cur, { CMARK_NODE_HEADING }) && !_has_parent_of_type(cur, { CMARK_NODE_BLOCK_QUOTE })) {
							p_rich_text_label->add_newline();
						}
					}

					_append_heading(p_rich_text_label, cur->as.heading.level, ev_type == CMARK_EVENT_ENTER);
				} break;
				case CMARK_NODE_EMPH: {
					if (ev_type == CMARK_EVENT_ENTER) {
						p_rich_text_label->push_italics();
					} else {
						p_rich_text_label->pop();
					}
				} break;
				case CMARK_NODE_STRONG: {
					if (ev_type == CMARK_EVENT_ENTER) {
						p_rich_text_label->push_bold();
					} else {
						p_rich_text_label->pop();
					}
				} break;
				case CMARK_NODE_LINK: {
					if (ev_type == CMARK_EVENT_ENTER) {
						String url = String::utf8(cmark_node_get_url(cur));
						String title = String::utf8(cmark_node_get_title(cur));
						p_rich_text_label->push_hint(title);
						p_rich_text_label->push_meta(url);
					} else {
						p_rich_text_label->pop();
						p_rich_text_label->pop();
					}
				} break;
				case CMARK_NODE_IMAGE: {
					if (ev_type == CMARK_EVENT_ENTER) {
						String url = String::utf8(cmark_node_get_url(cur));
						String title = String::utf8(cmark_node_get_title(cur));
						Ref<Texture2D> texture = ResourceLoader::load(url, "Texture2D");
						if (texture.is_valid()) {
							p_rich_text_label->add_image(texture, 0, 0, Color(1.0, 1.0, 1.0, 1.0), INLINE_ALIGNMENT_BOTTOM, Rect2(), Variant(), false, title);
						} else if (!title.is_empty()) {
							p_rich_text_label->add_text(title);
						}

						cmark_node *child = cmark_node_first_child(cur);
						if (child != nullptr && cmark_node_get_type(child) == CMARK_NODE_TEXT) {
							// cmark parses alt text as a text node, which for us results in text placed besides the image.
							// To account for that, we check if the sibling of this image node is said text node and if so,
							// we skip it.
							ev_type = cmark_iter_next(iter);
						}
					}
				} break;
				case CMARK_NODE_FOOTNOTE_REFERENCE: {
					if (ev_type == CMARK_EVENT_ENTER) {
						String key = "#footnotes:" + content;
						footnotes.push_back(key);
						p_rich_text_label->push_font_size(11);
						p_rich_text_label->push_table(1);
						p_rich_text_label->push_cell();
						p_rich_text_label->push_meta(key);
						p_rich_text_label->add_text("[" + itos(footnotes.size()) + "]");
					} else {
						p_rich_text_label->pop();
						p_rich_text_label->pop();
						p_rich_text_label->pop();
						p_rich_text_label->pop();
					}
				} break;
				case CMARK_NODE_FOOTNOTE_DEFINITION: {
					if (ev_type == CMARK_EVENT_ENTER) {
						if (rendered_footnotes == 0) {
							// If first footnote, render a thematic break for a better separation between content and footer
							_append_thematic_break(p_rich_text_label);
						} else {
							p_rich_text_label->add_newline();
						}
						String key = footnotes[rendered_footnotes];
						rendered_footnotes++;
						_append_list_item(p_rich_text_label, rendered_footnotes, 0, true, true, true);
					} else {
						_append_list_item(p_rich_text_label, rendered_footnotes, 0, false, true, true);
					}
				} break;
				default: {
					String node_type_string = String::utf8(cmark_node_get_type_string(cur));
					ERR_PRINT("Markdown Parser Bug: Unhandled CMark Node Type: " + node_type_string);
				}
			}
		}

		cmark_iter_free(iter);
	}

	p_rich_text_label->set_process_internal(true);

	return p_rich_text_label;
}

String Markdown::to_html() const {
	char *literal = _to_html();
	return _parse_literal(literal);
}

String Markdown::to_xml() const {
	char *literal = _to_xml();
	return _parse_literal(literal);
}

void Markdown::_bind_methods() {
	ClassDB::bind_static_method("Markdown", D_METHOD("parse_string", "markdown", "options"), &Markdown::parse_string, DEFVAL(0));

	ClassDB::bind_method(D_METHOD("set_options", "options"), &Markdown::set_options);
	ClassDB::bind_method(D_METHOD("get_options"), &Markdown::get_options);

	ClassDB::bind_method(D_METHOD("get_parsed_text"), &Markdown::get_parsed_text);

	ClassDB::bind_method(D_METHOD("render_into_rich_text_label", "rich_text_label"), &Markdown::render_into);
	ClassDB::bind_method(D_METHOD("to_html"), &Markdown::to_html);
	ClassDB::bind_method(D_METHOD("to_xml"), &Markdown::to_xml);
}

Markdown::~Markdown() {
	if (root != nullptr) {
		cmark_node_free(root);
	}
}