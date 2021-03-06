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
#include <QSettings>
#include <QMessageBox>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QString getMovieTitleFromJSON(QJsonObject& jsonObject);
    QList<QString> getRecommendationsFromJSON(QJsonObject& jsonObject);
    void exportMovieToFile();
    QString makeMovieJson(QString& title,QList<QString>& recommended,QString& id);
    bool writeInFile(QString& fileName, QString& movieJSON);

    bool requestsFinished();
    QJsonObject ObjectFromString(const QString& json);
    void stopAddingMovies();

public slots:
    void addNextMovie();
    void replyMovieFinished(QNetworkReply* reply);
    void replyRecommendationsFinished(QNetworkReply* reply);

private slots:
    void on_Run_clicked();

    void on_Stop_clicked();

    void on_setFilePath_clicked();

    void on_filePathText_textChanged(const QString &arg1);

    void on_apiKeyLineEdit_editingFinished();

    void on_actionAbout_triggered();

    void on_actionAbout_Qt_triggered();

private:
    Ui::MainWindow *ui;

    //lock to make sure we don't try to add multiple movies at the same time
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

    //Path of the file to export
    QString filePath;

    //Id of the movie we are currently adding
    int currentId;

    //Save settings
    QSettings *settings;

    //Timer that calls the add movie function every x secondes
    QTimer *timer;
};

#endif // MAINWINDOW_H
