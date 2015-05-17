# Chinese Touch 1.3 - Release Notes #

  * Unterstützung große Wörterbücher, wie CC-CEDICT, HanDeDict, WaDoku etc.
  * Mehr Kontrolle über Schwierigkeitsgrad (neu Wahlmöglichkeit: beliebig), Sortierreihenfolge (Vorkommen, Zeit, zufällig) und Bereich (aktuelle Lektion, einschließliche vorherige Lektionen) von aus dem Lektionsmenü aufgerufenen Wortlisten
  * Mit den neuen Stern- und Müllsymbolen lassen sich in großen Wörterbüchern gefundene Treffern dynamisch zur aktuellen Lektionsliste hinzufügen, bzw. aus dieser entfernen.
  * Chinesische Button-Beschriftungen größtenteils durch grafische Symbole ersetzt.
  * Nachladen von Lektionsmenüeinträgen und Textzeilen im Lesemodus verbessert.
  * Für im chinesischen Standard-Font fehlende japanische Zeichen auf einen speziellen japanischen Font zurückfallen. (instabil)
  * Darstellung der durchschnittlichen Vokabelbewertung pro Lektion/Buch/global im Lektionsmenü.
  * Neuer Lesetext und einige neue Vokabeln in Lektion 8 des englisch-chinesischen Wikibooks.
  * Die Unterstützung für das ARGV-Protokoll erlaubt es das Programmverzeichnis an einen beliebigen Pfad auf dem SD/Flash-Dateisystem zu legen, sofern die Kartensoftware das unterstützt.
  * Das im Lesemodus ausgewählte Zeichen wird nun immer auf dem oberen Bildschirm dargestellt, auch wenn dafür kein Treffern in einem Wörterbuch gefunden wurde. Dadurch kann man unbekannte Zeichen zumindest genau erkennen und notfalls woanders nachschlagen.
  * Darüber hinaus viele Fehler repariert und die Benutzeroberfläche verbessert.


# Chinese-Touch 1.2 - Neuerungen #

  * Mit Hilfe der neuen Runter- (下) und Hoch- (上) Knöpfe in den Wortlisten- und Textmodi ist es nun möglich einen neuen Textmodus für den aktuell dargestellten Wörterbucheintrag zu starten oder zu beenden. Dadurch wird es möglich unbekannte Wörter in Kommentaren nachzuschlagen, durch Wörter mit gemeinsamen Zeichen zu blättern usw.
  * In Wortlisten kann man nun, direkt am Anfang oder über den "s"-Knopf, auswählen welche Bestandteile des Wörterbucheintrages angezeigt werden sollen, wenn man zu einem neuen Wort wechselt. Dadurch wird es einfacher die Assoziationsmöglichkeiten zwischen den verschiedenen Bestandteilen (Hanzi, Pinyin, Übersetzung) zu trainieren.
  * Ein sparsameres Verwalten von Wörtern sollte größere Wörterbücher möglich machen (mehrere tausend Wörter?), allerdings können einige Operationen dadurch etwas langsamer werden.
  * Das Synchronisieren von Wörterbucheinträgen aus aktualisierten .dict-Dateien funktioniert nun, allerdings wird dieser Vorgang zur Zeit nur ausgelöst, wenn man die statische Vokabelliste einer zu aktualisierenden Lektion auswählt oder wenn man ihn explizit über das Einstellungsmenü ("s"-Knopf) für die gesamte Bibliothek startet. Es sei gewarnt, dass die Synchronisation bis zu einigen Minuten dauern kann, wenn viele Wörter auf einmal gelesen werden müssen. Automatisch startet der Vorgang derzeit nur beim ersten Programmstart, wenn noch keine words.db existiert.
  * Mehrfacheinträge für das gleiche chin. Wort (Zeichenkombination) innerhalb einer .dict-Datei werden nun korrekt gehandhabt.
  * Es ist nun möglich einzustellen ob ausgewählte Wörter oder Zeichen im Textmodus auch in vorherigen, nachfolgenden Lektionen oder anderen Büchern nachgeschlagen werden sollen. ("s"-Knopf)
  * Nicht bewertete Wörter erscheinen nun in der sehr schweren dynamischen Wortliste einer Lektion oder eines Buches, sodass man ermuntert wird diese früher oder später doch noch zu bewerten.
  * Buchtitel und Lektionsnummer werden nun unter dem Wörterbucheintrag dargestellt, wenn keiner der Bestandteile verborgen ist.