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

#include "SerialConnectorPlugin.h"
#include "ui_SerialConnectorPlugin.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QDebug>
#include <QDateTime>
#include <QSystemTrayIcon>
#include <QStandardPaths>
#include <QDesktopWidget>
#include <QEventLoop>
#include <QTimer>

#include "delay.h"

//#include "SettingsWindow.h"
#include "SerialDataConverter.h"
#include "PluginUtils.h"

#ifdef Q_OS_WIN
#define _WIN32_WINNT 0x0500
#define _WIN32_WINDOWS 0x0500
#define WINVER 0x0500
#include <windows.h>
#include <dbt.h>
#endif

const QString LogHeader = "SerialConnectorPlugin";

SerialConnectorPlugin::SerialConnectorPlugin(QWidget* parent)
    :  ConnectorPluginInterface(parent)
    , ui(new Ui::SerialConnectorPlugin)
    , m_recording(false)
    , m_receiving_thread(Q_NULLPTR)
    , m_connected(false)
    , m_settings(Q_NULLPTR)
    , m_recorder(Q_NULLPTR)
    , m_scipTerminal(Q_NULLPTR)
    , m_baudrateChanger(Q_NULLPTR)
    , m_sensorInfo(Q_NULLPTR)
{
    ui->setupUi(this);

    m_receiving_thread = new Thread(&receivingThreadProcess, this);
    m_sensor = new UrgDevice;
    m_settings = new SerialSettingsForm();
    m_settings->setWindowFlags(Qt::Popup);

    registerUsbNotifier();

    ui->loadActivityIndicator->hide();
    ui->moreButton->setEnabled(false);

    ui->rebootButton->setEnabled(false);
    ui->resetButton->setEnabled(false);
    ui->cmdButton->setEnabled(false);
    ui->cmdButton->setVisible(false);

    m_connection_widget = ui->connectionWidget;

    m_sensor->setConnection(&m_connectionDevice);
    ui->skipScanWidget->setEnabled(false);

    connect(ui->settingsButton, &QToolButton::clicked,
            this, [=](){
        QPoint position = ui->settingsButton->mapToGlobal(QPoint(ui->settingsButton->width(), ui->settingsButton->height()));
        if(position.y() + m_settings->geometry().height() > QApplication::desktop()->screenGeometry().height()){
            position.setY(position.y() - m_settings->geometry().height() - ui->settingsButton->height());
        }
        if(position.x() + m_settings->geometry().width() > QApplication::desktop()->screenGeometry().width()){
            position.setX(position.x() - m_settings->geometry().width() + ui->settingsButton->width());
        }
        m_settings->move(position);
        m_settings->show();
        m_settings->raise();
    });


    ui->recorder->hide();

    ui->moreButton->setVisible(false);
    setupConnections();

    //    ui->modesGroup->setEnabled(true);
    rescanPressed();

    m_range_view_widget = ui->rangeViewWidget;

    m_paused = false;
    m_connected = false;
    m_started = false;

    resetUi();
}

void SerialConnectorPlugin::addDebugInformation(const QString &info)
{
    //    new QListWidgetItem(info, ui->debugList);
}

void SerialConnectorPlugin::addRecorder(PluginManagerInterface *manager)
{
    if(manager){
        GeneralPluginInterface *recorderInterface = manager->makePlugin("org.kristou.UrgBenri.LogRecorderHelperPlugin", this);
        if(recorderInterface){
            m_recorder = qobject_cast<HelperPluginInterface *>(recorderInterface);
            if(m_recorder){
                if(m_recorder->hasFunction("setDeviceMethod")
                        && m_recorder->hasFunction("addSensorDataMethod")
                        && m_recorder->hasFunction("stopMethod")){
                    m_recorder->callFunction("setDeviceMethod", QVariantList() << qVariantFromValue((void *)m_sensor));
                    ui->recorder->layout()->addWidget(m_recorder);
                    m_recorder->setEnabled(false);
                    ui->recorder->setVisible(true);
                }
                else{
                    delete m_recorder;
                    m_recorder = Q_NULLPTR;
                }
            }
        }
    }
}

void SerialConnectorPlugin::addScipTerminal(PluginManagerInterface *manager)
{
    if(manager){
        GeneralPluginInterface *pluginInterface = manager->makePlugin("org.kristou.UrgBenri.ScipTerminalHelperPlugin", Q_NULLPTR);
        if(pluginInterface){
            m_scipTerminal = qobject_cast<HelperPluginInterface *>(pluginInterface);
            if(m_scipTerminal){
                if(m_scipTerminal->hasFunction("setDeviceMethod")){
                    m_scipTerminal->callFunction("setDeviceMethod", qVariantFromValue((void *)m_sensor));
                    connect(ui->cmdButton, &QPushButton::clicked,
                            this, [=](){
                        if (!m_sensor->isConnected())return;
                        if(m_scipTerminal) {
                            if(shouldStop()) return;
                            showPluginModal(m_scipTerminal, this);
                        }
                    });
                    ui->cmdButton->setIcon(pluginInterface->pluginIcon());
                    ui->cmdButton->setVisible(true);
                }
                else{
                    delete m_scipTerminal;
                    m_scipTerminal = Q_NULLPTR;
                }
            }
        }
    }
}

void SerialConnectorPlugin::addBaudrateChanger(PluginManagerInterface *manager)
{
    if(manager){
        GeneralPluginInterface *pluginInterface = manager->makePlugin("org.kristou.UrgBenri.BaudrateChangerHelperPlugin", Q_NULLPTR);
        if(pluginInterface){
            m_baudrateChanger = qobject_cast<HelperPluginInterface *>(pluginInterface);
            if(m_baudrateChanger){
                if(m_baudrateChanger->hasFunction("setDeviceMethod")
                        && m_baudrateChanger->hasFunction("lastBaudrateMethod")){
                    m_baudrateChanger->callFunction("setDeviceMethod", qVariantFromValue((void *)m_sensor));
                    QPushButton *button = new QPushButton(tr("Change Baudrate"), this);
                    button->setEnabled(false);
                    ui->extraGroup->layout()->addWidget(button);

                    connect(this, &SerialConnectorPlugin::languageChanged,
                            this, [=](){
                        if(button) button->setText(tr("Change Baudrate"));
                    });
                    connect(button, &QAbstractButton::clicked,
                            this, [=](){
                        if (!m_sensor->isConnected()) {
                            emit information(LogHeader,
                                             tr("The sensor is not connected."));
                            return;
                        }
                        bool wasStarted = false;
                        if(m_started){
                            if(shouldStop()) return;
                            wasStarted = true;
                        }

                        showPluginModal(m_baudrateChanger, this);

                        if(m_sensor->isConnected()){
                            if(wasStarted){
                                start();
                            }
                        }else{
                            m_connection_widget->selectBauderate(m_baudrateChanger->callFunction("lastBaudrateMethod").toLongLong());


                            if(QMessageBox::information(this, QApplication::applicationName(),
                                                        tr("Would you like to reconnect to the sensor?"),
                                                        QMessageBox::Yes | QMessageBox::No,
                                                        QMessageBox::No) == QMessageBox::Yes
                                    ) {
                                connectPressed(false, NULL, 0);
                                ui->connectionWidget->connectSelectedDevice();
                            }
                            else{
                                connectPressed(false, NULL, 0);
                            }
                        }
                    });

                    connect(this, &SerialConnectorPlugin::connexionReady,
                            this, [=](){
                        if(button) button->setEnabled(true);
                    });

                    connect(this, &SerialConnectorPlugin::connexionLost,
                            this, [=](){
                        if(button) button->setEnabled(false);
                    });

                    button->setIcon(pluginInterface->pluginIcon());
                    button->setVisible(true);
                }
                else{
                    delete m_baudrateChanger;
                    m_baudrateChanger = Q_NULLPTR;
                }
            }
        }
    }
}

void SerialConnectorPlugin::addSensorInformation(PluginManagerInterface *manager)
{
    if(manager){
        GeneralPluginInterface *pluginInterface = manager->makePlugin("org.kristou.UrgBenri.SensorInformationHelperPlugin", Q_NULLPTR);
        if(pluginInterface){
            m_sensorInfo = qobject_cast<HelperPluginInterface *>(pluginInterface);
            if(m_sensorInfo){
                if(m_sensorInfo->hasFunction("setDeviceMethod")
                        && m_sensorInfo->hasFunction("noReloadMethod")){
                    m_sensorInfo->callFunction("setDeviceMethod", qVariantFromValue((void *)m_sensor));
                    connect(ui->moreButton, &QPushButton::clicked,
                            this, [&](){
                        if (!m_sensor->isConnected()) {
                            emit information(LogHeader,
                                             tr("The sensor is not connected."));
                            return;
                        }
                        m_sensorInfo->callFunction("noReloadMethod", !m_started);
                        showPluginModal(m_sensorInfo, this);
                    });
                    ui->moreButton->setIcon(pluginInterface->pluginIcon());
                    ui->moreButton->setVisible(true);
                }
                else{
                    delete m_sensorInfo;
                    m_sensorInfo = Q_NULLPTR;
                }
            }
        }
    }
}

void SerialConnectorPlugin::setupConnections()
{
    connect(m_connection_widget, &SerialConnectionWidget::rescanRequest,
            this, &SerialConnectorPlugin::rescanPressed);
    connect(m_connection_widget, &SerialConnectionWidget::connectRequest,
            this, &SerialConnectorPlugin::connectPressed);
    connect(&m_connectionCheck, &SerialConnectionCheck::connected,
            this, &SerialConnectorPlugin::deviceConnected);
    connect(&m_connectionCheck, &SerialConnectionCheck::connectionFailed,
            this, &SerialConnectorPlugin::deviceConnectFailed);
    
    m_urg_finder.addBaseName("/dev/ttyACM");
    m_urg_finder.addBaseName("/dev/tty.usbmodem");
    m_urg_finder.addDriverName("URG Series USB Device Driver");
    m_urg_finder.addDriverName("URG-X002 USB Device Driver");
    
    connect(ui->rebootButton, &QAbstractButton::clicked,
            this, &SerialConnectorPlugin::rebootSensor);
    connect(ui->resetButton, &QAbstractButton::clicked,
            this, &SerialConnectorPlugin::resetSensor);

    connect(ui->captureMode, &ModeSwitcherWidget::captureModeChanged,
            this, &SerialConnectorPlugin::captureModeChanged);
    connect(ui->startStep, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &SerialConnectorPlugin::startStepChanged);
    connect(ui->endStep, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &SerialConnectorPlugin::endStepChanged);
    connect(ui->groupSteps, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &SerialConnectorPlugin::groupStepsChanged);
    connect(ui->skipScans, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &SerialConnectorPlugin::skipScansChanged);
    connect(&m_connectionCheck, &SerialConnectionCheck::progress,
            this, &SerialConnectorPlugin::initProgress);
    connect(m_receiving_thread, &Thread::finished,
            this, &SerialConnectorPlugin::receivingThreadFinished);
}

void SerialConnectorPlugin::setupShortcuts()
{
}

bool SerialConnectorPlugin::isIntensityMode()
{
    return ui->captureMode->isIntensitySelected();
}

SerialConnectorPlugin::~SerialConnectorPlugin()
{
    if(m_receiving_thread){
        if(m_receiving_thread->isRunning()){
            m_receiving_thread->stop();
        }
        delete m_receiving_thread;
    }
    if(m_sensor){
        m_sensor->stop();
        if(m_sensor->connection()) m_sensor->connection()->disconnect();

        m_connected = false;
        delete m_sensor;
    }

    if(m_settings) delete m_settings;
    if(m_scipTerminal) delete m_scipTerminal;

    delete ui;
}

void SerialConnectorPlugin::start()
{
    if (!m_connected) {
        return;
    }
    
    
    if (m_receiving_thread->isRunning()) {
        return;
    }
    
    addDebugInformation(tr("Acquisition started"));
    if (! m_paused) {
        
        if (!setCaptureMode()) {
            QMessageBox::critical(this, QApplication::applicationName(),
                                  tr("The connected device does not support the selected capture mode.") +
                                  "\n" +
                                  tr("Please choose another capture mode."));
            stop();
            return;
        }
        
        m_sensor->setCaptureRange(ui->startStep->value(), ui->endStep->value());
        m_sensor->setCaptureGroupSteps(ui->groupSteps->value());
        m_sensor->setCaptureFrameInterval(ui->skipScans->value());
        
        ui->loadActivityIndicator->show();
    }
    else {
        m_paused = false;
        
        ui->loadActivityIndicator->show();
    }

    m_started = true;
    m_receiving_thread->run();
    emit started();
}

void SerialConnectorPlugin::stop()
{
    addDebugInformation(tr("Stopping acquisition if active"));
    if (!m_connected) {
        return;
    }
    
    if(m_recorder) m_recorder->callFunction("stopMethod");

    if (m_receiving_thread->isRunning()) {
        m_receiving_thread->stop();
    }
    ui->loadActivityIndicator->hide();
    
    m_sensor->stop();
    addDebugInformation(tr("Sensor stopped"));

    m_paused = false;
    m_started = false;
    emit stopped();
}

void SerialConnectorPlugin::pause()
{
    if (!m_connected) {
        return;
    }
    
    if (m_receiving_thread->isRunning()) {
        m_receiving_thread->stop();
    }
    ui->loadActivityIndicator->hide();

    m_paused = true;
    m_started = false;
    emit paused();
}

int SerialConnectorPlugin::receivingThreadProcess(void* args)
{
    SerialConnectorPlugin* obj = static_cast<SerialConnectorPlugin*>(args);
    int errorCnt = 0;
    int maxErrorCnt = 5;

    SensorDataArray l_ranges;
    SensorDataArray l_levels;
    long l_timeStamp = 0;
    int n = 0;

    //    QTime timer;
    while (!obj->m_receiving_thread->exitThread) {
        if (errorCnt >= maxErrorCnt) {
            QMetaObject::invokeMethod(obj, "errorHandle");
            break;
        }

        //        timer.restart();
        n = obj->m_sensor->capture(l_ranges, l_levels, l_timeStamp);

        if (n == -2) { //Time out error
            errorCnt++;
            continue;
        }

        if(n <= 0){
            QMetaObject::invokeMethod(obj, "errorHappened");
        }

        obj->setLengthData(l_ranges, l_levels, l_timeStamp);

        errorCnt = 0;
    }
    return 0;
}

void SerialConnectorPlugin::errorHappened()
{
    addDebugInformation(tr("Sensor error: %1").arg(m_sensor->what()));
}

void SerialConnectorPlugin::captureModeChanged()
{
    ui->skipScanWidget->setEnabled(ui->captureMode->isContiousSelected());
    restart();
}

void SerialConnectorPlugin::errorHandle()
{
    //    qCritical() << "SerialCommunicatorWidget::errorHandle";

    emit error(LogHeader,
               tr("Error accured in the sensor.") + QString("\n%1")
               .arg(m_sensor->what()));
    addDebugInformation(tr("Error accured in the sensor.") + QString("\n%1"));

    if(m_settings->isAutoReconnectActive()){
        ui->connectionWidget->disconnectSelectedDevice();
        ui->connectionWidget->connectSelectedDevice();
    }
}

void SerialConnectorPlugin::onLoad(PluginManagerInterface *manager)
{
    addRecorder(manager);
    addScipTerminal(manager);
    addBaudrateChanger(manager);
    addSensorInformation(manager);

    resetUi();
}

void SerialConnectorPlugin::onUnload()
{

}

void SerialConnectorPlugin::setLengthData(const SensorDataArray &l_ranges, const SensorDataArray &l_levels, long l_timeStamp)
{
    if(m_recorder) m_recorder->callFunction("addSensorDataMethod", QVariantList() << qVariantFromValue((void *)&l_ranges)
                                            << qVariantFromValue((void *)&l_levels)
                                            << qVariantFromValue((void *)&l_timeStamp));

    PluginDataStructure data;
    data.ranges = l_ranges.steps;
    data.levels = l_levels.steps;
    data.timestamp = l_timeStamp;
    data.converter = QSharedPointer<DataConverterInterface>(new SerialDataConverter(l_ranges.converter));
    emit measurementDataReady(data);
}

void SerialConnectorPlugin::saveState(QSettings &settings)
{
    settings.setValue("capture_mode", ui->captureMode->state());

    settings.setValue("start_step", ui->startStep->value());
    settings.setValue("end_step", ui->endStep->value());
    settings.setValue("skip_scan", ui->skipScans->value());
    settings.setValue("group_steps", ui->groupSteps->value());

    if(m_baudrateChanger){
        settings.beginGroup(m_baudrateChanger->metaObject()->className());
        m_baudrateChanger->saveState(settings);
        settings.endGroup();
    }
    if(m_scipTerminal){
        settings.beginGroup(m_scipTerminal->metaObject()->className());
        m_scipTerminal->saveState(settings);
        settings.endGroup();
    }
    if(m_recorder){
        settings.beginGroup(m_recorder->metaObject()->className());
        m_recorder->saveState(settings);
        settings.endGroup();
    }
    if(m_sensorInfo){
        settings.beginGroup(m_sensorInfo->metaObject()->className());
        m_sensorInfo->saveState(settings);
        settings.endGroup();
    }

    settings.beginGroup("SettingsDialog");
    m_settings->saveState(settings);
    settings.endGroup();
}

void SerialConnectorPlugin::restoreState(QSettings &settings)
{
    ui->captureMode->setState(settings.value("capture_mode", "").toString());

    ui->startStep->setValue(settings.value("start_step", ui->startStep->value()).toInt());
    ui->endStep->setValue(settings.value("end_step", ui->endStep->value()).toInt());
    ui->skipScans->setValue(settings.value("skip_scan", ui->skipScans->value()).toInt());
    ui->groupSteps->setValue(settings.value("group_steps", ui->groupSteps->value()).toInt());

    if(m_baudrateChanger){
        settings.beginGroup(m_baudrateChanger->metaObject()->className());
        m_baudrateChanger->restoreState(settings);
        settings.endGroup();
    }
    if(m_scipTerminal){
        settings.beginGroup(m_scipTerminal->metaObject()->className());
        m_scipTerminal->restoreState(settings);
        settings.endGroup();
    }
    if(m_recorder){
        settings.beginGroup(m_recorder->metaObject()->className());
        m_recorder->restoreState(settings);
        settings.endGroup();
    }
    if(m_sensorInfo){
        settings.beginGroup(m_sensorInfo->metaObject()->className());
        m_sensorInfo->restoreState(settings);
        settings.endGroup();
    }

    settings.beginGroup("SettingsDialog");
    m_settings->restoreState(settings);
    settings.endGroup();
}

void SerialConnectorPlugin::loadTranslator(const QString &locale)
{
    qApp->removeTranslator(&m_translator);
    if (m_translator.load(QString("plugin.%1").arg(locale), ":/SerialConnectorPlugin")) {
        qApp->installTranslator(&m_translator);
    }

    if(m_recorder) m_recorder->loadTranslator(locale);
    if(m_scipTerminal) m_scipTerminal->loadTranslator(locale);
    if(m_baudrateChanger) m_baudrateChanger->loadTranslator(locale);
    if(m_sensorInfo) m_sensorInfo->loadTranslator(locale);
}

void SerialConnectorPlugin::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            if(ui) ui->retranslateUi(this);
            emit languageChanged();
            break;
        default:
            break;
    }
}

void SerialConnectorPlugin::updateCaptureMode(RangeCaptureMode mode)
{
    ui->captureMode->setCaptureMode(mode);
}

void SerialConnectorPlugin::rescanPressed()
{
    QStringList devices;
    m_urg_finder.find(devices);
    if(m_sensor->isConnected()){
        QString deviceName = m_sensor->connection()->getDevice();
        if(!devices.contains(deviceName)){
            stop();
            connectPressed(false, deviceName, 0);
            rescanPressed();
            QMessageBox::critical(this, QApplication::applicationName(),
                                  tr("Connected sensor suddenly disconnected!"));
        }
    }else{
        for (QStringList::iterator it = devices.begin();
             it != devices.end(); ++it) {
            if (m_urg_usb.isUsbCom(*it)) {
                *it = *it + " [URG]";
            }
        }
        m_connection_widget->setDevices(devices);
    }
}


void SerialConnectorPlugin::connectPressed(bool connection, const QString &device, long baude)
{
    if (connection) {
        addDebugInformation(tr("Connection requested"));
        m_connection_widget->setConnected(true);
        ui->loadActivityIndicator->setInvertedAppearance(true);
        ui->loadActivityIndicator->setValue(0);
        ui->loadActivityIndicator->show();
        m_connectionCheck.setConnectionRetriesCount(m_settings->connectionRetries());
        m_sensor->setConnectionDebug(m_settings->isDebugActive()
                                     , m_settings->debugDirectory()
                                     , m_settings->sendPrefix()
                                     , m_settings->recievePrefix());
        m_connectionCheck.setSensor(m_sensor);
        m_connectionCheck.setName(device);
        m_connectionCheck.setBauderate(baude);
        m_connectionCheck.check();
    }
    else {
        addDebugInformation(tr("Disconnection requested"));
        m_connectionCheck.stop();
        stop();
        resetUi();
        m_connected = false;
        m_connection_widget->setConnected(m_connected);

        if(m_recorder) m_recorder->setEnabled(m_connected);

        m_sensor->disconnect();
        addDebugInformation(tr("Sensor disconnected"));
        
        emit information(LogHeader,
                         tr("Serial device disconnected."));
        emit connexionLost();
    }
    ui->rebootButton->setEnabled(m_connected);
    ui->resetButton->setEnabled(m_connected);
    ui->cmdButton->setEnabled(m_connected);
    ui->moreButton->setEnabled(m_connected);
}

void SerialConnectorPlugin::deviceConnected()
{
    bool errorPresent = false;
    addDebugInformation(tr("Device connected"));
    
    if(m_connection_widget->selectedBaudrate() != m_sensor->connection()->baudrate()){
        QMessageBox::information(this, tr("Serial connection"),
                                 tr("The sensor is operate on a different speed."));
    }
    
    m_connection_widget->selectBauderate(m_sensor->connection()->baudrate());
    if (!m_sensor->loadParameters()) {
        emit error(LogHeader,
                   m_sensor->what());
    }

    if(m_settings->isShowSensorErrorWarningActive()){
        QString sensorStatus = QString::fromStdString(m_sensor->internalInformation().sensorSituation);
        if(sensorStatus.toLower().contains("error") &&
                !sensorStatus.toLower().contains("no error")){
            QMessageBox::critical(this, QApplication::applicationName(),
                                  tr("The sensor is in error state.") + "\n" +
                                  sensorStatus);
            errorPresent = true;
        }
    }

    ui->loadActivityIndicator->hide();
    ui->loadActivityIndicator->setInvertedAppearance(false);
    updateUiWithInfo();
    m_connected = true;
    ui->rebootButton->setEnabled(m_connected);
    ui->resetButton->setEnabled(m_connected);
    ui->cmdButton->setEnabled(m_connected);
    ui->moreButton->setEnabled(m_connected);
    m_connection_widget->setConnected(m_connected);

    if(m_recorder) m_recorder->setEnabled(m_connected);

    emit information(LogHeader,
                     tr("Serial device connected."));
    
    if(errorPresent){
        if(m_sensorInfo) m_sensorInfo->callFunction("noReloadMethod", !m_started);
    }else{
        if(m_settings->isAutoStartActive()){
            start();
        }
    }
    
    emit connexionReady();
}

void SerialConnectorPlugin::deviceConnectFailed()
{
    ui->loadActivityIndicator->hide();
    ui->loadActivityIndicator->setInvertedAppearance(false);
    stop();
    m_connected = false;

    ui->rebootButton->setEnabled(m_connected);
    ui->resetButton->setEnabled(m_connected);
    ui->cmdButton->setEnabled(m_connected);
    ui->moreButton->setEnabled(m_connected);
    m_connection_widget->setConnected(m_connected);
    resetUi();

    if(m_recorder) m_recorder->setEnabled(m_connected);

    QMessageBox::critical(this, QApplication::applicationName(),
                          tr("Connection failed.") + "\n" +
                          tr("Reason: %1").arg(m_sensor->what()));
    
    emit connexionLost();
}

void SerialConnectorPlugin::updateUiWithInfo()
{
    
    //    RangeSensorInformation urgInfo = m_urg->information();
    RangeSensorParameter urgParam = m_sensor->parameter();
    
    ui->modelLine->setText(m_sensor->getType());
    ui->serialLine->setText(m_sensor->getSerialNumber());
    
    ui->startStep->setMinimum(m_sensor->minArea());
    ui->startStep->setMaximum(m_sensor->maxArea());
    ui->startStep->setValue(m_sensor->minArea());
    
    ui->endStep->setMinimum(m_sensor->minArea());
    ui->endStep->setMaximum(m_sensor->maxArea());
    ui->endStep->setValue(m_sensor->maxArea());
    
    m_range_view_widget->setParameters(urgParam.area_front, urgParam.area_total);
    m_range_view_widget->setRange(m_sensor->minArea(), m_sensor->maxArea());

    if(m_settings->isCheckSupportedModesChecked()){
        ui->captureMode->setSupportedModes(m_sensor->supportedModes());
    }
}


void SerialConnectorPlugin::resetUi()
{
    ui->modelLine->setText("");
    ui->serialLine->setText("");
    
    ui->startStep->setValue(0);
    ui->startStep->setMinimum(0);
    ui->startStep->setMaximum(0);
    ui->endStep->setValue(0);
    ui->endStep->setMinimum(0);
    ui->endStep->setMaximum(0);
    ui->groupSteps->setValue(1);
    
    //    ui->modesGroup->setEnabled(true);
    m_range_view_widget->unsetParameters();
    
    //    ui->captureMode->reset();
}

void SerialConnectorPlugin::initProgress(int value)
{
    ui->loadActivityIndicator->setValue(value);
}

bool SerialConnectorPlugin::shouldStop()
{
    if (m_started) {
        switch (QMessageBox::question(
                    this,
                    QApplication::applicationName(),
                    tr("Data acquisition is running.") + "\n" +
                    tr("Do you want to stop it?"),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No)
                ) {
            case QMessageBox::Yes:
                stop();
                break;
            case QMessageBox::No:
                return true;
                break;
            default:
                break;
        }
    }

    return false;
}

void SerialConnectorPlugin::rebootSensor()
{
    if (m_sensor->isConnected()) {
        if(shouldStop()) return;
        QVector<string> lines;
        m_sensor->commandLines("RB\n", lines);
        m_sensor->commandLines("RB\n", lines);

        QEventLoop ev;
        QTimer timer;
        connect(&timer, &QTimer::timeout,
                &ev, &QEventLoop::quit);
        timer.start(1000);
        ev.exec();

        if(QMessageBox::information(this, QApplication::applicationName(),
                                    tr("Would you like to reconnect to the sensor?"),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No) == QMessageBox::Yes
                ) {
            connectPressed(false, NULL, 0);
            ui->connectionWidget->connectSelectedDevice();
        }
        else{
            connectPressed(false, NULL, 0);
        }

        emit information(LogHeader,
                         tr("The sensor is rebooted."));
    }
    else {
        emit information(LogHeader,
                         tr("The sensor is not connected."));
    }
}

void SerialConnectorPlugin::resetSensor()
{
    if (m_sensor->isConnected()) {
        bool started = isStarted();
        if(started && shouldStop()) return;
        QVector<string> lines;
        if(m_sensor->commandLines("RS\n", lines)){
            emit information(LogHeader,
                             tr("The sensor is reset."));
        }
        if(started) start();
    }
    else {
        emit information(LogHeader,
                         tr("The sensor is not connected."));
    }
}

void SerialConnectorPlugin::receivingThreadFinished()
{
    emit information(LogHeader,
                     tr("Data acquisition finished."));
    if(!m_receiving_thread->isRunning()) stop();
}

void SerialConnectorPlugin::serialBaudrateChanged(long baud)
{
    m_sensor->connection()->setBaudrate(baud);
    m_connection_widget->selectBauderate(baud);
}

bool SerialConnectorPlugin::setCaptureMode()
{
    RangeCaptureMode captureMode = ui->captureMode->captureMode();
    
    if (m_sensor->isSupportedMode(captureMode)) {
        m_sensor->setCaptureMode(captureMode);
        return true;
    }
    else {
        return false;
    }
}

void SerialConnectorPlugin::startStepChanged(int value)
{
    ui->endStep->setMinimum(value);
    
    m_range_view_widget->setRange(ui->startStep->value(), ui->endStep->value());
    restart();
}


void SerialConnectorPlugin::endStepChanged(int value)
{
    ui->startStep->setMaximum(value);
    
    m_range_view_widget->setRange(ui->startStep->value(), ui->endStep->value());
    restart();
}

void SerialConnectorPlugin::groupStepsChanged(int value)
{
    Q_UNUSED(value);
    restart();
}

void SerialConnectorPlugin::skipScansChanged(int value)
{
    Q_UNUSED(value);
    restart();
}


void SerialConnectorPlugin::receivingTimerProcess()
{
    SensorDataArray l_ranges;
    SensorDataArray l_levels;
    long l_timeStamp;
    
    int n = m_sensor->capture(l_ranges, l_levels, l_timeStamp);
    if (n <= 0) {
        return;
    }
    setLengthData(l_ranges, l_levels, l_timeStamp);

}

void SerialConnectorPlugin::restart()
{
    if (m_started) {
        stop();
        delay(m_sensor->scanMsec());
        start();
    }
    
    if (m_paused) {
        stop();
        delay(m_sensor->scanMsec());
        start();
        pause();
    }
}

void SerialConnectorPlugin::registerUsbNotifier()
{
#ifdef Q_OS_WIN
    GUID GUID_DEVINTERFACE_USB_DEVICE = {0x4D36E978, 0xE325, 0x11CE, {0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18}};
    // Register for device connect notification
    DEV_BROADCAST_DEVICEINTERFACE devInt;
    ZeroMemory( &devInt, sizeof(devInt) );
    devInt.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    devInt.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    devInt.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;

    HDEVNOTIFY m_hDeviceNotify =
            RegisterDeviceNotification( (HWND)winId(), &devInt, DEVICE_NOTIFY_WINDOW_HANDLE );
#endif
}

bool SerialConnectorPlugin::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
    Q_UNUSED(eventType);

#ifdef Q_OS_WIN32
    return winEvent(reinterpret_cast<MSG*>(message), result);
#endif
}

#ifdef Q_OS_WIN32

bool SerialConnectorPlugin::winEvent(MSG *message, long *result)
{
    //    qCritical() << "SerialConnectorPlugin::winEvent" << message->message << message->wParam;
    Q_UNUSED(result);

    if((message->message == WM_DEVICECHANGE) && ((message->wParam == DBT_DEVICEARRIVAL) ||
                                                 (message->wParam == DBT_DEVICEREMOVECOMPLETE))){
        rescanPressed();
    }

    return false; // let qt handle the rest
}
#endif

