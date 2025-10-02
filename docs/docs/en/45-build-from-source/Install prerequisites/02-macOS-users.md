# macOS

1. Install Xcode command line tools

    ```bash
    xcode-select --install
    ```

    <!-- https://osxdaily.com/2014/02/12/install-command-line-tools-mac-os-x/ -->

1. Install [Homebrew](https://brew.sh/)

    ```bash
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    ```

1. Open a Terminal window.
1. Run the following command to install the required packages.

    ```bash
    brew install libuv openssl ossp-uuid cmake
    ```
