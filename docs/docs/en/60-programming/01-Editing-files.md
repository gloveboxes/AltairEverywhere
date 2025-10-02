# Editing files

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

```c
#include <stdio.h>
#define STG_FILENAME 68
#define STG_EOF      68
#define STG_GET_BYTE 202

#define WG_EPNAME   110
#define WG_GET_URL  111
#define WWG_SET_URL 112
#define WG_SELECTED 113
#define WG_FILENAME 114
#define WG_GET_BYTE 201
#define WG_STATUS   33

#define WG_EOF       0
#define WG_WAITING   1
#define WG_DATAREADY 2
#define WG_FAILED    3

#define GET_STRING 200

FILE *fp_input;
FILE *fp_output;
char *endpoint;
char *filename;
char buf[128];
int selected_ep;

main(argc, argv) char **argv;
int argc;
{
    int source;
    int wg_result;

    fp_input = fopen("stdin", "r");

    printf("\nFile transfer utility.\n");
    printf("Transfer a file from Azure Sphere storage or the web over HTTP(s).\n\n");

    if (argc == 2)
    {
        while (get_endpoint() == -1)
        {
        }

        set_ep_url(endpoint);
        printf("\nEndpoint URL set to: %s\n", endpoint);
        exit();
    }

    if (argc == 1)
    {
        while (set_selected() == -1)
        {
        }

        while (get_filename() == -1)

            printf("\nTransferring file '%s' from endpoint '%d'\n\n", filename, selected_ep);
    }

    if ((fp_output = fopen(filename, "w")) == NULL)
    {
        printf("Failed to open %s\n", filename);
        exit();
    }

    switch (selected_ep)
    {
        case 0:
            set_stg_fname(filename);
            immutable_copy();
            break;
        case 1:
            set_fname_web(filename, strlen(filename), 0);
            wg_result = web_copy();
            break;
        case 2:
            set_fname_web(filename, strlen(filename), 1);
            wg_result = web_copy();
            break;
        default:
            break;
    }

    fclose(fp_output);
    if (wg_result == -1)
    {
        printf("\n\nWeb copy failed for file '%s'. Check filename and network connection\n", filename);
        unlink(filename);
    }
}

void set_ep_url(endpoint) char *endpoint;
{
    int len;
    int c;
    len = strlen(endpoint);
    for (c = 0; c < len; c++)
    {
        outp(WG_EPNAME, endpoint[c]);
    }

    outp(WG_EPNAME, 0);
}

/* get personal endpoint url */
void get_ep_url(endpoint) int endpoint;
{
    printf("2) ");
    char c;
    outp(WG_GET_URL, 0);
    while ((c = inp(GET_STRING)) != 0)
    {
        printf("%c", c);
    }
    printf("%c", '\n');
}

/* Get selected endpoint URL */
void get_selected_ep()
{
    printf("Selected endpoint: ");
    char c;
    outp(WG_SELECTED, 0);
    while ((c = inp(GET_STRING)) != 0)
    {
        printf("%c", c);
    }
    printf("%c", '\n');
}

/* Sets the filename to be copied with i8080 port 68 */
int set_stg_fname(filename)
char *filename;
{
    int c;
    int len;

    len = strlen(filename);

    for (c = 0; c < len; c++)
    {
        outp(STG_FILENAME, filename[c]);
    }
    outp(STG_FILENAME, 0);
    printf("filename set\n");
}

/* copies file byte stream from i8080 port 202 */
void immutable_copy()
{
    char ch;

    /* While not end of file read in next byte */
    while ((ch = inp(STG_EOF)) == 0)
    {
        fputc(inp(STG_GET_BYTE), fp_output);
        printf(".");
    }
}

/* set the selected endpoint url */
int set_selected()
{
    char *endpoint;

    printf("Select endpoint:\n");
    printf("0) Azure Sphere immutable storage\n");
    printf("1) https://raw.githubusercontent.com/AzureSphereCloudEnabledAltair8800/RetroGames/main\n");
    get_ep_url(1);

    printf("\nSelect endpoint number: ");
    endpoint = fgets(buf, 128, fp_input);

    if (strlen(endpoint) > 2)
    {
        return -1;
    }

    if (endpoint[0] >= '0' && endpoint[0] <= '2')
    {
        selected_ep = atoi(endpoint);
        return 0;
    }
    return -1;
}

/* Sets the filename to be copied with i8080 port 68 */
int set_fname_web(filename, len, endpoint)
char *filename;
int len;
int endpoint;
{
    /* Set web endpoint */
    outp(WWG_SET_URL, endpoint);

    int c;
    for (c = 0; c < len; c++)
    {
        outp(WG_FILENAME, filename[c]);
    }
    outp(WG_FILENAME, 0);
}

/* copies file byte stream */
int web_copy()
{
    char status;

    while ((status = inp(WG_STATUS)) != WG_EOF)
    {
        if (status == WG_FAILED)
        {
            return -1;
        }

        if (status == WG_DATAREADY)
        {
            fputc(inp(WG_GET_BYTE), fp_output);
            printf(".");
        }
        else
        {
            printf("*");
        }
    }

    return 0;
}

int get_filename()
{
    int len;

    printf("Enter filename to transfer: ");
    filename = fgets(buf, 128, fp_input);

    len = strlen(filename);

    if (len < 1 || len > 13)
    {
        printf("Filename must be 1 to 12 characters. Try again.\n");
        return -1;
    }

    filename[len - 1] = 0x00;

    return 1;
}

int get_endpoint()
{
    char url[5];
    int c;
    int len;

    printf("Enter endpoint url: ");

    endpoint = fgets(buf, 128, fp_input);

    len = strlen(endpoint);
    len--;
    endpoint[len] = 0x00;

    if (len < 8)
    {
        printf("Invalid endpoint url.\n");
        return -1;
    }

    for (c = 0; c < 4; c++)
    {
        url[c] = toupper(endpoint[c]);
    }
    url[4] = 0x00;

    if (strcmp(url, "HTTP"))
    {
        printf("Invalid endpoint url. Must start with 'http'.\n");
        return -1;
    }

    return 0;
}
```
