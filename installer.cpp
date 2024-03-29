/*****************************************************
 * SIF: Simple Installer Framework                   *
 * Simply, an alternative to Qt Installer Framework  *
 *                                                   *
 * Copyright © 2019 Simelo.Tech                         *
 *                                                   *
 * This project is under the GPLv3 license           *
 *****************************************************/

#include "installer.h"

Installer::Installer(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<Installer::InstallerStatus>("InstallerStatus");
    qRegisterMetaType<Installer::InstallationPathError>("InstallerPathError");
    qRegisterMetaType<QFile::FileError>("FileError");

    QString currentOSName = currentOS == QOperatingSystemVersion::Windows ? "windows" :
                            currentOS == QOperatingSystemVersion::MacOS ? "macos" : "unix";
    dataPath = QString(":/") + currentOSName + "/data";
    desktopEntriesPath = QString(":/") + currentOSName + "/desktop_entries";

    setupInitialInstallationPath();
    setupExtractingProcess();
}

void Installer::setupInitialInstallationPath()
{
    if (QCoreApplication::applicationName().isEmpty()) { // this should never happen, because it is set to the executable name
        qWarning("Application name not defined. Default to \"App\". Set an application name with `QCoreApplication::setApplicationName()`");
        QCoreApplication::setApplicationName("App");
    }

    if (currentOS == QOperatingSystemVersion::Windows) {
        bool is64BitCpu = QSysInfo::currentCpuArchitecture().contains("64");
        int is64BitApp = QSysInfo::WordSize == 64;
        bool appArchitectureMatchCpuArchitecture = (is64BitCpu && is64BitApp) || (!is64BitCpu && !is64BitApp);

        if (appArchitectureMatchCpuArchitecture) {
            setInstallationPath(win_baseInstallDir + "/" + QCoreApplication::organizationName() + (QCoreApplication::organizationName().isEmpty() ? "" : "/") + QCoreApplication::applicationName());
        } else if (is64BitCpu && !is64BitApp) {
            setInstallationPath(win64_baseInstallDirx86 + "/" + QCoreApplication::organizationName() + (QCoreApplication::organizationName().isEmpty() ? "" : "/") + QCoreApplication::applicationName());
        }
    } else if (currentOS == QOperatingSystemVersion::MacOS) {
        setInstallationPath(macos_baseInstallDir + "/" + QCoreApplication::organizationName() + (QCoreApplication::organizationName().isEmpty() ? "" : "/") + QCoreApplication::applicationName());
    } else {
        setInstallationPath(unix_baseInstallDir + "/" + QCoreApplication::organizationName() + (QCoreApplication::organizationName().isEmpty() ? "" : "/") + QCoreApplication::applicationName());
    }
}

void Installer::validateInstallationPath()
{
    QFileInfo installationPathInfo(installationPath);
    if (installationPath.isEmpty()) {
        setInstallationPathIsValid(false);
        setInstallationPathError(PathNotSpecified);
    } else if (installationPathInfo.isFile()) {
        setInstallationPathIsValid(false);
        setInstallationPathError(PathIsAFile);
    } else if (!installationPathInfo.isAbsolute()) {
        setInstallationPathIsValid(false);
        setInstallationPathError(PathIsNotAbsolute);
    } else if (QDir(installationPath).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).count() > 0) { // We currently do not allow updates
        setInstallationPathIsValid(false);
        setInstallationPathError(PathIsNotEmpty);
    } else {
        setInstallationPathIsValid(true);
        setInstallationPathError(NoPathError);
    }
}


//! Installation

void Installer::setupExtractingProcess()
{
    connect(&watcherSeeking, &QFutureWatcher<QPair<QFileInfoList, qint64>>::finished, [&]() {
        if (extractionCanceled) {
            setInstallerStatus(InstallerStatus::ExtractionCanceled);
            emit reversionFinished();
        } else {
            QPair<QFileInfoList, qint64> seekingResult = watcherSeeking.result();
            filesToExtract = seekingResult.first;
            setTotalSize(seekingResult.second);
            setInstallerStatus(ExtractingPackages);
            emit extractionStarted();
            QFuture<void> futureExtracting = QtConcurrent::run([&]() {
                if (!extractFiles()) {
                    setInstallerStatus(InstallerStatus::ErrorOccurred);
                }
            });
            watcherExtracting.setFuture(futureExtracting);
        }
    });

    connect(&watcherExtracting, &QFutureWatcher<void>::finished, [&]() {
        if (extractionCanceled) {
            setInstallerStatus(ExtractionCanceled);
            emit reversionFinished();
        } else if (installerStatus != InstallerStatus::ErrorOccurred) {
            setInstallerStatus(ExtractionFinished);
            emit extractionFinished();
        }
    });
}

void Installer::requestProgress()
{
    double progress = double(extractedSize)/totalSize;
    emit progressReported(progress);
}

void Installer::cancel()
{
    extractionCanceled = true;
}

bool Installer::addDesktopShortcut(const QString &linkName, const QString &executableEntryFilePath)
{
    QString newExecutableEntryFilePath(executableEntryFilePath);
    if (executableEntryFilePath.isEmpty()) {
        if (currentOS == QOperatingSystemVersion::Windows || currentOS == QOperatingSystemVersion::MacOS) {
            newExecutableEntryFilePath = installationPath + '/' + QCoreApplication::applicationName() + currentOS == QOperatingSystemVersion::Windows ? ".exe" : ".app";
        } else { // a desktop entry
            // for now, system-wide installations are not supported
            newExecutableEntryFilePath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + '/' + QCoreApplication::applicationName() + ".desktop";
        }
    } else if (currentOS == QOperatingSystemVersion::Windows && !newExecutableEntryFilePath.endsWith(".exe")) {
        newExecutableEntryFilePath += ".exe";
    } else if (currentOS == QOperatingSystemVersion::MacOS && !newExecutableEntryFilePath.endsWith(".app")) {
        newExecutableEntryFilePath += ".app";
    } else if (!newExecutableEntryFilePath.endsWith(".desktop")) {
        newExecutableEntryFilePath += ".desktop";
    }

    if (!QFile::exists(newExecutableEntryFilePath)) {
        qCritical("The file \'%s\' does not exists", qPrintable(newExecutableEntryFilePath));
        return false;
    }

    QString desktopPath = QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first();
    QString linkDesktop = desktopPath + '/' + (linkName.isEmpty() ? newExecutableEntryFilePath : linkName);

    if (currentOS == QOperatingSystemVersion::Windows) {
        linkDesktop += ".lnk";
    }
    if (QFile::exists(linkDesktop)) {
        if (!QFile::remove(linkDesktop)) {
            qCritical("Cannot remove link \'%s\'", qPrintable(linkDesktop));
            return false;
        }
    }

    QFile executableEntryFile(newExecutableEntryFilePath);
    if (!executableEntryFile.link(linkDesktop)) {
        qCritical("Cannot create link \'%s\' pointing to \'%s\': %s", qPrintable(linkDesktop), qPrintable(newExecutableEntryFilePath), qPrintable(executableEntryFile.errorString()));
        return false;
    }

    return true;
}

bool Installer::addWindowsStartMenuEntry(const QString &linkName, const QString &filePath)
{
    if (currentOS != QOperatingSystemVersion::Windows) {
        qInfo("Start menu entries can only be created on Windows");
        return false;
    }

    QString newFilePath(filePath);
    if (filePath.isEmpty()) {
        newFilePath = installationPath + '/' + QCoreApplication::applicationName() + ".exe";
    }

    QString startMenuPath      = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QString startMenuGroupPath = startMenuPath + '/' + QCoreApplication::organizationName();
    QString startMenuAppPath   = startMenuGroupPath + '/' + QCoreApplication::applicationName();
    QDir d;
    if (!d.exists(startMenuAppPath)) {
        if (!d.mkpath(startMenuAppPath)) {
            qCritical("Cannot create folder \'%s\'", qPrintable(startMenuAppPath));
            return false;
        }
    }

    QString linkFilePath = startMenuAppPath + '/' + linkName + ".lnk";
    if (QFile::exists(linkFilePath)) {
        qWarning("Link \'%s\' already exists, so it will be replaced", qPrintable(linkFilePath));
        if (!QFile::remove(linkFilePath)) {
            qCritical("Cannot remove link \'%s\'", qPrintable(linkFilePath));
            return false;
        }
    }
    QFile fileToLink(newFilePath);
    if (!fileToLink.link(linkFilePath)) {
        qCritical("Cannot create link \'%s\' pointing to \'%s\': %s", qPrintable(linkFilePath), qPrintable(newFilePath), qPrintable(fileToLink.errorString()));
        return false;
    }
    return true;
}

bool Installer::addDesktopEntry(const QString &name)
{
    if (currentOS == QOperatingSystemVersion::Windows || currentOS == QOperatingSystemVersion::MacOS) {
        qInfo("Desktop entries cannot be created on Windows or macOS");
        return false;
    }

    QString newName(name);
    if (!newName.endsWith(".desktop")) {
        newName += ".desktop";
    }

    QString desktopEntrySourceFilePath = desktopEntriesPath + '/' + newName;
    QFile desktopEntrySourceFile(desktopEntrySourceFilePath);
    if (!desktopEntrySourceFile.exists(desktopEntrySourceFilePath)) {
        qCritical("The file \'%s\' does not exists", qPrintable(desktopEntrySourceFilePath));
        return false;
    }

    QString desktopEntryPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QString desktopEntryFilePath = desktopEntryPath + '/' + newName;

    if (QFile::exists(desktopEntryFilePath)) {
        qWarning("The desktop entry \'%s\' already exists. Attempting to remove it before copy it...", qPrintable(desktopEntryFilePath));
        if (QFile::remove(desktopEntryFilePath)) {
            qWarning("Success on removing the old file");
        } else {
            qCritical("Failed to remove the old file");
            return false;
        }
    }

    if (!desktopEntrySourceFile.copy(desktopEntryFilePath)) {
        qCritical("Cannot create file \'%s\': %s", qPrintable(desktopEntryFilePath), qPrintable(desktopEntrySourceFile.errorString()));
        return false;
    }

    return true;
}


void Installer::addWindowsControlPanelUninstallerEntry(const QString &applicationDescription, const QString &applicationFilePath, const QString &uninstallerFilePath, const QString &modifierApplicationFilePath, const QString &repairerApplicationFilePath, const QString &moreInfoUrl)
{
    if (currentOS != QOperatingSystemVersion::Windows) {
        qInfo("Control Panel program entries can only be created on Windows");
        return;
    }

    QString newApplicationFilePath(applicationFilePath);
    if (applicationFilePath.isEmpty()) {
        newApplicationFilePath = installationPath + '/' + QCoreApplication::applicationName() + ".exe";
    }

    QString newUninstallerFilePath = uninstallerFilePath;
    if (uninstallerFilePath.isEmpty()) {
        newUninstallerFilePath = installationPath + "/uninstall.exe";
    }

    // for system-wide installations, use `QSettings::SystemScope`
    QSettings sUnins(QSettings::UserScope, QDir::toNativeSeparators("Microsoft/Windows/CurrentVersion/Uninstall"),QCoreApplication::applicationName());
    sUnins.setValue("Comments", applicationDescription);
    sUnins.setValue("DisplayIcon", QDir::toNativeSeparators(newApplicationFilePath));
    sUnins.setValue("DisplayName", QCoreApplication::applicationName());
    sUnins.setValue("DisplayVersion", QCoreApplication::applicationVersion());
    sUnins.setValue("EstimatedSize", int(totalSize/1000)); // the size is in KB
    sUnins.setValue("InstallDate", QDateTime::currentDateTime().toString(Qt::SystemLocaleShortDate));
    sUnins.setValue("InstallLocation", QDir::toNativeSeparators(installationPath));
    sUnins.setValue("ModifyPath", modifierApplicationFilePath);
    sUnins.setValue("RepairPath", repairerApplicationFilePath);
    sUnins.setValue("NoModify", modifierApplicationFilePath.isEmpty());
    sUnins.setValue("NoRepair", repairerApplicationFilePath.isEmpty());
    sUnins.setValue("Publisher", QCoreApplication::organizationName());
    sUnins.setValue("UninstallString", QDir::toNativeSeparators(newUninstallerFilePath));
    sUnins.setValue("UrlInfoAbout", moreInfoUrl);
}

void Installer::extractAll()
{
    QFuture<QPair<QFileInfoList, qint64>> futureSeeking = QtConcurrent::run([=]() {
        setInstallerStatus(FetchingFiles);
        emit fetchingStarted();
        return findFilesToExtract(dataPath);
    });
    watcherSeeking.setFuture(futureSeeking);
}

QPair<QFileInfoList, qint64> Installer::findFilesToExtract(const QString &path)
{
    qint64 size = 0;
    QFileInfoList files;

    QStringList pendingDirectoriesPath;
    pendingDirectoriesPath.append(path);

    while (!pendingDirectoriesPath.empty()) {
        QString pendingDirectoryPath = pendingDirectoriesPath.first();
        QDir currentDirectory(pendingDirectoryPath);

        for (auto fileInfo : currentDirectory.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (extractionCanceled) {
                return { QFileInfoList(), 0 };
            } else if (fileInfo.isDir()) {
                pendingDirectoriesPath.append(fileInfo.filePath());
            } else {
                size += fileInfo.size();
                files.append(fileInfo);
            }
        }
        pendingDirectoriesPath.removeFirst();
    }

    // now add the size of additional files
    for (auto pair : additionalFiles) {
        size += QFileInfo(pair.first).size();
    }

    return { files, size };
}

bool Installer::addFileToExtract(const QString &from, const QString &to)
{
    if (installerStatus != Idle) {
        qCritical("Files can only be added in IDLE installer status");
        return false;
    } else if (!QFile::exists(from)) {
        qCritical("File \'%s\' doesn't exists", qPrintable(from));
        return false;
    }

    additionalFiles << QPair<QString, QString>(from, to);
    return true;
}

bool Installer::extractFiles()
{
    for (auto fileInfo : filesToExtract) {
        QString originFilePath = fileInfo.filePath();
        QString destinationFilePath = installationPath + (installationPath.endsWith('/') ? "" : "/") + QString(originFilePath).remove(dataPath + '/');
        QDir d(destinationFilePath);
        if (!d.exists()) {
            if (!d.mkpath(QFileInfo(destinationFilePath).path())) {
                emit extractionError(QFile::PermissionsError, tr("Access denied to create the path: %1").arg(QFileInfo(destinationFilePath).path()));
                return false;
            }
        }
        QPair<QFile::FileError, QString> singleExtractionResult = extractSingleFile(originFilePath, destinationFilePath);
        if (extractionCanceled) {
            revertInstallation();
            return false;
        } else if (singleExtractionResult.first != QFile::NoError) {
            emit extractionError(singleExtractionResult.first, singleExtractionResult.second);
            return false;
        }
        extractedFiles.append(QFileInfo(destinationFilePath));
    }

    // extract additional files
    for (auto pair : additionalFiles) {
        QString origin = pair.first;
        QString destination = pair.second;
        QPair<QFile::FileError, QString> singleExtractionResult = extractSingleFile(origin, destination);
        if (extractionCanceled) {
            revertInstallation();
            return false;
        } else if (singleExtractionResult.first != QFile::NoError) {
            emit extractionError(singleExtractionResult.first, singleExtractionResult.second);
            return false;
        }
        extractedFiles.append(QFileInfo(destination));
    }

    return true;
}

QPair<QFile::FileError, QString> Installer::extractSingleFile(const QString &origin, const QString &destination)
{
    QFile in(origin);
    QFile out(destination);

    if (origin.isEmpty()) {
        return { QFile::FatalError, tr("Empty or null source file name: %1").arg(origin) };
    }
    if (QFile::exists(destination)) {
        return { QFile::CopyError, tr("Destination file exists: %1").arg(destination) };
    }
    if (!in.open(QFile::ReadOnly)) {
        return { in.error(), in.errorString() };
    } else {

        if (!out.open(QIODevice::ReadWrite)) {
            in.close();
            return { out.error(), out.errorString() };
        } else {
            char block[4096];
            qint64 bytesCopied = 0;
            while (!in.atEnd()) {
                if (extractionCanceled) {
                    out.remove();
                    return { QFile::AbortError, tr("Operation canceled") };
                }
                qint64 inFlow = in.read(block, sizeof(block));
                if (inFlow <= 0) {
                    break;
                }
                bytesCopied += inFlow;
                extractedSize += inFlow;

                if (inFlow != out.write(block, inFlow)) {
                    in.close();
                    out.close();
                    out.remove();
                    return { QFile::WriteError, tr("Failure to write block") };
                }
            }

            if (bytesCopied != in.size()) {
                out.remove();
                return { QFile::ReadError, tr("Unable to read from the source") };
            }
        } // Output file opened (ReadWrite)
    } // source file opened (ReadOnly)

    QFile::setPermissions(origin, in.permissions());
    in.close();
    out.close();
    return { QFile::NoError, QString() };
}

void Installer::revertInstallation()
{
    // TODO: Revert also Desktop, Start Menu and Control Panel entries, etc.
    for (auto fileInfo : extractedFiles) {
        QFile f(fileInfo.filePath());
        qint64 size = fileInfo.size();
        if (!f.remove()) {
            emit reversionFailed();
            return;
        }
        extractedSize -= size;
        QDir d(fileInfo.path());
        if (d.isEmpty()) {
            d.rmpath(d.path());
        }
    }
    // This should not be neccessary... but just in case...
    QDir installDir(installationPath);
    installDir.removeRecursively();
}

//! Access descriptors

QString Installer::getDataPath() const
{
    return dataPath;
}

void Installer::setDataPath(const QString &value)
{
    dataPath = value;
}

QString Installer::getInstallationPath() const
{
    return installationPath;
}

void Installer::setInstallationPath(const QString &value)
{
    if (installationPath != value) {
        installationPath = value;
        validateInstallationPath();
        emit installationPathChanged(value);
    }
}

bool Installer::getInstallationPathIsValid() const
{
    return installationPathIsValid;
}

void Installer::setInstallationPathIsValid(bool value) // private
{
    if (installationPathIsValid != value) {
        installationPathIsValid = value;
        emit installationPathIsValidChanged(value);
    }
}

Installer::InstallationPathError Installer::getInstallationPathError() const
{
    return installationPathError;
}

void Installer::setInstallationPathError(const InstallationPathError &value) // private
{
    if (installationPathError != value) {
        installationPathError = value;
        setInstallationPathErrorString(installationPathErrorMap[value]);
        emit installationPathErrorChanged(value);
    }
}

QString Installer::getInstallationPathErrorString() const
{
    return installationPathErrorString;
}

void Installer::setInstallationPathErrorString(const QString &value) // private
{
    if (installationPathErrorString != value) {
        installationPathErrorString = value;
        emit installationPathErrorStringChanged(value);
    }
}

qint64 Installer::getTotalSize() const
{
    return totalSize;
}

void Installer::setTotalSize(const qint64 &value) // private
{
    if (totalSize != value) {
        totalSize = value;
        emit totalSizeChanged(value);
    }
}

QOperatingSystemVersion::OSType Installer::getCurrentOS()
{
    return currentOS;
}

Installer::InstallerStatus Installer::getInstallerStatus() const
{
    return installerStatus;
}

void Installer::setInstallerStatus(const InstallerStatus &value) // private
{
    if (installerStatus != value) {
        installerStatus = value;
        emit installerStatusChanged(value);
    }
}
