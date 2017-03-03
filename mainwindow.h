#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QFileDialog>

#ifdef Q_OS_WIN
#include <windows.h> // for Sleep
#endif

#include <QDebug>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


    void AddMovie(QString fileName, QString movieId);
    QString getMovieTitleFromJSON(QJsonObject jsonObject);
    QList<QString> getRecommendationsFromJSON(QJsonObject jsonObject);
    void exportMovieToFile();
    QString makeMovieJson(QString title,QList<QString>recommended);
    bool writeInFile(QString fileName, QString movieJSON);

    bool requestsFinished();
    QJsonObject ObjectFromString(const QString& json);


public slots:
    void Run();
    void replyMovieFinished(QNetworkReply* reply);
    void replyRecommendationsFinished(QNetworkReply* reply);

private slots:
    void on_AddMovie_clicked();

    void on_Stop_clicked();

    void on_setFilePath_clicked();

private:
    Ui::MainWindow *ui;

    bool run;
    bool addingMovie;

    //TMDB apiKey
    QString apiKey;

    //Manager of the title request
    QNetworkAccessManager *managerMovieTitle;
    bool titleRequestFinished;

    //Manager of the recommendations request
    QNetworkAccessManager *managerRecommendations;
    bool recommendationsRequestFinished;

    //Current movie title
    QString movieTitle;

    //Current movie id
    QString movieId;

    //Current recommendations
    QList<QString> recommendationList;

    QString filePath;
};

#endif // MAINWINDOW_H
