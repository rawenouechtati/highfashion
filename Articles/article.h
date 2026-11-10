#ifndef ARTICLE_H
#define ARTICLE_H

#include <QString>
#include <QSqlQuery>
#include <QSqlQueryModel>

class Article {
    QString id;
    QString nom;
    QString maison;
    QString decennie;
    QString categorie;
    QString etat;
    QString prix;

public:
    Article() {}
    Article(QString id, QString nom, QString maison, QString decennie, QString categorie, QString etat, QString prix);

    QString getId() const { return id; }
    QString getNom() const { return nom; }
    QString getMaison() const { return maison; }
    QString getDecennie() const { return decennie; }
    QString getCategorie() const { return categorie; }
    QString getEtat() const { return etat; }
    QString getPrix() const { return prix; }

    void setId(const QString &id) { this->id = id; }
    void setNom(const QString &nom) { this->nom = nom; }
    void setMaison(const QString &maison) { this->maison = maison; }
    void setDecennie(const QString &decennie) { this->decennie = decennie; }
    void setCategorie(const QString &categorie) { this->categorie = categorie; }
    void setEtat(const QString &etat) { this->etat = etat; }
    void setPrix(const QString &prix) { this->prix = prix; }

    bool ajouter();
    bool supprimer(QString articleId);
    QSqlQueryModel* afficher();
};

#endif