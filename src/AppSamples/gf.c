#include <stdio.h>

#define GF_VERSION "1.1"

/* Function prototypes */
int get_endpoint();
int get_filename();
int set_selected();
int save_ep_url();
int set_stg_fname();
int immutable_copy();
int web_copy();
int set_fname_web();
int validate_endpoint();
int str_tolower();

#define STG_FILENAME 68
#define STG_EOF      68
#define STG_GET_BYTE 202

#define WG_IDX_RESET 109
#define WG_EP_NAME   110
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
char file_content[128];

int defaults()
{
    FILE *fp;
    int len, c;

    /* Load endpoint from gf.txt */
    fp = fopen("gf.txt", "r");
    if (fp != NULL)
    {
        if (fgets(file_content, 128, fp) != NULL)
        {
            len = strlen(file_content);
            if (len > 1 && len < 128)
            {
                /* Remove newline */
                if (file_content[len - 1] == '\n')
                {
                    file_content[len - 1] = 0x00;
                    len--;
                }

                outp(WG_IDX_RESET, 0);
                endpoint = file_content;
                for (c = 0; c < len; c++)
                {
                    outp(WG_EP_NAME, endpoint[c]);
                }
                outp(WG_EP_NAME, 0);
            }
        }
        fclose(fp);
        printf("Default endpoint loaded from gf.txt: %s\n", file_content);
    }
    return 0;
}

int main(argc, argv)
int argc;
char **argv;
{
    int source, wg_result;

    wg_result = 0;
    fp_input = fopen("stdin", "r");

    defaults();

    printf("\nGF (Get File) - File Transfer Utility v%s\n", GF_VERSION);
    printf("Transfer files from Azure Sphere storage or web over HTTP(s)\n");
    printf("Available flags: --help, --version, -e <url>\n\n");

    if (argc == 3)
    {
        if (strcmp(argv[1], "-e") == 0 || strcmp(argv[1], "-E") == 0)
        {
            endpoint = argv[2];
            str_tolower(endpoint);
            if (validate_endpoint(endpoint) == 0)
            {
                save_ep_url(endpoint);
                printf("\nEndpoint URL set to: %s\n", endpoint);
                return 0;
            }
            else
            {
                printf("Invalid endpoint URL. Must start with 'http'.\n");
                return -1;
            }
        }
        else
        {
            printf("Unrecognized option: %s\n", argv[1]);
            printf("Usage: gf [--help] [-e <url>]\n");
            return -1;
        }
    }

    if (argc == 2)
    {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "--HELP") == 0)
        {
            printf("GF (Get File) - File Transfer Utility v%s\n", GF_VERSION);
            printf("Transfer files from Azure Sphere storage or web over HTTP(s)\n\n");
            printf("Usage: gf [--help] [--version] [-e <url>]\n");
            printf("       gf (interactive mode)\n");
            printf("\nOptions:\n");
            printf("  --help       Show this help message\n");
            printf("  --version    Show version information\n");
            printf("  -e <url>     Set a custom HTTP/HTTPS endpoint URL\n");
            printf("               The URL will be used for web-based file transfers\n");
            printf("\nExamples:\n");
            printf("  gf -e http://localhost:5500     Set local development server\n");
            printf("  gf -e https://example.com/files Set remote HTTPS endpoint\n");
            printf("  gf                              Run in interactive mode\n");
            printf("\nInteractive mode allows you to:\n");
            printf("- Choose from predefined endpoints\n");
            printf("- Transfer files from Azure Sphere storage\n");
            printf("- Transfer files from web endpoints\n");
            return 0;
        }

        if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "--VERSION") == 0)
        {
            printf("GF (Get File) version %s\n", GF_VERSION);
            printf("Compiled for BDS C 1.6 on CP/M\n");
            return 0;
        }

        /* Invalid option */
        printf("Invalid option: %s\n", argv[1]);
        printf("Use 'gf --help' for usage information.\n");
        return -1;
    }

    if (argc == 1)
    {
        while (set_selected() == -1)
        {
        }

        while (get_filename() == -1)
        {
        }

        printf("\nTransferring file '%s' from endpoint '%d'\n\n", filename, selected_ep);
    }

    if ((fp_output = fopen(filename, "w")) == NULL)
    {
        printf("Error: Failed to create output file '%s'\n", filename);
        printf("Check disk space and write permissions.\n");
        return -1;
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
    return 0;
}

int save_ep_url(endpoint)
char *endpoint;
{
    int len, c;
    FILE *fp;

    /* write endpoint to gf.txt */
    fp = fopen("gf.txt", "w");
    if (fp != NULL)
    {
        fprintf(fp, "%s\n", endpoint);
        fclose(fp);
    }

    return 0;
}

/* get personal endpoint url */
int get_ep_url(endpoint)
int endpoint;
{
    char c;
    printf("%d) ", endpoint + 1);
    outp(WG_GET_URL, 0);
    while ((c = inp(GET_STRING)) != 0)
    {
        printf("%c", c);
    }
    printf("%c", '\n');
    return 0;
}

/* Get selected endpoint URL */
int get_selected_ep()
{
    char c;
    printf("Selected endpoint: ");
    outp(WG_SELECTED, 0);
    while ((c = inp(GET_STRING)) != 0)
    {
        printf("%c", c);
    }
    printf("%c", '\n');
    return 0;
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
    return 0;
}

/* copies file byte stream from i8080 port 202 */
int immutable_copy()
{
    char ch;
    int byte_count;

    byte_count = 0;
    printf("Copying from storage");
    
    /* While not end of file read in next byte */
    while ((ch = inp(STG_EOF)) == 0)
    {
        fputc(inp(STG_GET_BYTE), fp_output);
        byte_count++;
        if ((byte_count % 50) == 0)
        {
            printf(".");
        }
    }
    printf("\nTransfer complete: %d bytes copied\n", byte_count);
    return 0;
}

/* set the selected endpoint url */
int set_selected()
{
    char *endpoint;

    printf("Select endpoint:\n");
    printf("1) Azure Sphere immutable storage\n");
    printf("2) https://raw.githubusercontent.com/AzureSphereCloudEnabledAltair8800/RetroGames/main\n");
    get_ep_url(2);

    printf("\nSelect endpoint number: ");
    endpoint = fgets(buf, 128, fp_input);

    if (strlen(endpoint) > 2)
    {
        return -1;
    }

    if (endpoint[0] >= '1' && endpoint[0] <= '3')
    {
        selected_ep = atoi(endpoint) - 1;
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
    int c;
    
    /* Set web endpoint */
    outp(WWG_SET_URL, endpoint);
    outp(WG_IDX_RESET,0);

    for (c = 0; c < len; c++)
    {
        outp(WG_FILENAME, filename[c]);
    }
    outp(WG_FILENAME, 0);
    return 0;
}

/* copies file byte stream */
int web_copy()
{
    char status;
    int byte_count, dot_count;

    byte_count = 0;
    dot_count = 0;
    printf("Downloading from web");

    while ((status = inp(WG_STATUS)) != WG_EOF)
    {
        if (status == WG_FAILED)
        {
            printf("\nWeb transfer failed\n");
            return -1;
        }

        if (status == WG_DATAREADY)
        {
            fputc(inp(WG_GET_BYTE), fp_output);
            byte_count++;
            if ((byte_count % 50) == 0)
            {
                printf(".");
                dot_count++;
                if ((dot_count % 40) == 0)
                {
                    printf("\n");
                }
            }
        }
        else if (status == WG_WAITING)
        {
            printf("*");
        }
    }

    printf("\nDownload complete: %d bytes received\n", byte_count);
    return 0;
}

int get_filename()
{
    int len, i;

    printf("Enter filename to transfer: ");
    filename = fgets(buf, 128, fp_input);

    if (filename == NULL)
    {
        printf("Error reading filename.\n");
        return -1;
    }

    len = strlen(filename);

    if (len < 2 || len > 13)
    {
        printf("Filename must be 1 to 12 characters. Try again.\n");
        return -1;
    }

    /* Remove newline */
    filename[len - 1] = 0x00;
    len--;

    /* Check for valid characters */
    for (i = 0; i < len; i++)
    {
        if (filename[i] < 32 || filename[i] > 126)
        {
            printf("Invalid character in filename. Use printable ASCII only.\n");
            return -1;
        }
    }

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

int validate_endpoint(url)
char *url;
{
    char check_url[6];
    int c;
    int len;

    if (url == NULL)
    {
        return -1;
    }

    len = strlen(url);

    if (len < 8 || len > 120)
    {
        return -1;
    }

    /* Check for http:// or https:// */
    for (c = 0; c < 4 && c < len; c++)
    {
        check_url[c] = toupper(url[c]);
    }
    check_url[4] = 0x00;

    if (strcmp(check_url, "HTTP") == 0)
    {
        /* Verify it has :// after http */
        if (len > 6 && url[4] == ':' && url[5] == '/' && url[6] == '/')
        {
            return 0;
        }
    }

    return -1;
}

int str_tolower(str)
char *str;
{
    int i;
    
    for (i = 0; str[i] != '\0'; i++)
    {
        if (str[i] >= 'A' && str[i] <= 'Z')
        {
            str[i] = str[i] + 32;
        }
    }
    return 0;
}