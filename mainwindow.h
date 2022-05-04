#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "myserverview.h"
#include <QMainWindow>
#include <QTableWidget>
//#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void setContainer(const QHash<QString, QString> &container);

private slots:
    void onActionQuitTriggered();
    void onActionReqRespEditTriggered();
    void onActionAboutTriggered();
    void onAddButtonClicked();
    void onRemoveButtonClicked();
    void onSaveButtonClicked();
    void onCancelButtonClicked();

private:
    Ui::MainWindow *ui;

    QDialog *editorWindow = nullptr;
    QTableWidget *ReqRespTable = nullptr;
    QHash<QString, QString> ReqRespData;

    void fillTable();
};
#endif // MAINWINDOW_H
