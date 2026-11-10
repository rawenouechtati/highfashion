#include "article.h"

Article::Article(QString id, QString nom, QString maison, QString decennie, QString categorie, QString etat, QString prix) {
    this->id = id;
    this->nom = nom;
    this->maison = maison;
    this->decennie = decennie;
    this->categorie = categorie;
    this->etat = etat;
    this->prix = prix;
}

bool Article::ajouter() {
    QSqlQuery query;
    query.prepare("INSERT INTO ARTICLES (ID, NOM, MAISON, DECENNIE, CATEGORIE, ETAT, PRIX) "
                  "VALUES (:id, :nom, :maison, :decennie, :categorie, :etat, :prix)");

    query.bindValue(":id", id);
    query.bindValue(":nom", nom);
    query.bindValue(":maison", maison);
    query.bindValue(":decennie", decennie);
    query.bindValue(":categorie", categorie);
    query.bindValue(":etat", etat);
    query.bindValue(":prix", prix);

    return query.exec();
}

bool Article::supprimer(QString articleId) {
    QSqlQuery query;
    query.prepare("DELETE FROM ARTICLES WHERE ID = :id");
    query.bindValue(":id", articleId);

    return query.exec();
}

QSqlQueryModel* Article::afficher() {
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery("SELECT ID, NOM, MAISON, DECENNIE, CATEGORIE, ETAT, PRIX FROM ARTICLES");

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Maison"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Décennie"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Catégorie"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("État"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Prix (DT)"));

    return model;
}