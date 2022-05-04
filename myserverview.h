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

    void runServer(quint16 port);
    void sendToClient(QTcpSocket* pSocket, const QByteArray& arrData);
};

#endif // MYSERVERVIEW_H
