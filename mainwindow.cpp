#include "mainwindow.h"
#include "ui_mainwindow.h"

/**
 * @brief MainWindow class constructor
 * @param parent
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("TCP Device Emulator (Server)");

    QWidget* mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    MyServerView *serverView = new MyServerView;
    setCentralWidget(mainWidget);
    mainLayout->addWidget(serverView);
    mainWidget->setLayout(mainLayout);

    connect(ui->ActionQuit, SIGNAL(triggered()),
            this, SLOT(onActionQuitTriggered()));

    connect(ui->ActionReqRespEdit, SIGNAL(triggered()),
            this, SLOT(onActionReqRespEditTriggered()));

    connect(ui->ActionAbout, SIGNAL(triggered()),
            this, SLOT(onActionAboutTriggered()));

    connect(this, &MainWindow::setContainer,
            serverView,&MyServerView::onSetContainer);

    connect(serverView, &MyServerView::setTitle, this, &MainWindow::onSetTitle);

    QFile file("comands.db");
    if (file.exists()) {
        if (file.open(QIODevice::ReadOnly)) {
            QDataStream ds(&file);
            ds >> ReqRespData;
            file.close();
        }
    }

    serverView->onSetContainer(ReqRespData);
}

/**
 * @brief MainWindow class destructor
 */
MainWindow::~MainWindow()
{
    if (editorWindow != nullptr) {
        delete editorWindow;
        editorWindow = nullptr;
    }
    delete ui;
}

/**
 * @brief MainWindow::onSetTitle
 * @param title
 */
void MainWindow::onSetTitle(const QString &title)
{
    this->setWindowTitle(title);
}

/**
 * @brief Menu "Quit" action handler
 */
void MainWindow::onActionQuitTriggered()
{
    close();
}

/**
 * @brief onActionAboutTriggered
 */
void MainWindow::onActionAboutTriggered()
{
    QDialog *aboutWindow = new QDialog(this);
    aboutWindow->setWindowTitle("About");
    aboutWindow->resize(350, 230);
    aboutWindow->setModal(true);

    aboutWindow->setWindowFlags((aboutWindow->windowFlags())
                                & (~Qt::WindowContextHelpButtonHint));

    aboutWindow->setAttribute(Qt::WA_DeleteOnClose);

    QLabel* textLabel = new QLabel;
    QVBoxLayout *aboutLayot = new QVBoxLayout(aboutWindow);
    textLabel->setText(tr("<h2>TcpDeviceEmulator</h2>"
                          "<h4>Version 1.0.5</h4>"
                          "<p>Simple TCP Server (device emulator)</p>"));

    aboutLayot->addWidget(textLabel, 0, Qt::AlignCenter);
    aboutWindow->show();
}

/**
 * @brief Menu "Request/Response Editor" action handler
 */
void MainWindow::onActionReqRespEditTriggered()
{
    editorWindow = new QDialog(this);
    editorWindow->setWindowTitle("Request/Response Editor");
    editorWindow->resize(440, 300);
    editorWindow->setModal(true);

    editorWindow->setWindowFlags((editorWindow->windowFlags())
                                  & (~Qt::WindowContextHelpButtonHint));

    editorWindow->setAttribute(Qt::WA_DeleteOnClose);

    ReqRespTable = new QTableWidget(0, 2);
    QStringList ls;
    ls << "Request String" << "Respons String";
    ReqRespTable->setHorizontalHeaderLabels(ls);
    ReqRespTable->setColumnWidth(0, 200);
    ReqRespTable->setColumnWidth(1, 200);

    this->fillTable();

    QVBoxLayout *vLayot = new QVBoxLayout(editorWindow);
    QHBoxLayout *hLayout = new QHBoxLayout;
    QPushButton *addButton = new QPushButton("Add Row");
    addButton->setToolTip("Add a new row at the top of the table");
    QPushButton *remButton = new QPushButton("Remove Row");
    remButton->setToolTip("Remove selected row");
    QPushButton *cancelButton = new QPushButton("Cancel removing");
    QPushButton *saveButton = new QPushButton("Save");
    hLayout->addWidget(addButton);
    hLayout->addWidget(remButton);
    hLayout->addWidget(cancelButton);
    hLayout->addWidget(saveButton);
    vLayot->addWidget(ReqRespTable);
    vLayot->addLayout(hLayout);

    connect(addButton, SIGNAL(clicked()), this, SLOT(onAddButtonClicked()));
    connect(remButton, SIGNAL(clicked()), this, SLOT(onRemoveButtonClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(onCancelButtonClicked()));
    connect(saveButton, SIGNAL(clicked()), this, SLOT(onSaveButtonClicked()));

    editorWindow->show();
}

/**
 * @brief "Add row" button handler
 */
void MainWindow::onAddButtonClicked()
{
    QTableWidgetItem* item = new QTableWidgetItem;
    QTableWidgetItem* item1 = new QTableWidgetItem;
    ReqRespTable->insertRow(0);
    ReqRespTable->setItem(0, 0, item);
    ReqRespTable->setItem(0, 1, item1);
    item->setTextAlignment(Qt::AlignCenter);
    item1->setTextAlignment(Qt::AlignCenter);
}

/**
 * @brief "Remove row" button handler
 */
void MainWindow::onRemoveButtonClicked()
{
    QList<QTableWidgetSelectionRange> ranges = ReqRespTable->selectedRanges();
    int selectRows = ranges.count();

    for (int i = 0; i < selectRows; i++)
    {
        int topRow = ranges.at(i).topRow(); // get row
        ReqRespTable->removeRow(topRow);    // remove selected row
    }
}

/**
 * @brief "Cancel removing" button handler
 */
void MainWindow::onCancelButtonClicked()
{
    for (int i = 0; i < ReqRespTable->rowCount(); ++i) {
        ReqRespTable->removeRow(i);
    }
    ReqRespTable->setRowCount(0);

    this->fillTable();
}

/**
 * @brief "Save" button handler
 */
void MainWindow::onSaveButtonClicked()
{
    ReqRespData.clear();

    for (int i = 0; i < ReqRespTable->rowCount(); ++i) {
        ReqRespData.insert(ReqRespTable->item(i, 0)->text(),
                           ReqRespTable->item(i, 1)->text());
    }
    emit setContainer(ReqRespData);
    editorWindow->close();
    delete editorWindow;
    editorWindow = nullptr;

    QFile file("comands.db");
    if (file.open(QIODevice::WriteOnly)) {
        QDataStream ds(&file);
        ds << ReqRespData;
        file.close();
    }
}

/**
 * @brief Fills the table with data from the container
 */
void MainWindow::fillTable()
{
    QList<QString> keys = ReqRespData.keys();

    foreach (QString key, keys) {
        ReqRespTable->setRowCount(ReqRespTable->rowCount()+1);
        QTableWidgetItem* item = new QTableWidgetItem;
        item->setText(key);
        item->setTextAlignment(Qt::AlignCenter);
        ReqRespTable->setItem(ReqRespTable->rowCount()-1, 0, item);
        QTableWidgetItem* item1 = new QTableWidgetItem;
        item1->setText(ReqRespData.value(key));
        item1->setTextAlignment(Qt::AlignCenter);
        ReqRespTable->setItem(ReqRespTable->rowCount()-1, 1, item1);
    }
}
