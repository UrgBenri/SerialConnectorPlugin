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

#include "SerialConnectionWidgetSample.h"
#include "SerialConnectionWidget.h"
#include <QShortcut>
#include <cstdio>

using namespace qrk;
using namespace std;


struct SerialConnectionWidgetSample::pImpl {
    SerialConnectionWidgetSample* widget_;
    SerialConnectionWidget connection_widget_;

    vector<string> devices_;
    size_t last_index_;


    pImpl(SerialConnectionWidgetSample* widget)
        : widget_(widget), connection_widget_(widget), last_index_(0) {
    }


    void initializeForm(void) {
        // ウィジット
        widget_->connection_dummy_label_->hide();
        widget_->main_layout_->addWidget(&connection_widget_);

        // コンポーネント
        connect(&connection_widget_,
                SIGNAL(connectRequest(bool, const std::string &)),
                widget_, SLOT(connectPressed(bool, const std::string &)));
        connect(&connection_widget_, SIGNAL(rescanRequest()),
                widget_, SLOT(rescanPressed()));

        // Ctrl-q で終了させる
        (void) new QShortcut(Qt::CTRL + Qt::Key_Q, widget_, SLOT(close()));
    }


    void addDevice(void) {
        char buffer[13];
        snprintf(buffer, 13, "%d", last_index_++);

        devices_.push_back("device: " + string(buffer));
        connection_widget_.setDevices(devices_);
    }
};


SerialConnectionWidgetSample::SerialConnectionWidgetSample(QWidget* parent)
    : QWidget(parent), pimpl(new pImpl(this))
{
    setupUi(this);
    pimpl->initializeForm();
}


SerialConnectionWidgetSample::~SerialConnectionWidgetSample(void)
{
}


void SerialConnectionWidgetSample::connectPressed(bool connection,
        const string &device)
{
    if (connection) {
        fprintf(stderr, "connect request: %s\n", device.c_str());

    }
    else {
        fprintf(stderr, "disconnect request\n");
    }

    // 接続、切断、の要求を成功扱いにする
    pimpl->connection_widget_.setConnected(connection);
}


void SerialConnectionWidgetSample::rescanPressed(void)
{
    fprintf(stderr, "rescan pressed.\n");
    pimpl->addDevice();
}

