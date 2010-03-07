#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <list>
#include <string>

#include "freetype_renderer.h"
#include "text_button.h"

class Setting
{
	public:
		std::string name, description;
	public:
		Setting( std::string _name, std::string _description ) 
			: name(_name), description(_description) {}
		virtual ~Setting() {}
};

class BooleanSetting : public Setting
{
	public:
		bool& value;
	public:
		BooleanSetting( std::string _name, std::string _description, bool& _value )
			: Setting( _name, _description ), value(_value) {}
};

class Settings : public std::map<std::string,Setting*>
{
	public:
		~Settings();
		void add_setting( Setting* setting );
		Setting* get_setting( std::string name );
		bool get_boolean_setting( std::string name );
};

class SettingsDialog
{
	public:
		FreetypeRenderer& freetype_renderer;
		Settings& settings;
		RenderScreen settings_screen;
		TextButton ok_button, dummy_checkbox;
		TextButtonList text_buttons;
		TextButtonMapStorage checkboxes;
	public:
		SettingsDialog( FreetypeRenderer& _freetype_renderer, Settings& _settings );
		void render( Screen screen );
		void run_until_exit();
};

#endif // SETTINGS_DIALOG_H