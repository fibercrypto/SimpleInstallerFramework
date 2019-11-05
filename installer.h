/*****************************************************
 * SIF: Simple Installer Framework                   *
 * Simply, an alternative to Qt Installer Framework  *
 *                                                   *
 * Copyright © 2019 Simelo.Tech                         *
 *                                                   *
 * This project is under the GPLv3 license           *
 *****************************************************/

#ifndef INSTALLER_H
#define INSTALLER_H

#include <QObject>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QSysInfo>
#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QOperatingSystemVersion>
#include <QStandardPaths>

class Installer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString installationPath READ getInstallationPath WRITE setInstallationPath NOTIFY installationPathChanged)
    Q_PROPERTY(bool installationPathIsValid READ getInstallationPathIsValid NOTIFY installationPathIsValidChanged)
    Q_PROPERTY(InstallationPathError installationPathError READ getInstallationPathError NOTIFY installationPathErrorChanged)
    Q_PROPERTY(QString installationPathErrorString READ getInstallationPathErrorString NOTIFY installationPathErrorStringChanged)

    Q_PROPERTY(InstallerStatus installerStatus READ getInstallerStatus NOTIFY installerStatusChanged)
    Q_PROPERTY(quint64 totalSize READ getTotalSize NOTIFY totalSizeChanged)

    Q_PROPERTY(QOperatingSystemVersion::OSType currentOS READ getCurrentOS CONSTANT)

public:
    enum InstallationPathError { NoPathError, PathNotSpecified, PathIsAFile, PathIsNotAbsolute, PathIsNotEmpty };
    Q_ENUM(InstallationPathError)

    enum InstallerStatus { Idle, FetchingFiles, ExtractingPackages, ExtractionFinished, ExtractionCanceled, RevertingInstallation, ErrorOccurred };
    Q_ENUM(InstallerStatus)

    explicit Installer(QObject *parent = nullptr);

    QString getDataPath() const;
    void setDataPath(const QString &value);

    QString getInstallationPath() const;
    void setInstallationPath(const QString &value);

    bool getInstallationPathIsValid() const;

    InstallationPathError getInstallationPathError() const;

    QString getInstallationPathErrorString() const;

    InstallerStatus getInstallerStatus() const;

    qint64 getTotalSize() const;

    QOperatingSystemVersion::OSType getCurrentOS();

public slots:
    bool addFileToExtract(const QString &from, const QString &to);
    void extractAll();
    void requestProgress();
    void cancel();
    bool addDesktopShortcut(const QString &linkName = QCoreApplication::applicationName(), const QString &executableEntryFilePath = QString());
    bool addWindowsStartMenuEntry(const QString &linkName = QCoreApplication::applicationName(), const QString &filePath = QString());
    bool addDesktopEntry(const QString &name = QCoreApplication::applicationName());
    void addWindowsControlPanelUninstallerEntry(const QString &applicationDescription, const QString &applicationFilePath = QString(), const QString &uninstallerFilePath = QString(), const QString &modifierApplicationFilePath = QString(), const QString &repairerApplicationFilePath = QString(), const QString &moreInfoUrl = QString());

signals:
    void installationPathChanged(const QString &value);
    void installationPathIsValidChanged(bool value);
    void installationPathErrorChanged(InstallationPathError value);
    void installationPathErrorStringChanged(const QString &value);
    void installerStatusChanged(InstallerStatus value);
    void totalSizeChanged(qint64 value);

    void fetchingStarted();
    void extractionStarted();
    void progressReported(double progress);
    void extractionFinished();
    void reversionFinished(); // success on revert installation

    // Error handling
    void extractionError(QFile::FileError errorCode, const QString &errorString);
    void reversionFailed();

private:
    QString dataPath; // stored in resources, not expose to QML
    QString desktopEntriesPath; // stored in resources, not expose to QML

    QString installationPath; // expose to QML
    bool installationPathIsValid = false; // expose as "read only" to QML. Default invalid
    InstallationPathError installationPathError = PathNotSpecified; // expose as "read only" to QML
    QHash<InstallationPathError, QString> installationPathErrorMap = { // not expose to QML
        { NoPathError, QString() },
        { PathNotSpecified, tr("No path specified") },
        { PathIsAFile, tr("Path is a file") },
        { PathIsNotAbsolute, tr("Path is not absolute") },
        { PathIsNotEmpty, tr("Path is not empty") }
    };
    QString installationPathErrorString = installationPathErrorMap[installationPathError]; // expose as "read only" to QML

    // Platform-specific tweaks
    QString win_baseInstallDir = "C:/Program Files"; // not expose to QML
    QString win64_baseInstallDirx86 = "C:/Program Files (x86)"; // not expose to QML
    QString macos_baseInstallDir = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation).first();
    QString unix_baseInstallDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first() + "/Applications";

    InstallerStatus installerStatus = Idle; // expose as "read only" to QML
    qint64 extractedSize = 0; // not expose to QML, check progress every some interval using a Timer that calls requestProgress()
    qint64 totalSize = 0; // expose as "read only" to QML
    QFileInfoList filesToExtract; // not expose to QML
    // The following members are used to handle canceling
    QFileInfoList extractedFiles; // not expose to QML
    bool extractionCanceled = false; // not expose to QML
    QList<QPair<QString, QString>> additionalFiles; // not expose to QML

    QFutureWatcher<QPair<QFileInfoList, qint64>> watcherSeeking; // not expose to QML
    QFutureWatcher<void> watcherExtracting; // not expose to QML

    static const QOperatingSystemVersion::OSType currentOS = QOperatingSystemVersion::currentType(); // expose as "CONSTANT" to QML

    //! Private functions

    // Private access descriptors (from read-only properties)
    void setInstallationPathIsValid(bool value);
    void setInstallationPathError(const InstallationPathError &value);
    void setInstallationPathErrorString(const QString &value);
    void setInstallerStatus(const InstallerStatus &value);
    void setTotalSize(const qint64 &value);

    // Installation
    void setupInitialInstallationPath();
    void validateInstallationPath();
    void setupExtractingProcess();
    QPair<QFileInfoList, qint64> findFilesToExtract(const QString &path);
    bool extractFiles();
    QPair<QFileDevice::FileError, QString> extractSingleFile(const QString &origin, const QString &destination);
    void revertInstallation();
};

Q_DECLARE_METATYPE(QFile::FileError)

#endif // INSTALLER_H
