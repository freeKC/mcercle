#ifndef DIALOGINVOICE_H
#define DIALOGINVOICE_H

#include <QDialog>

#include "dbase.h"
#include "productView.h"

namespace Ui {
    class DialogInvoice;
}

class DialogInvoice : public QDialog
{
    Q_OBJECT

public:
    explicit DialogInvoice(QLocale &lang, database *pdata, unsigned char type, unsigned char state, QWidget *parent = 0);
    ~DialogInvoice();

    void setTitle(QString val);
    enum{PROPOSAL_TYPE, INVOICE_TYPE};
    enum{ADD, EDIT};


private slots:
    void calcul_Total();
    void checkConditions();

    void on_tableWidget_itemSelectionChanged();
    void on_toolButton_rm_clicked();
    void on_tableWidget_cellChanged(int row, int column);
    void on_doubleSpinBox_partPAYMENT_valueChanged(double arg1);    
    void on_pushButton_ok_clicked();
    void on_pushButton_cancel_clicked();
    void on_toolButton_add_clicked();

    void on_pushButton_createInv_clicked();

    void on_toolButton_addFreeline_clicked();

private:
    Ui::DialogInvoice *ui;
    customer *m_customer;
    product *m_product;
    invoice *m_invoice;
    proposal *m_proposal;
    service *m_service;
    service_common *m_serviceComm;
    QLocale m_lang;
    unsigned char m_DialogType, m_DialogState;
    int m_articleInv_Id;
    int m_index_tabInvoice;
    float m_totalPrice;

    invoice::InvoiceListItems m_ilist;
    productView *m_productView;    

#define COL_COUNT 7
#define COL_ID 0
#define COL_ID_PRODUCT 1
#define COL_NAME 2
#define COL_PRICE 3
#define COL_QUANTITY 4
#define COL_DISCOUNT 5
#define COL_TOTAL 6

    void setProposal(unsigned char proc);
    void setInvoice(unsigned char proc);

    void updateProposalItems();
    void removeProposalItems();
    void updateInvoiceItems();
    void removeInvoiceItems();
    void loadValues();
    void add_to_Table(int idProduct, QString name, float price);
    int getDiffQuantityOfItem(const int& id, int qteNew);

    void listProposalDetailsToTable(QString filter, QString field);
    void listInvoiceDetailsToTable(QString filter, QString field);
    void listServiceToTable();

    void createInvoiceFromProposal();

signals:
    void askRefreshProposalList();
    void askRefreshInvoiceList();
};

#endif // DIALOGINVOICE_H
