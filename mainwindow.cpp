#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    apiKey = "2d28c41e6dab17f0aa2ddb2a9cf2b8f0";

    //Manager of the title request
    managerMovieTitle = new QNetworkAccessManager(this);
    connect(managerMovieTitle, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyMovieFinished(QNetworkReply*)));

    //Manager of the recommendations request
    managerRecommendations = new QNetworkAccessManager(this);
    connect(managerRecommendations, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyRecommendationsFinished(QNetworkReply*)));

    movieTitle = QString();
    recommendationList = QList<QString>();
    addingMovie = false;
    filePath = "file.json";

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::Run()
{
    if(!run)
        return;

    qDebug()<<"AddMovie"<<endl;
    AddMovie("test.json","550");

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(Run()));
    timer->start(2010);
}

//Add a movie as json to the file
void MainWindow::AddMovie(QString fileName, QString movieId)
{
    //If we are still adding a movie, do not add another one at the same time
    if(addingMovie)
        return;

    //Clear the variables
    movieTitle = QString();
    recommendationList.clear();

    //Set the movie id
    this->movieId =movieId;

    //Note that whe start adding a movie
    titleRequestFinished=false;
    recommendationsRequestFinished=false;
    addingMovie = true;

   //request of the movie title by id
   QString requestTitleURL = "http://api.themoviedb.org/3/movie/"+movieId+"?api_key="+apiKey;
   managerMovieTitle->get(QNetworkRequest(QUrl(requestTitleURL)));

   //request of the recommendations by id
   QString requestRecommendationsURL = "http://api.themoviedb.org/3/movie/"+movieId+"/recommendations?api_key="+apiKey;
   managerRecommendations->get(QNetworkRequest(QUrl(requestRecommendationsURL)));
}

//Callback called when we get the response of the movie title request
void MainWindow::replyMovieFinished(QNetworkReply *reply)
{
    //Read the reply data as string
    QString replyString=reply->readAll();

    //Parse the string as a json
    QJsonObject object = ObjectFromString(replyString);

    //Get the movie title from the json
    movieTitle = getMovieTitleFromJSON(object);

    qDebug()<<movieTitle<<endl;

    //Note that we finished
    titleRequestFinished=true;

    //If all request ended, go to next step
    if(requestsFinished())
        exportMovieToFile();
}

//Callback called when we get the response of the recommendations request
void MainWindow::replyRecommendationsFinished(QNetworkReply *reply)
{
    //Read the reply data as string
    QString replyString=reply->readAll();

    //Parse the string as a json
    QJsonObject object = ObjectFromString(replyString);

    //Get a list of recommendation from the json
    recommendationList = getRecommendationsFromJSON(object);

    qDebug()<<recommendationList<<endl;

    //Note that we finished
    recommendationsRequestFinished=true;

    //If all request ended, go to next step
    if(requestsFinished())
        exportMovieToFile();
}

//Parse the json response of the title request
QString MainWindow::getMovieTitleFromJSON(QJsonObject jsonObject)
{
    //Get the title from the movie json object
    return jsonObject.find("title").value().toString();
}

//Parse the json response of the recommendations request
QList<QString> MainWindow::getRecommendationsFromJSON(QJsonObject jsonObject)
{
    //Array containing the results of the request
    QJsonArray resultArray = jsonObject.find("results").value().toArray();

    //List of movie title
    QList<QString> titles = QList<QString>();

    for(int i=0;i<resultArray.size();i++)
    {
        //Get the movie json object of this result
        QJsonObject movie = resultArray.at(i).toObject();

        //Get the title from the movie json object
        QString title = movie.find("title").value().toString();

        if(!title.isEmpty())
        {
            //Add its title to the list
            titles.append(title);
        }
    }

    return titles;
}

void MainWindow::exportMovieToFile()
{
    QString json = makeMovieJson(movieTitle,recommendationList);

    writeInFile(filePath,json);

    //Note that we finished ending a movie
    addingMovie = false;
}

//Build the json with the movie title and the recommendations
QString MainWindow::makeMovieJson(QString title,QList<QString>recommended)
{
    QString recommendedJson="";

    for(int i=0;i<recommended.length();i++)
    {
        if(i!=0)
            recommendedJson+=",";
        recommendedJson+="\""+recommended.at(i)+"\"";
    }

    return "{\"title\":\""+title+"\",\"related\":"+recommendedJson+"}";
}

//Write the movie json into the file
bool MainWindow::writeInFile(QString fileName, QString movieJSON)
{
    QFile file(fileName);
    if (file.open(QIODevice::ReadWrite | QIODevice::Append)) {
        QTextStream stream(&file);
        stream << movieJSON << endl;
    }
    else
    {
        return false;
    }

    file.close();
    return true;
}

//Parse a json from a string
QJsonObject MainWindow::ObjectFromString(const QString& json)
{
    QJsonObject obj;

    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());

    // check validity of the document
    if(!doc.isNull())
    {
        if(doc.isObject())
        {
            obj = doc.object();
        }
        else
        {
            qDebug() << "Document is not an object" << endl;
        }
    }
    else
    {
        qDebug() << "Invalid JSON...\n" << json << endl;
    }

    return obj;
}

bool MainWindow::requestsFinished()
{
    return titleRequestFinished && recommendationsRequestFinished;
}

void MainWindow::on_AddMovie_clicked()
{
    run=true;
    Run();
}

void MainWindow::on_Stop_clicked()
{
    run=false;
}

void MainWindow::on_setFilePath_clicked()
{
    QString file = QFileDialog::getSaveFileName(this, tr("Save as ..."),filePath,tr("JSON file (*.json)"));

    if(file.isEmpty()) {
        // Error ...
        return;
    }

    filePath = file;
}
