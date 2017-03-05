#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{    
    ui->setupUi(this);

    this->setWindowTitle("Tmdb Json Parser");

    //Manager of the title request
    managerMovieTitle = new QNetworkAccessManager(this);
    connect(managerMovieTitle, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyMovieFinished(QNetworkReply*)));

    //Manager of the recommendations request
    managerRecommendations = new QNetworkAccessManager(this);
    connect(managerRecommendations, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyRecommendationsFinished(QNetworkReply*)));

    //Initialize variables
    settings = new QSettings("Kanap","Tmdb_JSONParser",this);
    movieTitle = QString();
    recommendationList = QList<QString>();
    addingMovie = false;

    //Get values from settings, second arguement is default value
    filePath = settings->value("filePath","tmdb_Recommendations.json").toString();
    ui->filePathText->setText(filePath);
    ui->fromId->setValue(settings->value("fromId",0).toInt());
    ui->toId->setValue(settings->value("toId",100).toInt());
    apiKey = settings->value("apiKey","2d28c41e6dab17f0aa2ddb2a9cf2b8f0").toString();
    ui->apiKeyLineEdit->setText(apiKey);

    //Set the timer
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(addNextMovie()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

//Add a movie as json to the file
void MainWindow::addNextMovie()
{
    //If we are still adding a movie, do not add another one at the same time
    if(addingMovie)
        return;

    //Clear the variables
    movieTitle = QString();
    recommendationList.clear();

    //Go to the next movie
    currentId++;

    ui->progressBarTotal->setValue(currentId);

    //If we are on the last movie to add, stop the timer so this function won't be called another time
    if(currentId>=ui->toId->value())
    {
        stopAddingMovies();
    }

    //Set the movie id
    movieId =QString::number(currentId);
    ui->currentIdText->setText("Current Id : <b>"+movieId+"</b>");

    //Note that we start adding a movie
    titleRequestFinished=false;
    recommendationsRequestFinished=false;
    addingMovie = true;

   //request of the movie title by id
   QString requestTitleURL = "http://api.themoviedb.org/3/movie/"+movieId+"?api_key="+apiKey;
   managerMovieTitle->get(QNetworkRequest(QUrl(requestTitleURL)));

   //request of the recommendations by id
   QString requestRecommendationsURL = "http://api.themoviedb.org/3/movie/"+movieId+"/recommendations?api_key="+apiKey;
   managerRecommendations->get(QNetworkRequest(QUrl(requestRecommendationsURL)));

   ui->progressBarCurrent->setValue(1);
   ui->progressBarCurrent->setFormat("sending requests to tmdb...(%v / %m)");
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

    //Note that we finished
    recommendationsRequestFinished=true;

    //If all request ended, go to next step
    if(requestsFinished())
        exportMovieToFile();
}

//Parse the json response of the title request, return empty string if invalide
QString MainWindow::getMovieTitleFromJSON(QJsonObject jsonObject)
{
    //Get the title from the movie json object
    return jsonObject.find("title").value().toString();
}

//Parse the json response of the recommendations request, return empty list if invalide
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

//Export the movie data to a json file
void MainWindow::exportMovieToFile()
{
    ui->progressBarCurrent->setValue(3);
    ui->progressBarCurrent->setFormat("parsing data...(%v / %m)");

    //Check if we have not empty data
    if(!movieTitle.isEmpty() && !recommendationList.isEmpty())
    {
        //Build the json with the movie title and the recommendations
        QString json = makeMovieJson(movieTitle,recommendationList,movieId);

        ui->progressBarCurrent->setValue(4);
        ui->progressBarCurrent->setFormat("writing to file...(%v / %m)");

        //Write the movie json into the file
        if(!json.isEmpty())
            writeInFile(filePath,json);

        ui->progressBarCurrent->setValue(5);
        ui->progressBarCurrent->setFormat("complete (%v / %m)");
    }
    //If one of the value is empty, the data are invalide
    else
    {
        ui->progressBarCurrent->setValue(5);
        //if the movie title is missing
        if(movieTitle.isEmpty())
            ui->progressBarCurrent->setFormat("invalide movie data");
        //Otherwise, it's the recommendations that are missing
        else
            ui->progressBarCurrent->setFormat("invalide recommendations data");
    }

    //Note that we finished ending a movie
    addingMovie = false;

}

//Build the json with the movie title and the recommendations
QString MainWindow::makeMovieJson(QString title,QList<QString>recommended,QString id)
{
    QString recommendedJson="";

    for(int i=0;i<recommended.length();i++)
    {
        if(i!=0)
            recommendedJson+=",";
        recommendedJson+="\""+recommended.at(i)+"\"";
    }
    QString firstLine = "{\"index\":{\"_index\":\"content\",\"_type\":\"movie\",\"_id\":"+id+"}}";
    QString secondLine = "{\"title\":\""+title+"\",\"related\":["+recommendedJson+"]}";

    return firstLine+"\n"+secondLine;
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
    }

    return obj;
}

bool MainWindow::requestsFinished()
{
    bool finished = titleRequestFinished && recommendationsRequestFinished;
    if(finished)
    {
        ui->progressBarCurrent->setValue(2);
        ui->progressBarCurrent->setFormat("waiting all replies...(%v / %m)");
    }
    return finished;
}

void MainWindow::on_Run_clicked()
{
    //Set the starting id as -1 cause addNextMovie() increases it by 1 first
    currentId = ui->fromId->value()-1;

    //Set the value of the progress bar
    ui->progressBarTotal->setMinimum(ui->fromId->value());
    ui->progressBarTotal->setMaximum(ui->toId->value());

    //Disable ui
    ui->fromId->setEnabled(false);
    ui->toId->setEnabled(false);
    ui->filePathText->setEnabled(false);
    ui->setFilePath->setEnabled(false);
    ui->apiKeyLineEdit->setEnabled(false);
    ui->Run->setEnabled(false);

    //Save the current settings
    settings->setValue("filePath",filePath);
    settings->setValue("fromId",ui->fromId->value());
    settings->setValue("toId",ui->toId->value());
    settings->setValue("apiKey",apiKey);

    timer->start(550);
}

void MainWindow::on_Stop_clicked()
{
    stopAddingMovies();
}

//Stop adding movies
void MainWindow::stopAddingMovies()
{
    timer->stop();

    //Enable ui
    ui->fromId->setEnabled(true);
    ui->toId->setEnabled(true);
    ui->filePathText->setEnabled(true);
    ui->setFilePath->setEnabled(true);
    ui->apiKeyLineEdit->setEnabled(true);
    ui->Run->setEnabled(true);

    //Save the id we stopped at
    ui->fromId->setValue(currentId+1);
    //save the current id in regedit
    settings->setValue("fromId",ui->fromId->value());

    //If the from id is greater than target id, increase target id and save it
    if(ui->fromId->value()>=ui->toId->value())
    {
        ui->toId->setValue(ui->toId->value()+100);
        settings->setValue("toId",ui->toId->value());
    }

    ui->currentIdText->setText("Not running");
}

void MainWindow::on_setFilePath_clicked()
{
    QString file = QFileDialog::getSaveFileName(this, tr("Save as ..."),filePath,tr("JSON file (*.json)"));

    if(file.isEmpty()) {
        // Error ...
        return;
    }

    //Set the text of the line edit (which call another event setting the file path)
    ui->filePathText->setText(file);
}

void MainWindow::on_filePathText_textChanged(const QString &arg1)
{
    //set the file path to the content of the line edit
    filePath = arg1;
}

void MainWindow::on_apiKeyLineEdit_editingFinished()
{
    //Set the api key to the new text of this line edit
    apiKey = ui->apiKeyLineEdit->text();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this,"About Tmdb JSON parser","Made by : Antoine Collot\nfor Kanap\n\nESME Sudria\n03/03/2017");
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this,"About Qt");
}
