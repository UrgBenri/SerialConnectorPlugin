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

#include "SerialConnectionWidget.h"

#include <QSortFilterProxyModel>
#include <QDebug>

using namespace std;

#define URG_DEFAULT_BAUDERATE 115200

struct SerialConnectionWidget::pImpl {
    SerialConnectionWidget* widget_;
    bool is_empty_;


    pImpl(SerialConnectionWidget* widget) : widget_(widget), is_empty_(true) {
    }

    void initializeForm(void) {
        widget_->baude_combobox_->addItem("19.2Kbps", 19200);
        widget_->baude_combobox_->addItem("57.6Kbps", 57600);
        widget_->baude_combobox_->addItem("115.2Kbps", 115200);
        widget_->baude_combobox_->addItem("230.4Kbps", 230400);
        widget_->baude_combobox_->addItem("250Kbps", 250000);
        widget_->baude_combobox_->addItem("460.8Kbps", 460800);
        widget_->baude_combobox_->addItem("500Kbps", 500000);
        widget_->baude_combobox_->addItem("614.4Kbps", 614400);
        widget_->baude_combobox_->addItem("750Kbps", 750000);
        widget_->baude_combobox_->addItem("781.25Kbps", 781250);
        widget_->baude_combobox_->addItem("921.6Kbps", 921600);
        widget_->baude_combobox_->addItem("1Mbps", 1000000);
        widget_->baude_combobox_->addItem("1.5Mbps", 1500000);
        widget_->baude_combobox_->addItem("1.8432Mbps", 1843200);
        widget_->selectBauderate(URG_DEFAULT_BAUDERATE);

        widget_->device_combobox_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        widget_->baude_combobox_->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        connect(widget_->connect_button_, &QAbstractButton::clicked,
                widget_, &SerialConnectionWidget::connectPressed);
        connect(widget_->disconnect_button_, &QAbstractButton::clicked,
                widget_, &SerialConnectionWidget::disconnectPressed);
        connect(widget_->rescan_button_, &QAbstractButton::clicked,
                widget_, &SerialConnectionWidget::rescanPressed);
        connect(widget_->device_combobox_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                widget_, &SerialConnectionWidget::deviceChanged);
    }
};


SerialConnectionWidget::SerialConnectionWidget(QWidget* widget):
    QWidget(widget),
    m_isUSB(false),
    pimpl(new pImpl(this))
{
    setupUi(this);
    retranslateUi();
    pimpl->initializeForm();

    QStringList empty_devices;
    setDevices(empty_devices);
}

SerialConnectionWidget::~SerialConnectionWidget(void)
{
}

void SerialConnectionWidget::connectPressed()
{
    connect_button_->setEnabled(false);
    disconnect_button_->setEnabled(false);
    QString device_name = device_combobox_->currentText();
    QString device = device_name.mid(0, device_name.indexOf(' '));
    long currentBaude = baude_combobox_->itemData(baude_combobox_->currentIndex()).toDouble();

    emit connectRequest(true, device, currentBaude);
}

void SerialConnectionWidget::disconnectPressed()
{
    connect_button_->setEnabled(false);
    disconnect_button_->setEnabled(false);
    QString device_name = device_combobox_->currentText();
    QString device = device_name.mid(0, device_name.indexOf(' '));
    long currentBaude = baude_combobox_->itemData(baude_combobox_->currentIndex()).toDouble();

    emit connectRequest(false, device, currentBaude);
}

void SerialConnectionWidget::rescanPressed()
{
    emit rescanRequest();
}

void SerialConnectionWidget::deviceChanged(int index)
{

    if(device_combobox_->itemText(index).contains(" ")){
        selectBauderate(URG_DEFAULT_BAUDERATE);
        baude_combobox_->setEnabled(false);
        m_isUSB = true;
    }else{
        baude_combobox_->setEnabled(device_combobox_->isEnabled());
        m_isUSB = false;
    }
}

void SerialConnectionWidget::setConnected(bool connected)
{
    connect_button_->setEnabled(! connected);
    disconnect_button_->setEnabled(connected);
    device_combobox_->setEnabled(! connected);
    rescan_button_->setEnabled(! connected);

    deviceChanged(device_combobox_->currentIndex());
}


bool SerialConnectionWidget::isConnected(void) const
{
    return !connect_button_->isEnabled();
}

struct MyLessThan {
    bool operator()(const QString &s1, const QString &s2) const {
        QString ss1 = s1;
        QString ss2 = s2;
        ss1.remove("COM", Qt::CaseInsensitive);
        ss2.remove("COM", Qt::CaseInsensitive);

        if(ss1.contains(" ") && ss2.contains(" ")){
            ss1.remove(" [UST]", Qt::CaseInsensitive); ss1.remove(" [URG]", Qt::CaseInsensitive);
            ss2.remove(" [UST]", Qt::CaseInsensitive); ss2.remove(" [URG]", Qt::CaseInsensitive);
            return ss1.toInt() <= ss2.toInt();
        }else if(ss1.contains(" ")){
            return true;
        }else{
            return ss1.toInt() <= ss2.toInt();
        }
    }
};

QStringList SerialConnectionWidget::sortDevices(QStringList devices)
{
    QStringList result = devices;
    MyLessThan le;
    qSort(result.begin(), result.end(), le);
    return result;
}

void SerialConnectionWidget::setDevices(const QStringList &devices)
{
    int index = qMax(0, device_combobox_->currentIndex());
    device_combobox_->blockSignals(true);
    device_combobox_->clear();
    QStringList orderedDevices = sortDevices(devices);
    for (QStringList::const_iterator it = orderedDevices.begin();
         it != orderedDevices.end(); ++it) {
        device_combobox_->addItem(*it);

    }
    device_combobox_->blockSignals(false);

    device_combobox_->setCurrentIndex(index);

    pimpl->is_empty_ = device_combobox_->count() == 0;
    device_combobox_->setEnabled(!pimpl->is_empty_);
    connect_button_->setEnabled(!pimpl->is_empty_);
    disconnect_button_->setEnabled(false);

    if (pimpl->is_empty_) {
        device_combobox_->addItem(tr("Empty"));
    }
    deviceChanged(index);
}

void SerialConnectionWidget::selectDevice(const QString &portName)
{
    for(int i = 0; i < device_combobox_->count(); ++i){
        QString name = device_combobox_->itemText(i);
        QStringList parts = name.split(" ", QString::SkipEmptyParts);
        if((parts.size() > 0) && (parts[0].trimmed() == portName)){
            device_combobox_->setCurrentIndex(i);
            deviceChanged(i);
            return;
        }
    }
}

void SerialConnectionWidget::selectBauderate(long baud)
{
    for(int i = 0; i < baude_combobox_->count(); ++i){
        if(baude_combobox_->itemData(i).toDouble() == baud){
            baude_combobox_->setCurrentIndex(i);
            break;
        }
    }
}


QStringList SerialConnectionWidget::devices(void)
{
    QStringList devices;

    if (! pimpl->is_empty_) {
        size_t n = device_combobox_->count();
        for (size_t i = 0; i < n; ++n) {
            devices.push_back(device_combobox_->itemText(n));
        }
    }

    return devices;
}


void SerialConnectionWidget::setEnabled(bool enable)
{
    QWidget::setEnabled(enable);
}


void SerialConnectionWidget::setFocus(void)
{
    if (! connect_button_->isEnabled()) {
        rescan_button_->setFocus();
    }
    else {
        connect_button_->setFocus();
    }
}

void SerialConnectionWidget::connectSelectedDevice()
{
    if(connect_button_->isEnabled()){
        connectPressed();
    }
}

void SerialConnectionWidget::disconnectSelectedDevice()
{
    if(disconnect_button_->isEnabled()){
        disconnectPressed();
    }
}

void SerialConnectionWidget::fireRescan()
{
    rescanPressed();
}

void SerialConnectionWidget::fireConnect()
{
    if(connect_button_->isEnabled()){
        connectPressed();
    }
}

void SerialConnectionWidget::fireDisconnect()
{
    if(disconnect_button_->isEnabled()){
        disconnectPressed();
    }
}

bool SerialConnectionWidget::isDeviceUSB() const
{
    return m_isUSB;
}

QString SerialConnectionWidget::selectedDevice() const
{
    QString device_name = device_combobox_->currentText();
    return device_name.mid(0, device_name.indexOf(' '));
}

long SerialConnectionWidget::selectedBaudrate() const
{
    return baude_combobox_->itemData(baude_combobox_->currentIndex()).toDouble();
}

void SerialConnectionWidget::retranslateUi()
{
    if (pimpl->is_empty_ && device_combobox_) {
        device_combobox_->clear();
        device_combobox_->addItem(tr("Empty"));
    }
    if (connect_button_) {
        connect_button_->setToolTip(tr("Connect device"));
    }
    if (disconnect_button_) {
        disconnect_button_->setToolTip(tr("Disconnect device"));
    }
    if (rescan_button_) {
        rescan_button_->setToolTip(tr("Rescan"));
    }
}

void SerialConnectionWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            retranslateUi();
            break;
        default:
            break;
    }
}

