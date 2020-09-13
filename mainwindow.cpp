/*
Copyright (c) 2020 nastys

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QDir>
#include <QList>
#include <QTextEncoder>
#include <QMessageBox>

QFile currentFile;
QList<int> indexList;
QList<int> strLenList;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QDir tdir("text");
    QStringList dirs = tdir.entryList(QDir::Dirs);
    for(int i=2; i<dirs.size(); i++)
    {
        ui->comboBox_Language->addItem(dirs.at(i));
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_comboBox_Language_currentTextChanged(const QString &arg1)
{
    QDir tdir("text/"+arg1);
    QStringList dirs = tdir.entryList(QDir::Dirs);
    for(int i=2; i<dirs.size(); i++)
    {
        ui->listWidget_Script->addItem(dirs.at(i));
    }
}

void MainWindow::on_listWidget_Script_currentTextChanged(const QString &currentText)
{
    indexList.clear();
    strLenList.clear();
    ui->listWidget_Line->clear();
    currentFile.close();
    currentFile.setFileName("text/"+ui->comboBox_Language->currentText()+"/"+currentText+"/default.bin");
    currentFile.open(QIODevice::ReadWrite);
    qint32 textOffset;
    currentFile.seek(0x38);
    currentFile.read((char*)&textOffset, sizeof(textOffset));
    qint32 lsblOffset;
    currentFile.seek(textOffset+0x20);
    currentFile.read((char*)&lsblOffset, sizeof(lsblOffset));
    lsblOffset+=textOffset;
    qint32 lsblSize;
    currentFile.seek(lsblOffset+0x10);
    currentFile.read((char*)&lsblSize, sizeof(lsblSize));
    short next=0;
    while(next!=-1&&!currentFile.atEnd())
    {
        currentFile.read((char*)&next, sizeof(next));
    }
    int unk;
    currentFile.read((char*)&unk, sizeof(unk));
    while(currentFile.pos()<lsblOffset+lsblSize&&!currentFile.atEnd())
    {
        int startPos=currentFile.pos();
        QByteArray currStr;
        char currChar[2];
        do
        {
            currentFile.read(currChar, 2);
            currStr.append(currChar[0]);
            currStr.append(currChar[1]);
        }
        while(!(currChar[0]=='\0'&&currChar[1]=='\0')&&!currentFile.atEnd());

        QString str = QString::fromUtf16(reinterpret_cast<const ushort*>(currStr.constData()));

        strLenList.append(currStr.size()-4);
        do
        {
            currentFile.read(currChar, 2);
            strLenList[strLenList.size()-1]+=2;
        }
        while(currChar[0]=='\0'&&currChar[1]=='\0'&&!currentFile.atEnd());
        currentFile.seek(currentFile.pos()-2);

        ui->listWidget_Line->addItem(str);
        indexList.append(startPos);
    }
    if(!strLenList.isEmpty()) strLenList[strLenList.size()-1]-=2;
}

void MainWindow::on_listWidget_Line_currentTextChanged(const QString &currentText)
{
    ui->plainTextEdit_Text->clear();
    ui->plainTextEdit_Text->appendPlainText(currentText);
}

void MainWindow::on_plainTextEdit_Text_textChanged()
{
    if(strLenList.isEmpty()) ui->label_Size->clear();
    else
    {
        QString qstr = ui->plainTextEdit_Text->document()->toPlainText();
        QTextCodec *codec = QTextCodec::codecForName("UTF-16");
        QTextEncoder *encoder = codec->makeEncoder(QTextCodec::IgnoreHeader);
        QByteArray bytes = encoder->fromUnicode(qstr);

        ui->label_Size->setText(QString::number(bytes.size())+" / "+QString::number(strLenList.at(ui->listWidget_Line->currentRow())));
    }
}

void MainWindow::on_pushButton_Save_clicked()
{
    const int row = ui->listWidget_Line->currentRow();
    const int pos = indexList.at(row);
    QString qstr = ui->plainTextEdit_Text->document()->toPlainText();
    QTextCodec *codec = QTextCodec::codecForName("UTF-16");
    QTextEncoder *encoder = codec->makeEncoder(QTextCodec::IgnoreHeader);
    QByteArray bytes = encoder->fromUnicode(qstr);
    if(bytes.size()>strLenList.at(row))
    {
        QMessageBox::critical(this, "String Editor", "The string is too long!");
        return;
    }
    currentFile.seek(pos);
    currentFile.write(bytes);
    char zero = '\0';
    int end = pos+(strLenList.at(row));
    while(currentFile.pos()<end)
    {
        currentFile.write(&zero, 1);
    }
    currentFile.flush();
    on_listWidget_Script_currentTextChanged(ui->listWidget_Script->currentItem()->text());
}
