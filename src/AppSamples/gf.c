#include <stdio.h>

#define GF_VERSION "1.1"

/* Function prototypes */
int get_endpoint();
int get_filename();
int set_selected();
int save_ep_url();
int dxstgfn();
int dxstgcpy();
int dxwebcpy();
int dxwebfn();
int dxseturl();
int dxgeturl();
int validate_endpoint();
int str_tolower();

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
    int len;

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

                dxseturl(file_content, len);
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
    int wg_result, bytes_written;

    wg_result = 0;
    bytes_written = 0;
    fp_input = fopen("stdin", "r");

    defaults();

    printf("\nGF (Get File) - File Transfer Utility v%s\n", GF_VERSION);
    printf("Transfer files from Azure Sphere storage or web over HTTP(s)\n");
    printf("Available flags: --help, --version, -e <url>, -f <filename>\n\n");

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
        else if (strcmp(argv[1], "-f") == 0 || strcmp(argv[1], "-F") == 0)
        {
            filename = argv[2];

            if (strlen(file_content) == 0)
            {
                printf("Error: No endpoint URL found in gf.txt. Use -e to set an endpoint first.\n");
                return -1;
            }



            printf("\nDownloading file '%s' from URL '%s'\n", filename, file_content);

            if ((fp_output = fopen(filename, "w")) == NULL)
            {
                printf("Error: Failed to create output file '%s'\n", filename);
                printf("Check disk space and write permissions.\n");
                return -1;
            }

            /* Set selected_ep to the default endpoint (3rd, starting from zero) */
            selected_ep = 2; /* Default to the third endpoint */

            dxwebfn(filename, strlen(filename), selected_ep);
            wg_result = dxwebcpy(fp_output, &bytes_written);
            if (wg_result == 0)
            {
                printf(" done (%d bytes)\n", bytes_written);
            }
            else
            {
                printf(" failed\n");
            }

            fclose(fp_output);
            if (wg_result == -1)
            {
                printf("\n\nWeb copy failed for file '%s'. Check filename and network connection\n", filename);
                unlink(filename);
            }
            return 0;
        }
        else
        {
            printf("Unrecognized option: %s\n", argv[1]);
            printf("Usage: gf [--help] [-e <url>] [-f <filename>]\n");
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

        printf("\nTransferring file '%s' from endpoint '%d'\n\n", filename, selected_ep + 1);
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
            printf("Copying from storage...");
            dxstgfn(filename);
            if (dxstgcpy(fp_output, &bytes_written) == 0)
            {
                printf(" done (%d bytes)\n", bytes_written);
            }
            else
            {
                printf(" failed\n");
            }
            break;
        case 1:
            printf("Downloading from web endpoint %d...", selected_ep + 1);
            dxwebfn(filename, strlen(filename), 0);
            wg_result = dxwebcpy(fp_output, &bytes_written);
            if (wg_result == 0)
            {
                printf(" done (%d bytes)\n", bytes_written);
            }
            else
            {
                printf(" failed\n");
            }
            break;
        case 2:
            printf("Downloading from web endpoint %d...", selected_ep + 1);
            dxwebfn(filename, strlen(filename), 1);
            wg_result = dxwebcpy(fp_output, &bytes_written);
            if (wg_result == 0)
            {
                printf(" done (%d bytes)\n", bytes_written);
            }
            else
            {
                printf(" failed\n");
            }
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

    len = 0;
    while (url[len] != '\0')
    {
        len++;
    }

    if (len < 8 || len > 120)
    {
        return -1;
    }

    /* Check for http:// or https:// */
    for (c = 0; c < 4 && c < len; c++)
    {
        check_url[c] = (url[c] >= 'a' && url[c] <= 'z') ? url[c] - 32 : url[c];
    }
    check_url[4] = '\0';

    if (check_url[0] == 'H' && check_url[1] == 'T' && check_url[2] == 'T' && check_url[3] == 'P')
    {
        /* Verify it has :// after http */
        if (len > 6 && url[4] == ':' && url[5] == '/' && url[6] == '/')
        {
            return 0;
        }
    }

    return -1;
}

int save_ep_url(endpoint)
char *endpoint;
{
    FILE *fp;

    /* write endpoint to gf.txt */
    fp = fopen("gf.txt", "w");
    if (fp != NULL)
    {
        int i;
        i = 0;
        while (endpoint[i] != '\0')
        {
            fputc(endpoint[i], fp);
            i++;
        }
        fputc('\n', fp);
        fclose(fp);
    }

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

    len = 0;
    while (filename[len] != '\0')
    {
        len++;
    }

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

    len = 0;
    while (endpoint[len] != '\0')
    {
        len++;
    }
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

int set_selected()
{
    char *endpoint;
    char url_buffer[128];
    int url_len;

    printf("Select endpoint:\n");
    printf("1) Azure Sphere immutable storage\n");
    printf("2) https://raw.githubusercontent.com/AzureSphereCloudEnabledAltair8800/RetroGames/main\n");

    url_len = dxgeturl(2, url_buffer, 128);
    if (url_len > 0)
    {
        printf("3) %s\n", url_buffer);
    }

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
