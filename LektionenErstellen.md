## Einleitung ##

Wenn dir Chinese-Touch gefällt, wirst du dich vielleicht fragen wie man eigene Lektionen dafür erstellen kann. Dieser Artikel erklärt die dazu notwendigen Grundlagen und Schritte.

Chinese-Touch verwaltet dein Lehrmaterial in **Büchern** und **Lektionen**, wobei jedes Buch eine beliebige Anzahl von Lektionen enthalten kann und jede Lektion zur Zeit aus verschiedenen Texten (**Grammatik**, **Lesetext**, **Übungsaufgaben**) und einer **Wortliste** besteht.


## Verzeichnisstruktur ##

Chinese-Touch liest die oben aufgezählten Daten aus einer Verzeichnisstruktur auf der Speicherkarte, die du für deinen Nintendo-DS verwendest. Wenn du diese an deinen Rechner anschließt ergibt sich Ausgehend vom Wurzelverzeichnis der Speicherkarte folgende Verzeichnisstruktur:
| **Pfad** | **Bedeutung** |
|:---------|:--------------|
| /chinese-touch/ | Hauptverzeichnis von Chinese-Touch |
| /chinese-touch/books/ | Lehrbuchsammlung |
| /chinese-touch/books/_buchname_ | Sammlung aller zu diesem Buch gehörenden Daten |
| /chinese-touch/books/_buchname_/_buchname_.conf | Konfigurationsdatei mit Stammdaten dieses Buches |
| /chinese-touch/books/_buchname_/**01.conf** | **Stammdaten** der **Lektion 1** |
| /chinese-touch/books/_buchname_/01.**grammar** | Text mit Erklärungen zur **Grammatik**, zur Lektion 1 |
| /chinese-touch/books/_buchname_/01.**text** | **Lesetext** der Lektion 1 |
| /chinese-touch/books/_buchname_/01.**exercises** | **Übungsaufgaben** zur Lektion 1 |
| /chinese-touch/books/_buchname_/01.**dict** | **Wortliste** zur Lektion 1 |
| /chinese-touch/books/_buchname_/**02**.conf | Stammdaten der **Lektion 2** |
| /chinese-touch/books/_buchname_/_..._ | _usw._        |
| /chinese-touch/books/_anderes-buch_/_..._ | _usw._        |
| /chinese-touch/books/_..._ | _usw._        |

Die in der Tabelle aufgeführten Dateien sind tatsächlich einfache Textdateien mit einem leicht verständlichen Klartextformat, die sich mit jedem beliebigen Texteditor, der Texte in UTF-8 kodiert speichern kann, bearbeiten lassen. Die Wortlisten sind ein wenig komplizierter aufgebaut als die anderen Dateien. Man muss das Wortlistenformat aber nicht zwangsläufig verstehen, um sie selbst erstellen zu können. Das WortlistenErstellen wird in einem eigenen Artikel behandelt.

Die folgenden Tabellen beschreiben das Format der einzelnen Buch- und Lektionsdateien. Alle Dateien und Felder sind optional. Wenn Chinese-Touch eine Datei oder einen Wert nicht findet, wird die entsprechende Funktion deaktiviert bzw. die fehlende Information nicht angezeigt.


## Stammdaten ##

| **_buchname_.conf** (Stammdaten eines Buches) |
|:----------------------------------------------|
| **title**=Titel dieses Buches<br /> **description**=Kurzer Text, der beschreibt, welchen Inhalt dieses Buch hat<br /> **author**=Max Mustermann <br /> **publisher**=Name des Verlags<br /> **year**=2007 (Erscheinungsjahr) <br /> **isbn**=123-4-56789-123-4 |

| **_01_.conf** (Stammdaten der Lektion Nr. _1_) |
|:-----------------------------------------------|
| **title**=Titel der Lektion Nr. _1_ <br /> **description**=Kurzer Text, der beschreibt, was diese Lektion behandelt |


## Texte ##

Die .grammar-, .text- und .exercises-Dateien enthalten keine Schlüssel-Wert-Paare, wie die .conf-Dateien, sondern ausschließlich längere Fließtexte. Sie dürfen, wie alle von Chinese-Touch verwendeten Dateien, UTF-8-kodierte chinesische Zeichen oder andere Sonderzeichen enthalten. Explizite Zeilenumbrüche sind nicht nötig, da diese automatisch generiert werden, falls Zeichen den Bildschirmrand erreichen. Sie sind aber, z.B. zur Trennung inhaltlich abzugrenzender Abschnitte, durchaus erlaubt. Tabulator-Zeichen erzeugen eine Einrückung, die auch nach automatisch eingefügten Zeilenumbrüchen eingehalten wird. Gut lesbare Dialoge z.B. kann man also leicht erzeugen, indem man zwischen dem Namen des Sprechers und dem gesprochenen Text ein Tabulator-Zeichen einfügt.

| **_01_.grammar** (Text mit Erklärungen zur **Grammatik**, zur Lektion _1_) |
|:----------------------------------------------------------------------------|
| Ein Fließtext, der die in dieser Lektion behandelten grammatikalischen Regeln erklärt.  |

| **_01_.text** (Lesetext der Lektion _1_) |
|:-----------------------------------------|
| Ein längerer chinesischer Text. In der Regel müssen Lautumschrift oder Übersetzung nicht dazu geschrieben werden, da der Nutzer unbekannte Wörter ja auf dem Touch-Screen anklicken und so ihre Wörterbucheinträge, falls vorhanden, aufrufen kann. |

| **_01_.exercises** (Übungsaufgaben zur Lektion _1_) |
|:-----------------------------------------------------|
| Übungsaufgaben für diese Lektion. _Diese Funktion soll später noch mit interaktiven Möglichkeiten erweitert werden und hat derweil keine große Bedeutung, da man ja trotzdem Papier und Schreibstift benötigt um die Übungsaufgaben lösen zu können._  |


## Wortlisten ##

| **_01_.dict** (Wortliste zur Lektion _1_) |
|:------------------------------------------|
| _Wortlisten sind etwas komplexer und werden in einem eigenen Artikel behandelt. Siehe: WortlistenErstellen._|