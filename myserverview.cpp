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
    QString receiveData;

    QTcpSocket* pClientSocket = (QTcpSocket*)sender();

    for (;;) {
        receiveData += pClientSocket->readAll();
        if (pClientSocket->bytesAvailable() < 1) {
            break;
        }
    }

    while ((receiveData.right(1) == '\r') || (receiveData.right(1) == '\n'))
    {
        receiveData.chop(1); // Delete EOL
    }

    ui->textEdit->append("Received: " + receiveData);

    strTransmitData = "";

    if (ui->echoCheckBox->isChecked()) {
        strTransmitData = receiveData; // echo
    }
    else {
        switch (DeviceType)
        {
        case Unknown:
            runDefaultParser(receiveData);
            break;
        case Multimeter:
            runMultimeterParser(receiveData);
            break;
        case Chamber:
            runChamberParser(receiveData);
            break;
        case Calibrator:
            runCalibratorParser(receiveData);
            break;
        }
    }

    if (strTransmitData == "") return;         // no response

    switch(ui->eolComboBox->currentIndex()) // Add EOL to Tx data
    {
    case 0:
        strTransmitData.append('\n');
        break;
    case 1:
        strTransmitData.append('\r');
        break;
    case 2:
        strTransmitData.append('\r');
        strTransmitData.append('\n');
        break;
    case 3:
        strTransmitData.append('\n');
        strTransmitData.append('\r');
        break;
    default:
        break;
    }

    QByteArray data(strTransmitData.toStdString().c_str());
    sendToClient(pClientSocket, data);
    ui->textEdit->append("Sended: " + strTransmitData);
}

void MyServerView::runDefaultParser(const QString &rxdata)
{
    if (cmdContainer.contains(strReceiveData)) {
        strTransmitData = cmdContainer.value(rxdata); // response
    }
}

void MyServerView::runMultimeterParser(const QString &rxdata)
{
    if (rxdata == "*IDN?") {
        clbRun = false;
        strTransmitData = "Multimeter Emulator";
    }
    else if (rxdata == "MEAS:VOLT:DC?") {
        strTransmitData = "+5.57703936E+00";
    }
    else if (rxdata == "MEAS:VOLT:DC? 0.1") {
        strTransmitData = "+5.12343936E-03";
    }
    else if (rxdata == "MEAS:VOLT:DC? 10") {
        if (!clbRun) {
            strTransmitData = "+5.12343936E+00";
        }
        else {
            voltOut = 0.4 + 0.0015 * ((double)Dac1 - 0.5 * (double)Dac2 ) + 4.5 *
                    ((double)Dac2 / 4095.0) * ((pressValue - 0.12)/(2.55 - 0.12));
            if (voltOut < 0.01) voltOut = 0.01;
            if (voltOut > 4.99) voltOut = 4.99;
            strTransmitData = QString::number(voltOut, 'E', 8);
        }
    }
    else if (rxdata.left(5) == "#PRSP") {
        clbRun = true;
        strTransmitData = rxdata;
        strTransmitData.remove(0, 5);
        pressValue = strTransmitData.toDouble();
    }
    else if (rxdata.left(5) == "#DAC1") {
        strTransmitData = rxdata;
        strTransmitData.remove(0, 5);
        Dac1 = strTransmitData.toInt();
    }
    else if (rxdata.left(5) == "#DAC2") {
        strTransmitData = rxdata;
        strTransmitData.remove(0, 5);
        Dac2 = strTransmitData.toInt();
    }
}

void MyServerView::runChamberParser(const QString &rxdata)
{
    if (rxdata == "$00E -040.0 0101010001000000") {
        strChamberResponse = "-040.0 -040.0 0101010001000000";
        strTransmitData = "0";
    }
    else if (rxdata == "$00E -005.0 0101010001000000") {
        strChamberResponse = "-005.0 -005.0 0101010001000000";
        strTransmitData = "0";
    }
    else if (rxdata == "$00E 0030.0 0101010001000000") {
        strChamberResponse = "0030.0 0030.0 0101010001000000";
        strTransmitData = "0";
    }
    else if (rxdata == "$00E 0065.0 0101010001000000") {
        strChamberResponse = "0065.0 0065.0 0101010001000000";
        strTransmitData = "0";
    }
    else if (rxdata == "$00E 0100.0 0101010001000000") {
        strChamberResponse = "0100.0 0100.0 0101010001000000";
        strTransmitData = "0";
    }
    else if (rxdata == "$00I") {
        strTransmitData = strChamberResponse;
    }
}

void MyServerView::runCalibratorParser(const QString &rxdata)
{
    if (rxdata == "SOURCE:PRES:LEV:IMM:AMPL 0.12") {
        strPressSetpoint = "+1.2000003E-001";
        strTransmitData = "";
    }
    else if (rxdata == "SOURCE:PRES:LEV:IMM:AMPL 2.55") {
        strPressSetpoint = "+2.5499999E+000";
        strTransmitData = "";
    }
    else if (rxdata == "SOURCE:PRES:LEV:IMM:AMPL?") {
        strTransmitData = strPressSetpoint;
    }
    else if (rxdata == "MEAS:PRES?") {
        strTransmitData = strPressSetpoint;
    }
    else if (rxdata == "OUTP:MODE CONT") {
        pressMode = 1;
        strTransmitData = "";
    }
    else if (rxdata == "OUTP:STAT?") {
        strTransmitData = QString::number(pressMode);
        pressMode = 0;
    }
    else if (rxdata == "OUTP:STAB?") {
        strTransmitData = QString::number(1);
    }
    else if (rxdata == "UNIT:INDEX?") {
        strTransmitData = QString::number(5);
    }
    else if (rxdata == "SENS:MODE GAUGE") {
        pressType = 0;
        strTransmitData = "";
    }
    else if (rxdata == "SENS:MODE ABS") {
        pressType = 1;
        strTransmitData = "";
    }
    else if (rxdata == "SENS:ABS?") {
        strTransmitData = QString::number(pressType);
    }
    else if (rxdata == "CALC:LIM:UPP 3.825") {
        strPressLimit = "+3.8250004E+000";
        strTransmitData = "";
    }
    else if (rxdata == "CALC:LIM:UPP 10.0") {
        strPressLimit = "+9.9999998E+000";
        strTransmitData = "";
    }
    else if (rxdata == "CALC:LIM:UPP?") {
        strTransmitData = strPressLimit;
    }
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

    portWindow->setWindowFlags((portWindow->windowFlags())
                                  & (~Qt::WindowContextHelpButtonHint));

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
    QString DeviceName;

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

    switch (portNumber)
    {
    case 3001:
        DeviceType = Multimeter;
        DeviceName = "Device: Multimeter";
        ui->eolComboBox->setCurrentIndex(0);
        break;
    case 4001:
        DeviceType = Chamber;
        DeviceName = "Device: Chamber";
        ui->eolComboBox->setCurrentIndex(1);
        break;
    case 4040:
        DeviceType = Calibrator;
        DeviceName = "Device: Calibrator";
        ui->eolComboBox->setCurrentIndex(0);
        break;
    default:
        DeviceType = Unknown;
        DeviceName = "Device: Unknown";
        break;
    }
    emit setTitle(DeviceName);
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

