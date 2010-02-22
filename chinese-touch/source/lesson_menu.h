#ifndef LESSON_MENU_H
#define LESSON_MENU_H

#include <map>

#include "freetype_renderer.h"
#include "lesson.h"
#include "config.h"
#include "text_button.h"

class LessonMenuChoice
{
	public:
		Book* book;
		Lesson* lesson;
		enum ContentType
		{
			CONTENT_TYPE_NONE = 0,
			CONTENT_TYPE_NEW_WORDS,
			CONTENT_TYPE_GRAMMAR,
			CONTENT_TYPE_TEXT,
			CONTENT_TYPE_EXERCISES,
			CONTENT_TYPE_JUMP_DOWN,
			CONTENT_TYPE_JUMP_UP,
			CONTENT_TYPE_EXPLODE,
			CONTENT_TYPE_IMPLODE,
			CONTENT_TYPE_EASY_WORDS,
			CONTENT_TYPE_MEDIUM_WORDS,
			CONTENT_TYPE_HARD_WORDS,
			CONTENT_TYPE_IMPOSSIBLE_WORDS
		} content_type;
	public:
		LessonMenuChoice() : book(0), lesson(0), 
			content_type(CONTENT_TYPE_NONE) {}
};

class MenuEntry
{
	public:
		RenderScreenBuffer* text_surface;
		Book* book;
		Lesson* lesson;
		bool exploded;
		int top;
		int last_frame_rendered;
		static int BASE_HEIGHT;
		static int ACTIVE_HEIGHT;
		static int FONT_SIZE;
		static int TEXT_X_OFFSET;
		static int ICON_X_OFFSET;
		static int BUTTON_GAP;
		static int BUTTON_Y_OFFSET;
		static int BUTTON_WIDTH;
		static int BUTTON_HEIGHT;
		static int SMALL_BUTTON_WIDTH;
		static int NEW_WORDS_BUTTON_X_OFFSET;
		static int GRAMMAR_BUTTON_X_OFFSET;
		static int TEXT_BUTTON_X_OFFSET;
		static int EXERCISES_BUTTON_X_OFFSET;
		static int JUMP_DOWN_BUTTON_X_OFFSET;
		static int JUMP_UP_BUTTON_X_OFFSET;
		static int EXPLODE_BUTTON_X_OFFSET;
		static int RATED_WORDS_BUTTON_X_OFFSET;
		static int EASY_WORDS_BUTTON_X_OFFSET;
		static int MEDIUM_WORDS_BUTTON_X_OFFSET;
		static int HARD_WORDS_BUTTON_X_OFFSET;
		static int IMPOSSIBLE_WORDS_BUTTON_X_OFFSET;
	public:
		MenuEntry() : text_surface( new RenderScreenBuffer(200, MenuEntry::BASE_HEIGHT) ),
						book(0), lesson(0), exploded(false), top(0), last_frame_rendered(0) {}
		~MenuEntry()
		{
			delete this->text_surface;
		}
		void render_text( FreetypeRenderer& ft, const std::string& text );
		LessonMenuChoice::ContentType get_content_type_by_pos( int x, int y );
};
class MenuList : public std::map<void*,MenuEntry*>
{
	public:
		~MenuList();
};

class LessonMenu
{
public:
	FreetypeRenderer& freetype_renderer;
	Library& library;
	Config& config;
	RenderScreen info_screen, menu_screen;
	MenuList menu_list;
	int y_offset;
	int v_y;
	void* active_list_id;
	int frame_count;
	TextButton book_icon, lesson_icon, new_words_button, grammar_button, text_button, exercises_button,
		explode_button, implode_button, rating_bar, rating_easy, rating_medium, rating_hard, rating_impossible,
		jump_down_button, jump_up_button;
	TextButtonList text_buttons, lesson_buttons, book_buttons;
	static int BUTTON_ACTIVATION_SCROLL_LIMIT;
	/*! a factor f, where: f * prev_scroll_width = max_next_scroll_width 
		(used to filter out some erroneous touch readings, occurring under very light pressure) */
	static int MAX_ACCELERATION_FACTOR;
public:
	LessonMenu( FreetypeRenderer& _freetype_renderer, Library& _library, Config& _config );
	~LessonMenu();
	void render( Screen screen );
	void run_for_user_choice( LessonMenuChoice& choice );
	MenuList::iterator get_entry_by_pos( int x, int y );
	TextButton* get_button_by_content_type( LessonMenuChoice::ContentType content_type );
	bool activate_button_by_content_type( LessonMenuChoice::ContentType content_type );
	bool get_activation_by_content_type( LessonMenuChoice::ContentType content_type );
};


#endif //LESSON_MENU_H