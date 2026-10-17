#include "RelayReply.hpp"
#include <QCoreApplication>
#include <QTimer>
#include <QNetworkRequest>
#include <stdio.h>
int main(int argc,char**argv){if(argc!=2||!initializeRelayNetwork())return 2;QCoreApplication app(argc,argv);QNetworkRequest request(QUrl(QString::fromUtf8(argv[1])));RelayReply reply(request,"{}","invalid-token",&app);QObject::connect(&reply,SIGNAL(finished()),&app,SLOT(quit()));QTimer::singleShot(22000,&app,SLOT(quit()));app.exec();int code=reply.attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();printf("finished=%d status=%d error=%d bytes=%d\n",reply.isFinished(),code,reply.error(),reply.readAll().size());return reply.isFinished()&&code==401&&reply.error()==QNetworkReply::NoError?0:1;}
