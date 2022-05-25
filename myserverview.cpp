#include "myserverview.h"
#include "ui_myserverview.h"

//#include <QDebug>

/**
 * @brief Class constructor
 * @param port TCP Port
 * @param parent
 */
MyServerView::MyServerView(QWidget *parent) :
    QWidget(parent), ui(new Ui::MyServerView)
{
    ui->setupUi(this);

    nClients = 0;
    tcpPort = 5335; // Port by default

    connect(ui->ClearWindowButton, SIGNAL(clicked()),
            this, SLOT(onClearWindowButtonClicked()));

    connect(ui->PortPushButton, SIGNAL(clicked()),
            this, SLOT(onPortPushButtonClicked()));

    runServer(tcpPort);
}

/**
 * @brief Class destructor
 */
MyServerView::~MyServerView()
{
    if (tcpServer != nullptr) {
        tcpServer->close();
        delete tcpServer;
        tcpServer = nullptr;
    }
    delete ui;
}

/**
 * @brief
 */
void MyServerView::runServer(quint16 port)
{
    tcpServer = new QTcpServer(this);

    if (!tcpServer->listen(QHostAddress::Any, port)) {
        QMessageBox::critical(0, "Server Error",
                              "Unable to start the server:"
                              + tcpServer->errorString());
        tcpServer->close();
        return;
    }

    connect(tcpServer, SIGNAL(newConnection()),
            this, SLOT(onNewConnection()));

    ui->textEdit->setText("The server is listening on port "
                         + QString::number(port) + "...");

    ui->PortNumLabel->setText("Port: " + QString::number(port));

    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    for (QHostAddress &address: QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost)
            ui->ipLabel->setText("IP: " + address.toString() + " ");
    }
}

/**
 * @brief MyServerView::onNewConnection
 */
void MyServerView::onNewConnection()
{
    QTcpSocket* pClientSocket = tcpServer->nextPendingConnection();

//    connect(pClientSocket, SIGNAL(disconnected()), pClientSocket, SLOT(deleteLater()));
    connect(pClientSocket, SIGNAL(disconnected()), this, SLOT(onClientDisconnect()));
    connect(pClientSocket, SIGNAL(readyRead()), this, SLOT(onReadClient()));

    nClients++;
    ui->ClientsNumLabel->setText(QString::number(nClients));

    ui->textEdit->append("New client connected to port "
                         + QString::number(pClientSocket->localPort()));
}

/**
 * @brief MyServerView::onReadClient
 */
void MyServerView::onReadClient()
{
    QString rcv_str, snd_str;

    QTcpSocket* pClientSocket = (QTcpSocket*)sender();

    for (;;) {
        rcv_str += pClientSocket->readAll();
        if (pClientSocket->bytesAvailable() < 1) {
            break;
        }
    }

    while ((rcv_str.right(1) == '\r') || (rcv_str.right(1) == '\n'))
    {
        rcv_str.chop(1); // Delete EOL
    }

    ui->textEdit->append(rcv_str);

    if (!cmdContainer.contains(rcv_str)) {
        snd_str = rcv_str; // echo
    }
    else {
        snd_str = cmdContainer.value(rcv_str); // response
        if (snd_str == "") return;             // no response
    }

    switch(ui->eolComboBox->currentIndex()) // Add EOL to Tx data
    {
    case 0:
        snd_str.append('\n');
        break;
    case 1:
        snd_str.append('\r');
        break;
    case 2:
        snd_str.append('\r');
        snd_str.append('\n');
        break;
    case 3:
        snd_str.append('\n');
        snd_str.append('\r');
        break;
    default:
        break;
    }

    QByteArray data(snd_str.toStdString().c_str());
    sendToClient(pClientSocket, data);
    ui->textEdit->append("Server resp: " + snd_str);
}

/**
 * @brief MyServerView::sendToClient
 * @param pSocket
 * @param data
 */
void MyServerView::sendToClient(QTcpSocket* pSocket, const QByteArray& arrData)
{
    pSocket->write(arrData);
}

/**
 * @brief MyServerView::onClientDisconnect
 */
void MyServerView::onClientDisconnect()
{
    QTcpSocket* pClientSocket = (QTcpSocket*)sender();

    ui->textEdit->append("Client disconnected from port "
                         + QString::number(pClientSocket->localPort()));

    pClientSocket->close();
    pClientSocket->deleteLater();

    nClients--;
    ui->ClientsNumLabel->setText(QString::number(nClients));
}

/**
 * @brief MyServerView::onPortPushButtonClicked
 */
void MyServerView::onPortPushButtonClicked()
{
    portWindow = new QDialog(this);
    portWindow->setWindowTitle("TCP Port Number");
    portWindow->resize(200, 80);
    portWindow->setModal(true);
    portWindow->setWindowFlags(Qt::Drawer);
    portWindow->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *vLayot = new QVBoxLayout(portWindow);
    QHBoxLayout *hLayot = new QHBoxLayout;
    portEdit = new QLineEdit;
    QIntValidator *peValidator = new QIntValidator;
    peValidator->setRange(1024, 65535);
    portEdit->setValidator(peValidator);
    portEdit->setAlignment(Qt::AlignHCenter);
    QPushButton *portButton = new QPushButton("Set");
    hLayot->addWidget(portEdit);
    hLayot->addStretch();
    hLayot->addWidget(portButton);
    vLayot->addLayout(hLayot);

    connect(portButton, SIGNAL(clicked()),
            this, SLOT(onSetPortClicked()));

    portWindow->show();
}

/**
 * @brief MyServerView::onSetPortPClicked
 */
void MyServerView::onSetPortClicked()
{
    quint16 portNumber = 0;
    bool correctInput = false;

    correctInput = portEdit->hasAcceptableInput();

    if (correctInput)
        portNumber = portEdit->text().toUShort();

    if (correctInput && (portNumber > 1023)) {
        if (tcpServer != nullptr) {
            tcpServer->close();
            tcpServer->deleteLater();
            tcpServer = nullptr;
        }
        runServer(portNumber);
    }
    else {
        QMessageBox::information(this, tr("Incorrect input"),
                                 tr("The port number is incorrect!"));
        portEdit->clear();
    }

    portWindow->close();

    if (portWindow != nullptr) {
        delete portWindow;
        portWindow = nullptr;
    }
}


/**
 * @brief MyServerView::onClearWindowButtonClicked
 */
void MyServerView::onClearWindowButtonClicked()
{
    ui->textEdit->clear();
}

/**
 * @brief MyServerView::onSetContainer
 * @param container
 */
void MyServerView::onSetContainer(const QHash<QString, QString> &container)
{
    cmdContainer = container;
}
