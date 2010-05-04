#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <list>
#include <string>

#include "mode.h"
#include "freetype_renderer.h"
#include "text_button.h"

class Setting
{
	public:
		std::string name, description;
	public:
		Setting( const std::string& _name, const std::string& _description ) 
			: name(_name), description(_description) {}
		virtual ~Setting() {}
};

class BooleanSetting : public Setting
{
	public:
		bool& value;
	public:
		BooleanSetting( const std::string& _name, const std::string& _description, bool& _value )
			: Setting( _name, _description ), value(_value) {}
};

class SettingsLabel : public Setting
{
	public:
		SettingsLabel( const std::string& _name, const std::string& _description )
			: Setting( _name, _description ) {}
};

class ActionButton : public Setting
{
	public:
		std::string button_label;
	public:
		ActionButton( const std::string& _name, const std::string& _description, const std::string& _button_label )
			: Setting( _name, _description ), button_label(_button_label) {}
		virtual void run_action()=0;
};

class Settings : public std::map<std::string,Setting*>
{
	public:
		~Settings();
		void add_setting( Setting* setting );
		Setting* get_setting( const std::string& name );
		bool get_boolean_setting( const std::string& name );
};

class SettingsDialog : public Mode
{
	public:
		Settings& settings;
		RenderScreen settings_screen;
		std::string title;
		TextButton ok_button, dummy_checkbox, dummy_start_button;
		TextButtonMapStorage checkboxes, start_buttons;
	public:
		SettingsDialog( Program& _program, int _recursion_depth, Settings& _settings, const std::string& _title );
		virtual void init_mode();
		virtual void init_vram();
		virtual void render( Screen screen );
		virtual ButtonAction handle_button_pressed( TextButton* text_button );
};

#endif // SETTINGS_DIALOG_H