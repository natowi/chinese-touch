So how do you get your own word lists into your copy of Chinese Touch you may ask?

Luckily this is rather easy to accomplish. Chinese Touch automatically imports any user provided word lists from a directory structure on your flash memory card found at "YOUR-FLASH-DRIVE/chinese-touch/books/your-book-name/", assumed that YOUR-FLASH-DRIVE is the location your flash memory card appears at, when inserted into your pc, which may be a drive letter on Windows or some mount point on Linux. The string "your-book-name" is a sub directory name for your book you are free to choose. You can create any number of them. Those book directories can contain many files that are typically starting with a lesson number and having a file ending giving some hint what the files purpose is. Word lists also called dictionary files are called "LESSON-NUMBER.dict", e.g. 01.dict, 02.dict and so on. You can try to open one of the shipped dictionary files and have a look at its file structure which is not too complicated to understand and just describes some kind of table. You may copy on of those files to your own book directory and modify its contents within a text editor if you like, but there is a more convenient way to do so.

Start [OpenOffice](http://www.openoffice.org/) or StarOffice and create a new text document. Therein create a table with five columns and any number of rows. Every row describes one word definition, where column one contains the word in Chinese language, column two contains the pronunciation in pinyin, column number three takes the translation into your native language, column four is for a short abbreviated form of the word type and the fifth column takes an additional comment or example phrase. Any of the columns in exception of the first one is optional and can be left blank if you do not have or need the information. You are not even forced to create a list of Chinese words. The font shipped with Chinese Touch also supports Japanese, Korean and many Latin glyphs and could be replaced with another one if necessary.

|Creating a word list for Chinese Touch in OpenOffice|
|:---------------------------------------------------|
|![http://chinese-touch.googlecode.com/files/howto-create-word-tables-1.png](http://chinese-touch.googlecode.com/files/howto-create-word-tables-1.png)|

When your word list is completed go to the files menu and select the export dialogue. You can store your word list in a standard format e.g. .odt before you do that if you like.

|Starting the export dialogue|
|:---------------------------|
|![http://chinese-touch.googlecode.com/files/howto-create-word-tables-2.png](http://chinese-touch.googlecode.com/files/howto-create-word-tables-2.png)|

Now select the Media Wiki export filter and store your word list directly into your book directory on your flash memory card or elsewhere you can find it later. You can already give it its final name e.g. 036.dict, although the export filter will append an additional .txt ending to your file silently.

**Note:** Some versions of OpenOffice do not include a Media Wiki export filter. You can get it [here](http://wiki.services.openoffice.org/wiki/Odt2Wiki) in that case (instructions included).

|Selecting the Media Wiki export filter|
|:-------------------------------------|
|![http://chinese-touch.googlecode.com/files/howto-create-word-tables-3.png](http://chinese-touch.googlecode.com/files/howto-create-word-tables-3.png)|

To fix the file name go to the location you exported your word list to and find the newly created file, e.g. 036.dict.txt and edit the files name to 036.dict.

|Fixing the file name|
|:-------------------|
|![http://chinese-touch.googlecode.com/files/howto-create-word-tables-4.png](http://chinese-touch.googlecode.com/files/howto-create-word-tables-4.png)|
|![http://chinese-touch.googlecode.com/files/howto-create-word-tables-5.png](http://chinese-touch.googlecode.com/files/howto-create-word-tables-5.png)|

Finally you can open the newly created dictionary file with a text editor to control whether OpenOffice did its job properly. If it looks something like this, then Chinese Touch should be able to parse and display the files contents:

|![http://chinese-touch.googlecode.com/files/howto-create-word-tables-6.png](http://chinese-touch.googlecode.com/files/howto-create-word-tables-6.png)|
|:----------------------------------------------------------------------------------------------------------------------------------------------------|