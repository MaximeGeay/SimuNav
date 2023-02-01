#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGeoCoordinate>
#include <QTimer>
#include <QUdpSocket>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void afficheCapSpeed();
    void majDialCap(int nValue);
    void majSlideSpeed(double dValue);
    void diffuser(void);
    void stop(void);
    void traitement(void);
    void gestAffichage(void);

private:
    Ui::MainWindow *ui;

    QGeoCoordinate mPosCourante;
    QGeoCoordinate calcNextPos(QGeoCoordinate lastPos, int nCap, double dSpeed);
    void sendUdp(QString sUneTrame);
    double latMinToDec(QString sLatitude);
    double longMinToDec(QString sLongitude);
    QTimer *mTimerDiff;
    void sauvParam(void);
    void restaureParam(void);
    QString construitGGA(QGeoCoordinate position);
    QString construitVTG(int nCap, double dSpeed);
    QString construitRMC(QGeoCoordinate position);
    QString construitGLL(QGeoCoordinate position);
    QString construitZDA(void);
    QString construitScout(QGeoCoordinate position);
    QString construitHDT(int nCap);
    QString construitDBT(double dSonde);
    QString checksum(QString str);
    QUdpSocket *udpSocket;


};
#endif // MAINWINDOW_H
