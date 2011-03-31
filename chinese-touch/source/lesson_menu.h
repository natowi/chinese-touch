#ifndef LESSON_MENU_H
#define LESSON_MENU_H

#include <map>

#include "freetype_renderer.h"
#include "lesson.h"
#include "config.h"
#include "text_button.h"
#include "settings_dialog.h"
#include "chinese-touch.h"


class LessonMenuChoice
{
	public:
		Book* book;
		Lesson* lesson;
		enum ContentType
		{
			CONTENT_TYPE_NONE = 0,
			CONTENT_TYPE_GRAMMAR,
			CONTENT_TYPE_TEXT,
			CONTENT_TYPE_EXERCISES,
			CONTENT_TYPE_ANY_WORDS,
			CONTENT_TYPE_EASY_WORDS,
			CONTENT_TYPE_MEDIUM_WORDS,
			CONTENT_TYPE_HARD_WORDS,
			CONTENT_TYPE_IMPOSSIBLE_WORDS,
			CONTENT_TYPE_SEARCH
		} content_type;
		enum ContentRange
		{
			CONTENT_RANGE_LESSON = 0,
			CONTENT_RANGE_BOOK
		} content_range;
		enum ContentOrder
		{
			CONTENT_ORDER_INDEX = 0,
			CONTENT_ORDER_LATENCY,
			CONTENT_ORDER_RANDOM
		} content_order;
	public:
		LessonMenuChoice() : book(0), lesson(0), 
			content_type(CONTENT_TYPE_NONE), content_range(CONTENT_RANGE_LESSON),
			content_order(CONTENT_ORDER_LATENCY) {}
		static std::string get_enum_text( ContentType content_type );
		static std::string get_enum_text( ContentRange content_range );
		static std::string get_enum_text( ContentOrder content_order );
};


class LessonMenu;

class MenuEntry : public ButtonProvider
{
	public:
		LessonMenu& lesson_menu;
		RenderScreenBuffer* text_surface;
		bool exploded, text_rendered;
		int top;
		int last_frame_rendered;
		static int BASE_HEIGHT;
		static int ACTIVE_HEIGHT;
		static int FONT_SIZE;
		static int TEXT_X_OFFSET;
		static int ICON_X_OFFSET;
		static int BUTTON_GAP;
		static int BUTTON_Y_OFFSET;
		static int BUTTON_ACTIVE_Y_OFFSET;
		static int BUTTON_WIDTH;
		static int BUTTON_HEIGHT;
		static int SMALL_BUTTON_WIDTH;
		static int BUTTON_X_OFFSET;
		static int JUMP_DOWN_BUTTON_X_OFFSET;
		static int JUMP_UP_BUTTON_X_OFFSET;
		static int EXPLODE_BUTTON_X_OFFSET;
		static int RATED_WORDS_BUTTON_X_OFFSET;
		static int LESSON_RANGE_BUTTON_X_OFFSET;
		static int ORDER_BUTTON_X_OFFSET;
		static int EASY_WORDS_BUTTON_X_OFFSET;
		static int MEDIUM_WORDS_BUTTON_X_OFFSET;
		static int HARD_WORDS_BUTTON_X_OFFSET;
		static int IMPOSSIBLE_WORDS_BUTTON_X_OFFSET;
		TextButton rating_bar_any_extension, rating_bar, 
				rating_any, rating_easy, rating_medium, rating_hard, rating_impossible,
				order_index_button, order_latency_button, order_random_button;
		Rating cached_rating;
	public:
		MenuEntry( LessonMenu& _lesson_menu );
		virtual void* get_entry_id() = 0;
		virtual void init_button_vram();
		virtual ~MenuEntry()
		{
			delete this->text_surface;
		}
		virtual void render_buttons( OamState* oam_state, int& oam_entry );
		virtual void render_text( FreetypeRenderer& ft, const std::string& text );
		virtual ButtonAction handle_button_pressed( TextButton* text_button );
		virtual std::string get_title() = 0;
};


class BookEntry : public MenuEntry
{
	public:
		Book* book;
		TextButton book_icon, explode_button, implode_button;
	public:
		BookEntry( LessonMenu& _lesson_menu, Book* _book );
		virtual void* get_entry_id() { return static_cast<void*>(this->book); }
		virtual void init_button_vram();
		virtual void render_buttons( OamState* oam_state, int& oam_entry );
		virtual ButtonAction handle_button_pressed( TextButton* text_button );
		virtual std::string get_title() { return this->book->title; }
};


class LessonEntry : public MenuEntry
{
	public:
		Lesson* lesson;
		TextButton lesson_icon, grammar_button, text_button, exercises_button, 
			jump_down_button, jump_up_button, lesson_range_button, book_range_button;
	public:
		LessonEntry( LessonMenu& _lesson_menu, Lesson* _lesson );
		virtual void* get_entry_id() { return static_cast<void*>(this->lesson); }
		virtual void init_button_vram();
		virtual void render_buttons( OamState* oam_state, int& oam_entry );
		virtual ButtonAction handle_button_pressed( TextButton* text_button );
		virtual std::string get_title() { return this->lesson->title; }
};


class MenuList : public std::map<void*,MenuEntry*>
{
	public:
		~MenuList();
};


class DictionarySynchronizer : public ActionButton
{
	public:
		Program& program;
		RenderScreen& info_screen;
		bool full_sync;
	public:
		DictionarySynchronizer( const std::string& _name, const std::string& _description, 
								const std::string& _button_label, Program& _program, 
								RenderScreen& _screen, bool _full_sync=false )
			: ActionButton( _name, _description, _button_label ), 
			program(_program), info_screen(_screen), full_sync(_full_sync)
		{}
		virtual void run_action();
};


class IndexRebuilder : public ActionButton
{
	public:
		Program& program;
		RenderScreen& info_screen;
	public:
		IndexRebuilder( const std::string& _name, const std::string& _description, 
								const std::string& _button_label, Program& _program, 
								RenderScreen& _screen )
			: ActionButton( _name, _description, _button_label ), 
			program(_program), info_screen(_screen)
		{}
		virtual void run_action();
};


class LessonMenu : public Mode
{
public:
	LessonMenuChoice& choice;
	RenderScreen info_screen, menu_screen;
	MenuList menu_list;
	int y_offset;
	int v_y;
	void* active_list_id;
	int sub_frame_count;
	/*! As an optimization all buttons, including menu entry buttons 
		are stored as reference buttons in LessonMenu and copied to menu 
		entries, using shared vram. */
	TextButton book_icon, lesson_icon, grammar_button, text_button, exercises_button, 
		explode_button, implode_button, lesson_range_button, book_range_button,
		order_index_button, order_latency_button, order_random_button,
		rating_bar_any_extension, rating_bar, rating_any, rating_easy, rating_medium, rating_hard, rating_impossible,
		global_rating_bar_any_extension, global_rating_bar, global_rating_any, global_rating_easy, global_rating_medium, global_rating_hard, global_rating_impossible,
		jump_down_button, jump_up_button, settings_button, search_button, loading_symbol;
	static int BUTTON_ACTIVATION_SCROLL_LIMIT;
	/*! a factor f, where: f * prev_scroll_width = max_next_scroll_width 
		(used to filter out some erroneous touch readings, occurring under very light pressure) */
	static int MAX_ACCELERATION_FACTOR;
	Settings settings;
	touchPosition old_touch;
	int old_y_offset; //!< used for scrolling
	int old_abs_y_diff; //!< used for scrolling
	int pixels_scrolled; //!< used for scrolling
	Rating cached_global_rating;
public:
	LessonMenu( Program& _program, int _recursion_depth, LessonMenuChoice& _choice );
	virtual void init_mode();
	virtual void init_vram();
	virtual void init_button_vram();
	~LessonMenu();
	virtual void render( Screen screen );
	virtual ButtonAction handle_button_pressed( TextButton* text_button );
	virtual ButtonAction handle_touch_begin( touchPosition touch );
	virtual ButtonAction handle_touch_drag( touchPosition touch );
	virtual ButtonAction handle_touch_end( touchPosition touch );
	virtual ButtonAction handle_idle_cycles();
	void show_settings();
};


#endif //LESSON_MENU_H