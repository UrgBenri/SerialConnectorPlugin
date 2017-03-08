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

#include <fcntl.h>
#ifndef WIN32
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif
#include <cerrno>
#include <cstring>
#include <cstdio>


class RawSerialDevice
{
    enum {
        InvalidFd = -1,
    };

    string error_message_;
    int fd_;
    struct termios sio_;        //!< 通信ターミナル制御
    fd_set rfds_;               //!< タイムアウト制御


    bool waitReceive(int timeout) {
        // タイムアウト設定
        FD_ZERO(&rfds_);
        FD_SET(fd_, &rfds_);

        struct timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;

        if (select(fd_ + 1, &rfds_, NULL, NULL,
                   (timeout < 0) ? NULL : &tv) <= 0) {
            /* タイムアウト発生 */
            return false;
        }
        return true;
    }


public:
    RawSerialDevice(void) : error_message_("no error."), fd_(InvalidFd) {
    }


    const char* what(void) {
        return error_message_.c_str();
    }


    bool connect(const char* device, long baudrate) {
#ifndef Q_OS_MACX
        enum { O_EXLOCK = 0x0 }; // Linux では使えないのでダミーを作成しておく
#endif
        fd_ = open(device, O_RDWR | O_EXLOCK | O_NONBLOCK | O_NOCTTY);
        if (fd_ < 0) {
            // 接続に失敗
            error_message_ = string(device) + ": " + strerror(errno);
            return false;
        }
        int flags = fcntl(fd_, F_GETFL, 0);
        fcntl(fd_, F_SETFL, flags & ~O_NONBLOCK);

        // シリアル通信の初期化
        tcgetattr(fd_, &sio_);
        sio_.c_iflag = 0;
        sio_.c_oflag = 0;
        sio_.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
        sio_.c_cflag |= CS8 | CREAD | CLOCAL;
        sio_.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);

        sio_.c_cc[VMIN] = 0;
        sio_.c_cc[VTIME] = 0;

        // ボーレートの変更
        if (! setBaudrate(baudrate)) {
            return false;
        }
        return true;
    }


    void disconnect(void) {
        if (fd_ != InvalidFd) {
            close(fd_);
            fd_ = InvalidFd;
        }
    }


    bool isConnected(void) {
        return (fd_ == InvalidFd) ? false : true;
    }


    bool setBaudrate(long baudrate) {
        long baudrate_value = -1;
        enum { ErrorMessageSize = 256 };
        char error_message[ErrorMessageSize];

        switch (baudrate) {

        case 4800:
            baudrate_value = B4800;
            break;

        case 9600:
            baudrate_value = B9600;
            break;

        case 19200:
            baudrate_value = B19200;
            break;

        case 38400:
            baudrate_value = B38400;
            break;

        case 57600:
            baudrate_value = B57600;
            break;

        case 115200:
            baudrate_value = B115200;
            break;

        default:
            sprintf(error_message, "No handle baudrate value: %ld", baudrate);
            error_message_ = string(error_message);
            return false;
        }

        /* ボーレート変更 */
        cfsetospeed(&sio_, baudrate_value);
        cfsetispeed(&sio_, baudrate_value);
        tcsetattr(fd_, TCSANOW, &sio_);
        flush();

        return true;
    }


    int send(const char* data, int count) {
        if (! isConnected()) {
            error_message_ = "no connection.";
            return 0;
        }
        return write(fd_, data, count);
    }

    unsigned long receivedSize(){
        unsigned long length = 4094;
        ioctl(fd_, FIONREAD, &length);
        return length;
    }


    int receive(char buffer[], int count, int timeout) {
        int filled = 0;

        // 指定サイズの読み出しを行う
        int n = read(fd_, buffer, count);
        if (n < 0) {
            return n;
        }
        filled += n;

        // 受信が完了していたら、戻る
        if (filled >= count) {
            return filled;
        }

        // タイムアウト付きで読み出しを行う
        while (filled < count) {
            if (! waitReceive(timeout)) {
                break;
            }

            int required_n = count - filled;
            n = read(fd_, &buffer[filled], required_n);
            if (n <= 0) {
                /* 読み出しエラー。現在までの受信内容で戻る */
                break;
            }
            filled += n;
        }
        return filled;
    }


    void flush(void) {
        tcdrain(fd_);
        tcflush(fd_, TCIOFLUSH);
    }
};

