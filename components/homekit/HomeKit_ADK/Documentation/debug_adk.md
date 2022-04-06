Debugging ADK Applications
==========================

``` tabs::

   .. group-tab:: macOS

    **Using LLDB**

    .. code-block:: sh

     > lldb
     > (lldb) target create ./Output/Darwin-x86_64-apple-darwin$(uname -r)/Debug/BLE/Applications/<app_name>.OpenSSL
     > (lldb) run

    **Using Visual Studio Code**
    You can follow the instructions below to be able to debug ADK application from within the Visual Code editor.

    * Download VSCode: https://code.visualstudio.com/
    * To install c/c++ language support: within VSCode press ⌘+p and enter

    .. code-block:: bash

     ext install ms-vscode.cpptools

    * To install debugger: within VSCode, press ⌘+p and enter

    .. code-block:: bash

        ext install vadimcn.vscode-lldb

    .. Note::
        An alternative debugger is necessary because *llvm* no longer includes *lldb-mi* which the default debugger
        uses.

    * Create a workspace which points to the root of your ADK folder.
    * Click *Debug* > *Open Configuration*, which will open the *launch.json* file.
    * Set *type* to *lldb*
    * Set *program* to the application you want to run.

    Example *launch.json* file for *Lightbulb* app:

    .. code-block:: javascript

     {
        "version": "0.2.0",
        "configurations": [
            {
                "type": "lldb",
                "request": "launch",
                "name": "Debug",
                "program": "${workspaceFolder}/Output/Darwin-x86_64-apple-darwin19.0.0/Debug/IP/Applications/Lightbulb.OpenSSL",
                "args": [],
                "cwd": "${workspaceFolder}"
            }
        ]
     }

    **Getting Core Dumps or Crashes**

    By default, crashes are reported into *.crash* files which can be found in */Library/Logs/DiagnosticReports*
    (system-wide) and *~/Library/Logs/DiagnosticReports* (user). The *.crash* files are in plain text format and should
    include relevant information about the crash.

   .. group-tab:: Raspberry Pi

    .. code-block:: bash

     sudo gdb Output/Raspi-armv6k-unknown-linux-gnueabihf/Debug/IP/Applications/Lightbulb.OpenSSL `pidof Lightbulb.OpenSSL`

```
