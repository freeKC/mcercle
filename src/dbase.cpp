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

#include "dbase.h"
#include "mcercle.h"
#include "dialogwaiting.h"

#include <QMessageBox>
#include <QWidget>
#include <QByteArray>
#include <QBuffer>
#include <QDesktopServices>
#include <QFile>
#include <QVariant>
#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QApplication>
#include <QDebug>

/**
 * @brief database::database
 * @param lang
 * @param parent
 */
database::database(QLocale &lang, QWidget *parent): m_parent(parent) {
    m_name = QFileInfo(QStandardPaths::writableLocation ( QStandardPaths::DataLocation),"mcercle.db").absoluteFilePath();
    m_port = 3306;
    m_hostName = "localhost";
    m_login = "root";
    m_password = "";
    m_connected = false;
    m_lang = lang;
    /* Valeur entreprise */
    m_isTax = 0;
}

database::~database(){
    if(m_customer) m_customer->deleteLater();
    if(m_product) m_product->deleteLater();
}


/**
    connexion a la base de donnees
  */
char database::connect(){
    //test de la version 1.0
    if( m_name.contains(".fdb")){
        QMessageBox mBox(QMessageBox::Warning, tr("Attention"), tr("<b>mcercle ne supporte plus firebird!</b><br>Veuillez installer mcercle v1.9 pour la migration."),QMessageBox::Ok);
        mBox.exec();
        return DB_CON_ERR;
    }

    db = QSqlDatabase::addDatabase("Q"+m_bdd);
    db.setHostName( m_hostName );
    db.setPort( m_port );
    db.setDatabaseName( m_name);
    db.setUserName( m_login );
    db.setPassword( m_password );

    if (!db.open()) {
        QMessageBox mBox(QMessageBox::Critical, tr("Erreur"), tr("<b>La connexion avec la base de données n'a pas pu être établie!</b>"),QMessageBox::Ok);
        mBox.setDetailedText ( db.lastError().text() );
        mBox.exec();
        m_connected = false;
        return DB_CON_ERR;
    }
    else m_connected = true;

    // Si pas de tables on les crees
    QStringList tList = db.tables();
    if((!tList.contains("TAB_INFORMATIONS"))||(tList.count()< 14)){
        // Demande si on creer une nouvelle base de donnees
        QString mess;
        if(db.driverName() == "QSQLITE")mess = QLatin1String("Voulez-vous créer une nouvelle base de données ?\n\n")+ db.databaseName();
        else mess = QLatin1String("Voulez-vous créer de nouvelles tables dans la base de données ?\n\n") + db.databaseName();
        QMessageBox mBox(QMessageBox::Question, tr("Question"), mess ,QMessageBox::Yes | QMessageBox::No);
        mBox.setDefaultButton(QMessageBox::No);
        int ret = mBox.exec();

        if(ret == QMessageBox::Yes){
            if( !create() ){
                m_connected = false;
                return DB_CON_ERR;
            }
        }else{
            m_connected = false;
            return DB_CON_ERR;
        }
    }
    else m_connected = true;

    //DI SQLITE activation des foreign Keys
    if(db.driverName() == "QSQLITE"){
        QSqlQuery query;
        query.prepare("PRAGMA foreign_keys = ON;");

        if(!query.exec()) {
            QMessageBox::critical(m_parent, tr("Erreur"), query.lastError().text());
            return false;
        }
    }

    // AVOIR: SI DEFAUT LES RETURN PRECEDENT QUI ZAPP LA CREATION DES CLASS NE POSE PAS SOUCIS!?
    m_databaseVersion = databaseVersion();
    m_isTax = isTax();
    //Creation des sous class
    m_customer = new customer(db, m_parent);
    //instanciation de la class product
    m_product = new product(db, m_lang, m_parent);
    //class de la gestion des tax
    m_tax = new tax(m_parent);

    //Test de la version de la base de donnees... !!
    if(m_databaseVersion > MCERCLE::Dbase_support){
        QString mess = tr("Version de mcercle: ") + MCERCLE::Version;
        mess += tr("\nVersion de la base de donn�es: ") + QString::number(m_databaseVersion);
        mess += tr("\n\nVersions des bases de donn�es compatibles: <= ") + QString::number(MCERCLE::Dbase_support);
        QMessageBox mBox(QMessageBox::Warning, tr("Attention"), tr("mcercle ne support pas cette version de base de donn�es...\nMerci de faire �voluer mcercle."),QMessageBox::Ok);
        mBox.setDetailedText ( mess );
        mBox.exec();
        this->close();
        return DB_CON_ERR;
    }
    qDebug() << "version base:" <<QString::number(m_databaseVersion);

    // Mise a jour de la base de donnees
    if(m_databaseVersion < MCERCLE::Dbase_support){
        int ret = QMessageBox::warning(
                    this->m_parent,
                    tr("Attention"),
                    tr("Cette version de mcercle doit mettre � jour la base de donn�e pour fonctionner.\n\nVoulez-vous mettre � jour la base de donn�e"),
                    QMessageBox::Yes, QMessageBox::No | QMessageBox::Default
                    );

        if(ret == QMessageBox::Yes){
            QString logAll, log, mess;
            bool upgradeOk=true;
            if(m_databaseVersion <= 1) {
                if(!upgradeToV2(&log)) upgradeOk = false;
            }
            logAll += log;
            if(m_databaseVersion <= 2 ) {
                if(!upgradeToV3(&log)) upgradeOk = false;
            }
            logAll += log;
            if(m_databaseVersion <= 3 ) {
                if(!upgradeToV4(&log)) upgradeOk = false;
            }
            logAll += log;
            if(m_databaseVersion <= 4 ) {
                if(!upgradeToV5(&log)) upgradeOk = false;
            }
            logAll += log;

            QMessageBox mBox(QMessageBox::Information, tr("Information"), mess, QMessageBox::Ok);
            if(upgradeOk){
                mess += tr("La mise � jour de la base de donn�es a r�ussi !\n");
                mBox.setIcon( QMessageBox::Information );
            }
            else{
                mess += tr("La mise � jour contient des erreurs :-(\nAfficher les d�tails pour voir ce qui ne va pas.");
                mBox.setIcon( QMessageBox::Critical );
            }
            mBox.setText( mess );
            mBox.setDetailedText(logAll);
            mBox.exec();
        }
        else{
            this->close();
            return DB_CON_ERR;
        }
    }

    return DB_CON_OK;
}


/**
    Creation de la base de donnees
  */
bool database::create(){
    int bar=0;
    bool ret = true;
    //Affichage de la fenetre d attente
    DialogWaiting* m_DialogWaiting = new DialogWaiting();
    m_DialogWaiting->setTitle(tr("<b>Cr&#233;ation de la base de donn&#233;es</b>"));
    m_DialogWaiting->setDetail(tr("<i>Cr&#233;ation de la base de donn&#233;es...</i>"));
    m_DialogWaiting->setProgressBarRange(0,13);
    m_DialogWaiting->setModal(true);
    m_DialogWaiting->show();

    //Connexion et creation des tables
    if( m_connected ) {
        m_DialogWaiting->setProgressBar(bar++);
        m_DialogWaiting->setDetail(tr("<i>Cr&#233;ation des tables...</i>"));
        //Creation des tables
        if(!db.tables().contains("TAB_INFORMATIONS")) ret = createTable_informations();
        if((!db.tables().contains("TAB_BANK"))&&(ret)){
            m_DialogWaiting->setProgressBar(bar++);
            ret = createTable_bank();
        }
        if((!db.tables().contains("TAB_CUSTOMERS"))&&(ret)){
            m_DialogWaiting->setProgressBar(bar++);
            ret = createTable_customers();
        }
        if((!db.tables().contains("TAB_TAX"))&&(ret)){
            m_DialogWaiting->setProgressBar(bar++);
            ret = createTable_tax();
        }
        if((!db.tables().contains("TAB_PRODUCTS"))&&(ret)){
            m_DialogWaiting->setProgressBar(bar++);
            ret = createTable_products();
        }
        if((!db.tables().contains("TAB_PRODUCTS_CATEGORIES"))&&(ret)){
            m_DialogWaiting->setProgressBar(bar++);
            ret = createTable_products_categories();
        }
        if((!db.tables().contains("TAB_PROVIDERS"))&&(ret)){
            m_DialogWaiting->setProgressBar(bar++);
            ret = createTable_providers();
        }
        if((!db.tables().contains("TAB_SERVICES"))&&(ret)){
            m_DialogWaiting->setProgressBar(bar++);
            ret = createTable_services();
        }
        if((!db.tables().contains("TAB_SERVICES_COMMON"))&&(ret)){
            m_DialogWaiting->setProgressBar(bar++);
            ret = createTable_services_common();
        }
        if((!db.tables().contains("TAB_PROPOSALS"))&&(ret)){
            m_DialogWaiting->setProgressBar(bar++);
            ret = createTable_proposals();
        }
        if((!db.tables().contains("TAB_PROPOSALS_DETAILS"))&&(ret)){
            m_DialogWaiting->setProgressBar(bar++);
            ret = createTable_proposals_details();
        }
        if((!db.tables().contains("TAB_INVOICES"))&&(ret)){
            m_DialogWaiting->setProgressBar(bar++);
            ret = createTable_invoices();
        }
        if((!db.tables().contains("TAB_INVOICES_DETAILS"))&&(ret)){
            m_DialogWaiting->setProgressBar(bar++);
            ret = createTable_invoices_details();
        }
        if((!db.tables().contains("TAB_LINK_PROPOSALS_INVOICES"))&&(ret)){
            m_DialogWaiting->setProgressBar(bar++);
            ret = createTable_link_proposals_invoices();
        }

        if(ret) {
            //creation OK
            m_DialogWaiting->setDetail(tr("<i>Cr&#233;ation termin&#233;e avec succ&#232;s !</i>"));
            m_connected = true;
            return true;
        }else {
            // on supprime la database
            if(db.driverName() == "QSQLITE") {
                if ( !QFile::remove( m_name ) )    {
                    QMessageBox::critical(this->m_parent, tr("Erreur"), tr("Impossible de supprimer le fichier:\n")+m_name);
                }
            }
            m_DialogWaiting->setDetail(tr("<i>Erreur... veuillez contacter votre administrateur r&#233;seau.</i>"));
            m_connected = false;
            return false;

        }
    }
    else{
        //Erreur de connexion
        m_DialogWaiting->setDetail(tr("<i>Erreur... veuillez contacter votre administrateur r&#233;seau.</i>"));
        return false;
    }
    return false;
}


/**
    Creation de la table informations
  */
bool database::createTable_informations(){
    QString req ="CREATE TABLE TAB_INFORMATIONS("
            "ID             INTEGER NOT NULL ,"
            "DBASE_VERSION  INTEGER NOT NULL ,"
            "TAX            INTEGER,"
            "NAME           VARCHAR(64) NOT NULL,"
            "NUM            VARCHAR(64),"
            "NUM_TAX        VARCHAR(64),"
            "CAPITAL        VARCHAR(64),"
            "ADDRESS1       VARCHAR(128),"
            "ADDRESS2       VARCHAR(128),"
            "ADDRESS3       VARCHAR(128),"
            "ZIPCODE        VARCHAR(10),"
            "CITY           VARCHAR(64),"
            "PHONENUMBER    VARCHAR(24),"
            "FAXNUMBER      VARCHAR(24),"
            "EMAIL          VARCHAR(128),"
            "WEBSITE        VARCHAR(256),"
            "LOGO           BLOB,"
            "CG             TEXT,"
            "CA_TYPE        INTEGER NOT NULL ,"
            "PRINT_LINE1    VARCHAR(64),"
            "PRINT_LINE2    VARCHAR(64),"
            "PRIMARY KEY (ID)"
            ");";

    //fuck the diff
    if(db.driverName() == "QMYSQL")
        req.replace("AUTOINCREMENT","AUTO_INCREMENT");

    QSqlQuery query;
    query.prepare( req );
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }

    //INSERT
    query.prepare("INSERT INTO TAB_INFORMATIONS(DBASE_VERSION, TAX, NAME, CA_TYPE)"
                  "VALUES('4', '0', '','1');");
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }

    return true;
}

/**
    Creation de la table banque
  */
bool database::createTable_bank(){
    QString req ="CREATE TABLE TAB_BANK("
            "ID             INTEGER NOT NULL ,"
            "CODE_BANQUE    VARCHAR(5),"
            "CODE_GUICHET   VARCHAR(5),"
            "NUM_COMPTE     VARCHAR(11),"
            "KEY_RIB        VARCHAR(64),"
            "ADDRESS       VARCHAR(128), "
            "IBAN_1        VARCHAR(4),"
            "IBAN_2        VARCHAR(4),"
            "IBAN_3        VARCHAR(4),"
            "IBAN_4        VARCHAR(4),"
            "IBAN_5        VARCHAR(4),"
            "IBAN_6        VARCHAR(4),"
            "IBAN_7        VARCHAR(4),"
            "IBAN_8        VARCHAR(4),"
            "IBAN_9        VARCHAR(2),"
            "CODE_BIC    VARCHAR(11),"
            "PRIMARY KEY (ID)"
            ");";

    //fuck the diff
    if(db.driverName() == "QMYSQL")
        req.replace("AUTOINCREMENT","AUTO_INCREMENT");

    QSqlQuery query;
    query.prepare( req );
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }

    //INSERT
    query.prepare("INSERT INTO TAB_BANK(CODE_BANQUE)"
                  "VALUES('');");
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    return true;

}


/**
    Creation de la table client
  */
bool database::createTable_customers(){
    QString req = "CREATE TABLE TAB_CUSTOMERS ("
            "ID             INTEGER PRIMARY KEY AUTOINCREMENT,"
            "CREATIONDATE   TIMESTAMP,"
            "TYPE           INTEGER  NOT NULL,"
            "DESC_COMPAGNY  VARCHAR(128) ,"
            "LASTNAME       VARCHAR(64) NOT NULL,"
            "FIRSTNAME      VARCHAR(64) ,"
            "GENDER         VARCHAR(3) ,"
            "BIRTHDAY       DATE,"
            "ADDRESS1       VARCHAR(128) ,"
            "ADDRESS2       VARCHAR(128) ,"
            "ADDRESS3       VARCHAR(128) ,"
            "ZIPCODE        VARCHAR(10) ,"
            "CITY           VARCHAR(64) ,"
            "COUNTRY        VARCHAR(64) ,"
            "PHONENUMBER    VARCHAR(24) ,"
            "MOBILENUMBER   VARCHAR(24) ,"
            "EMAIL          VARCHAR(128) ,"
            "PROFESSION     VARCHAR(128) ,"
            "COMMENTS       VARCHAR(256) ,"
            "SENDING_PUB_EMAIL	INTEGER NOT NULL"
            ");";

    //fuck the diff
    if(db.driverName() == "QMYSQL")
        req.replace("AUTOINCREMENT","AUTO_INCREMENT");

    QSqlQuery query;
    query.prepare( req );
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    return true;
}

/**
    Creation de la table des taxes
  */
bool database::createTable_tax(){
    QString req = "CREATE TABLE IF NOT EXISTS TAB_TAX ("
            "ID             INTEGER PRIMARY KEY AUTOINCREMENT,"
            "TAX            NUMERIC(5,2) NOT NULL,"
            "DESCRIPTION    VARCHAR(256)"
            ");";
    //fuck the diff
    if(db.driverName() == "QMYSQL")
        req.replace("AUTOINCREMENT","AUTO_INCREMENT");

    QSqlQuery query;
    query.prepare( req );
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    return true;
}

/**
    Creation de la table produits
  */
bool database::createTable_products(){
    /*
 MANUAL
 Standard SQL requires that DECIMAL(5,2) be able to store any value with five digits and two decimals,
 so values that can be stored in the salary column range from -999.99 to 999.99.
 */
    QString req = "CREATE TABLE IF NOT EXISTS TAB_PRODUCTS("
            "ID             INTEGER PRIMARY KEY AUTOINCREMENT,"
            "CREATIONDATE   TIMESTAMP,"
            "NAME           VARCHAR(256) NOT NULL,"
            "CODE           VARCHAR(64) NOT NULL,"
            "CODE_USER      VARCHAR(64) ,"
            "SELLING_PRICE  NUMERIC(8,2) NOT NULL,"
            "BUYING_PRICE   NUMERIC(8,2) NOT NULL,"
            "TAX            NUMERIC(5,2),"
            "STOCK          INTEGER NOT NULL,"
            "STOCK_WARNING  INTEGER NOT NULL,"
            "STATE          INTEGER NOT NULL,"
            "IMAGE          BLOB,"
            "ID_PROVIDER    INTEGER NOT NULL,"
            "ID_CATEGORY    INTEGER NOT NULL"
            ");";
    //fuck the diff
    if(db.driverName() == "QMYSQL")
        req.replace("AUTOINCREMENT","AUTO_INCREMENT");

    QSqlQuery query;
    query.prepare( req );
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }

    //CONTRAINT
    query.prepare( "CREATE UNIQUE INDEX CODE_PRODUCT ON TAB_PRODUCTS(CODE);");
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    query.prepare( "CREATE UNIQUE INDEX USERCODE_PRODUCT ON TAB_PRODUCTS(CODE_USER);");
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    return true;
}

/**
    Creation de la table des categories de produits
  */
bool database::createTable_products_categories(){
    QString req = "CREATE TABLE IF NOT EXISTS TAB_PRODUCTS_CATEGORIES("
            "ID     INTEGER PRIMARY KEY AUTOINCREMENT,"
            "NAME   VARCHAR(64) NOT NULL,"
            "COLOR  VARCHAR(64)"
            ");";
    //fuck the diff
    if(db.driverName() == "QMYSQL")
        req.replace("AUTOINCREMENT","AUTO_INCREMENT");

    QSqlQuery query;
    query.prepare( req );
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    return true;
}



/**
    Creation de la table fournisseur
  */
bool database::createTable_providers(){
    QString req = "CREATE TABLE IF NOT EXISTS TAB_PROVIDERS("
            "ID             INTEGER PRIMARY KEY AUTOINCREMENT,"
            "CREATIONDATE   TIMESTAMP,"
            "NAME           VARCHAR(64)NOT NULL,"
            "ADDRESS1       VARCHAR(128),"
            "ADDRESS2       VARCHAR(128),"
            "ADDRESS3       VARCHAR(128),"
            "ZIPCODE        VARCHAR(10),"
            "CITY           VARCHAR(64),"
            "COUNTRY        VARCHAR(64),"
            "PHONENUMBER    VARCHAR(24),"
            "FAXNUMBER      VARCHAR(24),"
            "EMAIL          VARCHAR(128),"
            "CONTACT_NAME   VARCHAR(64)"
            ");";
    //fuck the diff
    if(db.driverName() == "QMYSQL")
        req.replace("AUTOINCREMENT","AUTO_INCREMENT");

    QSqlQuery query;
    query.prepare( req );
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    return true;
}

/**
    Creation de la table des services
  */
bool database::createTable_services(){
    QString req = "CREATE TABLE IF NOT EXISTS TAB_SERVICES ("
            "ID             INTEGER PRIMARY KEY AUTOINCREMENT,"
            "ID_CUSTOMER    INTEGER NOT NULL,"
            "TAX            NUMERIC(5,2),"
            "CREATIONDATE   TIMESTAMP,"
            "THEDATE        TIMESTAMP,"
            "PRICE          NUMERIC(8,2) NOT NULL,"
            "NAME           VARCHAR(256) NOT NULL,"
            "DESCRIPTION    VARCHAR(1024),"
            "FOREIGN KEY(ID_CUSTOMER) REFERENCES TAB_CUSTOMERS(ID) ON DELETE CASCADE"
            ");";
    //fuck the diff
    if(db.driverName() == "QMYSQL")
        req.replace("AUTOINCREMENT","AUTO_INCREMENT");

    QSqlQuery query;
    query.prepare( req );
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    return true;
}

/**
    Creation de la table des services commun
  */
bool database::createTable_services_common(){
    QString req = "CREATE TABLE IF NOT EXISTS TAB_SERVICES_COMMONS ("
            "ID             INTEGER PRIMARY KEY AUTOINCREMENT,"
            "TAX            NUMERIC(5,2),"
            "CREATIONDATE   TIMESTAMP,"
            "THEDATE        TIMESTAMP,"
            "PRICE          NUMERIC(8,2) NOT NULL,"
            "NAME           VARCHAR(256) NOT NULL,"
            "DESCRIPTION    VARCHAR(1024)"
            "); ";
    //fuck the diff
    if(db.driverName() == "QMYSQL")
        req.replace("AUTOINCREMENT","AUTO_INCREMENT");

    QSqlQuery query;
    query.prepare( req );
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    return true;
}

/**
    Creation de la table des propositions commerciales / Devis
  */
bool database::createTable_proposals(){
    QString req = "    CREATE TABLE IF NOT EXISTS TAB_PROPOSALS ("
            "ID             INTEGER PRIMARY KEY AUTOINCREMENT,"
            "ID_CUSTOMER    INTEGER NOT NULL,"
            "CREATIONDATE   TIMESTAMP,"
            "DATE           DATE NOT NULL,"
            "VALIDDATE      DATE NOT NULL,"
            "DELIVERYDATE         DATE NOT NULL,"
            "DELAY_DELIVERYDATE   INTEGER,"
            "CODE           VARCHAR(64) NOT NULL,"
            "TYPE_PAYMENT   VARCHAR(2),"
            "PRICE          NUMERIC(8,2) NOT NULL,"
            "STATE          INTEGER NOT NULL,"
            "DESCRIPTION    VARCHAR(256),"
            "FOREIGN KEY(ID_CUSTOMER) REFERENCES TAB_CUSTOMERS(ID) ON DELETE CASCADE"
            ");";
    //fuck the diff
    if(db.driverName() == "QMYSQL")
        req.replace("AUTOINCREMENT","AUTO_INCREMENT");

    QSqlQuery query;
    query.prepare( req );
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }

    //CONTRAINT
    query.prepare( "CREATE UNIQUE INDEX CODE_PROPOSALS ON TAB_PROPOSALS(CODE);");
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    return true;
}


/**
    Creation de la table de details des propositions commerciales / Devis
  */
bool database::createTable_proposals_details(){
    QString req = "CREATE TABLE IF NOT EXISTS TAB_PROPOSALS_DETAILS ("
            "ID             INTEGER PRIMARY KEY AUTOINCREMENT,"
            "ID_PROPOSAL    INTEGER NOT NULL,"
            "ID_PRODUCT     INTEGER,"
            "TYPE           INTEGER,"
            "NAME           TEXT NOT NULL,"
            "DISCOUNT       INTEGER NOT NULL,"
            "QUANTITY       INTEGER NOT NULL,"
            "TAX            NUMERIC(5,2),"
            "PRICE          NUMERIC(8,2) NOT NULL,"
            "ITEM_ORDER     INTEGER,"
            "FOREIGN KEY(ID_PROPOSAL) REFERENCES TAB_PROPOSALS(ID) ON DELETE CASCADE"
            "); ";
    //fuck the diff
    if(db.driverName() == "QMYSQL")
        req.replace("AUTOINCREMENT","AUTO_INCREMENT");

    QSqlQuery query;
    query.prepare( req );
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    return true;
}


/**
    Creation de la table des factures
  */
bool database::createTable_invoices(){
    QString req = "CREATE TABLE IF NOT EXISTS TAB_INVOICES ("
            "ID             INTEGER PRIMARY KEY AUTOINCREMENT,"
            "ID_CUSTOMER    INTEGER NOT NULL,"
            "CREATIONDATE   TIMESTAMP,"
            "DATE           DATE NOT NULL,"
            "LIMIT_PAYMENTDATE    DATE NOT NULL,"
            "PAYMENTDATE    DATE NOT NULL,"
            "CODE           VARCHAR(64) NOT NULL,"
            "TYPE_PAYMENT   VARCHAR(2) ,"
            "PART_PAYMENT   NUMERIC(8,2) NOT NULL,"
            "PRICE          NUMERIC(8,2) NOT NULL,"
            "STATE          INTEGER NOT NULL,"
            "DESCRIPTION    VARCHAR(256),"
            "FOREIGN KEY(ID_CUSTOMER) REFERENCES TAB_CUSTOMERS(ID) ON DELETE CASCADE"
            ");  ";
    //fuck the diff
    if(db.driverName() == "QMYSQL")
        req.replace("AUTOINCREMENT","AUTO_INCREMENT");

    QSqlQuery query;
    query.prepare( req );
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    //CONTRAINT
    query.prepare( "CREATE UNIQUE INDEX CODE_INVOCES ON TAB_INVOICES(CODE);");
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    return true;
}


/**
    Creation de la table de details des factures
  */
bool database::createTable_invoices_details(){
    QString req = "CREATE TABLE IF NOT EXISTS TAB_INVOICES_DETAILS ("
            "ID             INTEGER PRIMARY KEY AUTOINCREMENT,"
            "ID_INVOICE     INTEGER NOT NULL,"
            "ID_PRODUCT     INTEGER,"
            "TYPE           INTEGER,"
            "NAME           TEXT NOT NULL,"
            "DISCOUNT       INTEGER NOT NULL,"
            "QUANTITY       INTEGER NOT NULL,"
            "TAX            NUMERIC(5,2),"
            "PRICE          NUMERIC(8,2) NOT NULL,"
            "ITEM_ORDER     INTEGER,"
            "FOREIGN KEY(ID_INVOICE) REFERENCES TAB_INVOICES(ID) ON DELETE CASCADE"
            ");";
    //fuck the diff
    if(db.driverName() == "QMYSQL")
        req.replace("AUTOINCREMENT","AUTO_INCREMENT");

    QSqlQuery query;
    query.prepare( req );
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    return true;
}




/**
    Creation de la table de Lien entre propositions commerciales / Devis et Factures
  */
bool database::createTable_link_proposals_invoices(){
    QString req = "CREATE TABLE IF NOT EXISTS TAB_LINK_PROPOSALS_INVOICES ("
            "ID             INTEGER PRIMARY KEY AUTOINCREMENT,"
            "ID_PROPOSAL    INTEGER NOT NULL,"
            "ID_INVOICE     INTEGER NOT NULL,"
            "FOREIGN KEY(ID_INVOICE) REFERENCES TAB_INVOICES(ID) ON DELETE CASCADE,"
            "FOREIGN KEY(ID_PROPOSAL) REFERENCES TAB_PROPOSALS(ID) ON DELETE CASCADE"
            ");";
    //fuck the diff
    if(db.driverName() == "QMYSQL")
        req.replace("AUTOINCREMENT","AUTO_INCREMENT");

    QSqlQuery query;
    query.prepare( req );
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    return true;
}

/**
    Obtenir la version de la base de donnees
    @return version
  */
int database::databaseVersion() {
    if(!this->m_connected)return -1;
    int ver=-1;

    QSqlQuery query;
    query.prepare("SELECT DBASE_VERSION FROM TAB_INFORMATIONS;");

    if(query.exec()){
        query.next();
        ver = query.value(0).toInt();
    }
    else{
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
    }
    return ver;
}

/**
    Savoir si la societe est assujetti a la TVA
    @return true if TAX
  */
bool database::isTax() {
    if(!this->m_connected)return false;
    bool ret=false;

    QSqlQuery query;
    query.prepare("SELECT TAX FROM TAB_INFORMATIONS;");

    if(query.exec()){
        query.next();
        ret = query.value(0).toBool();
    }
    else{
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
    }
    return ret;
}

/**
      Applique les informations dans la base de donnee
      @return true si ok
  */
bool database::updateInfo(Informations &info) {
    if(!this->m_connected)return false;
    //Test si l enregistrement existe
    QString req = "SELECT COUNT(*) FROM TAB_INFORMATIONS WHERE ID=1;";
    int count=0;
    QSqlQuery query;
    query.prepare(req);

    if(query.exec()){
        query.next();
        count = query.value(query.record().indexOf("COUNT(*)")).toInt();
    }
    else{
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    //Si pas d enregistrement on en creer un
    if(count<=0){
        query.prepare("INSERT INTO TAB_INFORMATIONS(DBASE_VERSION, TAX, NAME,CA_TYPE)"
                      "VALUES('1', '0', '', '1');");
        if(!query.exec()) {
            QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
            return false;
        }
    }

    // Construction de la requette
    // Si le charactere speciaux "\'" existe on l'adapte pour la requette
    QString f;
    req = "UPDATE TAB_INFORMATIONS SET ";
    req += "NAME='" + info.name.replace("\'","''") + "',";
    req += "NUM='" + info.num.replace("\'","''") + "',";
    req += "NUM_TAX='" + info.numTax.replace("\'","''") + "',";
    req += "CAPITAL='" + info.capital.replace("\'","''") + "',";
    req += "ADDRESS1='" + info.address1.replace("\'","''") + "',";
    req += "ADDRESS2='" + info.address2.replace("\'","''") + "',";
    req += "ADDRESS3='" + info.address3.replace("\'","''") + "',";
    req += "ZIPCODE='" + info.zipCode.replace("\'","''") + "',";
    req += "CITY='" + info.city.replace("\'","''") + "',";
    req += "PHONENUMBER='" + info.phoneNumber.replace("\'","''") + "',";
    req += "FAXNUMBER='" + info.faxNumber.replace("\'","''") + "',";
    req += "EMAIL='" + info.email.replace("\'","''") + "',";
    req += "WEBSITE='" + info.webSite.replace("\'","''") + "', ";
    req += "TAX='" + QString::number(info.tax) + "', ";
    req += "CA_TYPE='" + QString::number(info.ca_type) + "', ";
    req += "PRINT_LINE1='" + info.line1 + "', ";
    req += "PRINT_LINE2='" + info.line2 + "' ";
    req += "WHERE ID='1';";

    query.prepare(req);
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    /* update cache */
    m_isTax = info.tax;
    return true;
}

/**
      Retourne les informations dans la base de donnee
      @return Informations
  */
bool database::getInfo(Informations &info) {
    if(!this->m_connected)return false;

    QSqlQuery query;
    query.prepare("SELECT * from TAB_INFORMATIONS;");

    if(query.exec()){
        query.next();
        info.name = query.value(query.record().indexOf("NAME")).toString();
        info.num = query.value(query.record().indexOf("NUM")).toString();
        info.numTax = query.value(query.record().indexOf("NUM_TAX")).toString();
        info.capital = query.value(query.record().indexOf("CAPITAL")).toString();
        info.address1 = query.value(query.record().indexOf("ADDRESS1")).toString();
        info.address2 = query.value(query.record().indexOf("ADDRESS2")).toString();
        info.address3 = query.value(query.record().indexOf("ADDRESS3")).toString();
        info.zipCode = query.value(query.record().indexOf("ZIPCODE")).toString();
        info.city = query.value(query.record().indexOf("CITY")).toString();
        info.phoneNumber = query.value(query.record().indexOf("PHONENUMBER")).toString();
        info.faxNumber = query.value(query.record().indexOf("FAXNUMBER")).toString();
        info.email = query.value(query.record().indexOf("EMAIL")).toString();
        info.webSite = query.value(query.record().indexOf("WEBSITE")).toString();
        info.tax = query.value(query.record().indexOf("TAX")).toInt();
        info.ca_type = query.value(query.record().indexOf("CA_TYPE")).toInt();
        info.line1 = query.value(query.record().indexOf("PRINT_LINE1")).toString();
        info.line2 = query.value(query.record().indexOf("PRINT_LINE2")).toString();
    }
    else{
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    /* update cache */
    m_isTax = info.tax;
    return true;
}

/**
      Applique les informations de la banque dans la base de donnee
      @return true si ok
  */
bool database::updateBank(Bank &b) {
    if(!this->m_connected)return false;
    //Test si l enregistrement existe
    QString req = "SELECT COUNT(*) FROM TAB_BANK WHERE ID=1;";
    int count=0;
    QSqlQuery query;
    query.prepare(req);

    if(query.exec()){
        query.next();
        count = query.value(query.record().indexOf("COUNT(*)")).toInt();
    }
    else{
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    //Si pas d enregistrement on en creer un
    if(count<=0){
        query.prepare("INSERT INTO TAB_BANK(CODE_BANQUE)"
                      "VALUES('');");
        if(!query.exec()) {
            QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
            return false;
        }
    }

    // Construction de la requette
    // Si le charactere speciaux "\'" existe on l'adapte pour la requette
    req = "UPDATE TAB_BANK SET ";
    req += "CODE_BANQUE='" + b.codeBanque.replace("\'","''") + "',";
    req += "CODE_GUICHET='" + b.codeGuichet.replace("\'","''") + "',";
    req += "NUM_COMPTE='" + b.numCompte.replace("\'","''") + "',";
    req += "KEY_RIB='" + b.keyRIB.replace("\'","''") + "',";
    req += "ADDRESS='" + b.address.replace("\'","''") + "',";
    req += "IBAN_1='" + b.IBAN1.replace("\'","''") + "',";
    req += "IBAN_2='" + b.IBAN2.replace("\'","''") + "',";
    req += "IBAN_3='" + b.IBAN3.replace("\'","''") + "',";
    req += "IBAN_4='" + b.IBAN4.replace("\'","''") + "',";
    req += "IBAN_5='" + b.IBAN5.replace("\'","''") + "',";
    req += "IBAN_6='" + b.IBAN6.replace("\'","''") + "',";
    req += "IBAN_7='" + b.IBAN7.replace("\'","''") + "', ";
    req += "IBAN_8='" + b.IBAN8.replace("\'","''") + "', ";
    req += "IBAN_9='" + b.IBAN9.replace("\'","''") + "', ";
    req += "CODE_BIC='" + b.codeBIC.replace("\'","''") + "'";
    req += "WHERE ID='1';";

    query.prepare(req);
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    return true;
}

/**
      Retourne les informations banque de la base de donnee
      @return bank
  */
bool database::getBank(Bank &b) {
    if(!this->m_connected)return false;

    QSqlQuery query;
    query.prepare("SELECT * from TAB_BANK;");
    if(query.exec()){
        query.next();
        b.codeBanque = query.value(query.record().indexOf("CODE_BANQUE")).toString();
        b.codeGuichet = query.value(query.record().indexOf("CODE_GUICHET")).toString();
        b.numCompte = query.value(query.record().indexOf("NUM_COMPTE")).toString();
        b.keyRIB = query.value(query.record().indexOf("KEY_RIB")).toString();
        b.address = query.value(query.record().indexOf("ADDRESS")).toString();
        b.IBAN1 = query.value(query.record().indexOf("IBAN_1")).toString();
        b.IBAN2 = query.value(query.record().indexOf("IBAN_2")).toString();
        b.IBAN3 = query.value(query.record().indexOf("IBAN_3")).toString();
        b.IBAN4 = query.value(query.record().indexOf("IBAN_4")).toString();
        b.IBAN5 = query.value(query.record().indexOf("IBAN_5")).toString();
        b.IBAN6 = query.value(query.record().indexOf("IBAN_6")).toString();
        b.IBAN7 = query.value(query.record().indexOf("IBAN_7")).toString();
        b.IBAN8 = query.value(query.record().indexOf("IBAN_8")).toString();
        b.IBAN9 = query.value(query.record().indexOf("IBAN_9")).toString();
        b.codeBIC = query.value(query.record().indexOf("CODE_BIC")).toString();
    }
    else{
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }
    return true;
}

/**
    Met a jour le logo dans la base de donnees
  */
bool database::updateLogoTable_informations(const QImage& image){
    if(!this->m_connected)return false;

    QByteArray data;
    QBuffer buf(&data);
    // writes image into ba in PNG format
    image.save(&buf,"PNG");

    QSqlQuery query;
    query.prepare( "UPDATE TAB_INFORMATIONS SET LOGO = :data" );
    query.bindValue( ":data", buf.data() );
    if(!query.exec()) {
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
        return false;
    }

    return true;
}

/**
    Recupere le logo dans la base de donnees
  */
QImage database::getLogoTable_informations(){
    QImage image;
    QByteArray data;
    if(!this->m_connected)return image;

    QSqlQuery query;
    query.prepare("SELECT LOGO FROM TAB_INFORMATIONS;");
    if(query.exec()){
        query.next();
        data = query.value(query.record().indexOf("LOGO")).toByteArray();
        image.loadFromData(data,"PNG");
    }
    else{
        QMessageBox::critical(this->m_parent, tr("Erreur"), query.lastError().text());
    }
    return image;
}


/**
   Met a jour la base de donnees en version 2
  */
bool database::upgradeToV2(QString *log) {
    QString req;
    bool done=true;
    QSqlQuery query;
    *log = "Mise a jour de la base de donn�es en version 2:";

    req =	"ALTER TABLE TAB_PROPOSALS_DETAILS ADD ITEM_ORDER INTEGER;";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    req =	"ALTER TABLE TAB_INVOICES_DETAILS ADD ITEM_ORDER INTEGER;";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    req =	"UPDATE TAB_INFORMATIONS SET DBASE_VERSION=2;";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    return done;
}

/**
   Met a jour la base de donnees en version 3
  */
bool database::upgradeToV3(QString *log) {
    QString req;
    bool done=true;
    QSqlQuery query;
    *log = "Mise a jour de la base de donn�es en version 3:";

    req =	"ALTER TABLE TAB_INVOICES ADD PAYMENTDATE;";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    // Pour les factures existante on prend la date de creation comme date de reglement
    req =	"UPDATE TAB_INVOICES SET PAYMENTDATE = DATE;";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    req =	"ALTER TABLE TAB_INFORMATIONS ADD CA_TYPE INT DEFAULT 0;";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    req =	"UPDATE TAB_INFORMATIONS SET DBASE_VERSION=3;";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    return done;
}

/**
   Met a jour la base de donnees en version 4
  */
bool database::upgradeToV4(QString *log) {
    QString req;
    bool done=true;
    QSqlQuery query;
    *log = "Mise a jour de la base de donn�es en version 4:";

    // ajout dune colonne
    req =	"ALTER TABLE TAB_INFORMATIONS ADD NUM_TAX VARCHAR(64);";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    //Creation dune table temporaire DEVIS
    req =	"ALTER TABLE 'TAB_PROPOSALS_DETAILS' RENAME TO 'TAB_PROPOSALS_DETAILS_TEMP';";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    //creation de la table modifiee
    req = "CREATE TABLE IF NOT EXISTS TAB_PROPOSALS_DETAILS ("
            "ID             INTEGER PRIMARY KEY AUTOINCREMENT,"
            "ID_PROPOSAL    INTEGER NOT NULL,"
            "ID_PRODUCT     INTEGER,"
            "NAME           TEXT NOT NULL,"
            "DISCOUNT       INTEGER NOT NULL,"
            "QUANTITY       INTEGER NOT NULL,"
            "TAX            NUMERIC(5,2),"
            "PRICE          NUMERIC(8,2) NOT NULL,"
            "ITEM_ORDER     INTEGER,"
            "FOREIGN KEY(ID_PROPOSAL) REFERENCES TAB_PROPOSALS(ID) ON DELETE CASCADE"
            "); ";
    if(db.driverName() == "QMYSQL")
        req.replace("AUTOINCREMENT","AUTO_INCREMENT");
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    //Copie des valeurs dans la nouvelle table !
    req =  "INSERT INTO 'TAB_PROPOSALS_DETAILS' SELECT * FROM 'TAB_PROPOSALS_DETAILS_TEMP';";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    //Suppression de la table temporaire
    req =  "DROP TABLE 'TAB_PROPOSALS_DETAILS_TEMP';";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    // ajout dune colonne
    req =	"ALTER TABLE TAB_PROPOSALS_DETAILS ADD TYPE INTEGER;";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    // Modification du type d item
    req =	"UPDATE TAB_PROPOSALS_DETAILS SET TYPE=1 WHERE ID_PRODUCT>0;";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    //Creation dune table temporaire FACTURE
    req =	"ALTER TABLE 'TAB_INVOICES_DETAILS' RENAME TO 'TAB_INVOICES_DETAILS_TEMP';";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    req = "CREATE TABLE IF NOT EXISTS TAB_INVOICES_DETAILS ("
            "ID             INTEGER PRIMARY KEY AUTOINCREMENT,"
            "ID_INVOICE     INTEGER NOT NULL,"
            "ID_PRODUCT     INTEGER,"
            "NAME           TEXT NOT NULL,"
            "DISCOUNT       INTEGER NOT NULL,"
            "QUANTITY       INTEGER NOT NULL,"
            "TAX            NUMERIC(5,2),"
            "PRICE          NUMERIC(8,2) NOT NULL,"
            "ITEM_ORDER     INTEGER,"
            "FOREIGN KEY(ID_INVOICE) REFERENCES TAB_INVOICES(ID) ON DELETE CASCADE"
            ");";
    if(db.driverName() == "QMYSQL")
        req.replace("AUTOINCREMENT","AUTO_INCREMENT");
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    //Copie des valeurs dans la nouvelle table !
    req =  "INSERT INTO 'TAB_INVOICES_DETAILS' SELECT * FROM 'TAB_INVOICES_DETAILS_TEMP';";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    //Suppression de la table temporaire
    req =  "DROP TABLE 'TAB_INVOICES_DETAILS_TEMP';";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    // Ajout dune colonne
    req =	"ALTER TABLE TAB_INVOICES_DETAILS ADD TYPE INTEGER;";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    // Modification du type d item
    req =	"UPDATE TAB_INVOICES_DETAILS SET TYPE=1 WHERE ID_PRODUCT>0;"; // ET UPDATE TAB_INVOICES_DETAILS SET TYPE=0 WHERE TYPE IS NULL;
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    //Ajout champ pour impression dans bdd
    req =	"ALTER TABLE TAB_INFORMATIONS ADD PRINT_LINE1 VARCHAR(64)";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    req =	"ALTER TABLE TAB_INFORMATIONS ADD PRINT_LINE2 VARCHAR(64)";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    //Update numero version bdd
    req =	"UPDATE TAB_INFORMATIONS SET DBASE_VERSION=4;";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    return done;
}

/**
   Met a jour la base de donnees en version 5
  */
bool database::upgradeToV5(QString *log) {
    QString req;
    bool done=true;
    QSqlQuery query;
    *log = "Mise a jour de la base de donn�es en version 5:";

    // Modification du type d item
    req =	"UPDATE TAB_INVOICES_DETAILS SET TYPE=1 WHERE ID_PRODUCT>0;";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    // Modification du type d item
    req =	"UPDATE TAB_INVOICES_DETAILS SET TYPE=0 WHERE TYPE IS NULL;";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    //Update numero version bdd
    req =	"UPDATE TAB_INFORMATIONS SET DBASE_VERSION=5;";
    *log += "\n\n"+ req;
    query.prepare( req );
    if(!query.exec()) {
        *log += "\n->" + query.lastError().text();
        done = false;
    }
    else
        *log += "\n-> FAIT";

    return done;
}
/// TODO -> creer une class update.cpp




void database::exportData(QString query, QString typeOfExport){

    QString fileName = QFileDialog::getSaveFileName(0, tr("Exporter '")+typeOfExport+tr("' vers ..."),
                                                    QDir::homePath(),
                                                    tr("CSV File (*.csv)"));

    if(fileName.isEmpty())
        return;

    QFile::remove(fileName);

    QSqlQuery querExport(db);
    querExport.prepare(query);

    if(!querExport.exec())
    {
        qDebug()<<"Query error = " << querExport.lastError();
        QMessageBox::warning(0,"Erreur exportation",querExport.lastError().text());
    }
    else
    {
        QString toSave;

        while(querExport.next())
        {
            QApplication::processEvents();

            const QSqlRecord record = querExport.record();

            QStringList recordexp;

            if(toSave.isEmpty())
            {
                for(int p = 0; p < record.count() ; p++)
                    recordexp<<record.fieldName(p);
                toSave+=recordexp.join(";");
                toSave+="\n";
            }

            recordexp.clear();
            for(int p = 0; p < record.count() ; p++)
                recordexp<<record.value(p).toString();
            toSave+=recordexp.join(";");
            toSave+="\n";
        }
        if(!toSave.isEmpty())
        {
            if(!fileName.contains(".csv"))
                fileName+=".csv";
            QFile file(fileName);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
            {
                QMessageBox::warning(0,QString("Erreur exportation"),QString("Impossible d'ouvrir le fichier:\n")
                                     +QString(file.errorString()));
            }
            else
            {
                QTextStream out(&file);
                out <<toSave;
            }

            QMessageBox::information(0,"Exportation","Exportation réussie");
        }
        else
            QMessageBox::information(0,"Exportation","Rien à exporter");
    }
    querExport.finish();

}


