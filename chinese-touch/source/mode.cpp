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
		if( (*ki)->oam == target_oam )
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
			if( !(*b_it)->active ) action |= BUTTON_ACTION_CHANGED;
			(*b_it)->active = true;
			this->current_active_button = *b_it;
			action |= BUTTON_ACTION_ACTIVE;
		}
		else
		{
			if( (*b_it)->active ) action |= BUTTON_ACTION_CHANGED;
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
			action |= BUTTON_ACTION_CHANGED;
		}
		else
		{
			if( (*b_it)->active ) action |= BUTTON_ACTION_CHANGED;
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

void Mode::run_until_exit()
{
	this->render( SCREEN_SUB );
	touchPosition old_touch;
    touchRead( &old_touch );
	bool touched = false;
    while( true )
    {
        scanKeys();
		int pressed = keysDown();
		if( pressed && this->handle_console_button_pressed(pressed)==BUTTON_ACTION_EXIT_MODE )
		{
			return;
		}
		
		int released = keysUp();
		int held = keysHeld();
		if( held & KEY_SELECT && pressed & KEY_UP )
		{
			ErrorConsole::init_screen( SCREEN_MAIN );
		}
		if( held & KEY_SELECT && pressed & KEY_DOWN )
		{
			ErrorConsole::init_screen( SCREEN_SUB );
		}
		if( held & KEY_SELECT && pressed & KEY_LEFT )
		{
			ErrorConsole::clear();
		}
		if( held & KEY_SELECT && pressed & KEY_RIGHT )
		{
			ErrorConsole::dump();
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
				if( action & BUTTON_ACTION_EXIT_MODE ) return;
				touched = true;
			} 
			else if( (old_touch.px != touch.px) || (old_touch.py != touch.py) )
			{
				action |= this->handle_touch_drag( touch ); 
				if( action & BUTTON_ACTION_EXIT_MODE ) return;
			}
			old_touch = touch;
			// update lower screen:
			// FIXME: should we call this from the upper layer exclusivley through a "changed"-handler?
			if( action & (BUTTON_ACTION_CHANGED | BUTTON_ACTION_PRESSED) ) 
				this->render( SCREEN_SUB );
		}
		else if( touched )
		{
			touched = false;
			// handle touch release events:
			ButtonAction action = BUTTON_ACTION_UNHANDLED;
			action |= this->handle_touch_end( old_touch );
			if( action & BUTTON_ACTION_EXIT_MODE ) return;
			// update lower screen:
			// FIXME: should we call this from the upper layer exclusivley through a "changed"-handler?
			if( action & (BUTTON_ACTION_CHANGED | BUTTON_ACTION_PRESSED) )
				this->render( SCREEN_SUB );
		}
		else
		{
			if( this->handle_idle_cycles()==BUTTON_ACTION_EXIT_MODE )
			{
				return;
			}
			swiWaitForVBlank();
		}
    }
}
