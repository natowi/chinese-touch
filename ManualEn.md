# Chinese Touch 1.5 - Manual #



## Lesson Menu ##
After startup Chinese Touch will show the lesson menu. Its structure is hierarchical. You can choose from books installed in the directory "/chinese-touch/books" on your flash cartridge. Each book may contain any number of lessons. Scroll the lesson menu by dragging it up or down using your stylus or finger tip. Activate individual menu entries and buttons with short tips. Its naturally and easy to use.

<table><tr><td><img src='http://chinese-touch.googlecode.com/svn/wiki/screenshots_1_5/menu_basics.png' /></td></tr></table>

<br />
### Lesson Menu: Book Entries and Global Functions ###
Menu entries with a large book icon (![http://chinese-touch.googlecode.com/svn/trunk/chinese-touch/graphics/accessories-dictionary.png](http://chinese-touch.googlecode.com/svn/trunk/chinese-touch/graphics/accessories-dictionary.png) or ![http://chinese-touch.googlecode.com/svn/trunk/chinese-touch/graphics/accessories-dictionary-open.png](http://chinese-touch.googlecode.com/svn/trunk/chinese-touch/graphics/accessories-dictionary-open.png)) in front of them are book entries. Books may contain multiple lessons about different aspects of Chinese. Some books do not contain any lessons but a dictionary of words, spelling and translations, that you can lookup from different locations within the program. When you activate a book entry a description of the books contents will be shown on the upper screen and a couple of buttons appear on the selected book entry. See the image below for a description of each of them. Some features not specific to the selected book, but with a global scope are grouped alongside the lower edge of the bottom screen. Those are also described.

<table><tr><td><img src='http://chinese-touch.googlecode.com/svn/wiki/screenshots_1_5/menu_book_buttons.png' /></td></tr></table>

<br />
### Lesson Menu: Lesson Entries ###
Menu entries with a large paper sheet icon (![http://chinese-touch.googlecode.com/svn/trunk/chinese-touch/graphics/text-x-generic.png](http://chinese-touch.googlecode.com/svn/trunk/chinese-touch/graphics/text-x-generic.png)) in front of them are lesson entries. Lessons typically introduce a set of new words, grammar rules and provide text and exercises for practice, though each of those is optional. When you activate a lesson entry a description of the lessons contents or purpose is shown on the upper screen and a couple of buttons appear on the selected lesson entry. See the image below for a description of each of them.
<table><tr><td><img src='http://chinese-touch.googlecode.com/svn/wiki/screenshots_1_5/menu_lesson_buttons.png' /></td></tr></table>

<br />
## Word Lists ##
### Word Lists: Settings ###
When you chose a word list from the lesson menu you will be asked how the dictionary entries within the list shall be displayed. When you are learning Chinese you typically want to train memorization of different aspects of words at different times. In the example shown below the user will to try to remember spelling and meaning of Chinese words within the list, so she disables "Show Pronunciation" and "Show Translation" and then proceeds through "OK".
<table><tr><td><img src='http://chinese-touch.googlecode.com/svn/wiki/screenshots_1_5/words_settings.png' /></td></tr></table>

<br />
### Word Lists: Handwriting and Self Control ###
It is well known, that writing down things you are learning can improve memorization. In the word list mode the entire lower screen is for writing. If you think you made an error just clear the screen using the clear button on the right screen edge. Control yourself afterwards using the yellow buttons on the top edge of the screen! In the example shown below the users answer was correct. But that was an easy word, right? ;)
<table><tr><td><img src='http://chinese-touch.googlecode.com/svn/wiki/screenshots_1_5/words_writing.png' /></td></tr></table>
<br />

### Word Lists: Rating ###
<table width='60%'><tr><td><img src='http://chinese-touch.googlecode.com/svn/wiki/screenshots_1_5/words_rating.png' /></td><td>Sometimes you won't get it right. Whatever your results are, you can adjust the words difficulty rating using the rating widget at the bottom screen edge. This way you can practice hard words more often using the corresponding selectors from the lesson menu. Your rating will also influence the mean difficulty ratings shown in the lesson menu, so you will see your learning progress.<br>
</td></tr></table>
<br />

### Word Lists: Function Overview ###
You can browse word lists easily using the left/right buttons seen in the upper left and upper right corners of the screen. Your visibility settings and the writing screen will be reset/cleared automatically if you choose so from the settings menu, that is always accessible through the settings button at the lower right corner. You can exit the word list when you are ready using the exit button at the lower left corner of the screen. See the images below for a complete list of supported features.
<table><tr><td><img src='http://chinese-touch.googlecode.com/svn/wiki/screenshots_1_5/words_buttons.png' /></td></tr><tr><td><img src='http://chinese-touch.googlecode.com/svn/wiki/screenshots_1_5/words_buttons2.png' /></td></tr></table>
<br />

## Reading Text ##
When you choose grammar, text or exercises from a lesson entry, then the text browser will be started. The text browser displays longer texts which you can easily scroll using the touchscreen similar to the way you navigate in the lesson menu.
<table><tr><td><img src='http://chinese-touch.googlecode.com/svn/wiki/screenshots_1_5/text_start_and_scrolling.png' /></td></tr></table>

<br />
### Reading Text: Context Lookups ###
Tapping on a character will display context-matching word list entries on the upper screen. This makes reading texts containing many unknown characters much less time consuming and less frustrating. The user interface of the text browser is similar to the word list browser. Use the arrow buttons to browse the list of matching words, hide or show portions of the word entry and rate the difficulty of the currently shown word. By default Chinese Touch will show the longest word from the context near the character you tap on. You can reach shorter hits by browsing the hit list.
<table><tr><td><img src='http://chinese-touch.googlecode.com/svn/wiki/screenshots_1_5/text_context_lookups.png' /></td></tr></table>
### Reading Text: Character Lookups and Settings ###
If you tap again on an already activated character the highlight color will change to blue and Chinese Touch will display words containing the highlighted character starting from the shortest. Depending on the dictionary used, this often will be an entry for the character itself. You may choose the sources Chinese Touch will try to lookup word entries from, using the text mode settings menu, reachable through the settings button in the lower right.
<table><tr><td><img src='http://chinese-touch.googlecode.com/svn/wiki/screenshots_1_5/text_character_lookup_and_settings.png' /></td></tr></table>

<br />
## Dictionary Search ##
<table><tr><td><img src='http://chinese-touch.googlecode.com/svn/wiki/screenshots_1_5/fulltext_search1.png' /></td><td><img src='http://chinese-touch.googlecode.com/svn/wiki/screenshots_1_5/fulltext_search2.png' /></td></tr></table>

<br />
## Creating Content ##

<br />
### Creating Content: Books and Lessons ###

<br />
### Creating Content: Text Files ###

<br />
### Creating Content: Dynamic Word Lists ###

<br />
### Creating Content: Static Word Lists ###
see HowtoCreateWordLists

#### Static Word Lists: Synchronizing Word Database ####
After creating or changing static word lists using the steps described above you need to synchronize your word database and recreate the fulltext index for new word entries to be available. This can be done from the global settings screen of the lesson menu as shown below:
<table><tr><td><img src='http://chinese-touch.googlecode.com/svn/wiki/screenshots_1_5/menu_settings_resync_and_index.png' /></td></tr></table>

<br />
### Creating Content: Large Dictionaries ###