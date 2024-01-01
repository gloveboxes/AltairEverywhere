# Starting the http server

## Starting the http server manually

You can start the http server manually by running the following command from the command line:

```shell
sudo python3 -m http.server 80
```

## Starting the http server as a service on Linux

1. From the command line, navigate to `Terminal` folder.
2. Update the http.service file with the correct path to the `index.html` file.
   
   ```shell
   sed -i "s|/path/to/directory|$(pwd)|g" httpserver.service
   ```

3. Copy the http.service file to the systemd folder.
   
   ```shell
    sudo cp httpserver.service /etc/systemd/system/
    ```

4. Reload the systemd daemon.
    
    ```shell
     sudo systemctl daemon-reload
     ```

5. Enable the httpserver service.

    ```shell
    sudo systemctl enable httpserver.service
    ```

6. Start the httpserver service.

    ```shell
    sudo systemctl start httpserver.service
    ```

7. Check the status of the httpserver service.

    ```shell
    sudo systemctl status httpserver.service
    ```

    If the service is running, you should see something like this:

    ```shell
    ● httpserver.service - HTTP Server
     Loaded: loaded (/etc/systemd/system/httpserver.service; enabled; preset: enabled)
     Active: active (running) since Tue 2024-01-02 07:06:49 AEDT; 30min ago
   Main PID: 11489 (python3)
      Tasks: 1 (limit: 9262)
        CPU: 114ms
     CGroup: /system.slice/httpserver.service
             └─11489 /usr/bin/python3 -m http.server 80
    ```

8. Open a web browser and navigate to `http://localhost` or `http://<ip address>` to view the web page.
