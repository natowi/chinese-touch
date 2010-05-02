#ifndef TEXT_VIEW_H
#define TEXT_VIEW_H

#include "freetype_renderer.h"
#include "lesson.h"
#include "config.h"
#include "text_button.h"
#include "settings_dialog.h"
#include "new_words.h"


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

class TextView : private std::list<BufferedLine*>, public Mode
{
public:
	//UILanguage& ui_language;
	RenderScreen word_screen, text_screen;
	Library& library;
	Text& text;
	Config* config;
	NewWordList current_words;
	WordListBrowser word_browser;
	int y_offset;
	int v_y;
	int sub_frame_count;
	RenderScreenBuffer* current_highlight;
	int current_highlight_x, current_highlight_y;
	static int LINE_HEIGHT;
	ContextMode context_mode;
	RenderChar* context_render_char;
	int recursion_depth;
	TextButton exit_button, settings_button, up_button;
	static int BUTTON_ACTIVATION_SCROLL_LIMIT;
	/*! a factor f, where: f * prev_scroll_width = max_next_scroll_width 
		(used to filter out some erroneous touch readings, occurring under very light pressure) */
	static int MAX_ACCELERATION_FACTOR;
	Settings settings;
	bool lookup_from_current_lesson, lookup_from_previous_lessons, 
		lookup_from_upcoming_lessons, lookup_from_other_books;
	std::string lookup_sql_cond;
	touchPosition old_touch;
	int old_y_offset; //!< used for scrolling
	int old_abs_y_diff; //!< used for scrolling
	int pixels_scrolled; //!< used for scrolling
public:
	TextView( /*UILanguage& _ui_language, */FreetypeRenderer& _ft, Library& _library, Text& _text, Config* _config );
	void init_mode();
	void init_vram();
	void init_button_vram();
	~TextView();
	void free_line_buffers();
	void render( Screen screen );
	virtual ButtonAction handle_button_pressed( TextButton* text_button );
	virtual ButtonAction handle_touch_begin( touchPosition touch );
	virtual ButtonAction handle_touch_drag( touchPosition touch );
	virtual ButtonAction handle_touch_end( touchPosition touch );
	virtual ButtonAction handle_idle_cycles();
	static void show_word_as_text( /*UILanguage& ui_language, */FreetypeRenderer& ft, Library& library, NewWord* word, Config* config, int recursion_depth = 0 );
	void show_settings();
	void restore_init_settings();
};

#endif // TEXT_VIEW_H