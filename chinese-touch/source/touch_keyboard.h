#ifndef TOUCH_KEYBOARD_H
#define TOUCH_KEYBOARD_H

#include "mode.h"
#include "ui_language.h"
#include "freetype_renderer.h"
#include "text_button.h"

typedef std::map<std::string,std::string> StringMap;

class TouchKeyboard : public ButtonProvider
{
protected:
	sqlite3* db;
public:
	RenderScreen& keyboard_screen;
	TextButton reference_key, layout_minus, layout_label, layout_plus;
	TextButtonSetStorage keys;
	StringList written_chars;
	std::string modifier;
	std::string layouts_path;
	StringList layouts;
	StringList::iterator current_layout;
	std::string current_layout_description, current_layout_language, current_layout_keys, current_layout_dead_keys;
protected:
	void close_db();
public:
	TouchKeyboard( ButtonProviderList& button_provider_list, FreetypeRenderer& _freetype_renderer, RenderScreen& _keyboard_screen, std::string& program_base_dir );
	~TouchKeyboard();
	void reload_current_layout();
	void init_button_vram();
	void render_buttons( OamState* target_oam, int& oam_entry );
	ButtonAction handle_button_pressed( TextButton* text_button );
	std::string handle_key_pressed( const std::string& input ) { return input; }
	void handle_text_changed() {}
	std::string get_written_text();
	void set_written_text( const std::string& text );
};

#endif // TOUCH_KEYBOARD_H