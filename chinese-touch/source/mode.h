#ifndef MODE_H
#define MODE_H

#include "freetype_renderer.h"
#include "text_button.h"


#define BUTTON_ACTION_UNHANDLED 0
#define BUTTON_ACTION_CHANGED 1
#define BUTTON_ACTION_ACTIVE (1 << 2)
#define BUTTON_ACTION_PRESSED (1 << 3)
#define BUTTON_ACTION_EXIT_MODE (1 << 4)
#define BUTTON_ACTION_SCREEN_SUB (1 << 5)
#define BUTTON_ACTION_SCREEN_MAIN (1 << 6)
#include "chinese-touch.h"

typedef int ButtonAction;

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
	virtual ButtonAction handle_console_button_event( int pressed, int held, int released ) { return BUTTON_ACTION_UNHANDLED; }
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
	Program& program;
	ButtonProviderList button_provider_list;
	int recursion_depth;
	static int MAX_RECURSION_DEPTH;
public:
	Mode( Program& _program, int _recursion_depth ) 
		: GlobalButtonHandler( *_program.ft ), program(_program), recursion_depth(_recursion_depth+1)
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
	virtual ButtonAction change_button_activation( touchPosition touch );
	virtual ButtonAction handle_touch_begin( touchPosition touch );
	virtual ButtonAction handle_touch_drag( touchPosition touch );
	virtual ButtonAction handle_touch_end( touchPosition touch );
	virtual ButtonAction handle_idle_cycles() { return BUTTON_ACTION_UNHANDLED; }
	void handle_changed( ButtonAction action );
	virtual ButtonAction handle_console_button_event( int pressed, int held, int released );
};

#endif // MODE_H