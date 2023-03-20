#ifndef MYSERVERVIEW_H
#define MYSERVERVIEW_H

#include <QtNetwork>
#include <QtWidgets>

class QTcpSocket;
class QTcpServer;

namespace Ui {
    class MyServerView;
}

class MyServerView : public QWidget
{
    Q_OBJECT

public:
    MyServerView(QWidget *parent = nullptr);
    ~MyServerView();

signals:
    void setTitle(const QString &title);

public slots:
    virtual void onNewConnection();
            void onReadClient();
            void onSetContainer(const QHash<QString, QString> &container);

private slots:
    void onClientDisconnect();
    void onSetPortClicked();
    void onPortPushButtonClicked();
    void onClearWindowButtonClicked();

private:
    Ui::MyServerView *ui;

    QTcpServer* tcpServer = nullptr;
    QDialog *portWindow = nullptr;
    QLineEdit *portEdit = nullptr;
    QHash<QString, QString> cmdContainer;
    quint32 nClients;
    quint16 tcpPort;
    QSettings settings;
    quint32 sendedCnt;
    int pressMode;
    int pressType;
    int Dac1 = 1634;
    int Dac2 = 3849;
    double voltOut = 0.1;
    double pressValue = 0.12;
    bool clbRun = false;
    QString strReceiveData, strTransmitData;
    QString strPressLimit ="+0.0000000E+000";
    QString strPressSetpoint = "+0.0000000E+000";
    QString strPressValue = "+0.0000000E+000";
    QString strChamberResponse = "0000.0 0101010001000000";

    enum {
        Unknown =0,
        Multimeter,
        Chamber,
        Calibrator
    } DeviceType;

    void runServer(quint16 port);
    void sendToClient(QTcpSocket* pSocket, const QByteArray& arrData);
    void runMultimeterParser(const QString &rxdata);
    void runChamberParser(const QString &rxdata);
    void runCalibratorParser(const QString &rxdata);
    void runDefaultParser(const QString &rxdata);
};

#endif // MYSERVERVIEW_H
