#ifndef TEXT_VIEW_H
#define TEXT_VIEW_H

#include "freetype_renderer.h"
#include "lesson.h"
#include "config.h"
#include "dictionary.h"
#include "text_button.h"

class BufferedLine : public RenderScreenBuffer
{
public:
	RenderCharList render_char_list;
	int top;
	int last_frame_rendered;
public:
	BufferedLine() : RenderScreenBuffer(256, 16), top(0), last_frame_rendered(0) {};
};

enum ContextMode
{
	CONTEXT_WORDS_BY_CONTEXT,
	CONTEXT_WORDS_BY_CHARCODE
};

class TextView : private std::list<BufferedLine*>
{
public:
	FreetypeRenderer& freetype_renderer;
	Config& config;
	Text& text;
	Dictionary& dict;
	RenderScreen word_screen, text_screen;
	int y_offset;
	int v_y;
	int sub_frame_count;
	NewWordList current_new_word_list;
	NewWordList::iterator current_new_word_list_it;
	RenderScreenBuffer* current_highlight;
	int current_highlight_x, current_highlight_y;
	static int LINE_HEIGHT;
	ContextMode context_mode;
	RenderChar* context_render_char;
	TextButton left_button, right_button, exit_button;
	TextButtonList text_buttons;
	static int BUTTON_ACTIVATION_SCROLL_LIMIT;
public:
	TextView( FreetypeRenderer& _ft, Config& _config, Text& _text, Dictionary& _dict );
	~TextView();
	void render( Screen screen );
	void run_until_exit();
};

#endif // TEXT_VIEW_H