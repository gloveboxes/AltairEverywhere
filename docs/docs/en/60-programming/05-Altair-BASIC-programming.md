# Altair BASIC programming

Bill Gates and Paul Allen wrote [Altair BASIC](https://en.wikipedia.org/wiki/Altair_BASIC?azure-portal=true). It was Microsoft's first product. At the time, [Altair BASIC](https://en.wikipedia.org/wiki/Altair_BASIC) was a huge step forward as it allowed people to write programs using a high-level programming language. For more information about Altair BASIC, see the [Altair BASIC reference manual](https://github.com/AzureSphereCloudEnabledAltair8800/Altair8800.manuals/blob/master/MITS_Altair8800Basic4.1Reference_April1977.pdf).

By default Altair emulator boots CP/M. CP/M is more flexible and you can save files. That said, it's fun to fire up the original Altair BASIC, just keep in mind that you can't save Altair BASIC applications.

## Boot Altair BASIC

To load Altair BASIC, follow these instructions.

1. From the web terminal, select **Ctrl+M** to enter the CPU monitor.
1. Type **basic**, followed by <kbd>Enter</kbd> to boot Altair BASIC.
1. You will be prompted for the following information:

    * **MEMORY SIZE?**: Select the Enter key to accept the default.
    * **TERMINAL  WIDTH?**: Select the Enter key to accept the default.
    * **WANT SIN-COS-TAN-ATN?**: Enter `Y` or `N`.

    Altair BASIC responds with the amount of memory and version information.

## Write and run an Altair BASIC app

1. From the web terminal, enter the following code to create an Altair BASIC application:

   ```basic
   10 for i = 1 to 1000
   20 print i
   30 next i
   ```

1. Run the program by entering the following command:

   ```basic
   run
   ```

   Your program counts to 1,000.

## Learn useful commands

As you work with Altair BASIC, you'll likely use these commands often:

* **loadx** : Loads a sample application into MBASIC memory
* **list** : Lists application code
* **run** : Runs the loaded application
* **new** : Clears the current application from MBASIC memory

Also, remember that you can use the **Ctrl+C** keyboard shortcut to stop a program.

## Load and run an application

There are several preloaded Altair BASIC applications:

* *STARTREK.BAS*
* *TICTACTOE.BAS*
* *SIMPLE.BAS*
* *LOOPY.BAS*
* *COUNT.BAS*

To load and run one of them:

1. Enter `loadx` followed by the application name in quotation marks. For example, for the Tic-Tac-Toe application, enter the following command:

   ```basic
   loadx "TICTACTOE.BAS"
   ```

1. Run the application by using the following command:

   ```basic
   run
   ```

1. The application starts, and you're prompted to go first:

   ```text
   *** WELCOME TO TIC-TAC-TOE ***
   > YOU ARE X's <
   DO YOU WANT TO GO FIRST?
   ```
