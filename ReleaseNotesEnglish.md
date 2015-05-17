# Chinese Touch 1.3 - Release Notes #

  * Support for context lookups in text mode and keyword lookups using a touch screen keyboard from large dictionaries like CC-CEDICT, HanDeDict, WaDoku etc.
  * More control about word list difficulty (any rating) ordering (occurrence, time, random) and scope (current lesson, including previous lessons, book, global) available from the lesson menu.
  * It is possible to assign and remove words found in dictionaries to/from lessons new word lists, using the star and trash symbols.
  * Replaced most Chinese button captions with more intuitive icons.
  * Improved loading of lesson menu entries and rendering of text lines in text mode to minimize usability impact.
  * Fallback to Japanese font for some glyphs missing in standard Chinese font. (unstable)
  * Added visualization for current mean rating per lesson/book/globally to lesson menu.
  * Added a new text and some new words to lesson 8 of English Chinese Wikibook.
  * Added ARGV support, so you can move the programs base directory to any location on your sd/flash filesystem, if your carts loader software supports this.
  * Selected character will now be rendered on the upper screen, even if no matching dictionary entry was found, so its easier to read and lookup by other means.
  * fixed many bugs; added some ui improvements


# Chinese Touch 1.2 - Release Notes #

  * Using the new down (下) and up (上) buttons in word lists and text modes you are now allowed to start/stop a new text mode for any word entry currently displayed on the upper screen. This way you can lookup unknown words in comments and browse through words sharing one of the characters easily.
  * The word list browser now allows to choose the components of word entries to be displayed, after switching to a new word in the list. This should easy memorization/association of/between different aspects of words (hanzi, pinyin, translation).
  * A more memory friendly handling of words should allow the creation of larger dictionaries (multiple thousands of words?), though this can slow down some operations slightly.
  * Synchronization of word entries from updated .dict-files now actually works, though this currently only happens when selecting the new words list from the corresponding lesson or for the whole library, when explicitly selected from the lesson menus settings screen ("s" button). Be warned that this can take up to a couple of minutes when done for many words at the same time. When there is no words.db (first run only) synchronization will start automatically at program start-up.
  * Multiple definitions of the same word (combination of characters) within the same lessons .dict-file are now handled correctly.
  * It is now possible to control whether selected words/characters in text mode should be looked up from previous/upcoming lessons or other books. ("s" button)
  * Unrated words now will appear in the very hard dynamic word list of a lesson/book by default, so you are encouraged to give a rating sooner or later.
  * Book title and lesson number will now be displayed with word entries, when no components are hidden.