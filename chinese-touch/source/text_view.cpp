#include <math.h>
#include <nds/arm9/sprite.h>
#include <nds/arm9/background.h>
#include <nds/arm9/input.h>

#include "text_view.h"
#include "unicode.h"
#include "error_console.h"
#include "sprite_helper.h"
#include "words_db.h"
#include "settings_dialog.h"
#include "fulltext_search.h"

#include "bottom_right_button.h"
#include "bottom_right_button_active.h"
#include "small_top_button.h"
#include "small_top_button_active.h"
#include "loading.h"


int TextView::LINE_HEIGHT = 16;
int TextView::BUTTON_ACTIVATION_SCROLL_LIMIT = 5;
int TextView::MAX_ACCELERATION_FACTOR = 10;

TextView::TextView( Program& _program, int _recursion_depth, Text& _text )
	: Mode(_program, _recursion_depth), word_screen(SCREEN_MAIN), text_screen(SCREEN_SUB), text(_text),
		word_browser(button_provider_list, *_program.ft, current_words, text_screen, *_program.library),
		y_offset(16), v_y(0), sub_frame_count(0), current_highlight(0),
		current_highlight_x(0), current_highlight_y(0), context_mode(CONTEXT_WORDS_BY_CONTEXT),
		context_render_char(0),
		settings_button(text_screen,"s",SpriteSize_16x16,text_screen.res_x-16,text_screen.res_y-16,_program.ft->latin_face,10,1,1),
		loading_symbol(text_screen,"",SpriteSize_32x32,text_screen.res_x/2-16,text_screen.res_y/2-16,program.ft->han_face,14,0,1),
		lookup_from_current_lesson(true), lookup_from_previous_lessons(true), 
		lookup_from_upcoming_lessons(true), lookup_from_other_books(true),
		old_y_offset(0), old_abs_y_diff(0), pixels_scrolled(0), render_info( 0, 0, 0, 0 ),
		prev_size(0), org_size(0)
{
	this->init_mode();
	
	// FIXME: settings dialog item ordering relies on std::map implementation for now; don't know if this is portable
	this->settings.add_setting( new SettingsLabel("0_lookup_label","Lookup selected words...") );
	this->settings.add_setting( new BooleanSetting("1_lookup_from_current_lesson","from current Lesson",this->lookup_from_current_lesson) );
	this->settings.add_setting( new BooleanSetting("2_lookup_from_previous_lessons","from previous Lessons",this->lookup_from_previous_lessons) );
	this->settings.add_setting( new BooleanSetting("3_lookup_from_upcoming_lessons","from upcoming Lessons",this->lookup_from_upcoming_lessons) );
	this->settings.add_setting( new BooleanSetting("4_lookup_from_other_books","from other Books",this->lookup_from_other_books) );
	
	this->text_buttons.push_back( &this->settings_button );
	this->text_buttons.push_back( &this->loading_symbol );
	
	// disable child mode buttons when recursion limit is reached:
	if( this->recursion_depth>=Mode::MAX_RECURSION_DEPTH )
	{
		this->word_browser.as_text_tab.hidden = this->word_browser.as_text_tab.disabled = true;
		this->word_browser.search_button.hidden = this->word_browser.search_button.disabled = true;
		this->word_browser.stroke_order_tab.hidden = this->word_browser.stroke_order_tab.disabled = true;
	}
	// ignore touch events on loading_symbol:
	this->loading_symbol.disabled = true;
	
	this->restore_init_settings();
	
	if( !utf8_to_ucs4((unsigned char*)this->text.c_str(), this->char_list) )
    {
        WARN( "error in utf-8 input (non fatal)" );
    }
	this->org_size = this->char_list.size();
	
	this->init_vram();
}

void TextView::init_mode()
{
	this->program.ft->init_screen( this->word_screen );
	this->word_screen.clear();
	this->word_screen.clear_bg();

	this->program.ft->init_screen( this->text_screen );
	this->text_screen.clear();
	
	Mode::init_mode();
}

void TextView::init_vram()
{
	Mode::init_vram();
}
	
void TextView::init_button_vram()
{
	// load sprite graphics into vram:
	this->settings_button.init_vram( bottom_right_buttonBitmap, this->settings_button.bg_vram );
	this->settings_button.init_vram( bottom_right_button_activeBitmap, this->settings_button.bg_active_vram );
	this->loading_symbol.init_vram( loadingBitmap, this->loading_symbol.bg_vram );

	ButtonProvider::init_button_vram();
}

TextView::~TextView()
{
	if( this->current_highlight ) delete this->current_highlight;
	this->free_line_buffers();
}

void TextView::free_line_buffers()
{
	for( TextView::iterator tv_it = this->begin();
		tv_it != this->end(); tv_it++ )
	{
		if( *tv_it ) delete *tv_it;
	}
	this->clear();
}

void TextView::render( Screen screen )
{
	NewWord* new_word = 0;
	if( this->word_browser.current_word != this->word_browser.words.end() )
	{
		new_word = *this->word_browser.current_word;
	}
	if( screen == SCREEN_MAIN )
	{
		this->word_screen.clear();
		if( new_word )
		{
			new_word->render( this->program, this->word_screen, this->word_browser );
		}
		else
		{
			std::string message;
			if( !this->current_highlight )
			{
				message = "Welcome to the Text Mode!";
				message += "\n ";
				message += "\n- Grab the text to scroll up and down";
				message += "\n- Tap a character to find context \n  matching words";
			}
			else
			{
				// Render currently highlighted character
				std::string character( this->text, this->context_render_char->uc_char.source_offset, 
													this->context_render_char->uc_char.source_length );
				NewWord *char_word = new NewWord( character, "", 0 );
				char_word->render( this->program, this->word_screen, this->word_browser );
				// Add the newly created dummy word to the WordListBrowser, so that the user 
				// can access a possibly existing stroke order image or exercise writing at least:
				this->word_browser.words.push_back( char_word );
				this->word_browser.current_word = this->word_browser.words.begin();
				this->render( SCREEN_SUB ); // make widgets available for dummy word
				// Provide stateful help messages to the user:
				if( this->context_mode == CONTEXT_WORDS_BY_CONTEXT )
				{
					message = "No context matching words found :(";
					message += "\n ";
					message += "\n- Tap again to search by character";
					message += "\n- Download dictionaries from:";
					message += "\n  http://code.google.com/p/chinese-touch";
				}
				else if( this->context_mode == CONTEXT_WORDS_BY_CHARCODE )
				{
					message = "No words with that character found :(";
					message += "\n ";
					message += "\n- Tap again for context matching words";
					message += "\n- Download dictionaries from:";
					message += "\n  http://code.google.com/p/chinese-touch";
				}
			}
			RenderStyle style;
			style.center_x = style.center_y = false;
			this->program.ft->render( this->word_screen, message, this->program.ft->latin_face, 8, 5, 60, &style );
		}
	}
	else if( screen == SCREEN_SUB )
	{
		this->sub_frame_count++;
		this->settings_button.hidden = this->settings_button.disabled = !this->text.lesson;
		// make add button available for words from other books or dictionaries, to allow them to be 
		// loosely associated with the current lesson:
		this->word_browser.add_button.hidden = 
			this->word_browser.add_button.disabled = 
				!( this->text.lesson && new_word 
					&& ( (new_word->lesson && new_word->lesson->number==0) /*dictionaries*/
							|| (new_word->lesson && new_word->lesson->book!=this->text.lesson->book) /*other books*/
							|| !new_word->lesson /*lost words*/ ) );
		
		int top = this->y_offset;
		this->text_screen.clear( 1 );
		if( this->current_highlight )
		{
			// set color index 0 of the background palette, used by the character selection highlight
			if( this->context_mode == CONTEXT_WORDS_BY_CONTEXT )
			{
				this->text_screen.palette[0] = 16<<10|24<<5|31; // orange
			}
			else if( this->context_mode == CONTEXT_WORDS_BY_CHARCODE )
			{
				this->text_screen.palette[0] = 31<<10|24<<5|24; // blue
			}
			this->current_highlight->render_to( this->text_screen, 
				this->current_highlight_x, this->current_highlight_y+this->y_offset,
				/*replace=*/true );
		}
		for( TextView::iterator line_it = this->begin();
			line_it != this->end() && top<this->text_screen.res_y; 
			line_it++, top+=TextView::LINE_HEIGHT )
		{
			if( top > -TextView::LINE_HEIGHT )
			{
				(*line_it)->render_to( this->text_screen, 0, top );
				(*line_it)->top = top;
				(*line_it)->last_frame_rendered = this->sub_frame_count;
			}
		}
	}
// FIXME only update sprites when needed, during text scrolling
#if 0
	bool updates_sprites = (old_pixels_scrolled < BUTTON_ACTIVATION_SCROLL_LIMIT) 
							&& (pixels_scrolled >= BUTTON_ACTIVATION_SCROLL_LIMIT);
#endif
	Mode::render( screen );
}

ButtonAction TextView::handle_button_pressed( TextButton* text_button )
{
	if( text_button == &this->settings_button )
	{
		this->show_settings();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_MAIN | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->word_browser.as_text_tab
		&& this->word_browser.current_word!=this->word_browser.words.end() )
	{
		this->free_vram();
		TextView::show_word_as_text( this->program, *this->word_browser.current_word, this->text.lesson, this->recursion_depth );
		this->init_mode();
		this->init_vram();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_MAIN | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->word_browser.search_button )
	{
		this->free_vram();
		FulltextSearch *fulltext_search = new FulltextSearch( this->program, this->recursion_depth, this->text.lesson );
		// explicitly replace exit button with dog-ear to show user, that she is in a sub mode
		fulltext_search->word_browser.exit_button.hidden = fulltext_search->word_browser.exit_button.disabled = true;
		fulltext_search->word_browser.dogear.hidden = fulltext_search->word_browser.dogear.disabled = false;
		fulltext_search->run_until_exit();
		delete fulltext_search;
		this->init_mode();
		this->init_vram();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_MAIN | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->word_browser.add_button
		&& this->word_browser.current_word!=this->word_browser.words.end()
		&& this->text.lesson )
	{
		NewWord* new_word = *this->word_browser.current_word;
		// loosely associate word with current lesson:
		new_word->lesson = this->text.lesson;
		new_word->id = new_word->file_id = new_word->file_offset = 0;
		// HACK: set duplicate_id to an unusual high value, to prevent overwriting duplicate words from *.dict-files
		// FIXME: this might overwrite previously associated duplicate words
		new_word->duplicate_id = 1000;
		this->program.words_db->add_or_write_word( *new_word );
		this->program.words_db->read_word( *new_word );
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_SUB;
	}
	if( text_button == &this->word_browser.stroke_order_tab 
		&& this->word_browser.current_word!=this->word_browser.words.end() )
	{
		this->free_vram();
		NewWordList *single_word_list = new NewWordList();
		single_word_list->push_back( *this->word_browser.current_word );
		NewWordsViewer *word_viewer = new NewWordsViewer( this->program, this->recursion_depth, *single_word_list, 
														  false /*no position saving*/, false /*no shuffle*/, 
														  false /*don't show settings*/ );
		word_viewer->word_browser.toggle_stroke_order();
		// explicitly replace exit button with dog-ear to show user, that she is in a sub mode
		word_viewer->word_browser.exit_button.hidden = word_viewer->word_browser.exit_button.disabled = true;
		word_viewer->word_browser.dogear.hidden = word_viewer->word_browser.dogear.disabled = false;
		word_viewer->run_until_exit();
		delete word_viewer;
		single_word_list->erase( single_word_list->begin(), single_word_list->end() );
		delete single_word_list;
		this->init_mode();
		this->init_vram();
		return BUTTON_ACTION_PRESSED | BUTTON_ACTION_SCREEN_MAIN | BUTTON_ACTION_SCREEN_SUB;
	}
	
	return ButtonProvider::handle_button_pressed( text_button );
}

ButtonAction TextView::handle_touch_begin(touchPosition touch)
{
	this->old_touch = touch;
	this->pixels_scrolled = 0;
	this->old_y_offset = this->y_offset;
	this->loading_symbol.hidden = true;
	
	return Mode::handle_touch_begin(touch);
}

ButtonAction TextView::handle_touch_drag( touchPosition touch )
{
	ButtonAction action = BUTTON_ACTION_UNHANDLED;
	// check for touch screen button activation, but only when the user 
	// is not already scrolling text:
	if( this->pixels_scrolled < BUTTON_ACTIVATION_SCROLL_LIMIT )
		action |= Mode::handle_touch_drag( touch );
	// do text scrolling, but only if we are not currently hovering a 
	// touch screen button:
	int y_diff = touch.py - old_touch.py;
	this->old_touch = touch;
	if( action == BUTTON_ACTION_UNHANDLED )
	{
		int abs_y_diff = abs( y_diff );
		if( abs_y_diff && ((old_abs_y_diff && (abs_y_diff <= old_abs_y_diff*TextView::MAX_ACCELERATION_FACTOR)) 
						|| (abs_y_diff <= TextView::MAX_ACCELERATION_FACTOR)) )
		{
			old_abs_y_diff = abs_y_diff;
			pixels_scrolled += abs(y_diff);
			this->y_offset += y_diff;
			this->v_y = y_diff;
			// FIXME: force render() not to updates sprites, to make scrolling effect faster
			this->render( SCREEN_SUB );
			return BUTTON_ACTION_CHANGED;
		}
	}
	
	return action;
}

ButtonAction TextView::handle_touch_end( touchPosition touch )
{
	ButtonAction action = BUTTON_ACTION_UNHANDLED;
	// ignore any touch screen buttons and characters currently pointed on,
	// when the user was scrolling text, so she does not press a button 
	// by accident:
	if( this->pixels_scrolled >= BUTTON_ACTIVATION_SCROLL_LIMIT )
		return BUTTON_ACTION_UNHANDLED;
	// else priority check normal touch screen buttons for a pressed event:
	else action |= Mode::handle_touch_end( touch );
	// if no other button was activated, fall back to character tap detection:
	if( action == BUTTON_ACTION_UNHANDLED )
	{
		// Dictionary nach relevanten Treffern durchsuchen:
		bool found = false;
		TextView::iterator prev_line_it = this->end();
		for( TextView::iterator line_it = this->begin();
			!found && line_it != this->end() && (*line_it)->top<this->text_screen.res_y; 
			prev_line_it=line_it++ )
		{
			BufferedLine* line = *line_it;
			if( line->last_frame_rendered==this->sub_frame_count
				&& old_touch.py > line->top  
				&& old_touch.py < line->top+TextView::LINE_HEIGHT  )
			{
				// create and fill tempory search list containing characters of 3 successive lines
				// for multi line context matches:
				UCCharList search_char_list;
				UCCharList::iterator search_char_it = search_char_list.begin();
				typedef std::list<TextView::iterator> SearchLineIterators;
				SearchLineIterators search_line_its;
				if( prev_line_it!=this->end() ) search_line_its.push_back( prev_line_it );
				search_line_its.push_back( line_it );
				TextView::iterator next_line_it = line_it; next_line_it++;
				if( next_line_it!=this->end() ) search_line_its.push_back( next_line_it );
				for( SearchLineIterators::iterator sli = search_line_its.begin();
					sli != search_line_its.end();
					sli++ )
				{
					for( RenderCharList::iterator r_it = (*(*sli))->render_char_list.begin();
						r_it != (*(*sli))->render_char_list.end();
						r_it++ )
					{
						search_char_list.push_back( (*r_it)->uc_char );
						// initiate search character position to first character of current line:
						if( r_it == line->render_char_list.begin() )
						{
							search_char_it = search_char_list.end(); search_char_it--;
						}
					}
				}
				// find character clicked on:
				for( RenderCharList::iterator char_it = line->render_char_list.begin();
					!found && char_it != line->render_char_list.end();
					char_it++, search_char_it++ )
				{
					RenderChar* curr_char = *char_it;
					if( old_touch.px > curr_char->x && old_touch.px < curr_char->x+curr_char->width )
					{
						action |= BUTTON_ACTION_PRESSED;
						if( this->context_render_char != curr_char 
							|| this->context_mode != CONTEXT_WORDS_BY_CONTEXT )
						{
							// first click (odd count) on a character: find words in current context:
							this->program.library->find_words_by_context( this->text, search_char_list, search_char_it, 6, this->word_browser.words, this->lookup_sql_cond, this->lookup_from_other_books );
							this->context_mode = CONTEXT_WORDS_BY_CONTEXT;
						}
						else
						{
							// second click (even count) on a character: find words containing selected character:
							std::string character( this->text, curr_char->uc_char.source_offset, curr_char->uc_char.source_length );
							this->program.library->find_words_by_characters( character, this->word_browser.words, this->lookup_sql_cond, this->lookup_from_other_books );
							this->context_mode = CONTEXT_WORDS_BY_CHARCODE;
						}
						this->context_render_char = curr_char;
						this->word_browser.current_word = this->word_browser.words.begin();
						// (re-)define and render highlight for selected character in text view:
						if( this->current_highlight ) delete this->current_highlight;
						this->current_highlight = new RenderScreenBuffer( curr_char->width, TextView::LINE_HEIGHT );
						this->current_highlight->clear();
						this->current_highlight_x = curr_char->x;
						this->current_highlight_y = line->top-this->y_offset+curr_char->y-1;
						this->render( SCREEN_SUB );
						// render first search result on main screen:
						this->render( SCREEN_MAIN );
						found = true;
						break;
					}
				}
			}
		}
	}
	
	return action;
}

ButtonAction TextView::handle_idle_cycles()
{
	this->pixels_scrolled = 0;
	/* check if there is still momentum from a previous text scrolling input 
		and animate the text based on simple physics */
	if( this->v_y )
	{
		int resistance = this->v_y / 4;
		if( !resistance ) resistance = this->v_y / 2;
		if( !resistance ) resistance = this->v_y;
		this->v_y -= resistance;
		this->y_offset += this->v_y;
		// FIXME: force render() not to updates sprites, to make scrolling effect faster
		this->render( SCREEN_SUB );
		return BUTTON_ACTION_CHANGED;
	}
	// render missing text lines until no characters where consumed within two consecutive iterations:
	else if( (this->prev_size != this->char_list.size())
				&& ((int)(this->size() * TextView::LINE_HEIGHT + this->y_offset)
					<= this->text_screen.res_y) ) 
	{
		if( this->loading_symbol.hidden )
		{
			this->loading_symbol.hidden = false;
			this->render( SCREEN_SUB );
		}
		else
		{
			this->loading_symbol.bg_rotation = (this->loading_symbol.bg_rotation > 0) ?  (this->loading_symbol.bg_rotation - 1) : 31;
		}
		this->prev_size = this->char_list.size();
		BufferedLine* buffered_line = new BufferedLine();
		RenderStyle render_style;
		render_style.multiline = false;
		render_style.indentation_offset = this->render_info.indentation_offset;
		this->render_info = this->program.ft->render( *buffered_line, this->char_list, 
			this->program.ft->latin_face, 8, 0, 0, &render_style, &buffered_line->render_char_list );
		this->push_back( buffered_line );
		this->render( SCREEN_SUB );
	}
	else
	{
		if( !this->loading_symbol.hidden )
		{
			this->loading_symbol.hidden = true;
			this->render( SCREEN_SUB );
		}
	}
	
	return Mode::handle_idle_cycles();
}

void TextView::show_word_as_text( Program& program, NewWord* word, Lesson* lesson, int recursion_depth )
{
	if( !word )
		return;
	Text new_text( "Word Entry", lesson );
	new_text += word->hanzi;
	if( word->hanzi.length() > 30 ) // n bytes, not characters
		new_text += "\n\n";
	new_text += " \t("+ word->pinyin + ")\n\n";
	for( Definitions::iterator di = word->definitions.begin(); di != word->definitions.end(); di++ )
	{
		//new_text += "\nDefinition (" + di->second->lang + "):\n";
		if( di->second->word_type.length() && di->second->word_type.find_first_not_of(" \t\n\r")!=std::string::npos )
			new_text += "-\t[" + di->second->word_type + "]\n\n";
		if( di->second->translation.length() && di->second->translation.find_first_not_of(" \t\n\r")!=std::string::npos )
			new_text += "*\t"+di->second->translation + "\n\n";
		if( di->second->comment.length() && di->second->comment.find_first_not_of(" \t\n\r")!=std::string::npos )
			new_text += "+\t"+di->second->comment + "\n\n";
		if( di->second->example.length() && di->second->example.find_first_not_of(" \t\n\r")!=std::string::npos )
			new_text += "#\t"+di->second->example + "\n\n";
	}
	TextView* text_view = new TextView( program, recursion_depth, new_text );
	// explicitly replace exit button with dog-ear to show user, that she is in a sub mode
	text_view->word_browser.exit_button.hidden = text_view->word_browser.exit_button.disabled = true;
	text_view->word_browser.dogear.hidden = text_view->word_browser.dogear.disabled = false;
	text_view->run_until_exit();
	delete text_view;
}

void TextView::show_settings()
{
	this->free_vram();
	SettingsDialog* settings_dialog = new SettingsDialog( this->program, this->recursion_depth, this->settings, "Text Viewer Settings" );
	settings_dialog->run_until_exit();
	delete settings_dialog;
	this->restore_init_settings();
	this->init_mode();
	this->init_vram();
}

void TextView::restore_init_settings()
{
	bool or_needed = false;
	std::stringstream extra_sql_cond;
	if( this->text.lesson && this->text.lesson->book )
	{
		if( this->lookup_from_current_lesson )
		{
			if( or_needed ) extra_sql_cond << " or ";
			extra_sql_cond << "book_id=" << this->text.lesson->book->id << " and lesson_number=" << this->text.lesson->number;
			or_needed = true;
		}
		if( this->lookup_from_previous_lessons )
		{
			if( or_needed ) extra_sql_cond << " or ";
			extra_sql_cond << "book_id=" << this->text.lesson->book->id << " and lesson_number<" << this->text.lesson->number;
			or_needed = true;
		}
		if( this->lookup_from_upcoming_lessons )
		{
			if( or_needed ) extra_sql_cond << " or ";
			extra_sql_cond << "book_id=" << this->text.lesson->book->id << " and lesson_number>" << this->text.lesson->number;
			or_needed = true;
		}
		if( this->lookup_from_other_books )
		{
			if( or_needed ) extra_sql_cond << " or ";
			extra_sql_cond << "book_id!=" << this->text.lesson->book->id << " or book_id is Null";
			or_needed = true;
		}
		// add impossible where predicate if user wants no lookups at all:
		if( !extra_sql_cond.str().length() )
		{
			extra_sql_cond << "1=0";
		}
	}
	this->lookup_sql_cond = extra_sql_cond.str();
}
