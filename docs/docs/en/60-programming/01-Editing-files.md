## Developer Workflow

There are two approaches to editing files for the Altair CP/M filesystem:

1. **Using External Tools (Preferred)**: You can edit files on your host machine using modern text editors or IDEs, and then transfer the files to the Altair CP/M filesystem. This approach allows for a more comfortable editing experience and better tooling support.
2. **Using the Altair Emulator**: Alternatively, you can use the built-in text editor in the Altair emulator to edit files directly on the CP/M filesystem. This method is straightforward but may be less efficient for larger changes.

## Editing Files with External Tools (Preferred)

The recommended approach is to use a modern text editor or IDE on your host machine to edit files, and then transfer them to the Altair CP/M filesystem. Here’s how you can do this:

1. **Choose a Text Editor or IDE**: Select a text editor or IDE that you are comfortable with. We'll be using [Visual Studio Code](https://code.visualstudio.com/){:target=_blank} in this example.
2. **Edit Your Files**: Make the necessary changes to your files using the chosen editor and optionally with Copilot and a Large Language Model (LLM) like OpenAI Codex or Claude Sonnet.
3. **Share your source files with a web server**: Use the [Visual Code Live Server (Five Server) extension](https://marketplace.visualstudio.com/items?itemName=ritwickdey.LiveServer){:target=_blank} to share your source files over HTTP. or alternatively, use any web server to share your files.

    ```shell
    python -m http.server 5500
    ```

4. Switch the Altair web terminal and set the http endpoint to the web server address. For example, if your web server is running on port 5500, set the endpoint to `http://<your_ip_address>:5500` using the `gf -e <your_ip_address>:5500` command.
5. Transfer your file to the Altair CP/M filesystem using the `gf -f <your_file>` command. For example, to transfer a file named `HELLO.BAS`, use the following command in the Altair CP/M command prompt:

    ```cpm
    gf -f hello.bas
    ```

6. Run the file on the Altair CP/M filesystem. For example, to run a BASIC program named `HELLO.BAS`, use the following command:

    ```cpm
    mbasic hello
    ```

### Using the CP/M SUBMIT Command to automate File Transfers

1. Set the `gf -e <your_ip_address>:5500` command to set the HTTP endpoint.
2. There are two submit files included on the `B:` drive in the Altair CP/M disk image: `asm.sub` and `c.sub`. You can use these submit files to automate the file transfer process.
3. To transfer a `c` source file, use the following command:

    ```cpm
    submit c <your_file.c>
    ```

    This will transfer the specified C source file to the Altair CP/M filesystem and then compile and link it using the BDS C compiler.

4. To transfer an assembly source file, use the following command:

    ```cpm
    submit asm <your_file.asm>
    ```

    This will transfer the specified assembly source file to the Altair CP/M filesystem and then assemble and link it using the ASM80 assembler.

## Editing Files with the Word-Master Text Editor

The Altair emulator includes the [MicroPro Word-Master](https://github.com/AzureSphereCloudEnabledAltair8800/Altair8800.manuals/blob/master/Word-Master_Manual.pdf){:target=_blank} text editor for editing documents and source code. Word-Master was advanced for its day, but by today's standards, not the most user-friendly.

### Ten-minute video introduction to editing files

The easiest way to edit files is with Visul Studio Code and then copy the file to the Altair CP/M filesystem.  Watch the video to learn more.

<iframe width="560" height="315" src="https://www.youtube.com/embed/3C_5WcSWqro" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>

### Editing files with Word-Master

It is recommended to use Visual Studio Code and the CP/M **gf** program to edit files and then copy them to the Altair filesystem. But for real retrocomputing diehards, the CP/M disk image includes the Word-Master text editor. To use Word-Master, you must switch the web terminal to character input mode.

To switch between line input mode and character input mode, select **Ctrl+L**. When you're finished with Word-Master, switch back to line input mode. Line input mode is a more efficient way for the web terminal to communicate with the Altair emulator.

![Screenshot of Altair running the Word-Master text editor.](img/word-master-character-mode.png)

For more information, view the [Word-Master user's guide](https://github.com/AzureSphereCloudEnabledAltair8800/Altair8800.manuals/blob/master/Word-Master_Manual.pdf?azure-portal=true){:target=_blank}.

The following table lists the Ctrl characters that Word-Master uses. This list is sourced from the [Experiencing the Altair 8800](https://glasstty.com/?p=1235){:target=_blank} blog.

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
