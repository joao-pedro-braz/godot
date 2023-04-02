/**************************************************************************/
/*  find_in_files_scanner.cpp                                             */
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

#include "find_in_files_scanner.h"

const int SCRIPT_SOURCE_TAG_LEN = String("script/source = \"").length();

Vector<FindInFilesScanner::ScanMatch> FindInFilesScanner::scan(String file_path, String pattern, bool match_case, bool whole_words) {
	Ref<FileAccess> file = FileAccess::open(file_path, FileAccess::READ);
	Vector<FindInFilesScanner::ScanMatch> matches;
	if (file.is_null()) {
		print_verbose(String("Cannot open file ") + file_path);
		return matches;
	}

	String extension = file->get_path().get_extension().to_lower();
	if (extension == "tscn") {
		BuiltinScriptFindInFilesScanner::scan(file, file_path, pattern, match_case, whole_words, matches);
	} else {
		// No specific match was found, use default scanner
		DefaultFindInFilesScanner::scan(file, file_path, file_path, pattern, match_case, whole_words, matches);
	}

	return matches;
}

void FindInFilesScanner::replace(String relative_path, String absolute_path, Vector<ScanLocation> &locations, bool match_case, bool whole_words, String search_text, String new_text) {
	Ref<FileAccess> file = FileAccess::open(absolute_path, FileAccess::READ);
	if (file.is_null()) {
		print_verbose(String("Cannot open file ") + file->get_path());
		return;
	}

	String extension = absolute_path.get_extension().to_lower();
	if (extension == "tscn") {
		BuiltinScriptFindInFilesScanner::replace(file, relative_path, locations, match_case, whole_words, search_text, new_text);
	} else {
		// No specific match was found, use default scanner
		DefaultFindInFilesScanner::replace(file, relative_path, locations, match_case, whole_words, search_text, new_text);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

void DefaultFindInFilesScanner::scan(Ref<FileAccess> file, String file_path, String display_text, String pattern, bool match_case, bool whole_words, Vector<FindInFilesScanner::ScanMatch> &matches, Size2i range) {
	// Line number starts at 1.
	for (int line_number = 1, begin = 0, end = 0; !file->eof_reached(); line_number++, begin = 0, end = 0) {
		String line = file->get_line();
		if (line_number < range.x) {
			continue;
		} else if (range.y > 0 && line_number > range.y) {
			break;
		}

		while (find_next(line, pattern, end, match_case, whole_words, begin, end)) {
			matches.append(FindInFilesScanner::ScanMatch{ file_path, display_text, line_number - range.x + 1, begin, end, line });
		}
	}
}

// Same as get_line, but preserves line ending characters.
class ConservativeGetLine {
public:
	String get_line(Ref<FileAccess> f) {
		_line_buffer.clear();

		char32_t c = f->get_8();

		while (!f->eof_reached()) {
			if (c == '\n') {
				_line_buffer.push_back(c);
				_line_buffer.push_back(0);
				return String::utf8(_line_buffer.ptr());

			} else if (c == '\0') {
				_line_buffer.push_back(c);
				return String::utf8(_line_buffer.ptr());

			} else if (c != '\r') {
				_line_buffer.push_back(c);
			}

			c = f->get_8();
		}

		_line_buffer.push_back(0);
		return String::utf8(_line_buffer.ptr());
	}

private:
	Vector<char> _line_buffer;
};
void DefaultFindInFilesScanner::replace(Ref<FileAccess> file, String file_path, const Vector<FindInFilesScanner::ScanLocation> &locations, bool match_case, bool whole_words, String search_text, String new_text, int start_offset) {
	// If the file is already open, I assume the editor will reload it.
	// If there are unsaved changes, the user will be asked on focus,
	// however that means either losing changes or losing replaces.

	String buffer;
	int current_line = 1;

	ConservativeGetLine conservative;

	String line = conservative.get_line(file);

	int offset = 0;

	for (int i = 0; i < locations.size(); ++i) {
		int repl_line_number = locations[i].line_number + start_offset;

		while (current_line < repl_line_number) {
			buffer += line;
			line = conservative.get_line(file);
			++current_line;
			offset = 0;
		}

		int repl_begin = locations[i].begin + offset;
		int repl_end = locations[i].end + offset;

		int _;
		if (!find_next(line, search_text, repl_begin, match_case, whole_words, _, _)) {
			// Make sure the replacement is still valid in case the file was tampered with.
			print_verbose(String("Occurrence no longer matches, replace will be ignored in {0}: line {1}, col {2}").format(varray(file->get_path(), repl_line_number, repl_begin)));
			continue;
		}

		line = line.left(repl_begin) + new_text + line.substr(repl_end);
		// Keep an offset in case there are successive replaces in the same line.
		offset += new_text.length() - (repl_end - repl_begin);
	}

	buffer += line;

	while (!file->eof_reached()) {
		buffer += conservative.get_line(file);
	}

	// Now the modified contents are in the buffer, rewrite the file with our changes.

	Error err = file->reopen(file_path, FileAccess::WRITE);
	ERR_FAIL_COND_MSG(err != OK, "Cannot create file in path '" + file->get_path() + "'.");

	file->store_string(buffer);
}

bool DefaultFindInFilesScanner::find_next(const String &line, String pattern, int from, bool match_case, bool whole_words, int &out_begin, int &out_end) {
	int end = from;

	while (true) {
		int begin = match_case ? line.find(pattern, end) : line.findn(pattern, end);

		if (begin == -1) {
			return false;
		}

		end = begin + pattern.length();
		out_begin = begin;
		out_end = end;

		if (whole_words) {
			if (begin > 0 && (is_ascii_identifier_char(line[begin - 1]))) {
				continue;
			}
			if (end < line.size() && (is_ascii_identifier_char(line[end]))) {
				continue;
			}
		}

		return true;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

void BuiltinScriptFindInFilesScanner::scan(Ref<FileAccess> file, String file_path, String search_text, bool match_case, bool whole_words, Vector<FindInFilesScanner::ScanMatch> &matches) {
	int matches_offset = matches.size();
	for (auto &match : _parse_tscn(file)) {
		file->seek(0);
		String relative_file_path = file_path + "::" + match.key;
		DefaultFindInFilesScanner::scan(file, relative_file_path, match.value.display_text, search_text, match_case, whole_words, matches, Size2i(match.value.script_start_line, match.value.line_idx - 1));

		// Let's handle some edge cases!
		if (matches.size() > matches_offset) {
			// We have new matches

			// Check if the first new match is on the first line, if so handle it separately
			if (matches.get(matches_offset).line_number == 1) {
				FindInFilesScanner::ScanMatch scan_match = matches.get(matches_offset);
				if (scan_match.begin - SCRIPT_SOURCE_TAG_LEN == scan_match.end - SCRIPT_SOURCE_TAG_LEN - 1) {
					// Edge case matching the double quote mark
					matches.remove_at(matches_offset);
				} else {
					// Another edge case where we need to remove "script/source = " from the resulting line
					String actual_line = scan_match.line.replace("script/source = \"", "");
					scan_match.file_path = relative_file_path;
					scan_match.begin = scan_match.begin - SCRIPT_SOURCE_TAG_LEN;
					scan_match.end = scan_match.end - SCRIPT_SOURCE_TAG_LEN;
					scan_match.line = actual_line;
					matches.insert(matches_offset, scan_match);
					matches.remove_at(matches_offset + 1);
				}
			}

			// Unescape all matched lines, this is necessary because of the way builtin scripts are stored.
			// They get escaped before being written into the file, so things like quotes gets encoded into \".
			int i = 0;
			for (auto &scan_match : matches.slice(matches_offset)) {
				String unescaped_line = scan_match.line.c_unescape();
				int diffs_found = 0;
				for (int j = 0; j <= scan_match.end; ++j) {
					if (scan_match.line[j] == unescaped_line[j - diffs_found]) {
						continue;
					}

					diffs_found++;
				}
				scan_match.begin -= diffs_found;
				scan_match.end -= diffs_found;
				scan_match.line = unescaped_line;
				matches.insert(matches_offset + i, scan_match);
				matches.remove_at(matches_offset + i + 1);
				i++;
			}

			matches_offset = matches.size();
		}
	}
}

void BuiltinScriptFindInFilesScanner::replace(Ref<FileAccess> file, String file_path, Vector<FindInFilesScanner::ScanLocation> &locations, bool match_case, bool whole_words, String search_text, String new_text) {
	for (auto &match : _parse_tscn(file)) {
		if ((file->get_path() + "::" + match.key) != file_path) {
			continue;
		}

		file->seek(0);
		// Account for the edge cases also found during scanning
		for (auto &location : locations) {
			if (location.line_number == 1) {
				location.begin += SCRIPT_SOURCE_TAG_LEN;
				location.end += SCRIPT_SOURCE_TAG_LEN;
			}
		}
		DefaultFindInFilesScanner::replace(file, file->get_path(), locations, match_case, whole_words, search_text, new_text, match.value.script_start_line - 1);
	}
}

HashMap<String, BuiltinScriptFindInFilesScanner::SubResource> BuiltinScriptFindInFilesScanner::_parse_tscn(Ref<FileAccess> file) {
	enum BuiltinScriptMarkers {
		SUB_RESOURCE_MARKER,
		SCRIPT_SOURCE_MARKER,
		SCRIPT_EOF_MARKER
	};
	HashMap<BuiltinScriptMarkers, String> builtin_script_markers_map;
	builtin_script_markers_map.insert(BuiltinScriptMarkers::SUB_RESOURCE_MARKER, "[sub_resource type=\"GDScript\" id=\"");
	builtin_script_markers_map.insert(BuiltinScriptMarkers::SCRIPT_SOURCE_MARKER, "script/source = \"");
	builtin_script_markers_map.insert(BuiltinScriptMarkers::SCRIPT_EOF_MARKER, "\"");

	int script_start_line = -1;
	String scene_id;
	String line = file->get_line();
	BuiltinScriptMarkers looking_for = BuiltinScriptMarkers::SUB_RESOURCE_MARKER;
	HashMap<String, BuiltinScriptFindInFilesScanner::SubResource> matches;
	HashMap<int, String> node_names;

	for (int line_idx = 1; !file->eof_reached(); line_idx++, line = file->get_line()) {
		String pattern = builtin_script_markers_map.get(looking_for);
		if (!line.begins_with(pattern)) {
			// Since we're going through the file any ways, let's look for nodes using the builtin scripts found so far, so we can assemble the display_name using the node name
			if (line.begins_with("[node name=\"")) {
				// Found a node, store it, alongside it's full name and line index
				String node_name = line.get_slice("[node name=\"", 1).get_slice("\"", 0);
				String parent = line.get_slice(" parent=\"", 1).get_slice("\"", 0);
				node_names.insert(line_idx, (parent != "." ? parent + "/" : "") + node_name);
			} else if (line.begins_with("script = SubResource(\"")) {
				// Found a script usage reference, try to match one of the found nodes with it
				for (auto &match : matches) {
					if (!line.contains(match.key)) {
						continue;
					}

					for (HashMap<int, String>::Iterator E = node_names.last(); E; --E) {
						if (E->key < line_idx) {
							// It should not be possible for a builtin script to be used in more than one Node, but...
							// it doesn't hurt to account for that. The resulting display_text might look like "res://foo.tscn::Main|Camera|Controller"
							if (match.value.display_text.is_empty()) {
								match.value.display_text = file->get_path() + "::" + E->value;
							} else {
								match.value.display_text += "|" + E->value;
							}
							break;
						}
					}
				}
			}

			continue;
		}

		switch (looking_for) {
			case BuiltinScriptMarkers::SUB_RESOURCE_MARKER: {
				// Parse this scene's id
				scene_id = line.get_slice(pattern, 1);
				scene_id = scene_id.get_slice("\"", 0);

				// Start looking for the actual script source next
				looking_for = BuiltinScriptMarkers::SCRIPT_SOURCE_MARKER;
			} break;
			case BuiltinScriptMarkers::SCRIPT_SOURCE_MARKER: {
				// Save the current file line, so that we can properly open the script later
				script_start_line = line_idx;

				// Start looking for the matching line
				looking_for = BuiltinScriptMarkers::SCRIPT_EOF_MARKER;
			} break;
			case BuiltinScriptMarkers::SCRIPT_EOF_MARKER: {
				// Found one builtin script!
				matches.insert(scene_id, BuiltinScriptFindInFilesScanner::SubResource{ script_start_line, line_idx, "" });
				looking_for = BuiltinScriptMarkers::SUB_RESOURCE_MARKER;
			} break;
		}
	}

	return matches;
}
