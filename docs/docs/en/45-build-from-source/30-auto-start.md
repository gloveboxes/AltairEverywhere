# Autostart the Altair services

The following details how to run the Altair emulator and web terminal as a services on Linux. This will ensure the Altair emulator and web terminal are started when the computer is booted.

1. From the command line, navigate to `Services` folder of the Altair emulator project you cloned from GitHub.

   ```shell
   cd Services
   ```

2. Update the .service files with the correct paths.
   
   ```shell
   sed -i "s|/path/to/directory|$(dirname $(pwd))|g" *.service 
   ```
3. Copy the .service files to the systemd folder.
   
   ```shell
    sudo cp *.service  /etc/systemd/system/
    ```
4. Reload the systemd daemon.
    
    ```shell
     sudo systemctl daemon-reload
     ```

5. Enable the services.
   
    ```shell
    sudo systemctl enable altair_emulator.service &&
    sudo systemctl enable altair_terminal.service
    ```

6. Start the services.

    ```shell
    sudo systemctl start altair_emulator.service &&
    sudo systemctl start altair_terminal.service
    ```

7. Check the status of the services.

    ```shell
    sudo systemctl status altair_emulator.service &&
    sudo systemctl status altair_terminal.service
    ```

    If the service is running, you should see something like this:

    ```shell
    ● altair_emulator.service - Altair 8800 Emulator
            Loaded: loaded (/etc/systemd/system/altair_emulator.service; enabled; vendor preset: enabled)
            Active: active (running) since Tue 2024-01-02 08:37:28 AEDT; 10min ago
        Main PID: 992844 (Altair_emulator)
            Tasks: 5 (limit: 191)
                CPU: 10min 51.649s
            CGroup: /system.slice/altair_emulator.service
                    └─992844 /home/pi/github/AltairEverywhere/AltairHL_emulator/build/Altair_emulator

    ```

    ```shell
    ● altair_terminal.service  - HTTP Server
            Loaded: loaded (/etc/systemd/system/altair_terminal.service ; enabled; preset: enabled)
            Active: active (running) since Tue 2024-01-02 07:06:49 AEDT; 30min ago
        Main PID: 11489 (python3)
            Tasks: 1 (limit: 9262)
                CPU: 114ms
            CGroup: /system.slice/altair_terminal.service 
                    └─11489 /usr/bin/python3 -m http.server 80
    ```
