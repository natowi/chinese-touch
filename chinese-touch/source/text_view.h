#ifndef TEXT_VIEW_H
#define TEXT_VIEW_H

#include "freetype_renderer.h"
#include "lesson.h"
#include "config.h"
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

class TextView : private std::list<BufferedLine*>, public NewWordRenderSettings
{
public:
	FreetypeRenderer& freetype_renderer;
	Config& config;
	Text& text;
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
	TextButton left_button, right_button, exit_button,
			hanzi_tab, pinyin_tab, latin_tab, rating_bar, 
			rating_easy, rating_medium, rating_hard, rating_impossible;
	TextButtonList text_buttons;
	static int BUTTON_ACTIVATION_SCROLL_LIMIT;
	/*! a factor f, where: f * prev_scroll_width = max_next_scroll_width 
		(used to filter out some erroneous touch readings, occurring under very light pressure) */
	static int MAX_ACCELERATION_FACTOR;
public:
	TextView( FreetypeRenderer& _ft, Config& _config, Text& _text );
	~TextView();
	void render( Screen screen, bool update_sprites=true );
	void run_until_exit();
};

#endif // TEXT_VIEW_H