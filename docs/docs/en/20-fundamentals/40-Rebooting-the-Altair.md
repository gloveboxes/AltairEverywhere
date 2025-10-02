# Rebooting the emulator

There are various reasons why you might want to reboot the Altair operating system.

1. You may *crash* the Altair operating system if you hack the current CPU instruction pointer or corrupt RAM. You might corrupt memory while programming in Assembly or with the virtual front panel.
1. You might have tried to access a drive letter that is not available. The only way to recover is to reboot the Altair.
1. You want to boot Altair BASIC and run the original Altair BASIC written by Bill Gates and Paul Allan.

You'll be glad to know it's easy to reboot the Altair.

## Reboot CP/M

1. From the web terminal, select **Ctrl+M** to enter the CPU monitor.
1. Type **r** (for Altair RESET), followed by <kbd>Enter</kbd> to boot CP/M.

## Reboot Altair BASIC

By default, the Altair emulator loads CP/M on startup. To load Altair BASIC, follow these instructions.

1. From the web terminal, select **Ctrl+M** to enter the CPU monitor.
1. Type **basic** (for Altair BASIC), followed by <kbd>Enter</kbd> to boot Altair BASIC.
1. You'll be prompted for the following information:

    * **MEMORY SIZE?**: Select the Enter key to accept the default.
    * **TERMINAL  WIDTH?**: Select Enter to accept the default.
    * **WANT SIN-COS-TAN-ATN?**: Enter `Y` or `N`.

    Altair BASIC responds with the amount of memory and version information.

For more information about Altair BASIC, refer to [Altair-BASIC-programming](../60-programming/05-Altair-BASIC-programming.md)
