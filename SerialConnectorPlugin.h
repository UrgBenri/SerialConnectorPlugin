/*
	This file is part of the UrgBenri application.

	Copyright (c) 2016 Mehrez Kristou.
	All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

	3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	Please contact kristou@hokuyo-aut.jp for more details.

*/

#ifndef SERIALCOMMUNICATORWIDGET_H
#define SERIALCOMMUNICATORWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QMutex>
#include <QPushButton>
#include <QTranslator>

#include "ConnectorPluginInterface.h"
#include "HelperPluginInterface.h"

#include "SerialConnectionWidget.h"
#include "RangeViewWidget.h"


#include "UrgDevice.h"
#include "UrgUsbCom.h"
#include "FindComPorts.h"
#include "SerialDevice.h"
#include "SerialSettingsForm.h"

#include "SerialConnectionCheck.h"

#include "Thread.h"


using namespace std;
using namespace qrk;

namespace Ui
{
class SerialConnectorPlugin;
}

class SerialConnectorPlugin : public ConnectorPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(ConnectorPluginInterface)
    Q_PLUGIN_METADATA(IID "org.kristou.UrgBenri.SerialConnectorPlugin")
    PLUGIN_FACTORY(SerialConnectorPlugin)
public:
    explicit SerialConnectorPlugin(QWidget* parent = 0);
    virtual ~SerialConnectorPlugin();


    bool isConnected() { return m_connected;}
    bool isStarted() { return m_started;}
    bool isPaused() { return m_paused;}

    static int receivingThreadProcess(void* args);

    void setLengthData(const SensorDataArray &l_ranges, const SensorDataArray &l_levels, long l_timeStamp);

    QString pluginName() const{return tr("Serial");}
    QIcon pluginIcon() const {return QIcon(":/SerialConnectorPlugin/tabIcon");}
    QString pluginDescription() const {return tr("Connect to LRF sensor using Serial interface");}
    PluginVersion pluginVersion() const {return PluginVersion(1, 0, 1);}
    QString pluginAuthorName() const {return "Mehrez Kristou";}
    QString pluginAuthorContact() const {return "mehrez@kristou.com";}
    int pluginLoadOrder() const {return 0;}
    bool pluginIsExperimental() const { return false; }

    QString pluginLicense() const { return "GPL"; }
    QString pluginLicenseDescription() const { return "GPL"; }

    void saveState(QSettings &settings);
    void restoreState(QSettings &settings);

    void loadTranslator(const QString &locale);

    bool loadFile(const QString &filename) { return false;}

    void setupConnections();

    void setupShortcuts();

    bool isIntensityMode();

signals:
    void languageChanged();

public slots:
    void start();
    void stop();
    void pause();
    void restart();
    void errorHandle();    

    void onLoad(PluginManagerInterface *manager);
    void onUnload();

protected:
    void changeEvent(QEvent* e);
    bool nativeEvent(const QByteArray &eventType, void *message, long *result);

private:
    void updateUiWithInfo();
    bool setCaptureMode();

    Ui::SerialConnectorPlugin* ui;

    bool m_recording;
    Thread *m_receiving_thread;
    SerialConnectionWidget* m_connection_widget;
    RangeViewWidget* m_range_view_widget;
    UrgUsbCom m_urg_usb;
    FindComPorts m_urg_finder;
    UrgDevice *m_sensor;
    SerialDevice m_connectionDevice;

    SerialConnectionCheck m_connectionCheck;

    SerialSettingsForm *m_settings;
    QTranslator m_translator;

    HelperPluginInterface *m_recorder;
    HelperPluginInterface *m_scipTerminal;

    bool m_paused;

    bool m_connected;
    bool m_started;

    HelperPluginInterface *m_sensorInfo;

    HelperPluginInterface *m_baudrateChanger;

    void addDebugInformation(const QString &info);

    void addRecorder(PluginManagerInterface *manager);
    void addScipTerminal(PluginManagerInterface *manager);
    void addBaudrateChanger(PluginManagerInterface *manager);
    void addSensorInformation(PluginManagerInterface *manager);

    bool shouldStop();
    void registerUsbNotifier();
#ifdef Q_OS_WIN
    bool winEvent(MSG *message, long *result);
#endif

public slots:
    void rescanPressed();

private slots:
    void connectPressed(bool connection, const QString &device, long baude);
    void deviceConnected();
    void deviceConnectFailed();

    void startStepChanged(int value);
    void endStepChanged(int value);
    void groupStepsChanged(int value);
    void skipScansChanged(int value);

    void receivingTimerProcess();
    void updateCaptureMode(RangeCaptureMode mode);
    void resetUi();

    void initProgress(int value);

    void receivingThreadFinished();

    void serialBaudrateChanged(long baud);
    void errorHappened();

    void captureModeChanged();

    void resetSensor();
    void rebootSensor();
};

#endif // SERIALCOMMUNICATORWIDGET_H

