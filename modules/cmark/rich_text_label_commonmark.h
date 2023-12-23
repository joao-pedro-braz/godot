/*************************************************************************/
/*  rich_text_label_commonmark.h                                         */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef RICH_TEXT_LABEL_COMMONMARK_H
#define RICH_TEXT_LABEL_COMMONMARK_H

#include "scene/gui/rich_text_effect.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/scroll_bar.h"
#include "scene/resources/text_paragraph.h"

class RichTextLabelCommonmark : public RichTextLabel {
	GDCLASS(RichTextLabelCommonmark, RichTextLabel);
	friend RichTextLabel;

	bool use_commonmark = false;
	String commonmark;

protected:
	static void _bind_methods();

public:
	void _notification(int p_what);
	virtual String get_text() const override;
	virtual void set_text(const String &p_commonmark) override;
	bool is_using_commonmark() const;
	void set_use_commonmark(bool p_enable);
	Error append_commonmark(const String &p_commonmark);
	Error parse_commonmark(const String &p_commonmark);
	String commonmark_to_html();

	RichTextLabelCommonmark() {}
	~RichTextLabelCommonmark() {}
};

#endif // RICH_TEXT_LABEL_COMMONMARK_H
