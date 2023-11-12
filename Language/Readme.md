# How to load a language file

1. The directory with languages (named `Language`) must be in same directory as PE-bear executable, or in User Data Directory (`Settings` -> `Configure...` -> `User Data Directory`).
2. Create a subdirectory with the name of the language version that you want to add (i.e. `zh_CN`)
3. Rename the Language file to `PELanguage.qm` and put it into the created folder
4. Restart PE-bear. Now, you should see the added language under `Settings` -> `Configure...` -> `Language`.
5. Select your language from the list, and restart PE-bear. The interface should be updated to the new language.

# How to translate
Download the language file, use QT linguist to load and select the language to be translated, and click Publish to generate qm file after translation.
# reference data
* https://doc.qt.io/qt-5/qtlinguist-index.html
* https://doc.qt.io/qt-5/linguist-translators.html
