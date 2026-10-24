#include <QFile>
#include <QTextStream>

#include "xmlopensavedata.h"

XMLOpenSaveData::XMLOpenSaveData(QObject *parent) :
    QObject(parent)
{
}

QDomElement XMLOpenSaveData::creation(QDomDocument &doc, QDomElement &parent, const QString &nom)
{
    QDomElement nouveau = doc.createElement(nom);
    parent.appendChild(nouveau);
    return nouveau;
}

QDomElement XMLOpenSaveData::creation(QDomDocument &doc, QDomElement &parent, const QString &nom, const QString &valeur)
{
    QDomElement nouveau = doc.createElement(nom);
    parent.appendChild(nouveau);
    nouveau.appendChild(doc.createTextNode(valeur));
    return nouveau;
}

QDomElement XMLOpenSaveData::creation(QDomDocument &doc, QDomElement &parent, const QString &nom, const bool valeur)
{
    return creation(doc, parent, nom, QString().setNum(valeur));
}

QDomElement XMLOpenSaveData::creation(QDomDocument &doc, QDomElement &parent, const QString &nom, const int valeur)
{
    return creation(doc, parent, nom, QString().setNum(valeur));
}

QDomElement XMLOpenSaveData::creation(QDomDocument &doc, QDomElement &parent, const QString &nom, const double valeur, char format, int precision)
{
    return creation(doc, parent, nom, QString().setNum(valeur, format, precision));
}

QDomElement XMLOpenSaveData::creationAttrNumero(QDomDocument &doc, QDomElement &parent, const QString &nom, const int attrVal)
{
    QDomElement nouveau = doc.createElement(nom);
    nouveau.setAttribute("numero", attrVal);
    parent.appendChild(nouveau);
    return nouveau;
}

void XMLOpenSaveData::openData(const QString &fichier)
{
    QDomDocument doc;
    QFile f(fichier);
    if (!f.open(QIODevice::ReadOnly))
        return;
    if (!doc.setContent(&f)) {
        f.close();
        return;
    }
    f.close();

    initialise();

    QDomElement root=doc.documentElement();
  //  cout<<"The Gamel name is "<<root.attribute("name","NO NAME")<<endl;
    QDomElement child=root.firstChild().toElement();
    while(!child.isNull()) {
        QDomElement grandChild=child.firstChild().toElement();
        while(!grandChild.isNull()) {
            QString num;
            if (child.hasAttribute("numero")) num = child.attribute("numero");
            decode(child.tagName(), num, grandChild.tagName(), grandChild.text());
            grandChild = grandChild.nextSibling().toElement();
        }
        child = child.nextSibling().toElement();
    }

    emit openNew();
}

