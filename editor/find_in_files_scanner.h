/**************************************************************************/
/*  find_in_files_scanner.h                                               */
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

#ifndef FIND_IN_FILES_SCANNER_H
#define FIND_IN_FILES_SCANNER_H

#include "core/io/file_access.h"
#include "find_in_files.h"

class FindInFilesScanner {
public:
	struct ScanMatch {
		String file_path; // Allows us to specify different paths for builtin scripts
		String display_text;
		int line_number;
		int begin;
		int end;
		String line;
	};
	struct ScanLocation {
		int line_number = 0;
		int begin = 0;
		int end = 0;
	};

	static Vector<ScanMatch> scan(String file_path, String pattern, bool match_case, bool whole_words);
	static void replace(String relative_path, String absolute_path, Vector<ScanLocation> &locations, bool match_case, bool whole_words, String search_text, String new_text);
};

class DefaultFindInFilesScanner {
public:
	static void scan(Ref<FileAccess> file, String file_path, String display_text, String pattern, bool match_case, bool whole_words, Vector<FindInFilesScanner::ScanMatch> &matches, Size2i range = Size2i(1, -1));

	static void replace(Ref<FileAccess> file, String file_path, const Vector<FindInFilesScanner::ScanLocation> &locations, bool match_case, bool whole_words, String search_text, String new_text, int start_offset = 0);

private:
	static bool find_next(const String &line, String pattern, int from, bool match_case, bool whole_words, int &out_begin, int &out_end);
};

class BuiltinScriptFindInFilesScanner {
public:
	static void scan(Ref<FileAccess> file, String file_path, String pattern, bool match_case, bool whole_words, Vector<FindInFilesScanner::ScanMatch> &matches);

	static void replace(Ref<FileAccess> file, String file_path, Vector<FindInFilesScanner::ScanLocation> &locations, bool match_case, bool whole_words, String search_text, String new_text);

private:
	struct SubResource {
		int script_start_line;
		int line_idx;
		String display_text;
	};

	static HashMap<String, BuiltinScriptFindInFilesScanner::SubResource> _parse_tscn(Ref<FileAccess> file);
};

#endif // FIND_IN_FILES_SCANNER_H
