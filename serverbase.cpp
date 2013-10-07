#include "serverbase.h"
#include "ui_serverbase.h"

ServerBase::ServerBase(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ServerBase)
{
    ui->setupUi(this);

    connect(&tcpServer,SIGNAL(newConnection()),this,SLOT(acceptConnection()));
}

ServerBase::~ServerBase()
{
    delete ui;
}

void ServerBase::start() //开始监听
{
    ui->startButton->setEnabled(false);
    bytesReceived =0;
    if(!tcpServer.listen(QHostAddress("127.0.0.1"),6666))
    {
        qDebug() << tcpServer.errorString();
        close();
        return;
    }
    ui->serverStatusLabel->setText(tr("监听"));
}

void ServerBase::acceptConnection()  //接受连接
{
    tcpServerConnection = tcpServer.nextPendingConnection();
    connect(tcpServerConnection,SIGNAL(readyRead()),this,SLOT(updateServerProgress()));
    connect(tcpServerConnection,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayError(QAbstractSocket::SocketError)));
    ui->serverStatusLabel->setText(tr("接受连接"));
    //tcpServer.close();
}

void ServerBase::updateServerProgress()  //更新进度条，接收数据
{
    QDataStream in(tcpServerConnection);
    in.setVersion(QDataStream::Qt_4_6);

    if(bytesReceived <= sizeof(qint64)*2)
    { //如果接收到的数据小于16个字节，那么是刚开始接收数据，我们保存到//来的头文件信息

        if((tcpServerConnection->bytesAvailable() >= sizeof(qint64)*2)&& (fileNameSize == 0))
        { //接收数据总大小信息和文件名大小信息
            in >> totalBytes >> fileNameSize;
            bytesReceived += sizeof(qint64) * 2;
        }

        if((tcpServerConnection->bytesAvailable() >= fileNameSize)
                && (fileNameSize != 0))
        {  //接收文件名，并建立文件
            in >> fileName;
            ui->serverStatusLabel->setText(tr("接收文件 %1 …").arg(fileName));
            bytesReceived += fileNameSize;
            localFile = new QFile(fileName);
            if(!localFile->open(QFile::WriteOnly))
            {
                qDebug() << "open file error!";
                return;
            }
        }

        else return;
    }


    if(bytesReceived < totalBytes)
    {  //如果接收的数据小于总数据，那么写入文件
        bytesReceived += tcpServerConnection->bytesAvailable();
        inBlock = tcpServerConnection->readAll();
        localFile->write(inBlock);
        inBlock.resize(0);
    }

    ui->serverProgressBar->setMaximum(totalBytes);
    ui->serverProgressBar->setValue(bytesReceived);
    //更新进度条
    if(bytesReceived == totalBytes)
    { //接收数据完成时
        //tcpServerConnection->reset();
        localFile->close();
        //ui->startButton->setEnabled(true);
        delete localFile;
        totalBytes = 0;
        bytesReceived = 0;
        fileNameSize = 0;
        ui->serverStatusLabel->setText(tr("接收文件 %1 成功！").arg(fileName));
    }
}

void ServerBase::displayError(QAbstractSocket::SocketError) //错误处理
{
    qDebug() << tcpServerConnection->errorString();
    tcpServerConnection->close();
    ui->serverProgressBar->reset();
    ui->serverStatusLabel->setText(tr("服务端就绪"));
    ui->startButton->setEnabled(true);
}

void ServerBase::on_startButton_clicked()
{
    totalBytes = 0;
    bytesReceived = 0;
    fileNameSize = 0;
    start();
}
