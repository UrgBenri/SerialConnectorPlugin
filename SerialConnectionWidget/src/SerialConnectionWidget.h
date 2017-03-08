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

#ifndef QRK_SERIAL_CONNECTION_WIDGET_H
#define QRK_SERIAL_CONNECTION_WIDGET_H

#include "ui_SerialConnectionWidgetForm.h"
#include <memory>

class SerialConnectionWidget :
        public QWidget, private Ui::SerialConnectionWidgetForm
{
    Q_OBJECT

public:
    SerialConnectionWidget(QWidget* parent = 0);
    virtual ~SerialConnectionWidget(void);

    void setConnected(bool connected);
    bool isConnected(void) const;

    void setDevices(const QStringList &devices);
    void selectDevice(const QString &portName);
    void selectBauderate(long baud);
    QStringList devices(void);

    void setEnabled(bool enable);
    void setFocus(void);

    void connectSelectedDevice();
    void disconnectSelectedDevice();

    void fireRescan();
    void fireConnect();
    void fireDisconnect();

    bool isDeviceUSB() const;

    QString selectedDevice() const;
    long selectedBaudrate() const;

signals:
    void connectRequest(bool connection, const QString &device, long baude);
    void rescanRequest(void);

protected:
    void changeEvent(QEvent *e);

private slots:
    void connectPressed();
    void disconnectPressed();
    void rescanPressed();
    void deviceChanged(int index);

private:
    SerialConnectionWidget(const SerialConnectionWidget &rhs);
    SerialConnectionWidget &operator = (const SerialConnectionWidget &rhs);
    bool m_isUSB;

    struct pImpl;
    std::auto_ptr<pImpl> pimpl;
    QStringList sortDevices(QStringList devices);
    void retranslateUi();
};

#endif /*! QRK_SERIAL_CONNECTION_WIDGET_H */

