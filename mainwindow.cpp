#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QSettings>
#include <QDateTime>

#define version "SimuNav 0.3"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mTimerDiff=new QTimer;
    udpSocket = new QUdpSocket(this);

    QObject::connect(ui->dial_cap,&QDial::valueChanged,this,&MainWindow::afficheCapSpeed);
    QObject::connect(ui->slid_Speed,&QSlider::valueChanged,this,&MainWindow::afficheCapSpeed);
    QObject::connect(ui->sp_Cap,QOverload<int>::of(&QSpinBox::valueChanged),this,&MainWindow::majDialCap);
    QObject::connect(ui->sp_Speed,QOverload<double>::of(&QDoubleSpinBox::valueChanged),this,&MainWindow::majSlideSpeed);
    QObject::connect(this->mTimerDiff,&QTimer::timeout,this,&MainWindow::traitement);
    QObject::connect(ui->btn_Diff,&QPushButton::clicked,this,&MainWindow::diffuser);
    QObject::connect(ui->btn_Stop,&QPushButton::clicked,this,&MainWindow::stop);
    this->setWindowTitle(version);

    restaureParam();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::afficheCapSpeed()
{
   int nCap=ui->dial_cap->value();
    if(nCap>180)
        nCap=nCap-180;
    else
        nCap=nCap+180;
   ui->sp_Cap->setValue(nCap);

   double dSpeed=ui->slid_Speed->value();
   dSpeed=dSpeed/10;
   ui->sp_Speed->setValue(dSpeed);

}

void MainWindow::majDialCap(int nValue)
{

    if(nValue>180)
        nValue=nValue-180;
    else
        nValue=nValue+180;

    ui->dial_cap->setValue(nValue);
}

void MainWindow::majSlideSpeed(double dValue)
{
    int nValue=dValue*10;
    ui->slid_Speed->setValue(nValue);
}

void MainWindow::diffuser()
{

    ui->btn_Diff->setEnabled(false);
    ui->btn_Stop->setEnabled(true);
    ui->le_IpDest->setEnabled(false);
    ui->sp_UdpPort->setEnabled(false);
    ui->sp_Periode->setEnabled(false);

    mPosCourante.setLatitude(latMinToDec(QString("%1,%2,%3").arg(ui->cb_HemiLat->currentText()).arg(ui->sp_DegLat->value()).arg(ui->sp_MinLat->value())));
    mPosCourante.setLongitude(longMinToDec(QString("%1,%2,%3").arg(ui->cb_HemiLong->currentText()).arg(ui->sp_DegLong->value()).arg(ui->sp_MinLong->value())));

    mTimerDiff->setInterval(ui->sp_Periode->value()*1000);
    mTimerDiff->start();

    sauvParam();
    udpSocket->bind(ui->sp_UdpPort->value(),QAbstractSocket::ShareAddress);

}

void MainWindow::stop()
{
    ui->btn_Diff->setEnabled(true);
    ui->btn_Stop->setEnabled(false);
    ui->le_IpDest->setEnabled(true);
    ui->sp_UdpPort->setEnabled(true);
    ui->sp_Periode->setEnabled(true);

    mTimerDiff->stop();
}

void MainWindow::traitement()
{
    mPosCourante=calcNextPos(mPosCourante,ui->sp_Cap->value(),ui->sp_Speed->value());

     QString sUneTrame=construitRMC(mPosCourante);
    sendUdp(sUneTrame);
    sUneTrame=construitVTG(ui->sp_Cap->value(),ui->sp_Speed->value());
    sendUdp(sUneTrame);
    sUneTrame=construitZDA();
    sendUdp(sUneTrame);
    sUneTrame=construitGGA(mPosCourante);
    sendUdp(sUneTrame);

}

QGeoCoordinate MainWindow::calcNextPos(QGeoCoordinate lastPos, int nCap, double dSpeed)
{
    double dDistance= dSpeed/3600*ui->sp_Periode->value()*1852;

    QGeoCoordinate currentPos=lastPos.atDistanceAndAzimuth(dDistance,nCap);

    return currentPos;

}

void MainWindow::sendUdp(QString sUneTrame)
{
    QByteArray datagram=sUneTrame.toLocal8Bit();
    qint64 res=udpSocket->writeDatagram(datagram.data(), datagram.size(),QHostAddress(ui->le_IpDest->text()), ui->sp_UdpPort->value());
    if(res==-1)
        ui->statusbar->showMessage("Trame non émise");
    else
        ui->statusbar->showMessage(datagram);
}

double MainWindow::latMinToDec(QString sLatitude)
{
    //format : H,DD,MM.ddddd,
        //         S,12,30.54443
        int nDeg=sLatitude.section(",",1,1).toInt();
        double dMin=sLatitude.section(",",2,2).toDouble();
        QString cHemi=sLatitude.section(",",0,0);

        double dLatDec=nDeg+(dMin/60);
        if(cHemi=="S")
            dLatDec=-dLatDec;

        return dLatDec;

}

double MainWindow::longMinToDec(QString sLongitude)
{
    //format :H,DDD,MM.ddddd
    //        E,044,55.20103
    int nDeg=sLongitude.section(",",1,1).toInt();
    double dMin=sLongitude.section(",",2,2).toDouble();
    QString cHemi=sLongitude.section(",",0,0);

    double dLongDec=nDeg+(dMin/60);
    if(cHemi=="W")
        dLongDec=-dLongDec;

    return dLongDec;
}

void MainWindow::sauvParam()
{
    QSettings settings("SimuNav","Config");
    settings.setValue("IpAddress",ui->le_IpDest->text());
    settings.setValue("UDPPort",ui->sp_UdpPort->value());
    settings.setValue("Periode",ui->sp_Periode->value());
    settings.setValue("LatHemi",ui->cb_HemiLat->currentIndex());
    settings.setValue("LatDeg",ui->sp_DegLat->value());
    settings.setValue("LatMin",ui->sp_MinLat->value());
    settings.setValue("LongHemi",ui->cb_HemiLong->currentIndex());
    settings.setValue("LongDeg",ui->sp_DegLong->value());
    settings.setValue("LongMin",ui->sp_MinLong->value());
    settings.setValue("Cap",ui->sp_Cap->value());
    settings.setValue("Speed",ui->sp_Speed->value());
}

void MainWindow::restaureParam()
{
    QSettings settings("SimuNav","Config");
    ui->le_IpDest->setText(settings.value("IpAddress","127.0.0.1").toString());
    ui->sp_UdpPort->setValue(settings.value("UDPPort",13000).toInt());
    ui->sp_Periode->setValue(settings.value("Periode",1).toDouble());
    ui->cb_HemiLat->setCurrentIndex(settings.value("LatHemi",0).toInt());
    ui->sp_DegLat->setValue(settings.value("LatDeg",47).toInt());
    ui->sp_MinLat->setValue(settings.value("LatMin",34.5).toDouble());
    ui->cb_HemiLong->setCurrentIndex(settings.value("LongHemi",0).toInt());
    ui->sp_DegLong->setValue(settings.value("LongDeg",7).toInt());
    ui->sp_MinLong->setValue(settings.value("LongMin",31.5).toDouble());
    ui->sp_Cap->setValue(settings.value("Cap",0).toInt());
    ui->sp_Speed->setValue(settings.value("Speed",5).toDouble());

}

QString MainWindow::construitGGA(QGeoCoordinate position)
{
    /*
$GPGGA       : Type de trame
064036.289   : Trame envoyée à 06 h 40 min 36 s 289 (heure UTC)
4836.5375,N  : Latitude 48,608958° Nord = 48° 36' 32.25" Nord
00740.9373,E : Longitude 7,682288° Est = 7° 40' 56.238" Est
1            : Type de positionnement (le 1 est un positionnement GPS)
04           : Nombre de satellites utilisés pour calculer les coordonnées
3.2          : Précision horizontale ou HDOP (Horizontal dilution of precision)
200.2,M      : Altitude 200,2, en mètres
,,,,,0000    : D'autres informations peuvent être inscrites dans ces champs
*0E          : Somme de contrôle de parité, un simple XOR sur les caractères entre $ et

*/

    QString sHeureCourante=QDateTime::currentDateTimeUtc().toString("hhmmss.zzz");
    QString sPosition=position.toString(QGeoCoordinate::DegreesMinutesWithHemisphere);
    QString sLatitude=sPosition.section(",",0,0);

    sLatitude=sLatitude.remove("°");
    sLatitude=sLatitude.remove(" ");
    sLatitude=sLatitude.remove("'");
    sLatitude=sLatitude.rightJustified(9,'0');
    sLatitude=sLatitude.insert(sLatitude.count()-1,",");
    sLatitude=sLatitude.section(",",0,0).leftJustified(9,'0')+","+sLatitude.section(",",1,1);

    QString sLongitude=sPosition.section(",",1,1);
    sLongitude=sLongitude.remove("°");
    sLongitude=sLongitude.remove(" ");
    sLongitude=sLongitude.remove("'");
    sLongitude=sLongitude.rightJustified(10,'0');
    sLongitude=sLongitude.insert(sLongitude.count()-1,",");
    sLongitude=sLongitude.section(",",0,0).leftJustified(10,'0')+","+sLongitude.section(",",1,1);

    QString sTrame=QString("$GPGGA,%1,%2,%3,1,15,1.0,0.0,M,,,,0000*").arg(sHeureCourante,sLatitude,sLongitude);
    QString sChecksum=checksum(sTrame);
    sTrame=sTrame+sChecksum+0x0D+0x0a;

    return sTrame;

}

QString MainWindow::construitVTG(int nCap,double dSpeed)
{
    /*
    Track Made Good and Ground Speed.

    eg1. $GPVTG,360.0,T,348.7,M,000.0,N,000.0,K*43
    eg2. $GPVTG,054.7,T,034.4,M,005.5,N,010.2,K


               054.7,T      True track made good
               034.4,M      Magnetic track made good
               005.5,N      Ground speed, knots
               010.2,K      Ground speed, Kilometers per hour


    eg3. $GPVTG,t,T,,,s.ss,N,s.ss,K*hh
    1    = Track made good
    2    = Fixed text 'T' indicates that track made good is relative to true north
    3    = not used
    4    = not used
    5    = Speed over ground in knots
    6    = Fixed text 'N' indicates that speed over ground in in knots
    7    = Speed over ground in kilometers/hour
    8    = Fixed text 'K' indicates that speed over ground is in kilometers/hour
    9    = Checksum

    The actual track made good and speed relative to the ground.

    $--VTG,x.x,T,x.x,M,x.x,N,x.x,K
    x.x,T = Track, degrees True
    x.x,M = Track, degrees Magnetic
    x.x,N = Speed, knots
    x.x,K = Speed, Km/hr
    */

    QString sCap=QString::number(nCap,'f',1).rightJustified(5,'0');
    QString sSpeed=QString::number(dSpeed,'f',1).rightJustified(5,'0');;
    QString sSpeed2=QString::number(dSpeed*1.852,'f',1).rightJustified(5,'0');;
    QString sTrame=QString("$GPVTG,%1,T,%1,M,%2,N,%3,K*").arg(sCap,sSpeed,sSpeed2);
    QString sChecksum=checksum(sTrame);
    sTrame=sTrame+sChecksum+0x0D+0x0a;

    return sTrame;




}

QString MainWindow::construitRMC(QGeoCoordinate position)
{
    /*
$GPRMC,053740.000,A,2503.6319,N,12136.0099,E,2.69,79.65,100106,,,A*69

$GPRMC       : type de trame
053740.000   : heure UTC exprimée en hhmmss.sss : 5 h 37 min 40 s
A            : état A=données valides, V=données invalides
2503.6319    : Latitude exprimée en ddmm.mmmm : 25° 03.6319' = 25° 03' 37,914"
N            : indicateur de latitude N=nord, S=sud
12136.0099   : Longitude exprimée en dddmm.mmmm : 121° 36.0099' = 121° 36' 00,594"
E            : indicateur de longitude E=est, W=ouest
2.69         : vitesse sur le fond en nœuds (2,69 kn = 3,10 mph = 4,98 km/h)
79.65        : route sur le fond en degrés
100106       : date exprimée en qqmmaa : 10 janvier 2006
,            : déclinaison magnétique en degrés (souvent vide pour un GPS)
,            : sens de la déclinaison E=est, W=ouest (souvent vide pour un GPS)
A            : mode de positionnement A=autonome, D=DGPS, E=DR
*69          : somme de contrôle de parité au format hexadécimal3

*/


    QString sHeureCourante=QDateTime::currentDateTimeUtc().toString("hhmmss.zzz");
    QString sDateCourante=QDateTime::currentDateTimeUtc().toString("ddMMyy");
    QString sPosition=position.toString(QGeoCoordinate::DegreesMinutesWithHemisphere);
    QString sLatitude=sPosition.section(",",0,0);
    sLatitude=sLatitude.remove("°");
    sLatitude=sLatitude.remove(" ");
    sLatitude=sLatitude.remove("'");
    sLatitude=sLatitude.rightJustified(9,'0');
    sLatitude=sLatitude.insert(sLatitude.count()-1,",");
    sLatitude=sLatitude.section(",",0,0).leftJustified(9,'0')+","+sLatitude.section(",",1,1);


    QString sLongitude=sPosition.section(",",1,1);
    sLongitude=sLongitude.remove("°");
    sLongitude=sLongitude.remove(" ");
    sLongitude=sLongitude.remove("'");
    sLongitude=sLongitude.rightJustified(10,'0');
    sLongitude=sLongitude.insert(sLongitude.count()-1,",");
    sLongitude=sLongitude.section(",",0,0).leftJustified(10,'0')+","+sLongitude.section(",",1,1);


    QString sCap=QString::number(ui->sp_Cap->value(),'f',1);
    QString sSpeed=QString::number(ui->sp_Speed->value(),'f',1);

    QString sTrame=QString("$GPRMC,%1,%2,%3,%4,%5,%6,,,A*").arg(sHeureCourante,sLatitude,sLongitude,sSpeed,sCap,sDateCourante);
    QString sChecksum=checksum(sTrame);
    sTrame=sTrame+sChecksum+0x0D+0x0a;

    return sTrame;


}

QString MainWindow::construitZDA()
{
    /*
$GPZDA

Date & Time

UTC, day, month, year, and local time zone.

$--ZDA,hhmmss.ss,xx,xx,xxxx,xx,xx
hhmmss.ss = UTC
xx = Day, 01 to 31
xx = Month, 01 to 12
xxxx = Year
xx = Local zone description, 00 to +/- 13 hours
xx = Local zone minutes description (same sign as hours)
*/

    QDateTime currentDH=QDateTime::currentDateTimeUtc();
    QString sTrame=QString("$GPZDA,%1,%2,%3,%4,00,00*").arg(currentDH.toString("hhmmss.zzz")).arg(currentDH.toString("dd")).arg(currentDH.toString("MM")).arg(currentDH.toString("yyyy"));
    QString sChecksum=checksum(sTrame);
    sTrame=sTrame+sChecksum+0x0D+0x0a;
    qDebug()<<sTrame;
    return sTrame;

}

QString MainWindow::checksum(QString str)
{
        unsigned char crc=0;
        //$ n'entre pas dans le calcul du checksum
        for(int i=1; i<str.length() && str.at(i).toLatin1()!='*';i++)
        {

           crc ^=str.at(i).toLatin1();
        }
        QString sCRC=QString("%1").arg(crc,2,16,QLatin1Char('0'));
        return sCRC;
}


