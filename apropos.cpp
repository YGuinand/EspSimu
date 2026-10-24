#include "apropos.h"
#include "ui_apropos.h"

Apropos::Apropos(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Apropos)
{
    ui->setupUi(this);

    // Version automatique depuis le .pro (APP_VERSION) : le jeton %APPVERSION%
    // pose dans apropos.ui est remplace ici. toHtml()/setHtml() : re-ecriture
    // du document deja parse par setupUi, mise en forme conservee.
    ui->textBrowser->setHtml(ui->textBrowser->toHtml().replace(
        QStringLiteral("%APPVERSION%"), QString::fromLatin1(APP_VERSION)));
}

Apropos::~Apropos()
{
    delete ui;
}
