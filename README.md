# SIF: Simple Installer Framework

*A Simple, easy-to-use, highly customizable Installer Framework built with Qt*

Copyright © 2019 Carlos Enrique Pérez Sánchez

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

    InstallationPathError   |  | Description
    -------------------------- | ---------------
    NoPathError             |  | No error, all is Ok
    PathNotSpecified        |  | No path specified
    PathIsAFile             |  | Path is a file
    PathIsNotAbsolute       |  | Path is not absolute
    PathIsNotEmpty          |  | Path is not empty

* `[read-only] installationPathErrorString: QString`

  Access descriptors: `getInstallationPathErrorString()`

  Notifier signal: installationPathErrorStringChanged(const QString &)

    Human-readable description of the error code represented by `installationPathError`.

* `[read-only] installerStatus: Installer::InstallerStatus`

  Access descriptors: `getInstallerStatus()`

  Notifier signal: installerStatusChanged(Installer::InstallerStatus)

    The status of the installer. Can be one of:

    InstallerInstallerStatus|  | Description
    -------------------------- | ---------------
    Idle                    |  | The installer is idle (i.e., in stand-by)
    FetchingFiles           |  | Searching for files to install
    ExtractingPackages      |  | Extracting files to the selected installation path
    ExtractionFinished      |  | The extraction finished successfully
    ExtractionCanceled      |  | The extraction was canceled
    RevertingInstallation   |  | The installation has been canceled and it's being reverted
    ErrorOccurred           |  | An error occurred

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

    Get the path where the installer search for files to install, by default in the resource path `":/os_name/data"`. For example, if you're on *Windows*, it search in `":/windows/data"`, if you're on *macOS*, it search in `":/macos/data"` and under a *Unix* (not *Darwin*) system it search in `":/unix/data"`.

* `Installer::setDataPath(const QString &newDataPath)`

    As explained earlier, by default, **SIF** search for files to install in the resource path `":/os_name/data"`. It is strongly recommended to **not** change the default path because it wipes out the cross-platform behavior, but you can still call this function if you really want to do so.

* `[slot] void Installer::extractAll()`

    Calling this method starts asynchronous fetching and extraction of all files. After this function is called, the `installerStatus` property is set to `InstallerStatus::FetchingFiles`. If any error occur, the `installerStatus` is set to `InstallerStatus::ErrorOccurred`. If the operation completes successfully, `installerStatus` is set to `InstallerStatus::ExtractionFinished`.

* `[slot] void Installer::requestProgress()`

    After calling this method, the signal `progressReported()` will be emitted with the progress of the extraction/reversion. A handle must then capture the signal and react accordingly. GUI applications should call this function periodically (i.e. using a `QTimer` or `Timer`) while the status of the installer is `InstallerInstallerStatus::ExtractingPackages` or `InstallerInstallerStatus::RevertingInstallation`.

* `[slot] void Installer::cancel()`

    Call this function to cancel the extraction process. The installer will then enter the `InstallerInstallerStatus::RevertingInstallation` status and start deleting the installed files. After that, the `installerStatus` property will be set to `InstallerInstallerStatus::ExtractionCanceled`.


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

* **Windows:**
    * Add start menu entry
    * Add control panel entry
    * Add desktop shortcut

* **macOS:**
    * Add desktop shortcut

* **Linux:**
    * Add desktop entry
    * Add desktop shortcut


## Licensing

This project is under the GPLv3 license (see [http://fsf.org/](http://fsf.org/) for more information).


## Contact information

e-mail: [thecrowporation@gmail.com](mailto:thecrowporation@gmail.com)
