#include "mode.h"
#include "greys256.h"
#include "error_console.h"


ButtonProvider::ButtonProvider(FreetypeRenderer& _freetype_renderer)
	: button_ft(_freetype_renderer), current_active_button(0)
{
}

ButtonProvider::ButtonProvider( ButtonProviderList& button_povider_list, FreetypeRenderer& _freetype_renderer ) 
	: button_ft(_freetype_renderer), current_active_button(0)
{
	button_povider_list.push_back( this );
}


int Mode::MAX_RECURSION_DEPTH = 10;

void Mode::init_mode()
{
	// prepare lower screen for sprite usage
	vramSetBankD( VRAM_D_SUB_SPRITE );
	oamInit( &oamSub, SpriteMapping_Bmp_1D_128, 0 );
	//oamAllocReset( &oamSub );
	oamEnable( &oamSub );
}

void ButtonProvider::init_button_vram()
{
	for( TextButtonList::iterator i=this->text_buttons.begin(); i!=this->text_buttons.end(); i++ )
	{
		(*i)->init_text_layer( this->button_ft );
	}
}

void Mode::init_vram()
{
	for( ButtonProviderList::iterator bpi = this->button_provider_list.begin();
		bpi != this->button_provider_list.end(); bpi++ )
	{
		(*bpi)->init_button_vram();
	}
	
	// load gray scale sprite palette for button text labels
	dmaCopy( greys256Pal, SPRITE_PALETTE_SUB, 256*2 );
	
	// initialize 32 unscaled rotation matrices each differing by 360/32 degrees:
	for( int i = 0; i < 32; i++ )
	{
		oamRotateScale( &oamSub, i, degreesToAngle((i*360)/32), intToFixed(1, 8), intToFixed(1, 8) );
	}
}

void ButtonProvider::free_button_vram()
{
	this->text_buttons.free_all();
}

void Mode::free_vram()
{
	for( ButtonProviderList::iterator bpi = this->button_provider_list.begin();
		bpi != this->button_provider_list.end(); bpi++ )
	{
		(*bpi)->free_button_vram();
	}
}

void ButtonProvider::render_buttons( OamState* target_oam, int& oam_entry )
{
	for( TextButtonList::iterator ki = this->text_buttons.begin(); ki != this->text_buttons.end(); ki++ )
	{
		if( (*ki)->get_oam() == target_oam )
			(*ki)->render_to( oam_entry );
	}
}

void Mode::render( Screen screen )
{
	OamState* target_oam=&oamSub;
	if( screen == SCREEN_MAIN )
	{
		target_oam = &oamMain;
	}
	else if( screen == SCREEN_SUB )
	{
		target_oam = &oamSub;
	}
	
	oamClear( target_oam, 0, 0 );
	int oam_entry = 0;
	for( ButtonProviderList::iterator bpi = this->button_provider_list.begin();
		bpi != this->button_provider_list.end(); bpi++ )
	{
		(*bpi)->render_buttons( target_oam, oam_entry );
	}
	swiWaitForVBlank();
	oamUpdate( target_oam );
}

ButtonAction ButtonProvider::handle_touch( int x, int y, GlobalButtonHandler* global_handler )
{
	ButtonAction action = BUTTON_ACTION_UNHANDLED;
	this->current_active_button = 0;
	for( TextButtonList::iterator b_it = this->text_buttons.begin(); 
			b_it != this->text_buttons.end(); b_it++ )
	{
		if( (*b_it)->is_responsible(x, y) )
		{
			if( !(*b_it)->active ) action |= BUTTON_ACTION_CHANGED | BUTTON_ACTION_SCREEN_SUB;
			(*b_it)->active = true;
			this->current_active_button = *b_it;
			action |= BUTTON_ACTION_ACTIVE;
		}
		else
		{
			if( (*b_it)->active ) action |= BUTTON_ACTION_CHANGED | BUTTON_ACTION_SCREEN_SUB;
			(*b_it)->active = false;
		}
	}
	return action;
}

ButtonAction ButtonProvider::handle_release( int x, int y, GlobalButtonHandler* global_handler )
{
	ButtonAction action = BUTTON_ACTION_UNHANDLED;
	this->current_active_button = 0;
	for( TextButtonList::iterator b_it = this->text_buttons.begin(); 
			b_it != this->text_buttons.end(); b_it++ )
	{
		if( (*b_it)->is_responsible(x, y)
			&& (*b_it)->active )
		{
			(*b_it)->active = false;
			action |= this->handle_button_pressed( *b_it );
			if( action & BUTTON_ACTION_EXIT_MODE ) return action;
			// optional mode-global button hook:
			if( global_handler && global_handler!=this ) 
				action |= global_handler->handle_button_pressed( *b_it );
			if( action & BUTTON_ACTION_EXIT_MODE ) return action;
			action |= BUTTON_ACTION_CHANGED | BUTTON_ACTION_SCREEN_SUB;
		}
		else
		{
			if( (*b_it)->active ) action |= BUTTON_ACTION_CHANGED | BUTTON_ACTION_SCREEN_SUB;
			(*b_it)->active = false;
		}
	}
	return action;
}

ButtonAction Mode::change_button_activation( touchPosition touch )
{
	ButtonAction action = BUTTON_ACTION_UNHANDLED;
	TextButton* active_button = 0;
	// handle button down events:
	for( ButtonProviderList::iterator bpi = this->button_provider_list.begin();
		bpi != this->button_provider_list.end(); bpi++ )
	{
		action |= (*bpi)->handle_touch( touch.px, touch.py, this );
		if( action & BUTTON_ACTION_EXIT_MODE ) return action;
		if( (*bpi)->current_active_button ) active_button = (*bpi)->current_active_button;
	}
	// Copy any active button from a button Provider to the GlobalButtonHandler:
	this->current_active_button = active_button;
	return action;
}

ButtonAction Mode::handle_touch_begin( touchPosition touch )
{
	return this->change_button_activation( touch );
}

ButtonAction Mode::handle_touch_drag( touchPosition touch )
{
	return this->change_button_activation( touch );
}

ButtonAction Mode::handle_touch_end( touchPosition touch )
{
	ButtonAction action = BUTTON_ACTION_UNHANDLED;
	// handle button release events:
	for( ButtonProviderList::iterator bpi = this->button_provider_list.begin();
		bpi != this->button_provider_list.end(); bpi++ )
	{
		action |= (*bpi)->handle_release( touch.px, touch.py, this );
		if( action & BUTTON_ACTION_EXIT_MODE ) return action;
	}
	return action;
}

ButtonAction Mode::handle_console_button_event( int pressed, int held, int released )
{
	ButtonAction action = BUTTON_ACTION_UNHANDLED;
	
	if( held & KEY_SELECT && pressed & KEY_UP )
	{
		// FIXME: make Error Console a real mode
		ErrorConsole::init_screen( SCREEN_MAIN );
		action |= BUTTON_ACTION_PRESSED;
	}
	if( held & KEY_SELECT && pressed & KEY_DOWN )
	{
		// FIXME: make Error Console a real mode
		ErrorConsole::init_screen( SCREEN_SUB );
		action |= BUTTON_ACTION_PRESSED;
	}
	if( held & KEY_SELECT && pressed & KEY_LEFT )
	{
		ErrorConsole::clear();
		action |= BUTTON_ACTION_PRESSED;
	}
	if( held & KEY_SELECT && pressed & KEY_RIGHT )
	{
		ErrorConsole::dump();
		action |= BUTTON_ACTION_PRESSED;
	}
	
	for( ButtonProviderList::iterator bpi = this->button_provider_list.begin();
		bpi != this->button_provider_list.end(); bpi++ )
	{
		if( *bpi != this )
		{
			action |= (*bpi)->handle_console_button_event( pressed, held, released );
			if( action & BUTTON_ACTION_EXIT_MODE ) return action;
		}
	}
	
	return action;
}

void Mode::handle_changed(ButtonAction action)
{
	if( action & (BUTTON_ACTION_CHANGED | BUTTON_ACTION_PRESSED) )
	{
		if( action & BUTTON_ACTION_SCREEN_MAIN )
			this->render( SCREEN_MAIN );
		if( action & BUTTON_ACTION_SCREEN_SUB )
			this->render( SCREEN_SUB );
	}
}

void Mode::run_until_exit()
{
	this->handle_changed( BUTTON_ACTION_CHANGED | BUTTON_ACTION_SCREEN_SUB | BUTTON_ACTION_SCREEN_MAIN );
	touchPosition old_touch;
    touchRead( &old_touch );
	bool touched = false;
    while( true )
    {
        scanKeys();
		int pressed = keysDown();
		int held = keysHeld();
		int released = keysUp();
		if( pressed || held || released )
		{
			ButtonAction action = this->handle_console_button_event( pressed, held, released );
			
			if( action & BUTTON_ACTION_EXIT_MODE ) return;
			else if( action != BUTTON_ACTION_UNHANDLED )
				this->handle_changed( action );
		}
		
		touchPosition touch;
        touchRead( &touch );
        if( keysCurrent() & KEY_TOUCH )
        {
			// handle touch begin/drag events:
			ButtonAction action = BUTTON_ACTION_UNHANDLED;
			if( !touched ) 
			{
				action |= this->handle_touch_begin( touch );
				touched = true;
			} 
			else if( (old_touch.px != touch.px) || (old_touch.py != touch.py) )
			{
				action |= this->handle_touch_drag( touch ); 
			}
			old_touch = touch;
			
			if( action & BUTTON_ACTION_EXIT_MODE ) return;
			else if( action != BUTTON_ACTION_UNHANDLED )
				this->handle_changed( action );
		}
		else if( touched )
		{
			touched = false;
			// handle touch release events:
			ButtonAction action = this->handle_touch_end( old_touch );
			
			if( action & BUTTON_ACTION_EXIT_MODE ) return;
			else if( action != BUTTON_ACTION_UNHANDLED )
				this->handle_changed( action );
		}
		else
		{
			ButtonAction action = this->handle_idle_cycles();			
			
			if( action & BUTTON_ACTION_EXIT_MODE ) return;
			else if( action != BUTTON_ACTION_UNHANDLED )
				this->handle_changed( action );
			swiWaitForVBlank();
		}
    }
}
