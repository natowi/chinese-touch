#ifndef MODE_H
#define MODE_H

#include "freetype_renderer.h"
#include "text_button.h"


enum ButtonAction
{
	BUTTON_ACTION_UNHANDLED=0,
	BUTTON_ACTION_HANDLED,
	BUTTON_ACTION_EXIT_MODE
};

class ButtonProvider;
typedef std::list<ButtonProvider*> ButtonProviderList;

class GlobalButtonHandler;

/*! Base class for complex sets of touch buttons and their corresponding functionality, Used by different modes but not beeing
	independent modes on their own, like TouchKeyboard and WordListBrowser. */
class ButtonProvider
{
public:
	FreetypeRenderer& button_ft;
	TextButtonList text_buttons;
	TextButton* current_active_button; //!< pointer to a currently activated button or 0
protected:
	/*! this c-tor is exclusivly for GlobalButtonHandler(), as it has to be called before
		Mode::button_provider_list is constructed and need to be inserted there afterwards */
	ButtonProvider( FreetypeRenderer& _freetype_renderer );
public:
	ButtonProvider( ButtonProviderList& button_povider_list, FreetypeRenderer& _freetype_renderer );
	virtual void init_button_vram();
	virtual void free_button_vram();
	virtual void render_buttons( OamState* target_oam, int& oam_entry );
	virtual ButtonAction handle_touch( int x, int y, GlobalButtonHandler* global_handler=0 );
	virtual ButtonAction handle_release( int x, int y, GlobalButtonHandler* global_handler=0 );
	virtual ButtonAction handle_button_pressed( TextButton* text_button ) { return BUTTON_ACTION_UNHANDLED; }
};

/*! Adapter class for Mode that is a ButtonProvider on its own, but receives Events for
	all buttons from all ButtonProviders used by the Mode not only from Buttons directly
	defined in that Mode */
class GlobalButtonHandler : public ButtonProvider
{
public:
	GlobalButtonHandler( FreetypeRenderer& _freetype_renderer )
		: ButtonProvider(_freetype_renderer ) {}
};

/*! Base class for all independent program modes, like NewWordsViewer, FulltextSearch, 
	that need to control the Systems graphic engines and receive input events. Button
	events can be received directly and from any number of ButtonProviders. */
class Mode : public GlobalButtonHandler
{
public:
	FreetypeRenderer& mode_ft;
	ButtonProviderList button_provider_list;
public:
	Mode( FreetypeRenderer& _freetype_renderer ) 
		: GlobalButtonHandler( _freetype_renderer ), mode_ft(_freetype_renderer) 
	{
		// Modes itself in its role as GlobalButtonHandler needs to be inserted manually, 
		// as it needs to be constructed before its button_provider_list is available
		// (normal ButtonProviders insert themselves during their construction)
		this->button_provider_list.push_back( this );
	}
	virtual void init_mode();
	virtual void init_vram();
	virtual void free_vram();
	virtual void render( Screen screen );
	virtual void run_until_exit();
	virtual ButtonAction handle_console_button_pressed( int pressed ) { return BUTTON_ACTION_UNHANDLED; }
	virtual ButtonAction handle_touch_begin( touchPosition touch ) { return BUTTON_ACTION_UNHANDLED; }
	virtual ButtonAction handle_touch_drag( touchPosition touch ) { return BUTTON_ACTION_UNHANDLED; }
	virtual ButtonAction handle_touch_end( touchPosition touch ) { return BUTTON_ACTION_UNHANDLED; }
	virtual ButtonAction handle_idle_cycles() { return BUTTON_ACTION_UNHANDLED; }
};

#endif // MODE_H