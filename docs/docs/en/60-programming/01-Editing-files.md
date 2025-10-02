The Altair emulator includes the [MicroPro Word-Master](https://github.com/AzureSphereCloudEnabledAltair8800/Altair8800.manuals/blob/master/Word-Master_Manual.pdf) text editor for editing documents and source code. Word-Master was advanced for its day, but by today's standards, not the most user-friendly.

## Ten-minute video introduction to editing files

The easiest way to edit files is with Visul Studio Code and then copy the file to the Altair CP/M filesystem.  Watch the video to learn more.

<iframe width="560" height="315" src="https://www.youtube.com/embed/3C_5WcSWqro" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>

<!-- ## Copying files to the Altair emulator

You can copy files to the Altair filesystem from a web server. The easiest way to serve files is with Visual Studio Code and the [Visual Code Live Server (Five Server) extension](https://marketplace.visualstudio.com/items?itemName=ritwickdey.LiveServer). The beauty of using Visual Studio Code is you can both edit files and web share the files with the Altair emulator.

Follow these steps to copy files to the Altair filesystem.

1. Install [Visual Studio Code](https://code.visualstudio.com/).
1. Install the [Visual Code Live Server (Five Server) extension](https://marketplace.visualstudio.com/items?itemName=ritwickdey.LiveServer).
1. Create a folder to be shared on your computer. This folder will contain the files to be copied to the Altair emulator filesystem.
1. Open the folder with Visual Studio Code
1. From Visual Studio Code, create a file named **HELLO.BAS** in the folder.
    > The CP/M filesystem limits filenames to eight characters, followed by a three-character extension, for example, FILENAME.TXT.
1. Copy the following code (including the ending blank line) to HELLO.BAS. Note that BASIC applications need to end with a blank line.

    ```basic
    10 print "Hello, world!"
    ```

1. Save the **HELLO.BAS** file.
1. Start the Live Server extension. Select **Go Live** from the Visual Studio taskbar.

    ![The image shows where the Live Share option in on the VS Code taskbar](img/select-live-share.png)

    The HTTP server starts and lists the local and network addresses for the web server. The following is an example of the addresses listed when you start Live Share.

    ```text
    Five Server running at:
    > Local:    http://localhost:5555
    > Network:  http://192.168.1.209:5555
    > Network:  http://10.211.55.2:5555
    > Network:  http://10.37.129.2:5555
    ```

    Use the **Network** address that corresponds to your network subnet. Your network subnet will likely start with **192.168.1**. -->

<!-- ## Copy a file from the Retro Games repo

1. Review the [Retro Games](https://github.com/AzureSphereCloudEnabledAltair8800/RetroGames) repo.
1. From the Altair web terminal CP/M command prompt, run the **Get File** command:

    ```cpm
    gf
    ```

1. Select endpoint 1 (GitHub)
1. Type the name of the file to be transferred. For example **LOVE.BAS**. Note, that the filenames are case sensitive.
1. Press <kbd>Enter</kbd> to start the transfer.
1. From the CP/M command line, start the game. For example

    ```cpm
    mbasic love
    ```

Note, a lot of the retro games in the repo expect to find **MENU.BAS** in the CP/M filesystem. So be sure to transfer MENU.BAS as well. -->

<!-- ## Editing files with VS Code

It's easier to edit files including Assembly, C, and BASIC code on your computer using [Visual Studio Code](https://code.visualstudio.com/), and then copy the file to the Altair filesystem using the CP/M GetFile program. -->

<!-- ### GetFile Configuration

GetFile enables you to copy files from a HTTP server to the Altair emulator filesystem. You can copy files from the retro games repo or serve files from VS Code. -->

## Editing files with Word-Master

It is recommended to use Visual Studio Code and the CP/M **gf** program to edit files and then copy them to the Altair filesystem. But for real retrocomputing diehards, the CP/M disk image includes the Word-Master text editor. To use Word-Master, you must switch the web terminal to character input mode.

To switch between line input mode and character input mode, select **Ctrl+L**. When you're finished with Word-Master, switch back to line input mode. Line input mode is a more efficient way for the web terminal to communicate with the Altair emulator.

![Screenshot of Altair running the Word-Master text editor.](img/word-master-character-mode.png)

For more information, view the [Word-Master user's guide](https://github.com/AzureSphereCloudEnabledAltair8800/Altair8800.manuals/blob/master/Word-Master_Manual.pdf?azure-portal=true).

The following table lists the Ctrl characters that Word-Master uses. This list is sourced from the [Experiencing the Altair 8800](https://glasstty.com/?p=1235) blog.

```text
VIDEO MODE SUMMARY

^O   INSERTION ON/OFF           RUB  DELETE CHR LEFT
^S   CURSOR LEFT CHAR           ^G   DELETE CHR RIGHT
^D   CURSOR RIGHT CHAR          ^\   DELETE WORD LEFT
^A   CURSOR LEFT WORD           ^T   DELETE WORD RIGHT
^F   CURSOR RIGHT WORD          ^U   DELETE LINE LEFT
^Q   CURSOR RIGHT TAB           ^K   DELETE LINE RIGHT
^E   CURSOR UP LINE             ^Y   DELETE WHOLE LINE
^X   CURSOR DOWN LINE           ^I   PUT TAB IN FILE
^^   CURSOR TOP/BOT SCREEN      ^N   PUT CRLF IN FILE
^B   CURSOR RIGHT/LEFT LINE     ^@   DO NEXT CHR 4X
^W   FILE DOWN 1 LINE           ^P   NEXT CHR IN FILE
^Z   FILE UP 1 LINE             ^V   NEXT CHR(S) TO VIDEO
^R   FILE DOWN SCREEN           ESC  EXIT VIDEO MODE
^C   FILE UP SCREEN             ^J   DISPLAY THIS

```

In character input mode, the following keyboard mappings will improve your editing experience:

```text
Keyboard key            Word-Master Ctrl Sequence
----------------------------------------------
Insert                  ^O   INSERTION ON/OFF
Delete                  ^G   DELETE CHR RIGHT
Cursor Left             ^S   CURSOR LEFT CHAR
Cursor Right            ^D   CURSOR RIGHT CHAR
Cursor Up               ^E   CURSOR UP LINE
Cursor Down             ^X   CURSOR DOWN LINE
```

## The Get File (gf) application

The gf.c application was initially written using the Word-Master text editor. Once it was working sufficiently well, the application was further refined using Visual Studio Code. The Get File (gf) application uses Intel 8080 IO Port instructions to transfer the file over HTTP to the CP/M filesystem.
