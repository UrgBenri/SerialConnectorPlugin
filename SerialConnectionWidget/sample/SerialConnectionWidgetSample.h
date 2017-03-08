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

#ifndef QRK_SERIAL_CONNECTION_WIDGET_SAMPLE_H
#define QRK_SERIAL_CONNECTION_WIDGET_SAMPLE_H

/*!
  \file
  \brief SerialConnectionWidget の動作確認ウィジット

  \author Satofumi KAMIMURA

  $Id: SerialConnectionWidgetSample.h 57 2012-06-12 04:43:55Z kristou $
*/

#include "ui_SerialConnectionWidgetSampleForm.h"
#include <memory>


class SerialConnectionWidgetSample
    : public QWidget, private Ui::SerialConnectionWidgetSampleForm
{
    Q_OBJECT;

public:
    SerialConnectionWidgetSample(QWidget* parent = 0);
    ~SerialConnectionWidgetSample(void);

private slots:
    void connectPressed(bool connection, const std::string &device);
    void rescanPressed(void);

private:
    SerialConnectionWidgetSample(const SerialConnectionWidgetSample &rhs);
    SerialConnectionWidgetSample &operator
        = (const SerialConnectionWidgetSample &rhs);

    struct pImpl;
    std::auto_ptr<pImpl> pimpl;
};

#endif /* !QRK_SERIAL_CONNECTION_WIDGET_SAMPLE_H */

