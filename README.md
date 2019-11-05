# SIF: Simple Installer Framework

*A Simple, easy-to-use, highly customizable Installer Framework built with Qt*

Copyright © 2019 Simelo.Tech

*This project is under the GPLv3 license*

---------------------------------------------


## Description

**Simple Installer Framework** (**SIF** for short) is an alternative to Qt Installer Framework. It is intended to be used as a statically compiled library. It is a non-GUI library, what means that you must provide the graphics by using either `Qt Widgets` or `Qt Quick`.

**SIF** features:

* Asynchronous fetching and extraction operations
* Make your own custom UI
* Designed to work well with both Qt Widgets and Qt Quick
* Logic separated from the view


## Requirements

The framework uses Qt. Any Qt 5 version should work, but I always recommend having the latest LTS release.


## How to add it to my project?

You can compile **SIF** from Qt Creator or running `qmake` then `make` (or equivalent) in the root of the project, and link the resulting static library to your project. For example, if you have the library `SimpleInstallerFramework.a` and its header file copied in `lib/SimpleInstallerFramework/` inside the root of your project, then you can link to it by simply adding to your `.pro` file:

```
LIBS += -L$$PWD/lib/SimpleInstallerFramework/ -lSimpleInstallerFramework
```

Otherwise, you can add the class `Installer` (i.e. the files `installer.h` and `installer.cpp`) to your project.


## How to interact with it?

First, set the organization and the application name:

```c++
    QCoreApplication::setOrganizationName("MyOrg");
    QCoreApplication::setApplicationName("MyApp");
```

You can also set the organization domain and the application version.

Second, the installer search for files to extract in the following resource locations:

* *Windows*: `":/windows/data"`
* *macOS*: `":/macos/data"`
* *Unix*: `":/unix/data"`

So you must **ensure that such paths can be found**. As a tip, the following folder structure is followed in our examples:

```
root/
|
|――resources/
|  |
|  |――windows/
|  |  |
|  |  |――data/
|  |
|  |――macos/
|  |  |
|  |  |――data/
|  |
|  |――unix/
|  |  |
|  |  |――data/
```

... and then, in each operating system folder, we create a `data.qrc` Qt resource file with the prefix `/os_name` and add it to the project (`.pro`) file.

**Simple Installer Framework** consists in just one class: `Installer`. Its properties, methods and signals are explained bellow.


### Properties:

* `installationPath: QString`

  Access descriptors: `getInstallationPath()` and `setInstallationPath()`

  Notifier signal: installationPathChanged(const QString &)
    
    This is the path where you want to install your software. By default, it chooses the appropriate path for you Operating System:

    - *Windows*:
      - `C:/Program files/Organization/ApplicationName` (if OS x86 and app x86 or OS x64 and app x64) or,
      - `C:/Program files (x86)/Organization/ApplicationName` (if OS x64 and app x86)
    - *macOS*:
      - `/Applications`
    - *Unix*:
      - `~/Applications`

    Observe that *Windows* and *macOS* default installation paths are not writable without elevated privilegies.  
    *Unix* refers to Linux or any other Unix-based OS except Darwin systems.

    Every time this property is set, it triggers an update of the property `installationPathIsValid`, so you must check the validity of the new path.

* `[read-only] installationPathIsValid: bool`

  Access descriptors: `getInstallationPathIsValid()`

  Notifier signal: installationPathIsValidChanged(bool)

    This property is updated when the `installationPath` changes. It is `true` when the installation path is valid, and `false` otherwise. You must check the state of this property when you set the installation path:

    ```c++
    // We'll assume we're on Windows x64 and app x64
    Installer inst; // installationPath = "C:/Program files/MyOrg/MyApp"
    QString newPath = QString("D:/My Games/") + QCoreApplication::setOrganizationName() + "/" + QCoreApplication::setApplicationName();
    inst.setInstallationPath(newPath);
    if (inst.getInstallationPathIsValid()) {
        // do the thing
    } else {
        // show a warning, ask again, etc...
    }
    ```

* `[read-only] installationPathError: Installer::InstallationPathError`

  Access descriptors: `getInstallationPathError()`

  Notifier signal: installationPathErrorChanged(Installer::InstallationPathError)

    The error code of the last operation that failed. You should use `installationPathErrorString` to show human-readable error messages. Can be one of:

    InstallationPathError    | Description
    ------------------------ | --------------------
    `NoPathError`            | No error, all is Ok
    `PathNotSpecified`       | No path specified
    `PathIsAFile`            | Path is a file
    `PathIsNotAbsolute`      | Path is not absolute
    `PathIsNotEmpty`         | Path is not empty

* `[read-only] installationPathErrorString: QString`

  Access descriptors: `getInstallationPathErrorString()`

  Notifier signal: installationPathErrorStringChanged(const QString &)

    Human-readable description of the error code represented by `installationPathError`.

* `[read-only] installerStatus: Installer::InstallerStatus`

  Access descriptors: `getInstallerStatus()`

  Notifier signal: installerStatusChanged(Installer::InstallerStatus)

    The status of the installer. Can be one of:

    InstallerInstallerStatus | Description
    ------------------------ | ---------------
    `Idle`                   | The installer is idle (i.e., in stand-by)
    `FetchingFiles`          | Searching for files to install
    `ExtractingPackages`     | Extracting files to the selected installation path
    `ExtractionFinished`     | The extraction finished successfully
    `ExtractionCanceled`     | The extraction was canceled
    `RevertingInstallation`  | The installation has been canceled and it's being reverted
    `ErrorOccurred`          | An error occurred

* `[read-only] totalSize: long long int`

  Access descriptors: `getTotalSize()`

  Notifier signal: totalSizeChanged(long long int)

    The total size of all files that are going to be installed.

* `[const] [read-only] currentOS: QOperatingSystemVersion::OSType`

  Access descriptors: `getCurrentOS()`

    The version of the operating system the application is running on. This property exists for convenience with Qt Quick UIs, because from C++ you can always use `QOperatingSystemVersion::currentType()` directly to get the same result.


### Methods:

* `Installer::Installer(QObject *parent = nullptr)`

    Constructs a new instance of an `Installer`.

* `[virtual] Installer::~Installer()`

    Destructs an `Installer` instance.

* `QString Installer::getDataPath()`

    Returns the path where the installer search for files to install, by default in the resource path `":/os_name/data"`. For example, if you're on *Windows*, it search in `":/windows/data"`, if you're on *macOS*, it search in `":/macos/data"` and under a *Unix* (not *Darwin*) system it search in `":/unix/data"`.

* `Installer::setDataPath(const QString &newDataPath)`

    As explained earlier, by default, **SIF** search for files to install in the resource path `":/os_name/data"`. It is strongly recommended to **not** change the default path because it wipes out the cross-platform behavior, but you can still call this function if you really want to do so.

* `[slot] void Installer::extractAll()`

    Calling this method starts asynchronous fetching and extraction of all files. After this function is called, the `installerStatus` property is set to `InstallerStatus::FetchingFiles`. If any error occur, the `installerStatus` is set to `InstallerStatus::ErrorOccurred`. If the operation completes successfully, `installerStatus` is set to `InstallerStatus::ExtractionFinished`.

* `[slot] void Installer::requestProgress()`

    After calling this method, the signal `progressReported()` will be emitted with the progress of the extraction/reversion. A handle must then capture the signal and react accordingly. GUI applications should call this function periodically (i.e. using a `QTimer` or `Timer`) while the status of the installer is `InstallerInstallerStatus::ExtractingPackages` or `InstallerInstallerStatus::RevertingInstallation`.

* `[slot] void Installer::cancel()`

    Call this function to cancel the extraction process. The installer will then enter the `InstallerInstallerStatus::RevertingInstallation` status and start deleting the installed files. After that, the `installerStatus` property will be set to `InstallerInstallerStatus::ExtractionCanceled`.

* `[slot] bool Installer::addFileToExtract(const QString &from, const QString &to)`

    Call this function to add more files to the extraction list. During installation, the file will be copied from the filepath pointed by `from` to the filepath pointed by `to`.  
    This is useful to install optional components, for example, you may want to optionally install the source code of your application in a zipped folder or a tarball. Another good use is to reduce the installer size if the same file must be copied to multiple locations, because you can include the file only once in resources, and add various destinations.  
    This function must be called **before** the fetching or extraction starts, i.e. when the installer is in the `Idle` status.  
    The total size of the files added with this function is also added to the total size of the installation.  
    Returns `true` if the file pointed by `from` exists, `false` otherwise.

* `[slot] bool Installer::addDesktopShortcut(const QString &linkName = QCoreApplication::applicationName(), const QString &executableEntryFilePath = QString())`

    Call this function to add a shortcut file named `linkName` in the desktop. The links points to `executableEntryFilePath`. By default, `linkName` is the application's name, and `executableEntryFilePath` is empty, which means that it points to the appropriate files depending of the operating system where the installer is running:

    OS      | Executable entry file path
    ------- | ------------------------------------------------------------------------
    Windows | `<Installation path>/<Application Name>.exe`
    macOS   | `<Installation path>/<Application Name>.app`
    Unix    | `<Applications Location>/<Application Name>.desktop`

    We strongly recommend to not specify a value to `executableEntryFilePath`, unless the name of the executable do not match the name of the application as set with `QCoreApplication::setApplicationName()`.  
    On Unix, a desktop entry must be previously created with `addDesktopEntry` for this function to work.  
    Returns `true` if the shortcut is created, `false` otherwise.

* `[slot] bool Installer::addWindowsStartMenuEntry(const QString &linkName = QCoreApplication::applicationName(), const QString &filePath = QString())`

    Call this function to add an entry named `linkName` (the application's name by default) that points to `filePath` to the Windows Start Menu. If the `filePath` is not specified, it defaults to `<Installation path>/<Application Name>.exe`. Returns `true` on success, `false` otherwise. This function does nothing on non-Windows operating systems and returns `false`.

* `[slot] bool Installer::addDesktopEntry(const QString &name = QCoreApplication::applicationName())`

    Call this function to add a desktop entry named `name` (the application's name by default).  
    The installer search for desktop entries in `:/os_name/desktop_entries`. For example, on Linux, a desktop entry named `My App` should be in `:/linux/desktop_entries/My App.desktop`.  
    If an entry with name `name` is found, it is copied to an appropriate location (currently `<Applications Location>`, i.e. `~/.local/share/applications`) and returns `true`. If the entry is not found or cannot be copied, returns `false`.
    This function does nothing on Windows or macOS operating systems and returns `false`.

* `[slot] void Installer::addWindowsControlPanelUninstallerEntry(const QString &applicationDescription, const QString &applicationFilePath = QString(), const QString &uninstallerFilePath = QString(), const QString &modifierApplicationFilePath = QString(), const QString &repairerApplicationFilePath = QString(), const QString &moreInfoUrl = QString())`

    Call this function to add an entry (usually for an uninstaller) in the Control Panel of a Windows operating system. A brief description of the application must be provided in `applicationDescription`. The rest of arguments are optional and will be explained bellow:

    * `applicationFilePath` is the application which icon will be shown in the entry, and if not specified, defaults to `<Installation path>/<Application Name>.exe`.
    * `uninstallerFilePath` is the path to the executable that will be used as uninstaller. If not specified, defaults to `<Installation path>/uninstall.exe`.
    * `modifierApplicationFilePath` is the path to the executable that will be used to modify the existing installation, usually an updater. By default, it's empty, which means that no application is provided.
    * `repairerApplicationFilePath` is the path to the executable that will be used to repair the existing installation. By default, it's empty, which means that no application is provided.
    * `moreInfoUrl` is an URL that will be used to obtain more information about the application. Empty by default.

    This function does nothing on non-Windows operating systems.


### Signals:

* `[signal] fetchingStarted()`
    
    This signal is emitted just before the installer starts searching files to extract.

* `[signal] extractionStarted()`
    
    This signal is emitted just before the installer starts extracting files.

* `[signal] progressReported(double progress)`
    
    This signal is emitted after `requestProgress()` is called. `progress` is the value of the progress of extraction/reversion.

* `[signal] extractionFinished()`
    
    This signal is emitted just after the installer finished extracting files.

* `[signal] reversionFinished()`
    
    This signal is emitted just after the installer finished deleting the installed files.

* `[signal] extractionError(QFile::FileError errorCode, const QString &errorString)`

    This signal is emitted when an error occur while extracting files. `errorCode` is the code of the error, and `errorString` is its description.

* `[signal] void reversionFailed()`

    This signal is emitted if occur an error during reversion.


## TODO

There's my *TODO* list:

* **All platforms**
    * Allow system-wide installation


## Licensing

This project is under the GPLv3 license (see [http://fsf.org/](http://fsf.org/) for more information).


## Contact information

e-mail: [contact@fibercryp.to](mailto:contact@fibercryp.to)
