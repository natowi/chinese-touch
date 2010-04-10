#include "mode.h"
#include "greys256.h"
#include "error_console.h"


void Mode::init_mode()
{
	// prepare lower screen for sprite usage
	vramSetBankD( VRAM_D_SUB_SPRITE );
	oamInit( &oamSub, SpriteMapping_Bmp_1D_128, 0 );
	//oamAllocReset( &oamSub );
	oamEnable( &oamSub );
}

void Mode::init_vram()
{
	for( TextButtonList::iterator i=this->text_buttons.begin(); i!=this->text_buttons.end(); i++ )
	{
		(*i)->init_text_layer( this->freetype_renderer );
	}
	// load gray scale sprite palette for button text labels
	dmaCopy( greys256Pal, SPRITE_PALETTE_SUB, 256*2 );
}

void Mode::free_vram()
{
	this->text_buttons.free_all();
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
	for( TextButtonList::iterator ki = this->text_buttons.begin(); ki != this->text_buttons.end(); ki++ )
	{
		if( (*ki)->oam == target_oam )
			(*ki)->render_to( oam_entry );
	}
	swiWaitForVBlank();
	oamUpdate( target_oam );
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
			if( !touched ) 
			{
				touched = true;
				old_touch = touch;
			}
			bool changed = false;
			for( TextButtonList::iterator b_it = this->text_buttons.begin(); 
					b_it != this->text_buttons.end(); b_it++ )
			{
				if( (*b_it)->is_responsible(touch.px, touch.py) )
				{
					changed |= !(*b_it)->active;
					(*b_it)->active = true;
				}
				else
				{
					changed |= (*b_it)->active;
					(*b_it)->active = false;
				}
			}
			if( changed ) 
				this->render( SCREEN_SUB );
			old_touch = touch;
		}
		else if( touched )
		{
			touched = false;
			for( TextButtonList::iterator b_it = this->text_buttons.begin(); 
					b_it != this->text_buttons.end(); b_it++ )
			{
				if( (*b_it)->is_responsible(old_touch.px, old_touch.py)
					&& (*b_it)->active )
				{
					(*b_it)->active = false;
					ButtonAction action = this->handle_button_pressed( *b_it );
					if( action == BUTTON_ACTION_EXIT_MODE )
					{
						return;
					}
					this->render( SCREEN_SUB );
				}
			}
		}
		else
		{
			swiWaitForVBlank();
			touched = false;
			bool changed = false;
			for( TextButtonList::iterator i=this->text_buttons.begin(); 
				i!=this->text_buttons.end(); i++ )
			{
				changed |= (*i)->active;
				(*i)->active = false;
			}
			if( changed )
				this->render( SCREEN_SUB );
		}
    }
}
