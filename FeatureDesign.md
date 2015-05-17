# Feature Design #
This is a list of design studies on planned program features, including graphical interface design and basic concepts of how stuff should work and why.

## Historic Stuff ##
[A series of slides](http://m20s08.vip-server.net/chinesisch/00_dummy.png.html) (external link) showing an older (pre-implementation) concept and UI design study for the character writing mode. The images where drawn in Gimp. Slide comments are in German. Most of the features seen in the mock-up are already implemented. One exception is the background grid which was considered less important and delayed for a later release. Another one is the dynamic, anti-aliased pen style, which would be nice to have. Tough I currently have no real concept for an implementation and do not want to play around.

## Flashcard Integration ##
Efficient character and word memorization could be achieved through a dynamic [flashcard](http://en.wikipedia.org/wiki/Flashcard) mode. One requirement is, that it has to fit into the book and lesson based learning scheme used so far.
The flashcard concept itself is very generic and allows any kind of information to be "stored" on ether side of a card. In CT I think it would be o.k. to restrict the cards structure to the one already seen in the writing and reading modes. This decision might prohibit some use cases, but allows semantic integration into other parts of the application.
Some kind of integration is desirable, because pure use of flashcards can be boring and frustrating, while a combination with reading of text containing a limited set of new words can be motivating. Consequentially in CT it would make sense to enable the user to manage different sets of flashcards dynamically by adding and removing individual words from a text or a static word list belonging to a lesson or by adding and removing whole sets of words from a lesson or a complete book.

New widgets and menus are required for management and use of flashcard sets. A slightly enhanced version of NewWordsViewer can be re-used for the actual flashcard mode.

  * TextView/NewWordsViewer:
    * button to add currently selected word to a flashcard set
      * needs to popup a quick menu of available flashcard sets including an option to create a new one
    * rating widget for word/flashcard difficulty
      * semi-hidden at right side of the touch screen?
      * stars, emoticons?
      * first step: simple per word rating
      * <img src='http://chinese-touch.googlecode.com/files/smilie_tab.png' /><img src='http://chinese-touch.googlecode.com/files/smilie_tab2.png' />
      * <img src='http://chinese-touch.googlecode.com/files/smilie_tab_detail_levels.png' />
      * The retractable side bar might be ok for NewWordsViewer but it could be anoying in TextView when covering the right most characters of some lines. I have to test this. An alternative position for the rating widget would be the lower screen edge. The top edge would be perfect in TextView, because it is next to the top screen that shows the selected word that is to be rated. But it is already in use by the hide/show selectors in NewWordsViewer...
      * <img src='http://chinese-touch.googlecode.com/files/smilie_tab3.png' /><img src='http://chinese-touch.googlecode.com/files/smilie_tab_bottom_detail.png' />
      * The start next to the rating bar could be used to summon a list of flashcard sets the currently selected word could be added to or removed from. Thus the flashcard set menu has to be some kind of hybrid checkbox-list widget visualizing which flashcard sets the current word is already assigned to and which not.
      * The emoticon an the upper right corner of the main screen shows the current rating for that word. It can be visible anytime, so the user knows about the current rating even if the rating bar on the sub screen is in retracted state.
  * NewWordsViewer (if flashcards use NewWordsViewer as a backend):
    * button to remove word from a flashcard set it is currently assigend to
      * needs to popup a quick menu of flashcard sets this word is currently assigned to
  * FlashcardMenu: list of currently managed flashcard sets
    * merge FlashcardMenu with LessonMenu?
      * display flashcards as one book and each currently managed flashcard set as a lesson of the virtual "flashcard book"?
    * buttons per flashcard set to:
      * browse whole set
      * start test with easy/medium/hard difficulty

## Settings Screens for Word List Browser and Text Mode ##
  * Concept rendering for the word list browser configuration: |![http://chinese-touch.googlecode.com/files/word_list_settings.png](http://chinese-touch.googlecode.com/files/word_list_settings.png)|![http://chinese-touch.googlecode.com/files/word_list_settings-2.png](http://chinese-touch.googlecode.com/files/word_list_settings-2.png)|
|:------------------------------------------------------------------------------------------------------------------------------------|:----------------------------------------------------------------------------------------------------------------------------------------|
    * should automatically pop-up after a (dynamic) word list has been loaded
  * A similar thing can be done for the text mode: |![http://chinese-touch.googlecode.com/svn/trunk/chinese-touch/blueprints/text_mode_settings.png](http://chinese-touch.googlecode.com/svn/trunk/chinese-touch/blueprints/text_mode_settings.png)|
    * The text mode configuration screen should not pop-up every time the text mode is started.
    * A reasonable default setting would be to lookup words from the current lesson and previous lessons. That way Learners would not be confused by alternative definitions from upcoming lessons and other books that could be from a different language in the worst case.
  * A small button should be provided that allows to enter the settings from within the word list and the text mode, when needed.