#ifndef XMLOPENSAVEDATA_H
#define XMLOPENSAVEDATA_H

#include <QObject>
#include <QDomImplementation>

class XMLOpenSaveData : public QObject
{
    Q_OBJECT

public:
    explicit XMLOpenSaveData(QObject *parent = 0);
    static QDomElement creation(QDomDocument &doc, QDomElement &parent, const QString &nom);
    static QDomElement creation(QDomDocument &doc, QDomElement &parent, const QString &nom, const QString &valeur);
    static QDomElement creation(QDomDocument &doc, QDomElement &parent, const QString &nom, const bool valeur);
    static QDomElement creation(QDomDocument &doc, QDomElement &parent, const QString &nom, const int valeur);
    static QDomElement creation(QDomDocument &doc, QDomElement &parent, const QString &nom, const double valeur, char format = 'g', int precision = 6);
    static QDomElement creationAttrNumero(QDomDocument &doc, QDomElement &parent, const QString &nom, const int attrVal);

    void openData(const QString &fichier);
    virtual void initialise() {}
    virtual void decode(const QString & /* obj */, const QString & /* num */, const QString & /* attr */, const QString & /* val */) {}

signals:
    void openNew();

public slots:

};

#endif // XMLOPENSAVEDATA_H
