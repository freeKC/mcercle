#ifndef PROPOSAL_H
#define PROPOSAL_H

#include <QStringList>
#include <vector>
#include <QLocale>
#include <QDateTime>
#include <QIcon>
#include "bdd/ibpp.h"


class proposal : public QObject {

private:
    IBPP::Database m_db;
    IBPP::Transaction m_tr;
    IBPP::Statement m_st;

    QWidget *m_parent;
    int m_id, m_idCustomer, m_state, m_delayDeliveryDate;
    float m_price;
    QDateTime m_creationDate;
    QDate m_userDate, m_deliveryDate, m_validDate;
    QString m_code, m_InvoiceCode, m_description, m_typePayment;
    QLocale m_lang;


public:
    //Valeur pour le placement dans un tableau.
    //Proposition commercial /devis
    typedef struct{
        std::vector<int> id;
        std::vector<QDate> userDate;
        std::vector<QDate> deliveryDate;
        std::vector<int> delayDeliveryDate;
        std::vector<QDate> validDate;
        QStringList code;
        QStringList codeInvoice; //Facture associee
        QStringList description;
        std::vector<float> price;
        std::vector<int> state;
    }ProposalList;

    typedef struct{
        QStringList customerFirstName;
        QStringList customerLastName;
        std::vector<int> id;
        std::vector<QDate> userDate;
        QStringList code;
        QStringList description;
        std::vector<float> price;
    }ProposalListAlert;

    //Details de la proposition
    typedef struct{
        std::vector<int> id;
        std::vector<int> idProduct;
        QStringList name;
        std::vector<int> discount;
        std::vector<int> quantity;
        std::vector<float> tax;
        std::vector<float> price;
    }ProposalListItems;

    //Un Item de la proposition
    typedef struct{
        int id;
        int idProduct;
        QString name;
        int discount;
        int quantity;
        float tax;
        float price;
    }ProposalItem;

    //State
    enum{WRITING, PROPOSED, VALIDATED};

    proposal(IBPP::Database db, IBPP::Transaction tr, IBPP::Statement st, QWidget *parent = 0);
    ~proposal();

    bool create();
    bool update();
    bool remove();
    bool loadFromID(const int& id);
    bool loadFromCode(const QString& code);
    bool isHere(const QString& code);
    int getLastId();

    //Appliquer les valeurs
    void setId(const int& ident){m_id = ident;}
    void setIdCustomer(const int& cIdent){m_idCustomer = cIdent;}
    void setPrice(const float& price){m_price = price;}
    void setTypePayment(const QString& type){m_typePayment = type;}
    void setState(const int& state){m_state = state;}
    void setCode(const  QString& code){m_code = code;}
    void setDescription(const  QString& description){m_description = description;}
    void setUserDate(const QDate& date){m_userDate = date;}
    void setDeliveryDate(const QDate& date){m_deliveryDate = date;}
    void setDelayDeliveryDate(const int& days){m_delayDeliveryDate = days;}
    void setValidDate(const QDate& date){m_validDate = date;}

    //recup les valeurs de la facture
    QIcon getIconState(int state);
    QString getTextState(int state);
    QString getCode(){return m_code;}
    QString getInvoiceCode(){return m_InvoiceCode;}
    QString getDescription(){return m_description;}
    QString getTypePayment(){return m_typePayment;}
    int getId(){return m_id;}
    int getIdCustomer(){return m_idCustomer;}
    int getState(){return m_state;}
    float getPrice(){return m_price;}
    QDateTime getCreationDate(){return m_creationDate;}
    QDate getUserDate(){return m_userDate;}
    QDate getDeliveryDate(){return m_deliveryDate;}
    int getDelayDeliveryDate(){return m_delayDeliveryDate;}
    QDate getValidDate(){return m_validDate;}


    void getProposalList(ProposalList& list, int id_customer, QString order, QString filter, QString field);
    void getProposalListAlert(ProposalListAlert& list);

    //Fonction pour creer le lien entre proposition commerciale et facture
    bool setLink( const int& idProposal, const int& idInvoice );

    //recup des articles de la facture
    void getProposalItemsList(ProposalListItems& list, QString order, QString filter, QString field);

    // Fonction sur un article
    void getProposalItem(ProposalItem& item);
    bool addProposalItem(ProposalItem& item);
    bool removeProposalItem(int Itemid);
    bool updateProposalItem(ProposalItem& item);
};



#endif // PROPOSAL_H
