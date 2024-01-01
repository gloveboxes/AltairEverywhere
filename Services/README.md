# Starting the Altair services

## Starting the http server manually

You can start the http server manually by running the following command from the command line:

```shell
sudo python3 -m http.server 80
```

## Starting the Altair Web Terminal as a service on Linux

1. From the command line, navigate to `Services` folder.
2. Update the http.service file with the correct path to the `index.html` file.
   
   ```shell
   sed -i "s|/path/to/directory|$(dirname $(pwd))|g" altair_terminal.service 
   ```

3. Copy the http.service file to the systemd folder.
   
   ```shell
    sudo cp altair_terminal.service  /etc/systemd/system/
    ```

4. Reload the systemd daemon.
    
    ```shell
     sudo systemctl daemon-reload
     ```

5. Enable the httpserver service.

    ```shell
    sudo systemctl enable altair_terminal.service 
    ```

6. Start the httpserver service.

    ```shell
    sudo systemctl start altair_terminal.service 
    ```

7. Check the status of the httpserver service.

    ```shell
    sudo systemctl status altair_terminal.service 
    ```

    If the service is running, you should see something like this:

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

8. Open a web browser and navigate to `http://localhost` or `http://<ip address>` to view the web page.


## Start the Altair Emulator as a service on Linux

1. From the command line, navigate to `Services` folder.
2. Update the altair_emulator.service file with the correct path to the `Altair Emulator` file.
   
   ```shell
   sed -i "s|/path/to/emulator|$(dirname $(pwd))|g" altair_emulator.service 
   ```
3. Copy the altair_emulator.service file to the systemd folder.
   
   ```shell
    sudo cp altair_emulator.service  /etc/systemd/system/
    ```
4. Reload the systemd daemon.
    
    ```shell
     sudo systemctl daemon-reload
     ```
5. Enable the altair_emulator service.
6. Start the altair_emulator service.
7. Check the status of the altair_emulator service.

    ```shell
    sudo systemctl status altair_emulator.service 
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