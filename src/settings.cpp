/**
  This file is a part of mcercle
  Copyright (C) 2010-2013 Cyril FRAUSTI

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.

*/

#include "settings.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QDesktopServices>
#else
#include <QStandardPaths>
#endif

#include <QDate>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QMessageBox>

Settings::Settings(QObject *parent) :
    QObject(parent)
{
}

QString Settings::getSettingsFile(){
    qDebug()<<"Settings file location"<<QFileInfo(QStandardPaths::writableLocation ( QStandardPaths::ConfigLocation),"settings").absoluteFilePath();
    return QFileInfo(QStandardPaths::writableLocation ( QStandardPaths::ConfigLocation),"settings").absoluteFilePath();
}

/**
    Recuperation des parametres de connexion pour la bdd
  */
QString Settings::getDatabase_bdd(){
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("connection");
    QString val = settings.value("bdd").toString();
    settings.endGroup();
    return val;
}

QString Settings::getDatabase_hostName(){
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("connection");
    QString val = settings.value("hostName").toString();
    settings.endGroup();
    return val;
}

int Settings::getDatabase_port(){
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("connection");
    int val = settings.value("port").toInt();
    settings.endGroup();
    return val;
}

QString Settings::getDatabase_databaseName(){
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("connection");
    QString val = settings.value("databaseName").toString();
    settings.endGroup();
    return val;
}

QString Settings::getDatabase_userName(){
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("connection");
    QString val = settings.value("userName").toString();
    settings.endGroup();
    return val;
}

QString Settings::getDatabase_userPassword(){
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("connection");
    QString val = settings.value("userPassword").toString();
    settings.endGroup();
    return val;
}

/**
 * @brief Settings::getPrintFont
 * @return
 */
QFont Settings::getPrintFont(){
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("print");
    QFont val( settings.value("font").toString() );
    settings.endGroup();
    return val;
}


/**
    Sauvegarde dans le fichier les donnees relatif a la base de donnees
  */
void Settings::setDatabase_bdd(const QString& bdd) {
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("connection");
    settings.setValue("bdd", bdd);
    settings.endGroup();
}

void Settings::setDatabase_hostName(const QString& hostName) {
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("connection");
    settings.setValue("hostName", hostName);
    settings.endGroup();
}

void Settings::setDatabase_port(const int& port) {
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("connection");
    settings.setValue("port",QString::number(port));
    settings.endGroup();
}

void Settings::setDatabase_databaseName(const QString& databaseName) {
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("connection");
    settings.setValue("databaseName", databaseName);
    settings.endGroup();
}

void Settings::setDatabase_userName(const QString& userName){
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("connection");
    settings.setValue("userName", userName);
    settings.endGroup();
}

void Settings::setDatabase_userPassword(const QString& userPassword) {
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("connection");
    settings.setValue("userPassword", userPassword);
    settings.endGroup();
}

/**
    Applique les valeurs par defaut de la bdd
  */
void Settings::setDatabase_default() {

    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("connection");
    settings.setValue("bdd", "SQLITE");
    settings.setValue("hostName", "localhost");

    qDebug()<<"Bdd location"<<QFileInfo(QStandardPaths::writableLocation ( QStandardPaths::DataLocation),"mcercle.db").absoluteFilePath();

    settings.setValue("databaseName", QFileInfo(QStandardPaths::writableLocation ( QStandardPaths::DataLocation),"mcercle.db").absoluteFilePath() );
    settings.endGroup();

    qDebug()<<"Set to settings "<<settings.fileName();
}


/**
 * @brief Settings::settingIsOk
 * @return
 */
bool Settings::settingIsOk() {
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("main");
    bool val = false;
    val = settings.value("settingState").toBool();
    settings.endGroup();
    return val;
}

void Settings::setSettingState(bool state) {
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("main");
    settings.setValue("settingState", state);
    settings.endGroup();
}

/**
    Position des listebox pour les recherches
  */
void Settings::setPositionListSearchProduct(int pos) {
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("list");
    settings.setValue("searchProduct", pos);
    settings.endGroup();
}

/**
 * @brief Settings::getPositionListSearchProduct
 * @return
 */
int Settings::getPositionListSearchProduct(){
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("list");
    int val = settings.value("searchProduct").toInt();
    settings.endGroup();
    return val;
}

/**
 * @brief setPrintFont
 * @param printFont
 */
void Settings::setPrintFont(const QString& printFont){
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("print");
    settings.setValue("font", printFont);
    settings.endGroup();
}

/**
 * @brief setDatebddSave
 * @param date
 */
void Settings::setDatebddSave(const QDate &date) {
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("date");
    settings.setValue("bddSave", date.toString(tr("dd-MM-yyyy")) );
    settings.endGroup();
}

/**
 * @brief Settings::getDatebddSave
 * @return
 */
QDate Settings::getDatebddSave(){
    QSettings settings(getSettingsFile(),QSettings::IniFormat);
    settings.beginGroup("date");
    QDate date;
    date = QDate::fromString( settings.value("bddSave").toString(), tr("dd-MM-yyyy") );
    settings.endGroup();
    return date;
}


