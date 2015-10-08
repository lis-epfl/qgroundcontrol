/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

// The above copyright block should be at the top of every file.

/// @file
///     @brief This file is used to control a group of muliple robot 
///             having in mind the MAV'RIC framework. 
///
///     @author Nicolas Dousse <nic.dousse@gmail.com>

#ifndef QGCSWARMCONTROL_H
#define QGCSWARMCONTROL_H

#include <QWidget>
#include "MAVLinkProtocol.h"
#include "UASManager.h"
#include "UASInterface.h"
#include <QListWidget>
#include <QMap>
#include "UASControlWidget.h"
#include "AutoPilotPluginManager.h"

namespace Ui {
class QGCSwarmControl;
}

class QGCSwarmControl : public QWidget
{
    Q_OBJECT

public:
    explicit QGCSwarmControl(QWidget *parent = 0);
    ~QGCSwarmControl();

public slots:

    void textMessageReceived(UASInterface* uas, QString message);
    void textEmit(int uasid, int component, int severity, QString message);

private slots:
    void continueAllButton_clicked(void);
    void Return2startButton_clicked(void);
    void launchScenario_clicked(void);
    void autoLanding_clicked(void);
    void autoTakeoff_clicked(void);

    void startLogging_clicked(void);
    void stopLogging_clicked(void);

    void UASCreated(UASInterface* uas);
    void RemoveUAS(UASInterface* uas);
    void ListWidgetClicked(QListWidgetItem* item);

    void newActiveUAS(UASInterface* uas);

    void refreshView(void);

    void setParameters_clicked(void);

    void startButton_clicked(void);
    void stopButton_clicked(void);

    void armButton_clicked(void);
    void disarmButton_clicked(void);

    void selectButton_clicked(void);

    void remoteItem_clicked(QListWidgetItem* item);

    void remoteButton_clicked(void);

    void setMode(int mode);

    void sendNewHomePosition(void);

    void strategyLaunchClicked(void);

private:
    Ui::QGCSwarmControl *ui;

    void updateModesList(UASInterface* uas);

    int wptReachedCnt;

protected:
    MAVLinkProtocol* mavlink;     ///< Reference to the MAVLink instance
    UASInterface *uas;
    QList<UASInterface*> UASlist;

    QMap<UASInterface*,QListWidgetItem*> uasToItemMapping;
    QMap<QListWidgetItem*,UASInterface*> itemToUasMapping;

    QMap<UASInterface*,QListWidgetItem*> uasToItemRemote;
    QMap<QListWidgetItem*,UASInterface*> itemToUasRemote;    

    QMap<QListWidgetItem*,int> itemToID;
    QMap<QListWidgetItem*,int> itemToIDRemote;

    UASInterface *uas_previous;

    QTimer updateTimer; ///< Only update at 1 Hz to not overload the GUI
    static const unsigned int updateInterval; ///< The update interval of the refresh function

    bool all_selected;

    bool mode_init;
    int modeIdx;                    ///< Current uas mode index
     QList<AutoPilotPluginManager::FullMode_t>  _modeList;  ///< Mode list for the current UAS

signals:
    void uasTextReceived(UASInterface* uas, QString message);

};

#endif // QGCSWARMCONTROL_H
